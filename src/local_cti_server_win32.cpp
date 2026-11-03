#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include "local_cti_server_win32.h"
#include "local_cti_contract.h"
#include <algorithm>
#include <cctype>
#include <cstdint>
#include <map>
#include <string>
#include <string_view>

namespace axv {
namespace {
std::string lower(std::string value) {
    std::transform(value.begin(),value.end(),value.begin(),[](unsigned char c){
        return static_cast<char>(std::tolower(c));
    });
    return value;
}
std::string trim(std::string value) {
    const auto first=value.find_first_not_of(" \t");
    if (first==std::string::npos) return {};
    const auto last=value.find_last_not_of(" \t");
    return value.substr(first,last-first+1);
}
bool secure_equal(std::string_view left,std::string_view right) {
    const std::size_t max=std::max(left.size(),right.size());
    unsigned int diff=static_cast<unsigned int>(left.size()^right.size());
    for (std::size_t i=0;i<max;++i) {
        const unsigned char a=i<left.size()?static_cast<unsigned char>(left[i]):0;
        const unsigned char b=i<right.size()?static_cast<unsigned char>(right[i]):0;
        diff|=static_cast<unsigned int>(a^b);
    }
    return diff==0;
}
void reply(SOCKET socket,int code,const char* reason,std::string_view body) {
    const std::string out=
        "HTTP/1.1 "+std::to_string(code)+" "+reason+"\r\n"
        "Content-Type: application/json\r\n"
        "Cache-Control: no-store\r\n"
        "X-Content-Type-Options: nosniff\r\n"
        "Connection: close\r\n"
        "Content-Length: "+std::to_string(body.size())+"\r\n\r\n"+std::string(body);
    std::size_t written=0;
    while (written<out.size()) {
        const int amount=send(socket,out.data()+written,
            static_cast<int>(out.size()-written),0);
        if (amount<=0) return;
        written+=static_cast<std::size_t>(amount);
    }
}
} // namespace

LocalCtiServer::~LocalCtiServer(){ stop(); }

bool LocalCtiServer::start(std::string token) {
    if (worker_.joinable() || !valid_cti_api_token(token)) return false;
    WSADATA data{};
    if (WSAStartup(MAKEWORD(2,2),&data)!=0) return false;
    SOCKET listener=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
    if (listener==INVALID_SOCKET) { WSACleanup(); return false; }
    const BOOL exclusive=TRUE;
    sockaddr_in addr{};
    addr.sin_family=AF_INET;
    addr.sin_addr.s_addr=htonl(INADDR_LOOPBACK);
    addr.sin_port=htons(port);
    if (setsockopt(listener,SOL_SOCKET,SO_EXCLUSIVEADDRUSE,
            reinterpret_cast<const char*>(&exclusive),sizeof(exclusive))!=0 ||
        bind(listener,reinterpret_cast<const sockaddr*>(&addr),sizeof(addr))!=0 ||
        listen(listener,4)!=0) {
        closesocket(listener);
        WSACleanup();
        return false;
    }
    listener_=static_cast<std::uintptr_t>(listener);
    token_=std::move(token);
    stopping_.store(false);
    ready_.store(false);
    running_.store(true);
    try { worker_=std::thread([this]{run();}); }
    catch (...) {
        running_.store(false);
        closesocket(listener);
        listener_=static_cast<std::uintptr_t>(INVALID_SOCKET);
        token_.assign(token_.size(),'\0');
        token_.clear();
        WSACleanup();
        return false;
    }
    return true;
}

void LocalCtiServer::stop() {
    stopping_.store(true);
    ready_.store(false);
    if (worker_.joinable()) worker_.join();
    if (listener_!=static_cast<std::uintptr_t>(INVALID_SOCKET)) {
        closesocket(static_cast<SOCKET>(listener_));
        listener_=static_cast<std::uintptr_t>(INVALID_SOCKET);
        WSACleanup();
    }
    requests_.clear();
    token_.assign(token_.size(),'\0');
    token_.clear();
    running_.store(false);
}

std::optional<CtiPendingRequest> LocalCtiServer::take() {
    return requests_.take();
}

void LocalCtiServer::run() {
    const SOCKET listener=static_cast<SOCKET>(listener_);
    while (!stopping_.load()) {
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(listener,&readfds);
        timeval timeout{};
        timeout.tv_usec=100000;
        const int selected=select(0,&readfds,nullptr,nullptr,&timeout);
        if (selected==SOCKET_ERROR) break;
        if (selected!=1) continue;
        SOCKET client=accept(listener,nullptr,nullptr);
        if (client==INVALID_SOCKET) continue;
        const DWORD socket_timeout=900;
        setsockopt(client,SOL_SOCKET,SO_RCVTIMEO,
            reinterpret_cast<const char*>(&socket_timeout),sizeof(socket_timeout));
        setsockopt(client,SOL_SOCKET,SO_SNDTIMEO,
            reinterpret_cast<const char*>(&socket_timeout),sizeof(socket_timeout));
        try { handle_client(static_cast<std::uintptr_t>(client)); }
        catch (...) { reply(client,500,"Internal Server Error",R"({"error":"internal"})"); }
        closesocket(client);
    }
    running_.store(false);
}

void LocalCtiServer::handle_client(std::uintptr_t handle) {
    SOCKET client=static_cast<SOCKET>(handle);
    // Deny oversized requests, HTTP pipelining and ambiguous framing.
    std::string packet;
    std::size_t delimiter=std::string::npos;
    while (packet.size()<=8192) {
        char data[2048];
        int got=recv(client,data,sizeof(data),0);
        if(got<=0) return;
        packet.append(data,static_cast<std::size_t>(got));
        delimiter=packet.find("\r\n\r\n");
        if(delimiter!=std::string::npos)break;
    }
    if(delimiter==std::string::npos || delimiter>4096) {
        reply(client,400,"Bad Request",R"({"error":"bad_request"})");
        return;
    }
    std::string header=packet.substr(0,delimiter);
    const auto first_line=header.find("\r\n");
    if(first_line==std::string::npos) {
        reply(client,400,"Bad Request",R"({"error":"bad_request"})");
        return;
    }
    const auto request_line=header.substr(0,first_line);
    const bool status=request_line=="GET /v1/status HTTP/1.1";
    const bool call=request_line=="POST /v1/calls HTTP/1.1";
    const bool current_call=request_line=="GET /v1/calls/current HTTP/1.1";
    constexpr std::string_view request_prefix="GET /v1/requests/";
    constexpr std::string_view cancel_prefix="DELETE /v1/requests/";
    constexpr std::string_view http_suffix=" HTTP/1.1";
    const bool request_result=
        request_line.compare(0,request_prefix.size(),request_prefix)==0 &&
        request_line.size()>request_prefix.size()+http_suffix.size() &&
        request_line.compare(request_line.size()-http_suffix.size(),
                             http_suffix.size(),http_suffix)==0;
    const bool cancel_request=
        request_line.compare(0,cancel_prefix.size(),cancel_prefix)==0 &&
        request_line.size()>cancel_prefix.size()+http_suffix.size() &&
        request_line.compare(request_line.size()-http_suffix.size(),
                             http_suffix.size(),http_suffix)==0;
    const bool all_events=request_line=="GET /v1/events HTTP/1.1";
    constexpr std::string_view event_prefix="GET /v1/events?after=";
    std::uint64_t event_cursor=0;
    bool events=all_events;
    if(!events && request_line.compare(0,event_prefix.size(),event_prefix)==0 &&
       request_line.size()>event_prefix.size()+http_suffix.size() &&
       request_line.compare(request_line.size()-http_suffix.size(),
                            http_suffix.size(),http_suffix)==0) {
        const std::string cursor=request_line.substr(event_prefix.size(),
            request_line.size()-event_prefix.size()-http_suffix.size());
        if(!parse_cti_event_cursor(cursor,event_cursor)) {
            reply(client,400,"Bad Request",R"({"error":"invalid_cursor"})");
            return;
        }
        events=true;
    }
    if (!status && !call && !current_call && !events && !request_result && !cancel_request) {
        reply(client,404,"Not Found",R"({"error":"not_found"})");
        return;
    }
    std::map<std::string,std::string> fields;
    std::size_t cursor=first_line+2;
    while(cursor<header.size()){
        const auto end=header.find("\r\n",cursor);
        const std::string line=header.substr(cursor,end==std::string::npos
            ?std::string::npos:end-cursor);
        const auto colon=line.find(':');
        if(colon==std::string::npos || colon==0) {
            reply(client,400,"Bad Request",R"({"error":"bad_request"})");return;
        }
        const auto name=lower(line.substr(0,colon));
        const auto value=trim(line.substr(colon+1));
        if(!fields.emplace(name,value).second){
            reply(client,400,"Bad Request",R"({"error":"duplicate_header"})");return;
        }
        if(end==std::string::npos)break;
        cursor=end+2;
    }
    // No wildcard CORS; browser-originated requests are not an API client.
    // A CRM browser needs a dedicated, user-confirmed protocol handler.
    if(fields.count("origin") || fields.count("referer") ||
       fields.count("transfer-encoding") ||
       fields["host"]!="127.0.0.1:27652") {
        reply(client,403,"Forbidden",R"({"error":"forbidden"})");
        return;
    }
    const auto authorization=fields.find("authorization");
    constexpr std::string_view prefix="Bearer ";
    if(authorization==fields.end() ||
       authorization->second.compare(0,prefix.size(),prefix)!=0 ||
       !secure_equal(std::string_view(authorization->second).substr(prefix.size()),token_)) {
        reply(client,401,"Unauthorized",R"({"error":"unauthorized"})");
        return;
    }
    if(status || current_call || events || request_result || cancel_request) {
        if(packet.size()!=delimiter+4 || fields.count("content-length")) {
            reply(client,400,"Bad Request",R"({"error":"bad_request"})");
            return;
        }
        if(status) {
            reply(client,200,"OK",
                  ready_.load()?R"({"ready":true})":R"({"ready":false})");
        } else if(current_call) {
            const auto json=cti_snapshot_json(journal_.snapshot());
            reply(client,200,"OK",json);
        } else if(events) {
            const auto json=cti_events_json(journal_.after(event_cursor));
            reply(client,200,"OK",json);
        } else {
            const auto used_prefix=cancel_request?cancel_prefix:request_prefix;
            const auto value=request_line.substr(
                used_prefix.size(),request_line.size()-used_prefix.size()-http_suffix.size());
            std::uint64_t id=0;
            if(!parse_cti_event_cursor(value,id) || id==0) {
                reply(client,400,"Bad Request",R"({"error":"invalid_request_id"})");
                return;
            }
            if(cancel_request) {
                const auto outcome=requests_.cancel(id);
                if(outcome==CtiCancelOutcome::NotFound) {
                    reply(client,404,"Not Found",R"({"error":"unknown_request"})");
                } else if(outcome==CtiCancelOutcome::TooLate) {
                    reply(client,409,"Conflict",R"({"error":"already_dispatching"})");
                } else {
                    reply(client,200,"OK",cti_request_json({id,CtiRequestState::Cancelled}));
                }
            } else {
                const auto record=requests_.get(id);
                if(!record) {
                    reply(client,404,"Not Found",R"({"error":"unknown_request"})");
                    return;
                }
                const auto json=cti_request_json(*record);
                reply(client,200,"OK",json);
            }
        }
        return;
    }
    if(fields["content-type"]!="application/json" &&
       !valid_cti_json_type(lower(fields["content-type"]))) {
        reply(client,415,"Unsupported Media Type",R"({"error":"content_type"})");
        return;
    }
    const auto length_string=fields.find("content-length");
    if(length_string==fields.end() || length_string->second.empty() ||
       length_string->second.size()>4) {
        reply(client,411,"Length Required",R"({"error":"content_length"})");return;
    }
    std::size_t length=0;
    for(unsigned char c:length_string->second) {
        if(c<'0'||c>'9') {
            reply(client,400,"Bad Request",R"({"error":"content_length"})");return;
        }
        length=length*10+static_cast<std::size_t>(c-'0');
    }
    if(!length || length>512) {
        reply(client,413,"Content Too Large",R"({"error":"content_length"})");return;
    }
    const std::size_t body_begin=delimiter+4;
    if(packet.size()>body_begin+length) {
        reply(client,400,"Bad Request",R"({"error":"extra_bytes"})");return;
    }
    while(packet.size()<body_begin+length) {
        char data[512];
        const int remaining=static_cast<int>(body_begin+length-packet.size());
        int got=recv(client,data,std::min(remaining,static_cast<int>(sizeof(data))),0);
        if(got<=0) return;
        packet.append(data,static_cast<std::size_t>(got));
    }
    const auto parsed=parse_cti_call_json(
        std::string_view(packet).substr(body_begin,length));
    if(!parsed) {
        reply(client,422,"Unprocessable Entity",R"({"error":"invalid_destination"})");
        return;
    }
    std::string key;
    if(const auto header=fields.find("idempotency-key");header!=fields.end()) {
        if(!valid_cti_idempotency_key(header->second)) {
            reply(client,400,"Bad Request",R"({"error":"invalid_idempotency_key"})");
            return;
        }
        key=header->second;
    }
    const auto submitted=requests_.submit_idempotent(*parsed,key,ready_.load());
    if(submitted.outcome==CtiSubmitOutcome::NotReady) {
        reply(client,409,"Conflict",R"({"error":"sip_not_ready"})");
        return;
    }
    if(submitted.outcome==CtiSubmitOutcome::Busy) {
        reply(client,429,"Too Many Requests",R"({"error":"busy"})");
        return;
    }
    if(submitted.outcome==CtiSubmitOutcome::Replayed) {
        const auto prior=cti_request_json({submitted.id,submitted.state});
        reply(client,200,"OK",prior.substr(0,prior.size()-1)+",\"replayed\":true}");
        return;
    }
    reply(client,202,"Accepted",cti_request_json({submitted.id,CtiRequestState::Queued}));
}
} // namespace axv
