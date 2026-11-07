#include "../src/ui_rounded_geometry.h"
#include <cassert>
#include <climits>
#include <cstdio>

int main() {
    using namespace axv;
    static_assert(rounded_rect_diameter(100,40,20)==40,"full pill allowed");
    static_assert(rounded_rect_diameter(70,35,20,1)==33,"inset clamps arc");
    static_assert(rounded_rect_diameter(0,40,20)==0,"empty bounds");
    static_assert(rounded_rect_diameter(40,40,0)==0,"square fallback");
    static_assert(rounded_rect_diameter(100,40,INT_MAX)==40,"no radius overflow");
    static_assert(rounded_rect_diameter(1,1,20,1)==0,"no negative inner rect");
    static_assert(rounded_rect_diameter(3,3,20,1)==1,"one pixel remains within bounds");

    unsigned checked=0;
    for (unsigned dpi : {96u,120u,144u,192u}) {
        const int inset=dpi_pixels(1,dpi);
        // Approved compact 3x4 keypad cell (129x64 logical pixels).
        const int width=dpi_pixels(129,dpi), height=dpi_pixels(64,dpi);
        const int curve=compact_button_diameter(width,height,dpi,true,inset);
        assert(curve>dpi_pixels(24,dpi) && curve<=height-2*inset);
        // Smaller footer/navigation buttons must not overshoot their fill.
        const int footer=compact_button_diameter(dpi_pixels(52,dpi),
            dpi_pixels(32,dpi),dpi,true,inset);
        assert(footer>0 && footer<=dpi_pixels(32,dpi)-2*inset);
        for(int w=1;w<=80;++w){
            for(int h=1;h<=80;++h){
                const int d=compact_button_diameter(w,h,dpi,true,inset);
                assert(d>=0 && d<=w && d<=h);
                if(d) assert(d<=w-2*inset && d<=h-2*inset);
                ++checked;
            }
        }
    }
    std::printf("DPI-aware rounded geometry: %u dimensions passed\n",checked);
    return 0;
}
