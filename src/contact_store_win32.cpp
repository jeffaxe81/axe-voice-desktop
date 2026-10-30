#include "contact_store.h"
#include "contact_backup.h"
#include <windows.h>
#include <shlobj.h>
#include <utility>

namespace axv {
namespace {
struct Handle {
 HANDLE value=INVALID_HANDLE_VALUE;
 explicit Handle(HANDLE h=INVALID_HANDLE_VALUE) noexcept:value(h){}
 ~Handle(){if(value!=INVALID_HANDLE_VALUE)CloseHandle(value);}
 Handle(const Handle&)=delete;
 Handle& operator=(const Handle&)=delete;
 bool valid()const noexcept{return value!=INVALID_HANDLE_VALUE;}
};
bool missing(DWORD e){return e==ERROR_FILE_NOT_FOUND||e==ERROR_PATH_NOT_FOUND;}
std::wstring parent(const std::wstring& p){auto i=p.find_last_of(L"\\/");return i==std::wstring::npos?L".":p.substr(0,i);}
bool ordinary(HANDLE h,bool directory){
 BY_HANDLE_FILE_INFORMATION info{};
 return GetFileInformationByHandle(h,&info)&&!(info.dwFileAttributes&FILE_ATTRIBUTE_REPARSE_POINT)&&
  bool(info.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)==directory;
}
// Keep the parent directory open without delete sharing until the operation ends.
HANDLE open_directory(const std::wstring& dir){return CreateFileW(dir.c_str(),FILE_READ_ATTRIBUTES,FILE_SHARE_READ|FILE_SHARE_WRITE,nullptr,OPEN_EXISTING,FILE_FLAG_BACKUP_SEMANTICS|FILE_FLAG_OPEN_REPARSE_POINT,nullptr);}
bool read_file(const std::wstring& path,std::string& bytes,bool& exists){
 Handle file(CreateFileW(path.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_FLAG_OPEN_REPARSE_POINT,nullptr));
 if(!file.valid()){if(missing(GetLastError())){exists=false;bytes.clear();return true;}return false;}
 if(!ordinary(file.value,false))return false;
 LARGE_INTEGER size{};
 if(!GetFileSizeEx(file.value,&size)||size.QuadPart<0||size.QuadPart>static_cast<LONGLONG>(contact_backup_limit))return false;
 bytes.resize(static_cast<std::size_t>(size.QuadPart));
 DWORD got=0;
 if(!bytes.empty()&&(!ReadFile(file.value,bytes.data(),static_cast<DWORD>(bytes.size()),&got,nullptr)||got!=bytes.size()))return false;
 exists=true;return true;
}
struct Temp {
 std::wstring path;
 bool committed=false;
 ~Temp(){if(!committed&&!path.empty())DeleteFileW(path.c_str());}
};
}
ContactStore::ContactStore(std::wstring path):path_(std::move(path)){
 if(path_.empty()){
  PWSTR local=nullptr;
  if(SUCCEEDED(SHGetKnownFolderPath(FOLDERID_LocalAppData,0,nullptr,&local))){
   try {path_=std::wstring(local)+L"\\AXEVoiceNativeCandidate\\contacts.json";}
   catch(...){CoTaskMemFree(local);throw;}
  }
  CoTaskMemFree(local);
 }
}
ContactStoreResult ContactStore::load(ContactBook& book) noexcept {
 loaded_=false;
 try {
  if(path_.empty())return ContactStoreResult::Error;
  auto dirpath=parent(path_);
  Handle dir(open_directory(dirpath));
  if(!dir.valid()&&!missing(GetLastError()))return ContactStoreResult::Error;
  if(dir.valid()&&!ordinary(dir.value,true))return ContactStoreResult::Error;
  std::string bytes;bool exists=false;
  if(!read_file(path_,bytes,exists))return ContactStoreResult::Error;
  ContactBook replacement;
  if(exists){
   auto drafts=decode_contact_backup(bytes);
   if(!drafts)return ContactStoreResult::Error;
   for(const auto& draft:*drafts)if(replacement.add(draft)!=ContactResult::Ok)return ContactStoreResult::Error;
  }
  book.swap(replacement);
  baseline_.swap(bytes);existed_=exists;loaded_=true;
  return ContactStoreResult::Ok;
 }catch(...){return ContactStoreResult::Error;}
}
ContactStoreResult ContactStore::save(const ContactBook& book) noexcept {
 if(!loaded_)return ContactStoreResult::Error;
 try {
  auto bytes=encode_contact_backup(book);
  if(!bytes||bytes->size()>contact_backup_limit)return ContactStoreResult::Error;
  auto dirpath=parent(path_);
  // Creation is confined to the immediate product directory, and only on save.
  if(!CreateDirectoryW(dirpath.c_str(),nullptr)&&GetLastError()!=ERROR_ALREADY_EXISTS)return ContactStoreResult::Error;
  Handle dir(open_directory(dirpath));
  if(!dir.valid()||!ordinary(dir.value,true))return ContactStoreResult::Error;
  auto lockpath=path_+L".lock";
  Handle lock(CreateFileW(lockpath.c_str(),GENERIC_READ|GENERIC_WRITE,0,nullptr,OPEN_ALWAYS,FILE_FLAG_OPEN_REPARSE_POINT,nullptr));
  if(!lock.valid()||!ordinary(lock.value,false))return ContactStoreResult::Error;
  std::string current;bool exists=false;
  if(!read_file(path_,current,exists))return ContactStoreResult::Error;
  if(exists!=existed_||current!=baseline_)return ContactStoreResult::Conflict;
  Temp temp;
  // CREATE_NEW protects against collisions with other instances and stale files.
  for(unsigned attempt=0;attempt<128;++attempt){
   auto candidate=path_+L".tmp."+std::to_wstring(GetCurrentProcessId())+L"."+
    std::to_wstring(GetTickCount64())+L"."+std::to_wstring(attempt);
   Handle file(CreateFileW(candidate.c_str(),GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr));
   if(!file.valid()){
    if(GetLastError()==ERROR_FILE_EXISTS||GetLastError()==ERROR_ALREADY_EXISTS)continue;
    return ContactStoreResult::Error;
   }
   temp.path.swap(candidate);
   DWORD written=0;
   if(!WriteFile(file.value,bytes->data(),static_cast<DWORD>(bytes->size()),&written,nullptr)||written!=bytes->size()||!FlushFileBuffers(file.value))return ContactStoreResult::Error;
   break;
  }
  if(temp.path.empty())return ContactStoreResult::Error;
  if(!MoveFileExW(temp.path.c_str(),path_.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))return ContactStoreResult::Error;
  temp.committed=true;
  baseline_.swap(*bytes);existed_=true;
  return ContactStoreResult::Ok;
 }catch(...){return ContactStoreResult::Error;}
}
}
