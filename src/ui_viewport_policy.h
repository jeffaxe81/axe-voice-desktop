#pragma once
#include <algorithm>
namespace axv {
// Pure viewport arithmetic; no Windows handles, SIP state or persistence.
inline int max_scroll_offset(int content_height, int visible_height) {
    return std::max(0, content_height - std::max(0,visible_height));
}
inline int clamp_scroll_offset(int requested, int content_height, int visible_height) {
    return std::max(0, std::min(max_scroll_offset(content_height,visible_height),requested));
}
inline int scroll_for_visible_range(int current, int content, int visible,
                                    int top, int bottom, int margin=0) {
    const int inset = std::max(0,margin);
    const int page = std::max(0,visible);
    // For very tall controls, prioritize their top edge.
    int next = current;
    if (top-inset < current) next = top-inset;
    else if (bottom+inset > current+page) {
        next = (bottom-top+2*inset > page) ? top-inset : bottom+inset-page;
    }
    return clamp_scroll_offset(next,content,page);
}
} // namespace axv
