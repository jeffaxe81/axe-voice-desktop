#include "../src/ui_dpi_policy.h"
using namespace axv;
static_assert(normalized_dpi(0)==96);
static_assert(normalized_dpi(96)==96);
static_assert(normalized_dpi(144)==144);
static_assert(normalized_dpi(480)==384);
static_assert(dpi_pixels(438,96)==438);
static_assert(dpi_pixels(438,144)==657);
static_assert(dpi_pixels(438,192)==876);
static_assert(dpi_pixels(1,144)==2);
static_assert(dpi_pixels(3,144)==5);
static_assert(dpi_pixels(-3,144)==-5);
static_assert(scaled_box({39,272,350,43},144).x==59);
static_assert(scaled_box({39,272,350,43},144).y==408);
static_assert(scaled_box({39,272,350,43},144).width==525);
static_assert(scaled_box({39,272,350,43},144).height==65);
static_assert(footer_boxes(438, 720, 48, 96).panel.y == 672);
static_assert(footer_boxes(438, 720, 48, 96).message.x == 12);
static_assert(footer_boxes(438, 720, 48, 96).message.height == 38);
static_assert(footer_boxes(20, 14, 48, 96).message.x == 9);
static_assert(footer_boxes(20, 14, 48, 96).message.height == 4);
static_assert(footer_boxes(438, 720, 60, 120).message.y == 665);

int main() {
    for(int a=0;a<=842;++a) {
        if(dpi_pixels(a,96)!=a) return 1;
        if(dpi_pixels(a,120)>dpi_pixels(a,144)) return 2;
        if(dpi_pixels(a,144)>dpi_pixels(a,192)) return 3;
    }
    // Geometry regression across 100%, 125%, 150%, 200% and 400% DPI.
    // Windows may temporarily make the client smaller than the footer.
    for(unsigned dpi : {96u, 120u, 144u, 192u, 384u}) {
        for(int width=1; width<=550; width+=3) {
            for(int height=1; height<=900; height+=7) {
                const auto boxes=footer_boxes(width,height,dpi_pixels(48,dpi),dpi);
                const auto p=boxes.panel, m=boxes.message;
                if(p.x!=0 || p.y<0 || p.width!=width || p.height<1 ||
                   p.y+p.height!=height) return 10;
                if(m.width<1 || m.height<1 || m.x<p.x || m.y<p.y ||
                   m.x+m.width>p.x+p.width ||
                   m.y+m.height>p.y+p.height) return 11;
            }
        }
    }
    return 0;
}
