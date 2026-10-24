#pragma once
#include <string>
namespace axv {
enum class AddressError { None, Empty, Whitespace, InvalidScheme, InvalidUser, InvalidHost, InvalidPort };
struct AddressResult { AddressError error; std::wstring normalized; explicit operator bool() const { return error == AddressError::None; } };
AddressResult validate_address(const std::wstring& input);
}
