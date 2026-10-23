#include "../src/ui_viewport_policy.h"
int main() {
    using namespace axv;
    if(max_scroll_offset(790,800)!=0 || max_scroll_offset(900,400)!=500) return 1;
    if(clamp_scroll_offset(-30,900,400)!=0 || clamp_scroll_offset(700,900,400)!=500) return 2;
    if(clamp_scroll_offset(130,900,400)!=130) return 3;
    // A below-viewport control should scroll into view.
    if(scroll_for_visible_range(0,900,400,660,700,8)!=308) return 4;
    // A preceding control scrolls upward, preserving a margin.
    if(scroll_for_visible_range(300,900,400,100,130,8)!=92) return 5;
    // Already visible: don't disturb current scroll.
    if(scroll_for_visible_range(300,900,400,350,390,8)!=300) return 6;
    // Tall control: align its top and clamp.
    if(scroll_for_visible_range(0,900,400,500,890,8)!=492) return 7;
    if(scroll_for_visible_range(0,900,400,500,910,8)!=492) return 8;
    // After content shrinks to fit, all offsets return to zero.
    if(clamp_scroll_offset(200,300,400)!=0) return 9;
    if(scroll_for_visible_range(0,900,400,-40,10,8)!=0) return 10;
    return 0;
}
