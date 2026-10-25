#pragma once
#include "baresip_sdk_headers.h"
#include <cstring>
namespace axv {
// Pinned Baresip4.12 duplicates auth_pass into mutable libre memory; Digest
// references that allocation via mem_ref(). Clear it on the SDK owner before
// account/UA destruction, including outstanding references, then release it.
inline void erase_sdk_account_password(struct account* account){
 const char* secret=account_auth_pass(account);
 if(secret){
  const auto size=std::strlen(secret);
  volatile char* bytes=const_cast<char*>(secret);
  for(std::size_t i=0;i<size;++i)bytes[i]=0;
 }
 (void)account_set_auth_pass(account,nullptr);
}
}
