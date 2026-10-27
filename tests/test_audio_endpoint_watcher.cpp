#include "audio_endpoint_watcher_win32.h"
#include <objbase.h>
#include <mmdeviceapi.h>
#include <cstdio>
#include <cstdlib>
#define CHECK(x) do { if (!(x)) { std::fprintf(stderr,"%s:%d: %s\n",__FILE__,__LINE__,#x); std::abort(); } } while(false)
#include <string>
#include <vector>
#include <iostream>
namespace {
HRESULT init_result=S_OK, create_result=S_OK, register_result=S_OK;
int initializes=0, uninitializes=0, creates=0, registrations=0;
bool null_create=false;
std::vector<std::string> events;
IMMNotificationClient* current=nullptr;
struct Enumerator final : IMMDeviceEnumerator {
 HRESULT QueryInterface(REFIID,void**) override { return E_NOINTERFACE; }
 ULONG AddRef() override { return 2; }
 ULONG Release() override { events.push_back("release"); delete this; return 0; }
 HRESULT RegisterEndpointNotificationCallback(IMMNotificationClient* sink) override {
  ++registrations; events.push_back("register");
  if (FAILED(register_result)) return register_result;
  current=sink; return S_OK;
 }
 HRESULT UnregisterEndpointNotificationCallback(IMMNotificationClient* sink) override {
  CHECK(current==sink); events.push_back("unregister");
  // A reentrant notification during teardown must already be harmless.
  CHECK(sink->OnDeviceAdded(nullptr)==S_OK);
  current=nullptr; return S_OK;
 }
};
void reset() {
 CHECK(!current); init_result=create_result=register_result=S_OK;
 initializes=uninitializes=creates=registrations=0; null_create=false; events.clear();
}
void failures() {
 reset(); init_result=RPC_E_CHANGED_MODE;
 { axv::AudioEndpointWatcher w; CHECK(!w.start()); CHECK(!w.active()); CHECK(!w.take_changed()); w.stop(); }
 CHECK(initializes==1 && creates==0 && uninitializes==0);
 reset(); create_result=E_FAIL;
 { axv::AudioEndpointWatcher w; CHECK(!w.start()); CHECK(!w.active()); }
 CHECK(uninitializes==1 && registrations==0);
 reset(); null_create=true;
 { axv::AudioEndpointWatcher w; CHECK(!w.start()); }
 CHECK(uninitializes==1);
 reset(); register_result=E_FAIL;
 { axv::AudioEndpointWatcher w; CHECK(!w.start()); CHECK(!w.active()); }
 CHECK((events==std::vector<std::string>{"initialize","create","register","release","uninitialize"}));
}
void callbacks_and_lifetime() {
 reset(); init_result=S_FALSE;
 axv::AudioEndpointWatcher w;
 CHECK(!w.active() && !w.take_changed());
 CHECK(w.start() && w.start());
 CHECK(initializes==1 && creates==1 && registrations==1);
 auto* sink=current;
 CHECK(sink->QueryInterface(IID_IUnknown,nullptr)==E_POINTER);
 void* out=reinterpret_cast<void*>(1);
 CHECK(sink->QueryInterface(GUID{99},&out)==E_NOINTERFACE && !out);
 for (auto iid : {IID_IUnknown,IID_IMMNotificationClient}) {
  CHECK(sink->QueryInterface(iid,&out)==S_OK && out==sink);
  CHECK(static_cast<IMMNotificationClient*>(out)->Release()==1);
 }
 CHECK(sink->AddRef()==2); CHECK(sink->Release()==1);
 CHECK(sink->OnDeviceStateChanged(nullptr,0)==S_OK); CHECK(w.take_changed()); CHECK(!w.take_changed());
 CHECK(sink->OnDeviceAdded(nullptr)==S_OK); CHECK(w.take_changed());
 CHECK(sink->OnDeviceRemoved(nullptr)==S_OK); CHECK(w.take_changed());
 CHECK(sink->OnPropertyValueChanged(nullptr,PROPERTYKEY{})==S_OK); CHECK(w.take_changed());
 for(auto flow : {eRender,eCapture,eAll}) for(auto role : {eConsole,eMultimedia,eCommunications}) {
  CHECK(sink->OnDefaultDeviceChanged(flow,role,nullptr)==S_OK);
  CHECK(w.take_changed()==(flow!=eAll && role!=eConsole));
 }
 sink->OnDeviceAdded(nullptr); sink->OnDeviceRemoved(nullptr); CHECK(w.take_changed()); CHECK(!w.take_changed());
 sink->AddRef(); // Simulate a callback retained across unregister and restart.
 events.clear(); w.stop(); w.stop();
 CHECK(!w.active() && !w.take_changed());
 CHECK((events==std::vector<std::string>{"unregister","release","uninitialize"}));
 CHECK(uninitializes==1);
 CHECK(w.start()); CHECK(current!=sink);
 sink->OnDeviceAdded(nullptr); sink->OnDeviceStateChanged(nullptr,1);
 sink->OnDefaultDeviceChanged(eCapture,eCommunications,nullptr);
 CHECK(!w.take_changed());
 current->OnDeviceAdded(nullptr); CHECK(w.take_changed());
 CHECK(sink->Release()==0);
 w.stop(); CHECK(uninitializes==2);
}
}
HRESULT CoInitializeEx(void* reserved,DWORD flags) {
 CHECK(!reserved && flags==COINIT_APARTMENTTHREADED);
 ++initializes; events.push_back("initialize"); return init_result;
}
void CoUninitialize() { ++uninitializes; events.push_back("uninitialize"); }
HRESULT CoCreateInstance(REFIID cls,IUnknown* outer,DWORD flags,REFIID iid,void** out) {
 CHECK(IsEqualIID(cls,CLSID_MMDeviceEnumerator) && !outer && flags==CLSCTX_INPROC_SERVER);
 CHECK(IsEqualIID(iid,IID_IMMDeviceEnumerator)); ++creates; events.push_back("create"); *out=nullptr;
 if(FAILED(create_result)) return create_result;
 if(!null_create) *out=new Enumerator;
 return S_OK;
}
int main() { failures(); callbacks_and_lifetime(); std::cout << "audio endpoint watcher tests passed\n"; }
