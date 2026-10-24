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
int main() {
    for(int a=0;a<=842;++a) {
        if(dpi_pixels(a,96)!=a) return 1;
        if(dpi_pixels(a,120)>dpi_pixels(a,144)) return 2;
        if(dpi_pixels(a,144)>dpi_pixels(a,192)) return 3;
    }
    return 0;
}
