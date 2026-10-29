{ Inno Setup 6 runs this code in its 32-bit engine, including x64 installs.
  These mutexes are existence markers: never wait on or own them. }
const
  GuardTokenQuery = $0008;
  GuardSynchronize = $00100000;
  GuardErrorFileNotFound = 2;
  GuardErrorPathNotFound = 3;
  GuardErrorAlreadyExists = 183;
  GuardInvalidHandle = $FFFFFFFF;
type
  TGuardTokenBuffer = array[0..4095] of Byte;
  TGuardSidCharacters = array[0..255] of Word;
var
  GuardSetupHandle: LongWord;

function GuardGetCurrentProcess: LongWord;
  external 'GetCurrentProcess@kernel32.dll stdcall';
function GuardOpenProcessToken(Process: LongWord; Access: LongWord;
  var Token: LongWord): Integer;
  external 'OpenProcessToken@advapi32.dll stdcall';
function GuardGetTokenInformation(Token: LongWord; InformationClass: LongWord;
  var Buffer: TGuardTokenBuffer; BufferSize: LongWord;
  var RequiredSize: LongWord): Integer;
  external 'GetTokenInformation@advapi32.dll stdcall';
function GuardConvertSidToStringSid(Sid: LongWord; var Text: LongWord): Integer;
  external 'ConvertSidToStringSidW@advapi32.dll stdcall';
function GuardStringLength(Text: LongWord): Integer;
  external 'lstrlenW@kernel32.dll stdcall';
procedure GuardCopySidPointer(var Destination: LongWord;
  var Source: TGuardTokenBuffer; Count: LongWord);
  external 'RtlMoveMemory@kernel32.dll stdcall';
procedure GuardCopyString(var Destination: TGuardSidCharacters; Source: LongWord; Count: LongWord);
  external 'RtlMoveMemory@kernel32.dll stdcall';
function GuardLocalFree(Memory: LongWord): LongWord;
  external 'LocalFree@kernel32.dll stdcall';
function GuardCloseHandle(Handle: LongWord): Integer;
  external 'CloseHandle@kernel32.dll stdcall';
function GuardCreateMutex(Attributes: LongWord; InitialOwner: Integer;
  Name: String): LongWord;
  external 'CreateMutexW@kernel32.dll stdcall';
function GuardOpenMutex(Access: LongWord; Inherit: Integer; Name: String): LongWord;
  external 'OpenMutexW@kernel32.dll stdcall';
function GuardCreateFile(Name: String; Access, Share, Attributes,
  Disposition, Flags, Template: LongWord): LongWord;
  external 'CreateFileW@kernel32.dll stdcall';

procedure GuardRelease;
begin
  if GuardSetupHandle <> 0 then begin
    GuardCloseHandle(GuardSetupHandle);
    GuardSetupHandle := 0;
  end;
end;

function GuardProcessSid(var Sid: String): Boolean;
var
  Token, RequiredSize, SidPointer, SidText, LastError: LongWord;
  Buffer: TGuardTokenBuffer;
  Characters: TGuardSidCharacters;
  TextLength, I: Integer;
begin
  Result := False;
  Token := 0;
  SidText := 0;
  if GuardOpenProcessToken(GuardGetCurrentProcess, GuardTokenQuery, Token) = 0 then begin
    LastError := DLLGetLastError;
    Log(Format('Upgrade guard: OpenProcessToken failed (%d)', [LastError]));
    Exit;
  end;
  try
    if GuardGetTokenInformation(Token, 1, Buffer, 4096, RequiredSize) = 0 then begin
      LastError := DLLGetLastError;
      Log(Format('Upgrade guard: TokenUser failed (%d)', [LastError]));
      Exit;
    end;
    if (RequiredSize < 8) or (RequiredSize > 4096) then Exit;
    GuardCopySidPointer(SidPointer, Buffer, 4);
    if SidPointer = 0 then Exit;
    if GuardConvertSidToStringSid(SidPointer, SidText) = 0 then begin
      LastError := DLLGetLastError;
      Log(Format('Upgrade guard: SID conversion failed (%d)', [LastError]));
      Exit;
    end;
    TextLength := GuardStringLength(SidText);
    if (TextLength <= 0) or (TextLength > 256) then Exit;
    { Copy exactly strlen * 2 bytes into a fixed UTF-16 buffer. }
    GuardCopyString(Characters, SidText, TextLength * 2);
    Sid := '';
    for I := 0 to TextLength - 1 do Sid := Sid + Chr(Characters[I]);
    Result := True;
  finally
    if SidText <> 0 then GuardLocalFree(SidText);
    GuardCloseHandle(Token);
  end;
end;

function GuardReserve: Boolean;
var
  Sid: String;
  RunningHandle, LastError: LongWord;
begin
  Result := False;
  if GuardSetupHandle <> 0 then Exit;
  if not GuardProcessSid(Sid) then Exit;
  GuardSetupHandle := GuardCreateMutex(0, 0,
    'Global\AXEVoiceNativeCandidate.Setup.' + Sid);
  LastError := DLLGetLastError;
  if GuardSetupHandle = 0 then begin
    Log(Format('Upgrade guard: CreateMutex failed (%d)', [LastError]));
    Exit;
  end;
  if LastError = GuardErrorAlreadyExists then begin
    GuardRelease;
    Exit;
  end;
  RunningHandle := GuardOpenMutex(GuardSynchronize, 0,
    'Global\AXEVoiceNativeCandidate.Running.' + Sid);
  LastError := DLLGetLastError;
  if RunningHandle <> 0 then begin
    GuardCloseHandle(RunningHandle);
    GuardRelease;
    Exit;
  end;
  if LastError <> GuardErrorFileNotFound then begin
    Log(Format('Upgrade guard: OpenMutex failed (%d)', [LastError]));
    GuardRelease;
    Exit;
  end;
  Result := True;
end;

function GuardLegacyExecutableAvailable: Boolean;
var
  ExePath: String;
  FileHandle, LastError: LongWord;
begin
  ExePath := ExpandConstant('{app}\AXE-Voice.exe');
  FileHandle := GuardCreateFile(ExePath, $40000000, 0, 0, 3, $80, 0);
  LastError := DLLGetLastError;
  if FileHandle <> GuardInvalidHandle then begin
    GuardCloseHandle(FileHandle);
    Result := True;
  end else begin
    { A fresh install may also have no destination directory yet.
      Never create, truncate or write the executable during this probe. }
    Result := (LastError = GuardErrorFileNotFound) or
      (LastError = GuardErrorPathNotFound);
    if not Result then
      Log(Format('Upgrade guard: exclusive executable probe failed (%d)', [LastError]));
  end;
end;

function GuardBlockedMessage: String;
begin
  Result := 'Feche o AXE Voice candidato e qualquer outro instalador antes de continuar. ' +
    'Se o problema persistir, verifique as permissões e tente novamente.';
end;

function InitializeSetup: Boolean;
begin
  Result := GuardReserve;
  if not Result then
    SuppressibleMsgBox(GuardBlockedMessage, mbError, MB_OK, IDOK);
end;

function PrepareToInstall(var NeedsRestart: Boolean): String;
begin
  Result := '';
  if not GuardLegacyExecutableAvailable then Result := GuardBlockedMessage;
end;

procedure CurStepChanged(CurStep: TSetupStep);
begin
  { ssPostInstall precedes processing [Run]. }
  if CurStep = ssPostInstall then GuardRelease;
end;

procedure DeinitializeSetup;
begin
  GuardRelease;
end;

function InitializeUninstall: Boolean;
begin
  Result := GuardReserve;
  if Result then Result := GuardLegacyExecutableAvailable;
  if not Result then begin
    GuardRelease;
    SuppressibleMsgBox(GuardBlockedMessage, mbError, MB_OK, IDOK);
  end;
end;

procedure DeinitializeUninstall;
begin
  GuardRelease;
end;
