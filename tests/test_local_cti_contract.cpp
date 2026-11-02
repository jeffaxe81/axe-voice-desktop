#include "../src/local_cti_contract.h"
#include <cstdio>
#include <string>
#include <string_view>
#define CHECK(x) do{if(!(x)){std::fprintf(stderr,"CTI contract check %d: %s\n",__LINE__,#x);return 1;}}while(false)
int main() {
    using namespace axv;
    CHECK(valid_cti_api_token("12345678901234567890123456789012"));
    CHECK(valid_cti_api_token("abcdeABCDE1234567890abcdeABCDE12"));
    CHECK(!valid_cti_api_token(""));
    CHECK(!valid_cti_api_token("short"));
    CHECK(!valid_cti_api_token(std::string(129,'a')));
    CHECK(!valid_cti_api_token("1234567890123456789012345678901 "));
    CHECK(!valid_cti_api_token("1234567890123456789012345678901!"));
    CHECK(valid_cti_json_type("application/json"));
    CHECK(valid_cti_json_type("application/json; charset=utf-8"));
    CHECK(!valid_cti_json_type("text/plain"));
    CHECK(!valid_cti_json_type("application/x-www-form-urlencoded"));
    CHECK(parse_cti_call_json(R"({"destination":"1001"})")==std::optional<std::wstring>{L"1001"});
    CHECK(parse_cti_call_json(R"( { "destination" : "+5511999999999" } )")==std::optional<std::wstring>{L"+5511999999999"});
    CHECK(parse_cti_call_json(R"({"destination":"sip:1001@pbx.example.org"})")==
        std::optional<std::wstring>{L"sip:1001@pbx.example.org"});
    CHECK(parse_cti_call_json(R"({"destination":"1002@pbx.example.org"})")==
        std::optional<std::wstring>{L"sip:1002@pbx.example.org"});
    CHECK(!parse_cti_call_json(R"({"destination":"100 1"})"));
    CHECK(!parse_cti_call_json(R"({"destination":"abc"})"));
    CHECK(!parse_cti_call_json(R"({"destination":"1001","force":true})"));
    CHECK(!parse_cti_call_json(R"({"destination":"1001","destination":"1002"})"));
    CHECK(!parse_cti_call_json(R"({"destination":"1001","bypass_registration":true})"));
    CHECK(!parse_cti_call_json(R"({"destination":"1001"}x)"));
    CHECK(!parse_cti_call_json(R"({"destination":"1001\n"})"));
    CHECK(!parse_cti_call_json(R"({"destination":123})"));
    CHECK(!parse_cti_call_json(R"({"destination":""})"));
    CHECK(!parse_cti_call_json(R"({"to":"1001"})"));
    CHECK(!parse_cti_call_json(std::string(600,'A')));
    return 0;
}
