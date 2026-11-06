#pragma once
// AXE Voice - approved Option 1 (Essencial claro) native Win32 skin.
// Drawing only: SIP state and call commands remain owned by main.cpp.
#include <windows.h>
#include <algorithm>
#include <string>
#include <cwchar>

namespace axv::essential {
inline int px(unsigned dpi,int v) { return MulDiv(v,static_cast<int>(dpi),96); }
inline RECT box(unsigned dpi,int x,int y,int w,int h) {
    return {px(dpi,x),px(dpi,y),px(dpi,x+w),px(dpi,y+h)};
}
inline void rounded(HDC dc,RECT r,COLORREF fill,COLORREF outline,int radius,unsigned dpi) {
    HBRUSH brush=CreateSolidBrush(fill);
    HPEN pen=CreatePen(PS_SOLID,px(dpi,1),outline);
    HGDIOBJ oldb=SelectObject(dc,brush),oldp=SelectObject(dc,pen);
    RoundRect(dc,r.left,r.top,r.right,r.bottom,px(dpi,radius),px(dpi,radius));
    SelectObject(dc,oldp); SelectObject(dc,oldb);
    DeleteObject(brush); DeleteObject(pen);
}
inline void line(HDC dc,unsigned dpi,int x1,int y1,int x2,int y2,COLORREF color,int weight=2) {
    HPEN p=CreatePen(PS_SOLID,std::max(1,px(dpi,weight)),color);
    HGDIOBJ old=SelectObject(dc,p);
    MoveToEx(dc,px(dpi,x1),px(dpi,y1),nullptr); LineTo(dc,px(dpi,x2),px(dpi,y2));
    SelectObject(dc,old); DeleteObject(p);
}
inline void dot(HDC dc,unsigned dpi,int cx,int cy,int radius,COLORREF color){
    const RECT r=box(dpi,cx-radius,cy-radius,radius*2,radius*2);
    HBRUSH b=CreateSolidBrush(color);HGDIOBJ old=SelectObject(dc,b);
    HGDIOBJ oldp=SelectObject(dc,GetStockObject(NULL_PEN));
    Ellipse(dc,r.left,r.top,r.right,r.bottom);
    SelectObject(dc,oldp);SelectObject(dc,old);DeleteObject(b);
}
inline void label(HDC dc,const wchar_t* txt,RECT r,unsigned dpi,int size,
                  int weight,COLORREF fg,UINT align=DT_CENTER){
    HFONT font=CreateFontW(-px(dpi,size),0,0,0,weight,FALSE,FALSE,FALSE,
         DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,
         DEFAULT_PITCH|FF_SWISS,L"Segoe UI");
    HGDIOBJ old=SelectObject(dc,font);
    SetBkMode(dc,TRANSPARENT);SetTextColor(dc,fg);
    DrawTextW(dc,txt,-1,&r,align|DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS|DT_NOPREFIX);
    SelectObject(dc,old);DeleteObject(font);
}
inline void logo(HDC dc,unsigned dpi){
    // Compact blue A mark, drawn at native DPI without a remote asset.
    const POINT raw[]={{20,64},{39,18},{59,18},{82,64},{66,64},{59,51},{43,51},{36,64}};
    POINT pts[8];for(int i=0;i<8;++i)pts[i]={px(dpi,raw[i].x),px(dpi,raw[i].y)};
    HBRUSH b=CreateSolidBrush(RGB(0,151,230));
    HGDIOBJ oldb=SelectObject(dc,b),oldp=SelectObject(dc,GetStockObject(NULL_PEN));
    Polygon(dc,pts,8);SelectObject(dc,oldp);SelectObject(dc,oldb);DeleteObject(b);
    rounded(dc,box(dpi,43,36,16,6),RGB(255,255,255),RGB(255,255,255),5,dpi);
    dot(dc,dpi,50,30,3,RGB(255,255,255));
    line(dc,dpi,42,43,48,46,RGB(255,255,255),3);
    line(dc,dpi,48,46,57,41,RGB(255,255,255),3);
}
inline void icon(HDC dc,unsigned dpi,int id,int cx,int cy,COLORREF fg){
    // Simple vector glyphs, placed above labels rather than overlapping them.
    switch(id){
    case 2001: // keypad grid
        for(int r=0;r<3;++r)for(int c=0;c<3;++c)
            dot(dc,dpi,cx+(c-1)*8,cy+(r-1)*8,2,fg);
        break;
    case 2013: // contacts
        dot(dc,dpi,cx,cy-7,6,fg);
        rounded(dc,box(dpi,cx-11,cy+1,22,15),RGB(250,252,255),fg,12,dpi);
        break;
    case 2011: // history
        dot(dc,dpi,cx,cy,13,fg);
        dot(dc,dpi,cx,cy,11,RGB(250,252,255));
        line(dc,dpi,cx,cy,cx,cy-7,fg,2);line(dc,dpi,cx,cy,cx+6,cy+3,fg,2);
        break;
    case 2015: // settings gear - ring and four tick marks
        dot(dc,dpi,cx,cy,12,fg);
        dot(dc,dpi,cx,cy,9,RGB(250,252,255));
        dot(dc,dpi,cx,cy,4,fg);
        line(dc,dpi,cx-15,cy,cx-10,cy,fg);
        line(dc,dpi,cx+10,cy,cx+15,cy,fg);
        line(dc,dpi,cx,cy-15,cx,cy-10,fg);
        line(dc,dpi,cx,cy+10,cx,cy+15,fg);
        break;
    default: break;
    }
}
inline void shell(HDC dc,unsigned dpi,bool dark,bool connected,const wchar_t* account){
    const COLORREF navy=dark?RGB(243,249,255):RGB(8,31,69);
    const COLORREF paper=dark?RGB(9,24,49):RGB(255,255,255);
    const COLORREF border=dark?RGB(55,83,117):RGB(222,229,239);
    // Header 0..73, connection band 73..127, bottom navigation 669..752.
    HBRUSH bg=CreateSolidBrush(paper);
    FillRect(dc,&box(dpi,0,0,444,752),bg);DeleteObject(bg);
    line(dc,dpi,0,72,444,72,border,1);
    line(dc,dpi,0,126,444,126,border,1);
    line(dc,dpi,0,668,444,668,border,1);
    if(!dark)logo(dc,dpi);
    else logo(dc,dpi);
    label(dc,L"AXE Voice",box(dpi,92,12,243,56),dpi,27,FW_BOLD,navy,DT_LEFT);
    dot(dc,dpi,30,100,8,connected?RGB(6,192,61):RGB(220,63,63));
    label(dc,connected?L"Conectado":L"Desconectado",
           box(dpi,49,78,133,45),dpi,18,FW_SEMIBOLD,navy,DT_LEFT);
    line(dc,dpi,181,85,181,114,border,2);
    label(dc,account&&*account?account:L"Ramal --",
          box(dpi,208,78,146,45),dpi,17,FW_NORMAL,navy,DT_LEFT);
    rounded(dc,box(dpi,20,133,404,66),
            dark?RGB(27,57,93):RGB(247,249,252),
            dark?RGB(88,123,167):RGB(191,205,224),13,dpi);
}
inline void button(HDC dc,RECT r,unsigned dpi,int id,bool disabled,bool pressed,bool dark){
    const COLORREF navy=dark?RGB(245,249,255):RGB(8,31,69);
    const COLORREF white=dark?RGB(24,50,81):RGB(255,255,255);
    const COLORREF edge=dark?RGB(65,99,137):RGB(186,201,222);
    const COLORREF muted=dark?RGB(181,198,221):RGB(64,83,112);
    const int digit_index=id-2100;
    COLORREF fill=white,stroke=edge,fg=navy;
    int curve=12;
    const bool nav=(id==2001||id==2013||id==2011||id==2015);
    if(nav){
        fill=id==2001?(dark?RGB(2,86,139):RGB(216,244,255)):white;
        stroke=fill;fg=id==2001?RGB(0,111,202):navy;curve=11;
    }else if(id==1001||id==1018){
        fill=disabled?RGB(83,164,112):RGB(5,170,76);stroke=fill;fg=RGB(255,255,255);
    }else if(id==2304){
        fill=disabled?RGB(177,119,123):RGB(206,48,57);stroke=fill;fg=RGB(255,255,255);
    }else if(id==2003){
        stroke=RGB(0,179,235);curve=25;
    }else if(id==2201){
        fill=dark?RGB(27,57,93):RGB(247,249,252);
        stroke=fill;
    }else if(id==2801||id==2802){
        stroke=fill;curve=6;
    }else if(pressed){
        fill=dark?RGB(36,94,149):RGB(220,240,252);
    }
    rounded(dc,r,fill,stroke,curve,dpi);
    if(nav){
        int center=(r.left+r.right)/2;
        const int top=r.top+px(dpi,21);
        // Helper expects logical coordinates: convert physical coordinates back.
        icon(dc,dpi,id,MulDiv(center,96,dpi),MulDiv(top,96,dpi),fg);
        RECT txt=r;txt.top=r.top+px(dpi,39);txt.bottom=r.bottom-px(dpi,2);
        const wchar_t* caption=id==2001?L"Teclado":id==2013?L"Contatos":id==2011?L"Histórico":L"Ajustes";
        label(dc,caption,txt,dpi,14,id==2001?FW_SEMIBOLD:FW_NORMAL,fg);
    }else if(digit_index>=0&&digit_index<12){
        const wchar_t* nums[]={L"1",L"2",L"3",L"4",L"5",L"6",L"7",L"8",L"9",L"*",L"0",L"#"};
        const wchar_t* letters[]={L"",L"ABC",L"DEF",L"GHI",L"JKL",L"MNO",L"PQRS",L"TUV",L"WXYZ",L"",L"+",L""};
        RECT digit=r;digit.top+=px(dpi,2);digit.bottom=r.top+px(dpi,39);
        label(dc,nums[digit_index],digit,dpi,29,FW_BOLD,fg);
        RECT sub=r;sub.top=r.top+px(dpi,36);sub.bottom=r.bottom-px(dpi,2);
        label(dc,letters[digit_index],sub,dpi,14,FW_NORMAL,muted);
    }else if(id==1001||id==1018||id==2304){
        const wchar_t* title=id==2304?L"Encerrar":id==1018?L"Atender":L"Ligar";
        RECT text=r;text.left+=px(dpi,35);
        label(dc,title,text,dpi,26,FW_SEMIBOLD,fg);
        label(dc,L"☎",box(dpi,MulDiv(r.left,96,dpi)+125,
            MulDiv((r.top+r.bottom)/2,96,dpi)-20,46,40),dpi,28,FW_BOLD,fg);
    }else if(id==2003){
        label(dc,dark?L"☾":L"☼",r,dpi,28,FW_NORMAL,fg);
    }else if(id==2201){
        label(dc,L"⌫",r,dpi,29,FW_NORMAL,fg);
    }else if(id==2801){
        label(dc,L"—",r,dpi,21,FW_NORMAL,fg);
    }else if(id==2802){
        label(dc,L"×",r,dpi,25,FW_NORMAL,fg);
    }else{
        wchar_t caption[80]{}; // Text is provided by main.cpp below for live mute/hold.
        const int chars=GetWindowTextW(WindowFromDC(dc),caption,80);
        (void)chars;
        // The actual button caption is drawn by caller's text-aware variant.
    }
    if(disabled && !nav) {
        // Disabled actions remain visible; the command gate is unchanged.
    }
}
inline void action_caption(HDC dc,RECT r,unsigned dpi,const wchar_t* title,bool dark,bool disabled){
    label(dc,title,r,dpi,15,FW_SEMIBOLD,
          disabled?(dark?RGB(150,173,201):RGB(105,123,145)):
                   (dark?RGB(245,249,255):RGB(8,31,69)));
}
} // namespace axv::essential
