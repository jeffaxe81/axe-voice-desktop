#include "../src/baresip_registration_watchdog.h"
#include <cstdint>
#include <limits>

int main() {
    using namespace axv;
    RegistrationDeadline d;
    if (d.check(1,0,false)!=RegistrationDeadlineVerdict::Unarmed) return 1;
    if (d.arm(0,10,100) || d.arm(1,10,0)) return 2;
    if (!d.arm(1,10,100) || d.arm(2,10,100)) return 3;
    if (d.check(1,10,false)!=RegistrationDeadlineVerdict::Pending) return 4;
    if (d.check(1,109,false)!=RegistrationDeadlineVerdict::Pending) return 5;
    if (d.check(1,109,true)!=RegistrationDeadlineVerdict::CallbackConfirmed) return 6;
    if (d.check(1,110,true)!=RegistrationDeadlineVerdict::Expired) return 7;
    if (d.check(2,15,true)!=RegistrationDeadlineVerdict::SessionRevoked) return 8;
    if (d.check(1,9,false)!=RegistrationDeadlineVerdict::ClockRegressed) return 9;
    d.clear();
    if (d.armed() || d.check(1,110,false)!=RegistrationDeadlineVerdict::Unarmed)
        return 10;
    const auto max=std::numeric_limits<std::uint64_t>::max();
    if (!d.arm(5,max-40,80)) return 11;
    if (d.check(5,max-1,false)!=RegistrationDeadlineVerdict::Pending) return 12;
    // Never use start+timeout: that addition could wrap and falsely expire.
    if (d.check(5,1,false)!=RegistrationDeadlineVerdict::ClockRegressed) return 13;
    d.clear();
    if (!d.arm(6,0,1)) return 14;
    if (d.check(6,0,false)!=RegistrationDeadlineVerdict::Pending ||
        d.check(6,1,false)!=RegistrationDeadlineVerdict::Expired) return 15;
    return 0;
}
