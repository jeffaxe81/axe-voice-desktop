// Exercise the real window procedure with a modal nested message loop. Including
// its implementation lets this fixture seed RAM-only contacts without touching
// a person's persisted contact file or introducing a production test hook.
#include <windows.h>
#include <cstdio>
#include <cstdlib>
#include <new>
namespace {
void* watched_contacts=nullptr;
bool nested_close=false,freed_during_close=false,timer_ran=false;
void released(void* p){if(p==watched_contacts&&nested_close)freed_during_close=true;std::free(p);}
}
void* operator new(std::size_t n){if(void*p=std::malloc(n?n:1))return p;throw std::bad_alloc();}
void operator delete(void*p)noexcept{released(p);}
void operator delete(void*p,std::size_t)noexcept{released(p);}
#include "../src/contact_dialog_win32.cpp"
namespace {
BOOL CALLBACK dismiss(HWND window,LPARAM){
 wchar_t type[32]{};GetClassNameW(window,type,32);
 if(lstrcmpW(type,L"#32770")==0)EndDialog(window,IDNO);
 return TRUE;
}
void CALLBACK close_owner(HWND owner,UINT,UINT_PTR id,DWORD){
 KillTimer(owner,id);timer_ran=true;nested_close=true;
 axv::close_contact_book();DestroyWindow(owner);
 EnumThreadWindows(GetCurrentThreadId(),dismiss,0);
 nested_close=false;
}
}
#define CHECK(x) do{if(!(x)){std::fprintf(stderr,"contact shutdown line%d: %s\n",__LINE__,#x);return 1;}}while(false)
int main(){
 HWND owner=CreateWindowExW(0,L"STATIC",L"Contact shutdown fixture",WS_OVERLAPPEDWINDOW,0,0,400,400,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);
 CHECK(owner);
 HWND edit=CreateWindowExW(0,L"EDIT",L"",WS_CHILD,0,0,100,25,owner,nullptr,GetModuleHandleW(nullptr),nullptr);CHECK(edit);
 axv::show_contact_book(owner,edit);
 HWND contacts=FindWindowW(L"AXVContactBookV1",nullptr);CHECK(contacts&&axv::state);
 axv::ContactBook seed;
 CHECK(seed.add({L"A",L"1001",false})==axv::ContactResult::Ok);
 CHECK(seed.add({L"B",L"1002",false})==axv::ContactResult::Ok);
 axv::state->book.swap(seed);axv::refresh(*axv::state);
 HWND list=GetDlgItem(contacts,103),name=GetDlgItem(contacts,104);CHECK(list&&name);
 SendMessageW(list,LB_SETCURSEL,0,0);
 SendMessageW(contacts,WM_COMMAND,MAKEWPARAM(103,LBN_SELCHANGE),reinterpret_cast<LPARAM>(list));
 CHECK(SetWindowTextW(name,L"Unsaved edit"));
 watched_contacts=const_cast<axv::Contact*>(axv::state->book.items().data());
 CHECK(SetTimer(owner,1,20,close_owner));
 SendMessageW(list,LB_SETCURSEL,1,0);
 SendMessageW(contacts,WM_COMMAND,MAKEWPARAM(103,LBN_SELCHANGE),reinterpret_cast<LPARAM>(list));
 watched_contacts=nullptr;
 CHECK(timer_ran&&!freed_during_close);
 CHECK(!IsWindow(contacts)&&!IsWindow(owner));
 CHECK(!axv::state);
 axv::ContactBook current,untouched;axv::ContactStore store;
 CHECK(untouched.add({L"Keep",L"1003",false})==axv::ContactResult::Ok);
 CHECK(axv::import_contacts(owner,current,untouched,store).status==axv::ContactTransferStatus::Cancelled);
 CHECK(untouched.items().size()==1&&untouched.items()[0].destination==L"1003");
 CHECK(axv::export_contacts(owner,current).status==axv::ContactTransferStatus::Cancelled);
 std::puts("Contact window state survived nested owner shutdown; modal action cancelled safely");
}
