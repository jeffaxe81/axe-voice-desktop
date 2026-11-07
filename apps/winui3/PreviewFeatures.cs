// AXE Voice WinUI 3 preview-only interaction models.
// No SIP commands, account credentials, call history or native SDK handles.
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Security.Cryptography;
using System.Text;
using System.Text.Json;

namespace AXEVoice.WinUI3;

public static class DialerEditing
{
    public const int MaxLength=128;
    public static string AppendKey(string? current,string key)
    {
        string text=current??"";
        if(key.Length!=1 || !"0123456789*#".Contains(key,StringComparison.Ordinal) ||
           text.Length>=MaxLength)return text;
        return text+key;
    }
    // VirtualKey integer values: digits 48-57, numpad 96-105, Back=8, Escape=27.
    // Enter is intentionally NOT an action while the native SIP engine is detached.
    public static string ApplyKey(string? current,int virtualKey)
    {
        string text=current??"";
        if(virtualKey==27)return "";
        if(virtualKey==8)return text.Length>0?text[..^1]:text;
        if(virtualKey>=48 && virtualKey<=57)
            return AppendKey(text,((char)virtualKey).ToString());
        if(virtualKey>=96 && virtualKey<=105)
            return AppendKey(text,((char)('0'+virtualKey-96)).ToString());
        return text;
    }
}

public enum PreviewSizeMode { Compact=0, Comfortable=1 }
public readonly record struct PreviewDimensions(int Width,int Height);

public static class PreviewWindowSizing
{
    public static PreviewDimensions Dimensions(PreviewSizeMode mode) =>
        mode==PreviewSizeMode.Comfortable?new(400,680):new(355,602);
}

public sealed class PreviewProfile
{
    public List<string> Favorites {get;set;} = new();
    public PreviewSizeMode Size {get;set;} = PreviewSizeMode.Compact;

    public bool AddFavorite(string? destination)
    {
        if(string.IsNullOrWhiteSpace(destination) || destination.Length>DialerEditing.MaxLength ||
           destination.Any(char.IsControl) || Favorites.Count>=10 ||
           Favorites.Contains(destination,StringComparer.OrdinalIgnoreCase))return false;
        Favorites.Add(destination);
        return true;
    }
    public bool RemoveFavorite(string? destination)
    {
        if(destination is null)return false;
        var existing=Favorites.FirstOrDefault(
            v=>string.Equals(v,destination,StringComparison.OrdinalIgnoreCase));
        return existing is not null && Favorites.Remove(existing);
    }
}

public sealed class PreviewProfileStore
{
    public string StoragePath {get;}
    public PreviewProfileStore(string filePath) => StoragePath=filePath;

    public PreviewProfile Load()
    {
        try {
            if(!File.Exists(StoragePath))return new PreviewProfile();
            byte[] ciphertext=File.ReadAllBytes(StoragePath);
            byte[] clear=ProtectedData.Unprotect(ciphertext,null,DataProtectionScope.CurrentUser);
            try {
                var stored=JsonSerializer.Deserialize<PreviewProfile>(clear);
                if(stored is null)return new PreviewProfile();
                var sanitized=new PreviewProfile{
                    Size=stored.Size==PreviewSizeMode.Comfortable
                        ?PreviewSizeMode.Comfortable:PreviewSizeMode.Compact};
                foreach(string value in stored.Favorites??new List<string>())
                    sanitized.AddFavorite(value);
                return sanitized;
            }
            finally {CryptographicOperations.ZeroMemory(clear);}
        } catch (Exception ex) when(
            ex is IOException or UnauthorizedAccessException or CryptographicException
                or JsonException or ArgumentException) {
            // Missing/invalid profile must not block startup or expose plaintext.
            return new PreviewProfile();
        }
    }

    public bool Save(PreviewProfile profile)
    {
        string temp=StoragePath+".pending";
        try {
            string? parent=Path.GetDirectoryName(StoragePath);
            if(!string.IsNullOrEmpty(parent)) Directory.CreateDirectory(parent);
            byte[] clear=JsonSerializer.SerializeToUtf8Bytes(profile);
            try {
                byte[] protectedBytes=ProtectedData.Protect(
                    clear,null,DataProtectionScope.CurrentUser);
                File.WriteAllBytes(temp,protectedBytes);
                File.Move(temp,StoragePath,true);
            } finally {CryptographicOperations.ZeroMemory(clear);}
            return true;
        } catch (Exception ex) when(
            ex is IOException or UnauthorizedAccessException or CryptographicException
                or ArgumentException) {
            try {if(File.Exists(temp))File.Delete(temp);}catch(IOException){}
            return false;
        }
    }

    public static PreviewProfileStore ForCurrentUser() =>
        new(Path.Combine(Environment.GetFolderPath(
            Environment.SpecialFolder.LocalApplicationData),
            "AXEVoice","WinUI3","preview-profile.dat"));
}

public readonly record struct NativeCallUiProjection(
    bool CanDial,bool CanAnswer,bool CanHangup,
    bool CanConnect,bool CanDisconnect,string StatusLabel);

public static class NativeCallUiProjector
{
    public static NativeCallUiProjection Project(
        bool bridgeAvailable,uint backendAttached,uint connection,uint view,
        uint nativeCanDial,uint nativeCanAnswer,uint nativeCanHangup,
        ulong session,ulong callId,bool destinationValid)
    {
        bool installed=bridgeAvailable && backendAttached==1;
        bool verified=installed && connection==2 && session!=0;
        bool idle=view==0;
        bool incoming=view==1;
        bool active=view==2 || view==3;
        string status=!bridgeAvailable?"● BRIDGE INDISPONÍVEL":
            !installed?"● MOTOR SIP NÃO INSTALADO":
            connection==1?"● REGISTRANDO SIP":
            !verified?"● SIP DESCONECTADO":
            incoming?"● CHAMADA RECEBIDA":
            view==2?"● CHAMANDO":
            view==3?"● EM CONVERSAÇÃO":
            "● SIP REGISTRADO";
        return new(
            verified && idle && callId==0 && nativeCanDial==1 && destinationValid,
            verified && incoming && callId!=0 && nativeCanAnswer==1,
            verified && (incoming || active) && callId!=0 && nativeCanHangup==1,
            installed && connection==0,
            installed && connection!=0 && idle,
            status);
    }
}

public static class SipDiagnosticReport
{
    // Fixed schema: no destinations, IP, account, passwords, SIP raw headers,
    // hostnames, logs or user-supplied data are copied to Clipboard.
    public static string Format(bool bridgeAvailable,bool backendAttached,
                                uint abiVersion,uint connection=0,uint view=0)
    {
        string state=!bridgeAvailable || !backendAttached
            ?"SIP indisponível"
            :connection==1?"Registrando SIP"
            :connection!=2?"Sem registro SIP confirmado"
            :view==1?"Chamada recebida"
            :view==2?"Chamada originada"
            :view==3?"Em conversação"
            :"Registro SIP confirmado";
        return "AXE Voice WinUI 3 — diagnóstico local\\n"+
            "Bridge C++: "+(bridgeAvailable?"carregada":"indisponível")+"\\n"+
            "ABI: "+(bridgeAvailable?abiVersion.ToString():"não disponível")+"\\n"+
            "Motor SIP: "+(backendAttached&&bridgeAvailable
                ?"SDK nativo disponível":"SIP indisponível")+"\\n"+
            "Estado: "+state+"\\n"+
            "Áudio/RTP: requer homologação física\\n"+
            "Dados de conta, destino e credenciais: não incluídos";
    }
}
