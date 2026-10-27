#pragma once
#include "native_account_config.h"
#include <string>
namespace axv {
inline bool valid_native_transfer_uri(const std::string& uri){
 if(uri.empty() || uri.size()>512)return false;
 for(unsigned char c:uri)if(c<=32 || c>=127 || c=='%' || c=='?' || c=='"' || c=='\\' || c=='<' || c=='>')return false;
 const bool secure=uri.compare(0,5,"sips:")==0;
 if(!secure && uri.compare(0,4,"sip:")!=0)return false;
 const auto split=uri.find(';');
 const auto base=uri.substr(0,split);
 if(split!=std::string::npos){
  const auto suffix=uri.substr(split);
  if(suffix!=";transport=tls" && suffix!=";transport=udp")return false;
  if(secure && suffix!=";transport=tls")return false;
 }
 const auto at=base.find('@');const auto start=secure?5u:4u;
 if(at==std::string::npos || at<=start)return false;
 for(std::size_t i=start;i<at;++i){
  const char c=base[i];
  if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='_'||c=='.'||c=='-'||c=='+'||c=='*'||c=='#'))return false;
 }
 const std::wstring wide(base.begin(),base.end());
 const auto parsed=validate_address(wide);
 return parsed && parsed.normalized==wide;
}
inline NativeDestinationResult native_transfer_destination_result(const std::wstring& typed,const SipAccount& account){
 auto result=native_call_destination_result(typed,account);
 if(!result || valid_native_transfer_uri(result.uri))return result;
 return {NativeDestinationError::InvalidAddress,{}};
}
inline std::string native_transfer_destination(const std::wstring& typed,const SipAccount& account){
 return native_transfer_destination_result(typed,account).uri;
}
}
