#pragma once
#include <cstdint>
#include <string>
#include <vector>
namespace axv {
struct ContactDraft {std::wstring name,destination;bool favorite=false;};
struct Contact {std::uint64_t id=0;std::wstring name,destination;bool favorite=false;};
enum class ContactResult {Ok,Invalid,Duplicate,Full,NotFound,Error};
bool valid_contact_draft(const ContactDraft&) noexcept;
class ContactBook {
public:
 static constexpr std::size_t capacity=500;
 const std::vector<Contact>& items() const noexcept {return items_;}
 const Contact* find(std::uint64_t id) const noexcept;
 ContactResult add(const ContactDraft&) noexcept;
 ContactResult update(std::uint64_t id,const ContactDraft&) noexcept;
 ContactResult remove(std::uint64_t id) noexcept;
 ContactResult set_favorite(std::uint64_t id,bool) noexcept;
 void swap(ContactBook&) noexcept;
private:
 std::vector<Contact> items_;
 std::uint64_t next_id_=1;
};
}
