#include "../src/ui_dpi_layout.h"
int main() {
    using namespace axv;
    UiLogicalRect r{39,272,350,43};
    auto a=scale_rect(r,96);
    if(a.x!=39||a.y!=272||a.width!=350||a.height!=43) return 1;
    auto b=scale_rect(r,120);
    if(b.x!=49||b.y!=340||b.width!=438||b.height!=54) return 2;
    auto c=scale_rect(r,144);
    if(c.x!=59||c.y!=408||c.width!=525||c.height!=65) return 3;
    auto d=scale_rect(r,192);
    if(d.x!=78||d.y!=544||d.width!=700||d.height!=86) return 4;
    if(scale_rect(r,144).x!=c.x||scale_rect(r,96).x!=a.x) return 5;
    if(scale_dpi(-39,144)!=-59||scale_dpi(0,240)!=0) return 6;
    if(scale_dpi(350,0)!=350||scale_dpi(350,900)!=2800) return 7;
    if(scale_dpi(246,192)>=scale_dpi(272,192)) return 8;
    return 0;
}
