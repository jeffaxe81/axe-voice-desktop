#include "contact_favorites.h"

namespace axv {

ContactResult toggle_contact_favorite(ContactBook& book,std::uint64_t id) noexcept {
 const Contact* contact=book.find(id);
 if(!contact) return ContactResult::NotFound;
 const bool favorite=!contact->favorite;
 return book.set_favorite(id,favorite);
}

std::size_t favorite_contact_count(const ContactBook& book) noexcept {
 std::size_t count=0;
 for(const Contact& contact:book.items()) {
  if(contact.favorite) ++count;
 }
 return count;
}

}
