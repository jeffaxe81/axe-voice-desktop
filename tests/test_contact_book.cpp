#include "../src/contact_book.h"
#include "../src/contact_query.h"
#include "../src/contact_favorites.h"
#include "../src/contact_backup.h"
#include <cstdio>
#define CHECK(x) do{if(!(x)){std::fprintf(stderr,"contact check failed line%d: %s\n",__LINE__,#x);return 1;}}while(false)
int main(){
 using namespace axv;
 ContactBook book;
 CHECK(book.add({L"Jos\u00e9",L"1001",false})==ContactResult::Ok);
 const auto first=book.items()[0].id;
 CHECK(book.add({L"Suporte",L"sip:support@example.com",true})==ContactResult::Ok);
 CHECK(book.add({L"Duplicado",L"1001",false})==ContactResult::Duplicate);
 CHECK(book.update(first,{L"Novo",L"bad destination",false})==ContactResult::Invalid);
 CHECK(book.find(first)->name==L"Jos\u00e9");
 CHECK(find_contacts(book,L"JOS\u00c9")==std::vector<std::uint64_t>{first});
 CHECK(find_contacts(book,L"",true).size()==1);
 CHECK(toggle_contact_favorite(book,first)==ContactResult::Ok&&favorite_contact_count(book)==2);
 auto bytes=encode_contact_backup(book);CHECK(bytes);
 auto decoded=decode_contact_backup(*bytes);CHECK(decoded&&decoded->size()==2&&(*decoded)[0].name==L"Jos\u00e9");
 CHECK(!decode_contact_backup("{\"version\":1,\"version\":1,\"contacts\":[]}"));
 CHECK(!decode_contact_backup("{\"version\":2,\"contacts\":[]}"));
 CHECK(!decode_contact_backup(*bytes+"trailing"));
 CHECK(!decode_contact_backup(std::string(contact_backup_limit+1,' ')));
 CHECK(!valid_contact_draft({std::wstring(65,L'x'),L"1002",false}));
 CHECK(!valid_contact_draft({L"bad\nname",L"1002",false}));
 CHECK(!valid_contact_draft({L"Senha",L"sip:user:password@example.com",false}));
 CHECK(book.remove(first)==ContactResult::Ok);
 CHECK(book.add({L"Novo",L"1003",false})==ContactResult::Ok&&book.items().back().id!=first);
 CHECK(!book.find(first));
 std::puts("Contact CRUD, search, favorites and bounded Unicode backup passed");
}
