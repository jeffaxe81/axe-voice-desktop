// SDK-enabled WinUI 3 host smoke. No credentials, REGISTER or external SIP.
#include "../src/frontend_shared/axv_frontend_api.h"
#include <cstdio>
#include <cstdint>
int main(){
    if(axv_frontend_abi_version()!=2)return 1;
    AxvFrontendSnapshot snapshot{};
    if(axv_frontend_get_snapshot(&snapshot,sizeof(snapshot))!=AXV_FRONTEND_OK)
        return 2;
    if(snapshot.backend_attached!=1 || snapshot.connection!=0 ||
       snapshot.can_dial || snapshot.can_answer || snapshot.can_hangup ||
       snapshot.session || snapshot.call_id)return 3;
    if(axv_frontend_dial(L"1001",4,123)!=AXV_FRONTEND_ERROR_REJECTED)
        return 4;
    if(axv_frontend_answer(123,456)!=AXV_FRONTEND_ERROR_REJECTED)
        return 5;
    if(axv_frontend_hangup(123,456)!=AXV_FRONTEND_ERROR_REJECTED)
        return 6;
    if(axv_frontend_disconnect()!=AXV_FRONTEND_OK)return 7;
    if(axv_frontend_get_snapshot(&snapshot,sizeof(snapshot))!=AXV_FRONTEND_OK ||
       snapshot.connection!=0 || snapshot.can_dial || snapshot.can_hangup)return 8;
    std::puts("SDK WinUI3 candidate host: idle, no secrets, commands rejected, shutdown OK");
    return 0;
}
