#pragma once
#include "contact_book.h"
namespace axv {
enum class ContactStoreResult {Ok,Conflict,Error};
class ContactStore {
public:
 // Empty path selects the per-user LocalAppData product directory.
 explicit ContactStore(std::wstring path=L"");
 // Failure leaves the book unchanged and disables saving until a successful load.
 ContactStoreResult load(ContactBook&) noexcept;
 // Save requires a successful load and compares the exact on-disk baseline.
 ContactStoreResult save(const ContactBook&) noexcept;
private:
 std::wstring path_;
 std::string baseline_;
 bool loaded_=false,existed_=false;
};
}
