#include "contact_dialog.h"
#include "contact_favorites.h"
#include "contact_query.h"
#include "contact_ui_actions.h"
#include <algorithm>
#include <commctrl.h>
#include <memory>
#include <string>
#include <vector>
namespace axv {
namespace {
enum {
  Search = 101,
  Only,
  List,
  Name,
  Destination,
  New,
  Save,
  Delete,
  Favorite,
  Use,
  Copy,
  Import,
  Export,
  Status,
  Count,
  LabelName,
  LabelDest
};
struct Window {
  HWND hwnd = nullptr, owner = nullptr, edit = nullptr;
  HFONT font = nullptr;
  ContactBook book;
  ContactStore store;
  std::vector<std::uint64_t> rows;
  std::uint64_t selected = 0;
  bool dirty = false, updating = false, loaded = false;
  UINT dpi = 96;
};
std::shared_ptr<Window> state;
HWND child(Window &w, int id) { return GetDlgItem(w.hwnd, id); }
void status(Window &w, const wchar_t *s) {
  SetWindowTextW(child(w, Status), s);
}
std::wstring value(HWND h) {
  int n = GetWindowTextLengthW(h);
  std::wstring s(static_cast<size_t>(n) + 1, L'\0');
  GetWindowTextW(h, &s[0], n + 1);
  s.resize(n);
  return s;
}
bool live(const Window& w) noexcept {
  return w.hwnd && IsWindow(w.hwnd) && IsWindow(w.owner);
}
bool discard(Window &w) {
  if(!live(w))return false;
  if(!w.dirty)return true;
  const int result=MessageBoxW(w.hwnd,L"Descartar as alterações não salvas?",L"Contatos",MB_YESNO|MB_ICONQUESTION);
  return live(w) && result==IDYES;
}
void fill(Window &w, std::uint64_t id) {
  w.updating = true;
  w.selected = id;
  const auto *c = w.book.find(id);
  SetWindowTextW(child(w, Name), c ? c->name.c_str() : L"");
  SetWindowTextW(child(w, Destination), c ? c->destination.c_str() : L"");
  w.updating = false;
  w.dirty = false;
}
void refresh(Window &w) {
  if(!live(w))return;
  w.updating = true;
  w.rows = find_contacts(w.book, value(child(w, Search)),
                         SendMessageW(child(w, Only), BM_GETCHECK, 0, 0) ==
                             BST_CHECKED);
  SendMessageW(child(w, List), LB_RESETCONTENT, 0, 0);
  int selected = -1;
  for (size_t i = 0; i < w.rows.size(); ++i) {
    const auto *c = w.book.find(w.rows[i]);
    if (!c)
      continue;
    std::wstring label =
        (c->favorite ? L"★ " : L"") + c->name + L"   —   " + c->destination;
    SendMessageW(child(w, List), LB_ADDSTRING, 0,
                 reinterpret_cast<LPARAM>(label.c_str()));
    if (c->id == w.selected)
      selected = static_cast<int>(i);
  }
  SendMessageW(child(w, List), LB_SETCURSEL, selected, 0);
  std::wstring count = std::to_wstring(w.rows.size()) + L" exibidos / " +
                       std::to_wstring(w.book.items().size()) +
                       L" contatos (máximo 500)";
  SetWindowTextW(child(w, Count), count.c_str());
  w.updating = false;
}
bool persist(Window &w, ContactBook &next) {
  if (!w.loaded) {
    status(w, L"Não foi possível carregar os contatos. Feche e reabra esta "
              L"janela antes de salvar.");
    return false;
  }
  auto result = w.store.save(next);
  if (result != ContactStoreResult::Ok) {
    status(w, result == ContactStoreResult::Conflict
                  ? L"Os contatos foram alterados em outra janela. Feche e "
                    L"reabra para recarregar; as alterações não foram salvas."
                  : L"Não foi possível salvar. Os contatos existentes foram "
                    L"preservados.");
    return false;
  }
  w.book.swap(next);
  status(w, L"Salvo.");
  return true;
}
int px(Window &w, int n) { return MulDiv(n, w.dpi, 96); }
void place(Window &w, int id, int x, int y, int width, int height) {
  MoveWindow(child(w, id), px(w, x), px(w, y), px(w, width), px(w, height),
             TRUE);
}
void layout(Window &w) {
  RECT r{};
  GetClientRect(w.hwnd, &r);
  int width = MulDiv(r.right, 96, w.dpi), height = MulDiv(r.bottom, 96, w.dpi);
  int inner = width - 24;
  place(w, Search, 12, 12, inner - 158, 26);
  place(w, Only, width - 158, 12, 146, 26);
  place(w, List, 12, 46, inner, std::max(90, height - 314));
  int y = std::max(140, height - 236);
  place(w, Count, 12, y - 24, inner, 20);
  place(w, LabelName, 12, y, 90, 22);
  place(w, Name, 104, y, inner - 92, 26);
  place(w, LabelDest, 12, y + 34, 90, 22);
  place(w, Destination, 104, y + 34, inner - 92, 26);
  int bw = (inner - 24) / 4;
  int ids[] = {New, Save, Delete, Favorite, Use, Copy, Import, Export};
  for (int i = 0; i < 8; ++i)
    place(w, ids[i], 12 + (i % 4) * (bw + 8), y + 72 + (i / 4) * 36, bw, 28);
  place(w, Status, 12, y + 150, inner, 70);
}
void command(Window &w, int id, int code) {
  if(!live(w))return;
  if ((id == Name || id == Destination) && code == EN_CHANGE) {
    if (!w.updating)
      w.dirty = true;
    return;
  }
  if (id == Search && code == EN_CHANGE) {
    refresh(w);
    return;
  }
  if (id == Only) {
    refresh(w);
    return;
  }
  if (id == List && (code == LBN_SELCHANGE || code == LBN_DBLCLK)) {
    int i = static_cast<int>(SendMessageW(child(w, List), LB_GETCURSEL, 0, 0));
    if (i < 0 || static_cast<size_t>(i) >= w.rows.size())
      return;
    auto chosen = w.rows[i];
    if (chosen != w.selected) {
      if (!discard(w)) {
        refresh(w);
        return;
      }
      fill(w, chosen);
    }
    if (code == LBN_DBLCLK)
      command(w, Use, 0);
    return;
  }
  if (id == New) {
    if (discard(w)) {
      fill(w, 0);
      refresh(w);
      SetFocus(child(w, Name));
    }
    return;
  }
  if (id == Save) {
    ContactDraft draft{value(child(w, Name)), value(child(w, Destination)),
                       false};
    if (auto *c = w.book.find(w.selected))
      draft.favorite = c->favorite;
    ContactBook next = w.book;
    auto result = w.selected ? next.update(w.selected, draft) : next.add(draft);
    if (result != ContactResult::Ok) {
      status(w,
             result == ContactResult::Duplicate
                 ? L"Este destino já existe."
                 : L"Informe um nome válido (até 64 caracteres) e um destino "
                   L"válido (até 128 caracteres). Limite: 500 contatos.");
      return;
    }
    if (persist(w, next)) {
      if (!w.selected)
        for (const auto &c : w.book.items())
          if (c.destination == draft.destination) {
            w.selected = c.id;
            break;
          }
      fill(w, w.selected);
      refresh(w);
    }
    return;
  }
  if (id == Import) {
    if (!discard(w))
      return;
    if (!w.loaded) {
      status(w, L"Feche e reabra esta janela para recarregar os contatos antes "
                L"de importar.");
      return;
    }
    ContactBook output;
    auto result = import_contacts(w.hwnd, w.book, output, w.store);
    if (result.status == ContactTransferStatus::Ok) {
      w.book.swap(output);
      fill(w, 0);
      refresh(w);
      std::wstring message = L"Importados: " + std::to_wstring(result.added) +
                             L"; ignorados: " +
                             std::to_wstring(result.skipped) + L" duplicados.";
      status(w, message.c_str());
    } else if (result.status != ContactTransferStatus::Cancelled)
      status(w, result.status == ContactTransferStatus::Conflict
                    ? L"Os contatos foram alterados. Feche e reabra esta "
                      L"janela para recarregar."
                    : L"Falha ao importar; os contatos existentes foram "
                      L"preservados.");
    return;
  }
  if (id == Export) {
    if(!w.loaded){status(w,L"Reabra a agenda e carregue os contatos antes de exportar.");return;}
    auto result = export_contacts(w.hwnd, w.book);
    if (result.status != ContactTransferStatus::Cancelled)
      status(w, result.status == ContactTransferStatus::Ok
                    ? L"Contatos exportados."
                    : L"Não foi possível exportar os contatos.");
    return;
  }
  const auto *c = w.book.find(w.selected);
  if (!c) {
    status(w, L"Selecione um contato primeiro.");
    return;
  }
  if (id == Copy) {
    status(w, copy_contact_destination(w.hwnd, *c)
                  ? L"Destino copiado."
                  : L"Não foi possível copiar o destino.");
    return;
  }
  if (id == Use) {
    if (!discard(w))
      return;
    if (use_contact_destination(w.edit, *c)) {
      fill(w, w.selected);
      ShowWindow(w.hwnd, SW_HIDE);
      ShowWindow(w.owner, SW_RESTORE);
      SetForegroundWindow(w.owner);
      SetFocus(w.edit);
      status(w, L"Destino preenchido no discador. Confira antes de ligar.");
    } else
      status(w, L"Não foi possível preencher o discador.");
    return;
  }
  if (id == Delete || id == Favorite) {
    if (!discard(w))
      return;
    if (id == Delete &&
        MessageBoxW(w.hwnd, L"Excluir o contato selecionado?", L"Contatos",
                    MB_YESNO | MB_ICONQUESTION) != IDYES)
      return;
    if(!live(w))return;
    ContactBook next = w.book;
    auto result = id == Delete ? next.remove(c->id)
                               : toggle_contact_favorite(next, c->id);
    if (result == ContactResult::Ok && persist(w, next)) {
      fill(w, id == Delete ? 0 : w.selected);
      refresh(w);
    }
  }
}
LRESULT CALLBACK proc(HWND h, UINT msg, WPARAM wp, LPARAM lp) {
  // A modal picker/confirmation pumps main-window shutdown messages. Retain
  // this dispatch's original state even when close_contact_book clears globals.
  std::shared_ptr<Window> dispatch_lifetime;
  try {
    auto *w = reinterpret_cast<Window *>(GetWindowLongPtrW(h, GWLP_USERDATA));
    if (msg == WM_NCCREATE) {
      w = static_cast<Window *>(
          reinterpret_cast<CREATESTRUCTW *>(lp)->lpCreateParams);
      w->hwnd = h;
      SetWindowLongPtrW(h, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(w));
    }
    if(state && state.get()==w)dispatch_lifetime=state;
    if (!w)
      return DefWindowProcW(h, msg, wp, lp);
    switch (msg) {
    case WM_CREATE: {
      HDC dc = GetDC(h);
      if (dc) {
        int dpi = GetDeviceCaps(dc, LOGPIXELSX);
        if (dpi > 0)
          w->dpi = static_cast<UINT>(dpi);
        ReleaseDC(h, dc);
      }
      w->font =
          CreateFontW(-px(*w, 14), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                      DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                      CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
      auto add = [&](int id, const wchar_t *cls, const wchar_t *text,
                     DWORD style) {
        HWND c = CreateWindowExW(
            (id == List || id == Search || id == Name || id == Destination)
                ? WS_EX_CLIENTEDGE
                : 0,
            cls, text, WS_CHILD | WS_VISIBLE | style, 0, 0, 0, 0, h,
            reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),
            GetModuleHandleW(nullptr), nullptr);
        SendMessageW(c, WM_SETFONT,
                     reinterpret_cast<WPARAM>(
                         w->font ? w->font : GetStockObject(DEFAULT_GUI_FONT)),
                     TRUE);
        return c;
      };
      add(Search, L"EDIT", L"", WS_TABSTOP | ES_AUTOHSCROLL);
      SendMessageW(child(*w, Search), EM_SETLIMITTEXT, 128, 0);
      SendMessageW(child(*w, Search), EM_SETCUEBANNER, TRUE,
                   reinterpret_cast<LPARAM>(L"Buscar nome ou destino"));
      add(Only, L"BUTTON", L"Só &favoritos", WS_TABSTOP | BS_AUTOCHECKBOX);
      add(List, L"LISTBOX", L"",
          WS_TABSTOP | LBS_NOTIFY | WS_VSCROLL | LBS_NOINTEGRALHEIGHT);
      add(LabelName, L"STATIC", L"&Nome:", 0);
      add(Name, L"EDIT", L"", WS_TABSTOP | ES_AUTOHSCROLL);
      SendMessageW(child(*w, Name), EM_SETLIMITTEXT, 64, 0);
      add(LabelDest, L"STATIC", L"&Destino:", 0);
      add(Destination, L"EDIT", L"", WS_TABSTOP | ES_AUTOHSCROLL);
      SendMessageW(child(*w, Destination), EM_SETLIMITTEXT, 128, 0);
      int ids[] = {New, Save, Delete, Favorite, Use, Copy, Import, Export};
      const wchar_t *labels[] = {L"No&vo",
                                 L"&Salvar",
                                 L"E&xcluir",
                                 L"Alternar f&avorito",
                                 L"&Usar no discador",
                                 L"&Copiar destino",
                                 L"&Importar JSON",
                                 L"&Exportar JSON"};
      for (int i = 0; i < 8; ++i)
        add(ids[i], L"BUTTON", labels[i], WS_TABSTOP | BS_PUSHBUTTON);
      add(Count, L"STATIC", L"", 0);
      add(Status, L"STATIC", L"", 0);
      w->loaded = w->store.load(w->book) == ContactStoreResult::Ok;
      refresh(*w);
      status(*w, w->loaded ? L"Escolha um contato ou crie um novo. Usar um "
                             L"contato apenas preenche o discador."
                           : L"Não foi possível carregar os contatos. O "
                             L"salvamento está bloqueado para proteger os "
                             L"dados. Feche e reabra para tentar novamente.");
      layout(*w);
      return 0;
    }
    case WM_SIZE:
      layout(*w);
      return 0;
    case WM_GETMINMAXINFO: {
      auto *m = reinterpret_cast<MINMAXINFO *>(lp);
      m->ptMinTrackSize = {px(*w, 640), px(*w, 520)};
      return 0;
    }
    case WM_DPICHANGED: {
      w->dpi = HIWORD(wp);
      HFONT previous = w->font;
      w->font =
          CreateFontW(-px(*w, 14), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                      DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                      CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
      for (HWND control = GetWindow(h, GW_CHILD); control;
           control = GetWindow(control, GW_HWNDNEXT))
        SendMessageW(control, WM_SETFONT,
                     reinterpret_cast<WPARAM>(
                         w->font ? w->font : GetStockObject(DEFAULT_GUI_FONT)),
                     TRUE);
      if (previous)
        DeleteObject(previous);
      RECT *r = reinterpret_cast<RECT *>(lp);
      SetWindowPos(h, nullptr, r->left, r->top, r->right - r->left,
                   r->bottom - r->top, SWP_NOZORDER | SWP_NOACTIVATE);
      layout(*w);
      return 0;
    }
    case WM_COMMAND:
      command(*w, LOWORD(wp), HIWORD(wp));
      return 0;
    case WM_CLOSE:
      if (discard(*w))
        DestroyWindow(h);
      return 0;
    case WM_DESTROY:
      if (w->font) {
        DeleteObject(w->font);
        w->font = nullptr;
      }
      w->hwnd = nullptr;
      return 0;
    }
    return DefWindowProcW(h, msg, wp, lp);
  } catch (...) {
    if (dispatch_lifetime && dispatch_lifetime->hwnd)
      status(*dispatch_lifetime, L"Não foi possível concluir a operação. Os dados "
                     L"existentes foram preservados.");
    return msg == WM_NCCREATE ? FALSE : 0;
  }
}
} // namespace
void show_contact_book(HWND owner, HWND destination_edit) noexcept {
  try {
    if (state && state->hwnd) {
      ShowWindow(state->hwnd, SW_RESTORE);
      SetForegroundWindow(state->hwnd);
      return;
    }
    WNDCLASSW wc{};
    wc.lpfnWndProc = proc;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.lpszClassName = L"AXVContactBookV1";
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1);
    RegisterClassW(&wc);
    state = std::make_shared<Window>();
    state->owner = owner;
    state->edit = destination_edit;
    HWND h = CreateWindowExW(WS_EX_CONTROLPARENT, wc.lpszClassName,
                             L"Contatos — AXE Voice", WS_OVERLAPPEDWINDOW,
                             CW_USEDEFAULT, CW_USEDEFAULT, 760, 600, owner,
                             nullptr, wc.hInstance, state.get());
    if (h) {
      ShowWindow(h, SW_SHOW);
      SetFocus(child(*state, Search));
    }
  } catch (...) {
    MessageBoxW(owner, L"Não foi possível abrir os contatos.", L"Contatos",
                MB_OK | MB_ICONERROR);
  }
}
bool contacts_owns_message(const MSG &m) noexcept {
  return state && state->hwnd &&
         (m.hwnd == state->hwnd || IsChild(state->hwnd, m.hwnd));
}
bool translate_contacts_message(MSG &m) noexcept {
  if (!contacts_owns_message(m))
    return false;
  if (m.message == WM_KEYDOWN) {
    if (m.wParam == VK_ESCAPE) {
      SendMessageW(state->hwnd, WM_CLOSE, 0, 0);
      return true;
    }
    if ((GetKeyState(VK_CONTROL) & 0x8000) && m.wParam == 'S') {
      try {
        command(*state, Save, 0);
      } catch (...) {
      }
      return true;
    }
  }
  return IsDialogMessageW(state->hwnd, &m) != FALSE;
}
void close_contact_book() noexcept {
  if (state && state->hwnd)
    DestroyWindow(state->hwnd);
  state.reset();
}
} // namespace axv
