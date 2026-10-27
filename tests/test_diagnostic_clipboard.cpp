#include "../src/diagnostic_clipboard_win32.h"
#include "../src/diagnostic_export.h"
#include <cstdlib>
#include <cstring>
#include <string>
#define CHECK(x) do{if(!(x))std::abort();}while(0)
static int fail=0,allocated=0,freed=0,opened=0,closed=0,emptied=0,transferred=0;
static bool locked=false;
static void* memory=nullptr;
static std::size_t bytes=0;
static std::wstring copied;
extern "C" HGLOBAL GlobalAlloc(unsigned flags,std::size_t size){CHECK(flags==GMEM_MOVEABLE);if(fail==1)return nullptr;++allocated;bytes=size;memory=std::malloc(size);return memory;}
extern "C" void* GlobalLock(HGLOBAL h){CHECK(h==memory);if(fail==2)return nullptr;locked=true;return h;}
extern "C" int GlobalUnlock(HGLOBAL h){CHECK(h==memory && locked);locked=false;return 0;}
extern "C" HGLOBAL GlobalFree(HGLOBAL h){CHECK(h==memory && !locked);++freed;std::free(h);memory=nullptr;return nullptr;}
extern "C" int OpenClipboard(HWND owner){CHECK(owner && !locked);if(fail==3)return 0;++opened;return 1;}
extern "C" int EmptyClipboard(){CHECK(opened && !locked);++emptied;return fail!=4;}
extern "C" HANDLE SetClipboardData(unsigned format,HANDLE h){
 CHECK(format==CF_UNICODETEXT && opened && emptied && !locked && h==memory);
 if(fail==5)return nullptr;
 ++transferred;copied=static_cast<const wchar_t*>(h);
 CHECK(bytes==(copied.size()+1)*sizeof(wchar_t));return h;
}
extern "C" int CloseClipboard(){CHECK(opened);++closed;return 1;}
int main(){
 auto reset=[](){CHECK(memory==nullptr);fail=allocated=freed=opened=closed=emptied=transferred=0;copied.clear();};
 CHECK(!axv::copy_call_diagnostic(nullptr,{}));CHECK(!allocated && !opened);
 auto owner=reinterpret_cast<HWND>(1);
 for(int error=1;error<=5;++error){reset();fail=error;CHECK(!axv::copy_call_diagnostic(owner,{}));
  CHECK(!memory && !locked && allocated==freed && opened==closed && !transferred);
  if(error<=3)CHECK(!emptied);
 }
 reset();axv::CallReadiness ready{true,true,true,true,true,false,true};
 CHECK(axv::copy_call_diagnostic(owner,ready));
 CHECK(allocated==1 && !freed && opened==1 && closed==1 && transferred==1 && !locked);
 CHECK(copied==axv::format_diagnostic_export(ready));
 // Simulated system clipboard owns/frees the successful allocation.
 std::free(memory);memory=nullptr;
 reset();const axv::NativeSecurityStatus security{true,true,true,true};
 CHECK(axv::copy_call_diagnostic(owner,ready,security));
 CHECK(copied==axv::format_diagnostic_export(ready,security));
 CHECK(copied.find(L"TLS obrigatorio; CA confiavel exigida")!=copied.npos);
 CHECK(copied.find(L"Registro SIP atual: verificado")!=copied.npos);
 std::free(memory);memory=nullptr;
}
