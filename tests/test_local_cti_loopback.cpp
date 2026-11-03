#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <winsock2.h>
#include <ws2tcpip.h>
#include "../src/local_cti_server_win32.h"
#include <cstdio>
#include <string>
#include <string_view>
#define CHECK(x) do {if(!(x)){std::fprintf(stderr,"CTI HTTP check %d: %s\n",__LINE__,#x);return 1;}} while(false)

std::string http_request(std::string_view request) {
    SOCKET socket_fd=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
    if(socket_fd==INVALID_SOCKET)return {};
    sockaddr_in endpoint{};
    endpoint.sin_family=AF_INET;
    endpoint.sin_addr.s_addr=htonl(INADDR_LOOPBACK);
    endpoint.sin_port=htons(axv::LocalCtiServer::port);
    DWORD timeout=3000;
    setsockopt(socket_fd,SOL_SOCKET,SO_RCVTIMEO,
        reinterpret_cast<const char*>(&timeout),sizeof(timeout));
    if(connect(socket_fd,reinterpret_cast<const sockaddr*>(&endpoint),sizeof(endpoint))!=0){
        closesocket(socket_fd);return {};
    }
    const std::string wire(request);
    std::size_t sent=0;
    while(sent<wire.size()){
        const int n=send(socket_fd,wire.data()+sent,
                         static_cast<int>(wire.size()-sent),0);
        if(n<=0){closesocket(socket_fd);return {};}
        sent+=static_cast<std::size_t>(n);
    }
    std::string answer;
    char bytes[1024];
    while(answer.size()<8192){
        const int got=recv(socket_fd,bytes,sizeof(bytes),0);
        if(got<=0)break;
        answer.append(bytes,static_cast<std::size_t>(got));
    }
    closesocket(socket_fd);
    return answer;
}
std::string request(std::string_view method_and_path,std::string_view body,
                    std::string_view token,std::string_view extra_headers={}) {
    std::string text(method_and_path);
    text+=" HTTP/1.1\r\nHost: 127.0.0.1:27652\r\n";
    if(!token.empty())text+="Authorization: Bearer "+std::string(token)+"\r\n";
    text+=extra_headers;
    if(method_and_path.substr(0,4)=="POST")
        text+="Content-Type: application/json\r\nContent-Length: "+
              std::to_string(body.size())+"\r\n";
    text+="\r\n";
    text+=body;
    return http_request(text);
}
bool code(std::string_view response,int number){
    return response.find("HTTP/1.1 "+std::to_string(number)+" ")==0;
}
int main() {
    constexpr std::string_view token="ABCDEFGHIJKLMNOPQRSTUVWXYZ123456";
    WSADATA startup{};
    CHECK(WSAStartup(MAKEWORD(2,2),&startup)==0);
    {
        axv::LocalCtiServer service;
        CHECK(!service.start("invalid"));
        CHECK(service.start(std::string(token)));
        CHECK(service.active());
        // Read-only CTI endpoints deny unauthenticated/browser requests.
        CHECK(code(request("GET /v1/calls/current","",{}),401));
        CHECK(code(request("GET /v1/events","",{}),401));
        CHECK(code(request("GET /v1/request-events","",{}),401));
        CHECK(code(request("GET /v1/request-events?after=2","",{}),401));
        CHECK(code(request("GET /v1/request-events?after=oops","",token),400));
        CHECK(code(request("GET /v1/request-events?after=18446744073709551616","",token),400));
        CHECK(code(request("GET /v1/request-events","",token,
                           "Origin: https://crm.example.org\r\n"),403));
        CHECK(code(request("GET /v1/request-events","",token,
                           "Content-Length: 0\r\n"),400));
        const auto empty_requests=request("GET /v1/request-events","",token);
        CHECK(code(empty_requests,200) &&
              empty_requests.find(R"("events":[])")!=std::string::npos);
        CHECK(code(request("GET /v1/requests/1","",{}),401));
        CHECK(code(request("DELETE /v1/requests/1","",{}),401));
        CHECK(code(request("DELETE /v1/requests/not-a-number","",token),400));
        CHECK(code(request("DELETE /v1/requests/0","",token),400));
        CHECK(code(request("DELETE /v1/requests/1","",token),404));
        CHECK(code(request("DELETE /v1/requests/1","",token,
                           "Origin: https://crm.example.org\r\n"),403));
        CHECK(code(request("GET /v1/requests/0","",token),400));
        CHECK(code(request("GET /v1/requests/invalid","",token),400));
        CHECK(code(request("GET /v1/requests/1","",token),404));
        CHECK(code(request("GET /v1/requests/1","",token,
                           "Origin: https://crm.example.org\r\n"),403));
        CHECK(code(request("GET /v1/events?after=garbage","",token),400));
        CHECK(code(request("GET /v1/events?after=18446744073709551616","",token),400));
        CHECK(code(request("GET /v1/calls/current","",token,
                           "Origin: https://crm.example.org\r\n"),403));
        CHECK(code(request("GET /v1/events","",token,
                           "Referer: https://crm.example.org\r\n"),403));
        CHECK(code(request("GET /v1/calls/current","",token,
                           "Content-Length: 0\r\n"),400));
        auto first_status=request("GET /v1/calls/current","",token);
        CHECK(code(first_status,200) &&
              first_status.find("\"phase\":\"offline\"")!=std::string::npos);
        axv::SipRuntimeObservation observed{};
        observed.process=axv::RuntimeProcess::Running;
        observed.sip=axv::SipStatus::Registered;
        observed.session=1;
        service.observe(observed);
        const auto unverified=request("GET /v1/calls/current","",token);
        CHECK(code(unverified,200) &&
              unverified.find("\"phase\":\"unverified\"")!=std::string::npos);
        observed.evidence=axv::SipEvidence::VerifiedNativeApi;
        service.observe(observed);
        const auto registered=request("GET /v1/calls/current","",token);
        CHECK(code(registered,200) &&
              registered.find("\"registered\":true")!=std::string::npos &&
              registered.find("\"active_call\":false")!=std::string::npos);
        observed.sip=axv::SipStatus::InCall;
        observed.call_id=88;
        observed.last_call_id=88;
        service.observe(observed);
        const auto active=request("GET /v1/calls/current","",token);
        CHECK(code(active,200) &&
              active.find("\"phase\":\"in_call\"")!=std::string::npos &&
              active.find("\"active_call\":true")!=std::string::npos &&
              active.find("88")==std::string::npos);
        const auto events=request("GET /v1/events?after=2","",token);
        CHECK(code(events,200) &&
              events.find("\"phase\":\"in_call\"")!=std::string::npos &&
              events.find("\"sequence\":3")!=std::string::npos &&
              events.find("\"sequence\":1") ==std::string::npos);
        const auto nothing=request("GET /v1/events?after=3","",token);
        CHECK(code(nothing,200) &&
              nothing.find("\"events\":[]")!=std::string::npos);
        CHECK(code(request("GET /v1/status","",token),200));
        CHECK(code(request("GET /v1/status","",{}),401));
        CHECK(code(request("POST /v1/calls",R"({"destination":"1001"})",{}),401));
        CHECK(code(request("POST /v1/calls",R"({"destination":"1001"})",token,
                           "Origin: https://crm.example.org\r\n"),403));
        CHECK(code(request("POST /v1/calls",R"({"destination":"1001"})",token,
                           "Transfer-Encoding: chunked\r\n"),403));
        CHECK(code(request("POST /v1/calls",R"({"destination":"1001"})",token),409));
        service.set_ready(true);
        CHECK(code(request("POST /v1/calls",R"({"destination":"abc"})",token),422));
        const auto first_post=request("POST /v1/calls",R"({"destination":"1001"})",token);
        CHECK(code(first_post,202) &&
              first_post.find(R"("request_id":1)")!=std::string::npos &&
              first_post.find(R"("status":"queued")")!=std::string::npos);
        auto request_events=request("GET /v1/request-events?after=0","",token);
        CHECK(code(request_events,200) &&
              request_events.find(R"("sequence":1,"request_id":1,"status":"queued")")!=std::string::npos &&
              request_events.find("1001")==std::string::npos);
        CHECK(code(request("GET /v1/request-events?after=1","",token),200));
        CHECK(code(request("POST /v1/calls",R"({"destination":"1002"})",token),429));
        auto queued=request("GET /v1/requests/1","",token);
        CHECK(code(queued,200) && queued.find(R"("status":"queued")")!=std::string::npos);
        auto call=service.take();
        CHECK(call && call->destination==L"1001" && call->id==1);
        CHECK(!service.take());
        // Taking a request never frees the single inflight slot.
        CHECK(code(request("POST /v1/calls",R"({"destination":"1002"})",token),429));
        const auto dispatching=request("GET /v1/requests/1","",token);
        CHECK(code(dispatching,200) &&
              dispatching.find(R"("status":"dispatching")")!=std::string::npos);
        request_events=request("GET /v1/request-events?after=1","",token);
        CHECK(code(request_events,200) &&
              request_events.find(R"("sequence":2,"request_id":1,"status":"dispatching")")!=std::string::npos);
        CHECK(!service.complete(2,true));
        CHECK(code(request("DELETE /v1/requests/1","",token),409));
        CHECK(service.complete(1,true));
        CHECK(!service.complete(1,true));
        const auto submitted=request("GET /v1/requests/1","",token);
        CHECK(code(submitted,200) &&
              submitted.find(R"("status":"submitted")")!=std::string::npos &&
              submitted.find("1001")==std::string::npos);
        request_events=request("GET /v1/request-events?after=2","",token);
        CHECK(code(request_events,200) &&
              request_events.find(R"("sequence":3,"request_id":1,"status":"submitted")")!=std::string::npos);
        const auto second_post=request("POST /v1/calls",R"({"destination":"1002"})",token);
        CHECK(code(second_post,202) &&
              second_post.find(R"("request_id":2)")!=std::string::npos);
        call=service.take();
        CHECK(call && call->destination==L"1002" && call->id==2);
        CHECK(service.complete(2,false));
        const auto rejected=request("GET /v1/requests/2","",token);
        CHECK(code(rejected,200) &&
              rejected.find(R"("status":"rejected")")!=std::string::npos);
        CHECK(code(request("DELETE /v1/requests/2","",token),409));
        const auto third=request("POST /v1/calls",R"({"destination":"1003"})",token);
        CHECK(code(third,202) && third.find(R"("request_id":3)")!=std::string::npos);
        const auto cancelled=request("DELETE /v1/requests/3","",token);
        CHECK(code(cancelled,200) &&
              cancelled.find(R"("status":"cancelled")")!=std::string::npos);
        CHECK(code(request("DELETE /v1/requests/3","",token),409));
        const auto cancellation_status=request("GET /v1/requests/3","",token);
        CHECK(code(cancellation_status,200) &&
              cancellation_status.find(R"("status":"cancelled")")!=std::string::npos);
        CHECK(!service.take()); // GUI must not receive cancelled requests.
        request_events=request("GET /v1/request-events?after=7","",token);
        CHECK(code(request_events,200) &&
              request_events.find(R"("sequence":8,"request_id":3,"status":"cancelled")")!=std::string::npos);
        CHECK(code(request("GET /v1/request-events?after=8","",token),200));
        CHECK(code(request("DELETE /v1/requests/3","",token,
                           "Content-Length: 0\r\n"),400));
        constexpr std::string_view retry_header=
            "Idempotency-Key: client_0123456789abcdef\r\n";
        CHECK(code(request("POST /v1/calls",R"({"destination":"1004"})",token,
                           "Idempotency-Key: bad\r\n"),400));
        const auto original=request("POST /v1/calls",
            R"({"destination":"1004"})",token,retry_header);
        CHECK(code(original,202) &&
              original.find(R"("request_id":4)")!=std::string::npos);
        const auto repeat_queued=request("POST /v1/calls",
            R"({"destination":"1004"})",token,retry_header);
        CHECK(code(repeat_queued,200) &&
              repeat_queued.find(R"("replayed":true)")!=std::string::npos &&
              repeat_queued.find(R"("request_id":4)")!=std::string::npos);
        call=service.take();
        CHECK(call && call->id==4 && call->destination==L"1004");
        service.set_ready(false);
        const auto repeat_inflight=request("POST /v1/calls",
            R"({"destination":"9999"})",token,retry_header);
        CHECK(code(repeat_inflight,200) &&
              repeat_inflight.find(R"("status":"dispatching")")!=std::string::npos);
        CHECK(!service.take());
        CHECK(service.complete(4,true));
        const auto repeat_submitted=request("POST /v1/calls",
            R"({"destination":"9999"})",token,retry_header);
        CHECK(code(repeat_submitted,200) &&
              repeat_submitted.find(R"("status":"submitted")")!=std::string::npos);
        CHECK(!service.take());
        request_events=request("GET /v1/request-events?after=10","",token);
        CHECK(code(request_events,200) &&
              request_events.find(R"("sequence":11,"request_id":4,"status":"submitted")")!=std::string::npos &&
              request_events.find("client_0123456789abcdef")==std::string::npos);
        const auto no_extra_events=request("GET /v1/request-events?after=11","",token);
        CHECK(code(no_extra_events,200) &&
              no_extra_events.find(R"("events":[])")!=std::string::npos);
        CHECK(code(request("POST /v1/calls",R"({"destination":"1005"})",token),409));
        service.stop();
        CHECK(!service.active());
        CHECK(!service.take());
    }
    WSACleanup();
    return 0;
}
