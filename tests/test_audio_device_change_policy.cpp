#include "../src/audio_device_change_policy.h"
#include <cstdio>
#include <cstdlib>
#define CHECK(x) do { if (!(x)) { std::fprintf(stderr,"%s:%d: %s\n",__FILE__,__LINE__,#x); std::abort(); } } while(false)
#include <cstdint>
#include <limits>

using axv::AudioDeviceChangePolicy;

int main() {
    AudioDeviceChangePolicy p;
    CHECK(!p.should_refresh(10000, false));
    p.changed(0);
    p.enter(0);
    CHECK(!p.should_refresh(249, false));
    CHECK(!p.should_refresh(4999, false));
    CHECK(p.should_refresh(5000, false));
    CHECK(!p.should_refresh(5000, true));
    p.refreshed(5000);
    p.changed(5100);
    CHECK(!p.should_refresh(5349, false));
    CHECK(p.should_refresh(5350, false));
    p.changed(5300);
    CHECK(!p.should_refresh(5549, false));
    CHECK(p.should_refresh(5550, false));
    CHECK(!p.should_refresh(5550, true));
    p.refreshed(5550);
    CHECK(!p.should_refresh(10549, false));
    CHECK(p.should_refresh(10550, false));

    p.enter(0);
    for (std::uint64_t t = 100; t <= 4900; t += 100) p.changed(t);
    CHECK(!p.should_refresh(4999, false));
    CHECK(p.should_refresh(5000, false));
    p.leave();
    p.changed(5000);
    CHECK(!p.should_refresh(100000, false));
    p.enter(10000);
    CHECK(!p.should_refresh(14999, false));
    CHECK(p.should_refresh(15000, false));
    p.changed(12000);
    p.changed(11000);
    CHECK(!p.should_refresh(12249, false));
    CHECK(p.should_refresh(12250, false));
    p.refreshed(11500);
    CHECK(!p.should_refresh(16999, false));
    CHECK(p.should_refresh(17000, false));

    constexpr auto max = std::numeric_limits<std::uint64_t>::max();
    p.enter(max - 100);
    CHECK(!p.should_refresh(max - 1, false));
    CHECK(p.should_refresh(max, false));
    p.changed(max - 50);
    CHECK(!p.should_refresh(max - 1, false));
    CHECK(p.should_refresh(max, false));
    p.refreshed(max);
    CHECK(!p.should_refresh(0, false));
    CHECK(p.should_refresh(max, false));
    p.leave();
    p.enter(0);
    p.changed(0);
    CHECK(!p.should_refresh(249, false));
    CHECK(p.should_refresh(250, false));

    CHECK(axv::is_audio_device_change(0x8000));
    CHECK(axv::is_audio_device_change(0x8004));
    CHECK(axv::is_audio_device_change(0x0007));
    CHECK(axv::is_audio_device_change(0x0018));
    for (std::uintptr_t kind = 0; kind <= 0x10000; ++kind)
        CHECK(axv::is_audio_device_change(kind) ==
               (kind == 0x8000 || kind == 0x8004 || kind == 7 || kind == 0x18));
    CHECK(!axv::is_audio_device_change(std::numeric_limits<std::uintptr_t>::max()));
}
