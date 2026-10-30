#include <windows.h>
#include "../src/contact_store.h"
#include "../src/contact_ui_actions.h"
#include <cstdio>
#define CHECK(x) do{if(!(x)){std::fprintf(stderr,"contact store line%d: %s\n",__LINE__,#x);return 1;}}while(false)
namespace {
struct Files {std::wstring dir,path;~Files(){DeleteFileW(path.c_str());DeleteFileW((path+L".lock").c_str());RemoveDirectoryW(dir.c_str());}};
}
int main(){
 using namespace axv;
 wchar_t root[MAX_PATH]{};CHECK(GetTempPathW(MAX_PATH,root));
 Files files{std::wstring(root)+L"AXEContacts."+std::to_wstring(GetCurrentProcessId()),L""};
 CHECK(CreateDirectoryW(files.dir.c_str(),nullptr));files.path=files.dir+L"\\contacts.json";
 ContactBook book,other;ContactStore store(files.path),concurrent(files.path);
 CHECK(store.load(book)==ContactStoreResult::Ok&&book.items().empty());
 CHECK(concurrent.load(other)==ContactStoreResult::Ok);
 CHECK(book.add({L"Jos\u00e9",L"1001",true})==ContactResult::Ok);
 CHECK(store.save(book)==ContactStoreResult::Ok);
 CHECK(other.add({L"Outra",L"1002",false})==ContactResult::Ok);
 CHECK(concurrent.save(other)==ContactStoreResult::Conflict);
 ContactStore reload(files.path);ContactBook loaded;
 CHECK(reload.load(loaded)==ContactStoreResult::Ok&&loaded.items().size()==1&&loaded.items()[0].name==L"Jos\u00e9");
 HWND edit=CreateWindowExW(0,L"EDIT",L"original",WS_POPUP|ES_AUTOHSCROLL,0,0,200,30,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);
 CHECK(edit);SendMessageW(edit,EM_SETLIMITTEXT,128,0);
 CHECK(use_contact_destination(edit,loaded.items()[0]));
 wchar_t text[129]{};GetWindowTextW(edit,text,129);CHECK(std::wstring(text)==L"1001");
 SendMessageW(edit,EM_SETLIMITTEXT,2,0);
 CHECK(!use_contact_destination(edit,loaded.items()[0]));
 GetWindowTextW(edit,text,129);CHECK(std::wstring(text)==L"1001");
 DestroyWindow(edit);
 HANDLE lock=CreateFileW((files.path+L".lock").c_str(),GENERIC_READ|GENERIC_WRITE,0,nullptr,OPEN_EXISTING,0,nullptr);
 CHECK(lock!=INVALID_HANDLE_VALUE);
 const auto blocked=reload.save(loaded);CloseHandle(lock);CHECK(blocked==ContactStoreResult::Error);
 HANDLE corrupt=CreateFileW(files.path.c_str(),GENERIC_WRITE,0,nullptr,TRUNCATE_EXISTING,0,nullptr);CHECK(corrupt!=INVALID_HANDLE_VALUE);
 DWORD written=0;const bool wrote=WriteFile(corrupt,"invalid",7,&written,nullptr)!=FALSE;CloseHandle(corrupt);CHECK(wrote&&written==7);
 CHECK(reload.load(loaded)==ContactStoreResult::Error&&loaded.items().size()==1);
 CHECK(reload.save(loaded)==ContactStoreResult::Error);
 std::puts("Contact persistence, conflict preservation, failed-load protection and dialer prefill passed");
}
