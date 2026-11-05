#pragma once
// AXE Voice preferences: modal Windows dialog, separate from the compact dialer.
// Values represent existing app controls; no credentials are persisted here.
#include <windows.h>
#include <string>
#include <vector>
#include <array>

namespace axv {
struct FalewebPreferences {
 std::wstring server,user,port,password,ca_file;
 bool tls=false;
 bool save_password=false;
 bool saved_password_exists=false;
 int ringtone=2;
 bool mute_ringtone=false;
 std::vector<std::wstring> inputs,outputs,rings;
 int input_index=-1,output_index=-1,ring_index=-1;
};
struct FalewebPrefsDialog {
 FalewebPreferences data;
 std::array<std::vector<HWND>,6> controls{};
 std::array<HWND,6> tabs{};
 HWND server=nullptr,user=nullptr,port=nullptr,password=nullptr,ca=nullptr,tls=nullptr;
 HWND save_password=nullptr;
 HWND ringtone=nullptr,muted=nullptr,inputs=nullptr,outputs=nullptr,rings=nullptr;
 int page=0;
};
inline std::wstring pref_edit_text(HWND control){
 if(!control)return {};
 const int length=GetWindowTextLengthW(control);
 if(length<0 || length>4096)return {};
 std::wstring result(static_cast<size_t>(length)+1,L'\0');
 GetWindowTextW(control,result.data(),length+1);
 result.resize(static_cast<size_t>(length));
 return result;
}
// Coordinates in the reference design are logical 96-DPI pixels; scale
// controls on high-density displays independently from dialog template units.
inline int pref_px(HWND window,int logical){
 const unsigned dpi=GetDpiForWindow(window);
 return MulDiv(logical,static_cast<int>(dpi?dpi:96),96);
}
inline HWND pref_control(HWND dialog,FalewebPrefsDialog* state,int page,
 LPCWSTR cls,LPCWSTR value,DWORD style,int x,int y,int w,int h){
 HWND result=CreateWindowExW(0,cls,value,WS_CHILD|WS_VISIBLE|style,
 pref_px(dialog,x),pref_px(dialog,y),pref_px(dialog,w),pref_px(dialog,h),
 dialog,nullptr,GetModuleHandleW(nullptr),nullptr);
 if(result){
  state->controls[page].push_back(result);
  SendMessageW(result,WM_SETFONT,reinterpret_cast<WPARAM>(GetStockObject(DEFAULT_GUI_FONT)),TRUE);
 }
 return result;
}
inline void pref_label(HWND dialog,FalewebPrefsDialog* state,int page,
 LPCWSTR title,int x,int y,int w,int h=22){
 pref_control(dialog,state,page,L"STATIC",title,SS_LEFT|SS_NOPREFIX|SS_ENDELLIPSIS,x,y,w,h);
}
inline HWND pref_edit(HWND dialog,FalewebPrefsDialog* state,int page,
 const std::wstring& initial,int x,int y,int w,bool password=false){
 HWND result=pref_control(dialog,state,page,L"EDIT",initial.c_str(),
 WS_BORDER|WS_TABSTOP|ES_AUTOHSCROLL|(password?ES_PASSWORD:0),
 x,y,w,25);
 if(result)SendMessageW(result,EM_LIMITTEXT,password?256:512,0);
 return result;
}
inline void pref_list_items(HWND combo,const std::vector<std::wstring>& choices,int selected){
 if(!combo)return;
 for(const auto& item:choices)
  SendMessageW(combo,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(item.c_str()));
 if(selected>=0 && selected<static_cast<int>(choices.size()))
  SendMessageW(combo,CB_SETCURSEL,selected,0);
}
inline void pref_select_page(FalewebPrefsDialog* state,int page){
 state->page=page;
 for(int i=0;i<6;++i){
  for(const auto child:state->controls[i])
   ShowWindow(child,i==page?SW_SHOW:SW_HIDE);
  if(state->tabs[i])SendMessageW(state->tabs[i],BM_SETSTATE,i==page,0);
 }
}
inline INT_PTR CALLBACK faleweb_preferences_proc(HWND dialog,UINT message,WPARAM wp,LPARAM lp){
 auto* state=reinterpret_cast<FalewebPrefsDialog*>(GetWindowLongPtrW(dialog,DWLP_USER));
 if(message==WM_INITDIALOG){
  state=reinterpret_cast<FalewebPrefsDialog*>(lp);
  SetWindowLongPtrW(dialog,DWLP_USER,lp);
  SetWindowTextW(dialog,L"AXE Voice - Preferencias");
  RECT client{};GetClientRect(dialog,&client);
  const wchar_t* titles[]={L"Identificacao",L"Gerais",L"Audio",L"Contatos",L"Chamada",L"Avancadas"};
  const int usable=pref_px(dialog,498);
  const int tabw=usable/6;
  for(int i=0;i<6;++i){
   state->tabs[i]=CreateWindowExW(0,L"BUTTON",titles[i],
    WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_PUSHBUTTON,
    pref_px(dialog,12)+i*tabw,pref_px(dialog,15),tabw-pref_px(dialog,3),
    pref_px(dialog,34),dialog,
    reinterpret_cast<HMENU>(static_cast<INT_PTR>(700+i)),
    GetModuleHandleW(nullptr),nullptr);
   SendMessageW(state->tabs[i],WM_SETFONT,reinterpret_cast<WPARAM>(GetStockObject(DEFAULT_GUI_FONT)),TRUE);
  }
  // Identification is backed by the live native registration controls.
  pref_label(dialog,state,0,L"Servidor SIP / Dominio",28,75,210);
  state->server=pref_edit(dialog,state,0,state->data.server,240,72,230);
  pref_label(dialog,state,0,L"Ramal / Usuario SIP",28,115,200);
  state->user=pref_edit(dialog,state,0,state->data.user,240,112,230);
  pref_label(dialog,state,0,L"Porta do servidor",28,155,200);
  state->port=pref_edit(dialog,state,0,state->data.port,240,152,230);
  pref_label(dialog,state,0,L"Senha SIP",28,195,206);
  state->password=pref_edit(dialog,state,0,L"",240,192,230,true);
  state->save_password=pref_control(dialog,state,0,L"BUTTON",
   L"Salvar senha no Gerenciador de Credenciais do Windows",
   BS_AUTOCHECKBOX|WS_TABSTOP,28,226,450,27);
  if(state->save_password)
   SendMessageW(state->save_password,BM_SETCHECK,
    state->data.save_password?BST_CHECKED:BST_UNCHECKED,0);
  pref_label(dialog,state,0,
   state->data.saved_password_exists
    ?L"Senha ja salva. Deixe em branco para manter a senha armazenada."
    :L"Opcional: sem marcar, a senha nao sera armazenada.",
   28,254,445,28);
  state->tls=pref_control(dialog,state,0,L"BUTTON",L"Usar TLS (CA validada)",
   WS_TABSTOP|BS_AUTOCHECKBOX,28,293,280,26);
  if(state->tls)SendMessageW(state->tls,BM_SETCHECK,state->data.tls?BST_CHECKED:BST_UNCHECKED,0);
  pref_label(dialog,state,0,L"Certificado CA PEM",28,332,200);
  state->ca=pref_edit(dialog,state,0,state->data.ca_file,190,329,280);
  // General options map to the existing ringtone state.
  pref_label(dialog,state,1,L"Notificacao de chamadas recebidas",28,83,385);
  state->muted=pref_control(dialog,state,1,L"BUTTON",L"Habilitar alerta sonoro",
   BS_AUTOCHECKBOX|WS_TABSTOP,28,121,330,27);
  if(state->muted)SendMessageW(state->muted,BM_SETCHECK,state->data.mute_ringtone?BST_UNCHECKED:BST_CHECKED,0);
  pref_label(dialog,state,1,L"Toque de chamada",28,168,340);
  state->ringtone=pref_control(dialog,state,1,L"COMBOBOX",L"",
   CBS_DROPDOWNLIST|WS_VSCROLL|WS_TABSTOP,28,199,442,180);
  pref_list_items(state->ringtone,{L"Classico",L"Suave",L"Padrao",L"Moderno",L"Agudo",L"MP3 personalizado"},
   state->data.ringtone);
  pref_label(dialog,state,1,L"Selecao de MP3 personalizada disponivel nos ajustes de audio.",28,251,442,38);
  // Device choices are supplied by the app's current Windows audio inventory.
  const wchar_t* audio_names[]={L"Entrada - Microfone / Headset",L"Saida - Voz / Fone",L"Saida - Toque"};
  HWND* combos[]={&state->inputs,&state->outputs,&state->rings};
  const std::vector<std::wstring>* lists[]={&state->data.inputs,&state->data.outputs,&state->data.rings};
  const int indices[]={state->data.input_index,state->data.output_index,state->data.ring_index};
  for(int i=0;i<3;++i){
   pref_label(dialog,state,2,audio_names[i],28,83+i*85,440);
   *combos[i]=pref_control(dialog,state,2,L"COMBOBOX",L"",
    CBS_DROPDOWNLIST|WS_VSCROLL|WS_TABSTOP,28,113+i*85,442,180);
   pref_list_items(*combos[i],*lists[i],indices[i]);
  }
  pref_label(dialog,state,3,L"Agenda local e contatos do AXE Voice",28,85,440);
  pref_label(dialog,state,3,L"O gerenciamento da agenda permanece no recurso CONTATOS.",28,125,440,55);
  pref_label(dialog,state,4,L"Discagem e transferencia",28,85,440);
  pref_label(dialog,state,4,L"As regras da central e as rotas seguem no PABX SIP.",28,125,440,55);
  pref_label(dialog,state,5,L"Seguranca e conectividade",28,85,440);
  pref_label(dialog,state,5,L"TLS exige certificado CA confiavel. Opcoes de codecs / QoS e atualizacoes nao sao alteradas por esta janela.",28,126,440,65);
  const int bottom=static_cast<int>(client.bottom)-pref_px(dialog,43);
  auto button=[&](LPCWSTR title,int id,int x){
   HWND h=CreateWindowExW(0,L"BUTTON",title,WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_PUSHBUTTON,
    x,bottom,pref_px(dialog,98),pref_px(dialog,31),dialog,reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),
    GetModuleHandleW(nullptr),nullptr);
   SendMessageW(h,WM_SETFONT,reinterpret_cast<WPARAM>(GetStockObject(DEFAULT_GUI_FONT)),TRUE);
  };
  button(L"SALVAR",IDOK,static_cast<int>(client.right)-pref_px(dialog,222));
  button(L"CANCELAR",IDCANCEL,static_cast<int>(client.right)-pref_px(dialog,115));
  pref_select_page(state,0);
  return TRUE;
 }
 if(!state)return FALSE;
 if(message==WM_COMMAND){
  const int id=LOWORD(wp);
  if(id>=700 && id<706){pref_select_page(state,id-700);return TRUE;}
  if(id==IDCANCEL){EndDialog(dialog,IDCANCEL);return TRUE;}
  if(id==IDOK){
   state->data.server=pref_edit_text(state->server);
   state->data.user=pref_edit_text(state->user);
   state->data.port=pref_edit_text(state->port);
   state->data.password=pref_edit_text(state->password);
   state->data.save_password=SendMessageW(state->save_password,BM_GETCHECK,0,0)==BST_CHECKED;
   state->data.ca_file=pref_edit_text(state->ca);
   state->data.tls=SendMessageW(state->tls,BM_GETCHECK,0,0)==BST_CHECKED;
   state->data.mute_ringtone=SendMessageW(state->muted,BM_GETCHECK,0,0)!=BST_CHECKED;
   state->data.ringtone=static_cast<int>(SendMessageW(state->ringtone,CB_GETCURSEL,0,0));
   state->data.input_index=static_cast<int>(SendMessageW(state->inputs,CB_GETCURSEL,0,0));
   state->data.output_index=static_cast<int>(SendMessageW(state->outputs,CB_GETCURSEL,0,0));
   state->data.ring_index=static_cast<int>(SendMessageW(state->rings,CB_GETCURSEL,0,0));
   if(state->data.save_password && state->data.password.empty() &&
      !state->data.saved_password_exists){
    MessageBoxW(dialog,L"Digite uma senha para habilitar o salvamento seguro.",
      L"Senha SIP",MB_OK|MB_ICONWARNING);
    pref_select_page(state,0);
    SetFocus(state->password);
    return TRUE;
   }
   EndDialog(dialog,IDOK);return TRUE;
  }
 }
 if(message==WM_CLOSE){EndDialog(dialog,IDCANCEL);return TRUE;}
 return FALSE;
}
inline bool edit_faleweb_preferences(HWND owner,FalewebPreferences& data){
 struct alignas(DWORD) Template {
  DLGTEMPLATE header{};
  WORD menu=0,cls=0,title=0;
 } spec{};
 spec.header.style=WS_POPUP|WS_VISIBLE|WS_CAPTION|WS_SYSMENU|DS_MODALFRAME|DS_CENTER;
 spec.header.cx=355;spec.header.cy=330;
 FalewebPrefsDialog state{};
 state.data=data;
 const auto result=DialogBoxIndirectParamW(GetModuleHandleW(nullptr),&spec.header,
  owner,faleweb_preferences_proc,reinterpret_cast<LPARAM>(&state));
 if(result!=IDOK)return false;
 data=std::move(state.data);
 return true;
}
} // namespace axv
