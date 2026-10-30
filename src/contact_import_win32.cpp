#include "contact_ui_actions.h"
#include "contact_backup.h"
#include <commdlg.h>
#include <string>

namespace axv {
namespace {
class ImportHandle {
public:
 explicit ImportHandle(HANDLE handle) noexcept : handle_(handle) {}
 ~ImportHandle() { if (handle_ != INVALID_HANDLE_VALUE) CloseHandle(handle_); }
 ImportHandle(const ImportHandle&) = delete;
 ImportHandle& operator=(const ImportHandle&) = delete;
 HANDLE get() const noexcept { return handle_; }
private:
 HANDLE handle_;
};
bool read_import(const wchar_t* path, std::string& bytes) {
 ImportHandle file(CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, nullptr,
     OPEN_EXISTING, FILE_FLAG_OPEN_REPARSE_POINT | FILE_FLAG_SEQUENTIAL_SCAN, nullptr));
 if (file.get() == INVALID_HANDLE_VALUE || GetFileType(file.get()) != FILE_TYPE_DISK) return false;
 BY_HANDLE_FILE_INFORMATION info{};
 if (!GetFileInformationByHandle(file.get(), &info) ||
     (info.dwFileAttributes & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT))) return false;
 LARGE_INTEGER length{};
 if (!GetFileSizeEx(file.get(), &length) || length.QuadPart < 0 ||
     length.QuadPart > static_cast<LONGLONG>(contact_backup_limit)) return false;
 bytes.resize(static_cast<std::size_t>(length.QuadPart));
 std::size_t offset = 0;
 while (offset < bytes.size()) {
  DWORD received = 0;
  if (!ReadFile(file.get(), bytes.data() + offset,
      static_cast<DWORD>(bytes.size() - offset), &received, nullptr) || received == 0) return false;
  offset += received;
 }
 char extra = 0;
 DWORD received = 0;
 return ReadFile(file.get(), &extra, 1, &received, nullptr) && received == 0;
}
}

ContactTransferResult import_contacts(HWND owner, const ContactBook& current,
                                    ContactBook& output, ContactStore& store) noexcept {
 try {
  wchar_t path[32768]{};
  OPENFILENAMEW dialog{};
  dialog.lStructSize = sizeof(dialog);
  dialog.hwndOwner = owner;
  dialog.lpstrFilter = L"Backup de contatos (*.json)\0*.json\0\0";
  dialog.lpstrFile = path;
  dialog.nMaxFile = static_cast<DWORD>(sizeof(path) / sizeof(path[0]));
  dialog.lpstrTitle = L"Importar contatos";
  dialog.lpstrDefExt = L"json";
  dialog.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR | OFN_EXPLORER;
  if (!GetOpenFileNameW(&dialog))
   return {CommDlgExtendedError() ? ContactTransferStatus::Error : ContactTransferStatus::Cancelled};
  std::string bytes;
  if (!read_import(path, bytes)) return {ContactTransferStatus::Error};
  auto drafts = decode_contact_backup(bytes);
  if (!drafts) return {ContactTransferStatus::Invalid};
  ContactBook candidate = current;
  std::size_t added = 0, skipped = 0;
  for (const auto& draft : *drafts) {
   const auto result = candidate.add(draft);
   if (result == ContactResult::Ok) ++added;
   else if (result == ContactResult::Duplicate) ++skipped;
   else return {result == ContactResult::Error ? ContactTransferStatus::Error : ContactTransferStatus::Invalid};
  }
  const auto preview = L"Novos contatos: " + std::to_wstring(added) +
      L"\nDuplicados ignorados: " + std::to_wstring(skipped) + L"\n\nConfirmar importação?";
  const int choice = MessageBoxW(owner, preview.c_str(), L"Importar contatos",
      MB_OKCANCEL | MB_ICONQUESTION | MB_DEFBUTTON2);
  if (choice != IDOK) return {choice == 0 ? ContactTransferStatus::Error : ContactTransferStatus::Cancelled};
  if (added != 0) {
   const auto saved = store.save(candidate);
   if (saved != ContactStoreResult::Ok)
    return {saved == ContactStoreResult::Conflict ? ContactTransferStatus::Conflict : ContactTransferStatus::Error};
  }
  output.swap(candidate);
  return {ContactTransferStatus::Ok, added, skipped};
 } catch (...) { return {ContactTransferStatus::Error}; }
}
}
