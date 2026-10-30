#include "contact_ui_actions.h"
#include "contact_backup.h"
#include <commdlg.h>
#include <string>
#include <vector>

namespace axv {
namespace {
struct ExportHandle {
 HANDLE value=INVALID_HANDLE_VALUE;
 explicit ExportHandle(HANDLE h) noexcept:value(h){}
 ~ExportHandle(){if(value!=INVALID_HANDLE_VALUE)CloseHandle(value);}
 ExportHandle(const ExportHandle&)=delete;
 ExportHandle& operator=(const ExportHandle&)=delete;
 bool valid()const noexcept{return value!=INVALID_HANDLE_VALUE;}
};
struct ExportTemp {
 std::wstring path;
 bool committed=false;
 ~ExportTemp(){if(!committed&&!path.empty())DeleteFileW(path.c_str());}
};
bool ordinary_export_file(HANDLE file,bool directory) noexcept {
 BY_HANDLE_FILE_INFORMATION info{};
 return GetFileType(file)==FILE_TYPE_DISK&&GetFileInformationByHandle(file,&info)&&
  !(info.dwFileAttributes&FILE_ATTRIBUTE_REPARSE_POINT)&&
  bool(info.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)==directory;
}
bool export_destination_allowed(const std::wstring& path) noexcept {
 ExportHandle file(CreateFileW(path.c_str(),FILE_READ_ATTRIBUTES,
  FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,
  FILE_FLAG_OPEN_REPARSE_POINT|FILE_FLAG_BACKUP_SEMANTICS,nullptr));
 if(file.valid())return ordinary_export_file(file.value,false);
 return GetLastError()==ERROR_FILE_NOT_FOUND;
}
}
ContactTransferResult export_contacts(HWND owner,const ContactBook& book) noexcept {
 const ContactTransferResult error{ContactTransferStatus::Error};
 try {
  auto bytes=encode_contact_backup(book);
  if(!bytes||bytes->size()>contact_backup_limit)return error;
  std::vector<wchar_t> filename(32768,L'\0');
  const std::wstring initial=L"contatos-backup.json";
  initial.copy(filename.data(),initial.size());
  OPENFILENAMEW dialog{};
  dialog.lStructSize=sizeof(dialog);
  dialog.hwndOwner=owner;
  dialog.lpstrFilter=L"Backup de contatos (*.json)\0*.json\0\0";
  dialog.nFilterIndex=1;
  dialog.lpstrFile=filename.data();
  dialog.nMaxFile=static_cast<DWORD>(filename.size());
  dialog.lpstrDefExt=L"json";
  dialog.lpstrTitle=L"Exportar contatos";
  dialog.Flags=OFN_OVERWRITEPROMPT|OFN_PATHMUSTEXIST|OFN_NOCHANGEDIR;
  if(!GetSaveFileNameW(&dialog))return CommDlgExtendedError()==0?
   ContactTransferResult{ContactTransferStatus::Cancelled}:error;
  const std::wstring destination(filename.data());
  const auto separator=destination.find_last_of(L"\\/");
  if(separator==std::wstring::npos)return error;
  // Retain the directory against rename/removal while writing the sibling temp.
  const auto parent=destination.substr(0,separator+1);
  ExportHandle directory(CreateFileW(parent.c_str(),FILE_READ_ATTRIBUTES,
   FILE_SHARE_READ|FILE_SHARE_WRITE,nullptr,OPEN_EXISTING,
   FILE_FLAG_BACKUP_SEMANTICS|FILE_FLAG_OPEN_REPARSE_POINT,nullptr));
  if(!directory.valid()||!ordinary_export_file(directory.value,true)||
   !export_destination_allowed(destination))return error;
  ExportTemp temp;
  for(unsigned attempt=0;attempt<128;++attempt){
   auto candidate=parent+L".axv-contact-export."+std::to_wstring(GetCurrentProcessId())+
    L"."+std::to_wstring(GetTickCount64())+L"."+std::to_wstring(attempt)+L".tmp";
   ExportHandle file(CreateFileW(candidate.c_str(),GENERIC_WRITE,0,nullptr,
    CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr));
   if(!file.valid()){
    const auto code=GetLastError();
    if(code==ERROR_FILE_EXISTS||code==ERROR_ALREADY_EXISTS)continue;
    return error;
   }
   // Only a successful CREATE_NEW grants cleanup ownership of this path.
   temp.path.swap(candidate);
   DWORD written=0;
   if(!WriteFile(file.value,bytes->data(),static_cast<DWORD>(bytes->size()),&written,nullptr)||
    written!=bytes->size()||!FlushFileBuffers(file.value))return error;
   const HANDLE completed=file.value;
   file.value=INVALID_HANDLE_VALUE;
   if(!CloseHandle(completed))return error;
   break;
  }
  if(temp.path.empty()||!export_destination_allowed(destination))return error;
  if(!MoveFileExW(temp.path.c_str(),destination.c_str(),
   MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))return error;
  temp.committed=true;
  return {ContactTransferStatus::Ok};
 }catch(...){return error;}
}
}
