// Deterministic policy/property testing, NOT 10 million real phone calls.
// Five groups of 2 million synthetic scenarios each, covering production
// policy functions only. No accounts, internet, SIP registration, Windows UI.
#include "../src/ui_viewport_policy.h"
#include "../src/ui_dpi_policy.h"
#include "../src/dialpad_model.h"
#include "../src/sip_runtime_observation.h"
#include "../src/baresip_event_reducer.h"
#include "../src/codec_policy.h"
#include "../src/release_scope_policy.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <limits>
#include <string>
#include <string_view>

namespace {
constexpr std::uint64_t kSeed = 0x4158455631304DULL; // "AXEV10M"
constexpr std::uint64_t kPerGroup = 2'000'000;
constexpr std::uint64_t kGroupCount = 5;
constexpr std::uint64_t kTotal = kPerGroup * kGroupCount;
constexpr std::int64_t kMin = std::numeric_limits<int>::min();
constexpr std::int64_t kMax = std::numeric_limits<int>::max();

struct Rng {
    std::uint64_t state=kSeed;
    std::uint64_t next() noexcept {
        // SplitMix64: stable across compilers, no random_device or entropy.
        auto z=(state+=0x9e3779b97f4a7c15ULL);
        z=(z^(z>>30))*0xbf58476d1ce4e5b9ULL;
        z=(z^(z>>27))*0x94d049bb133111ebULL;
        return z^(z>>31);
    }
    std::uint32_t small(std::uint32_t max) noexcept {
        return static_cast<std::uint32_t>(next()%max);
    }
    int coord(std::uint64_t i) noexcept {
        switch(i%64) {
        case 0: return std::numeric_limits<int>::min();
        case 1: return std::numeric_limits<int>::max();
        case 2: return -1;
        case 3: return 0;
        case 4: return 1;
        case 5: return 96;
        case 6: return 384;
        default: {
            std::int64_t x=static_cast<std::int64_t>(next()&0xffffffffULL);
            if (x>kMax) x-=0x1'0000'0000LL;
            return static_cast<int>(x);
        }
        }
    }
};

[[noreturn]] void fail(const char* domain,std::uint64_t scenario, const char* issue) {
    std::cerr << "10M property FAIL " << domain << " index=" << scenario
              << " reason=" << issue << " seed=" << kSeed << '\n';
    std::exit(1);
}

int bounded(std::int64_t v) {
    return static_cast<int>(std::max(kMin,std::min(kMax,v)));
}

int viewport_limit(int content,int visible) {
    return bounded(std::max<std::int64_t>(0,
        static_cast<std::int64_t>(content)-std::max(0,visible)));
}

void viewport(Rng& rng) {
    for(std::uint64_t i=0;i<kPerGroup;++i) {
        int content=rng.coord(i),visible=rng.coord(i+1),current=rng.coord(i+2);
        int top=rng.coord(i+3),bottom=rng.coord(i+4),margin=rng.coord(i+5);
        const int limit=viewport_limit(content,visible);
        if(axv::max_scroll_offset(content,visible)!=limit)
            fail("viewport",i,"overflow or range mismatch");
        const int clamped=static_cast<int>(std::max<std::int64_t>(0,
            std::min<std::int64_t>(current,limit)));
        if(axv::clamp_scroll_offset(current,content,visible)!=clamped)
            fail("viewport",i,"clamp mismatch");
        const auto inset=static_cast<std::int64_t>(std::max(margin,0));
        const auto page=static_cast<std::int64_t>(std::max(visible,0));
        std::int64_t next=current;
        if(static_cast<std::int64_t>(top)-inset<current)
            next=static_cast<std::int64_t>(top)-inset;
        else if(static_cast<std::int64_t>(bottom)+inset>
                static_cast<std::int64_t>(current)+page) {
            next=(static_cast<std::int64_t>(bottom)-top+2*inset>page)
                ? static_cast<std::int64_t>(top)-inset
                : static_cast<std::int64_t>(bottom)+inset-page;
        }
        const int expected=static_cast<int>(std::max<std::int64_t>(0,
            std::min<std::int64_t>(next,limit)));
        const auto actual=axv::scroll_for_visible_range(
            current,content,visible,top,bottom,margin);
        if(actual!=expected || actual<0 || actual>limit)
            fail("viewport",i,"visible-range mismatch");
    }
}
void dpi(Rng& rng) {
    for(std::uint64_t i=0;i<kPerGroup;++i) {
        const int logical=rng.coord(i);
        const unsigned raw_dpi=rng.small(2048);
        const unsigned used=std::min(384u,std::max(96u,raw_dpi));
        const auto scaled=static_cast<std::int64_t>(logical)*used;
        const auto rounded=scaled<0 ? -((-scaled+48)/96) : (scaled+48)/96;
        if(axv::dpi_pixels(logical,raw_dpi)!=bounded(rounded))
            fail("dpi",i,"saturating scale mismatch");
        const int width=1+static_cast<int>(rng.small(3800));
        const int height=1+static_cast<int>(rng.small(3200));
        const int preferred=rng.coord(i+1)%280;
        const auto boxes=axv::footer_boxes(width,height,preferred,raw_dpi);
        const auto p=boxes.panel,m=boxes.message;
        if(p.x!=0||p.y<0||p.width!=width||p.height<1||
           static_cast<std::int64_t>(p.y)+p.height!=height||
           m.width<1||m.height<1||m.x<p.x||m.y<p.y||
           static_cast<std::int64_t>(m.x)+m.width>
                static_cast<std::int64_t>(p.x)+p.width||
           static_cast<std::int64_t>(m.y)+m.height>
                static_cast<std::int64_t>(p.y)+p.height)
            fail("dpi",i,"footer outside client");
    }
}
void dialpad(Rng& rng) {
    const std::wstring base(170,L'7');
    constexpr std::array<wchar_t,17> actions{
        L'1',L'2',L'3',L'4',L'5',L'6',L'7',L'8',L'9',L'*',L'0',L'#',
        L'@',L'\n',L'\b',L'\0',L';'};
    for(std::uint64_t i=0;i<kPerGroup;++i) {
        const auto len=static_cast<std::size_t>(rng.small(171));
        const auto start=static_cast<std::size_t>(rng.small(230));
        const auto end=static_cast<std::size_t>(rng.small(230));
        const auto limit=static_cast<std::size_t>(rng.small(190));
        const auto action=actions[rng.small(static_cast<std::uint32_t>(actions.size()))];
        const auto plan=axv::plan_dial_edit(
            std::wstring_view(base.data(),len),start,end,action,limit);
        auto a=std::min(std::min(start,len),std::min(end,len));
        auto b=std::max(std::min(start,len),std::min(end,len));
        bool expected=false;
        std::wstring replacement;
        if(action==L'\0') {expected=true;a=0;b=len;}
        else if(action==L'\b') {
            if(a==b&&a>0)--a;
            expected=a!=b;
        }else{
            const auto keys=axv::dialpad_keys();
            const bool allowed=std::find(keys.begin(),keys.end(),action)!=keys.end();
            expected=allowed && len-(b-a)<limit;
            if(expected)replacement.push_back(action);
        }
        if(plan.accepted!=expected||plan.start!=a||plan.end!=b||
           plan.replacement!=replacement)
            fail("dialpad",i,"edit plan not safe");
        // Exercise append/backspace independently on the same randomized
        // keypad action; never allow any hidden SIP metacharacter.
        if((i&7)==0) {
            std::wstring destination(base.data(),len);
            const auto keys=axv::dialpad_keys();
            const bool allowed=std::find(keys.begin(),keys.end(),action)!=keys.end();
            const bool admitted=axv::dial_append(destination,action,limit);
            if(admitted!=(allowed && len<limit)||
               destination.size()!=len+(admitted?1:0))
                fail("dialpad",i,"append admitted invalid key or limit");
            axv::dial_backspace(destination);
            if(destination.size()!=(len+(admitted?1:0)-
                 (len+(admitted?1:0)>0?1:0)))
                fail("dialpad",i,"backspace unsafe");
        }
    }
}
void sip(Rng& rng) {
    for(std::uint64_t i=0;i<kPerGroup;++i) {
        const auto process=static_cast<axv::RuntimeProcess>(rng.small(4));
        const auto status=static_cast<axv::SipStatus>(rng.small(8));
        const auto evidence=static_cast<axv::SipEvidence>(rng.small(2));
        const std::uint64_t session=1+rng.small(4096);
        const std::uint64_t current=(rng.small(5)==0)?session+1:
                                    (rng.small(5)==0?0:session);
        const auto call_id=static_cast<std::uint64_t>(rng.small(2048));
        axv::SipRuntimeObservation obs{process,status,evidence,session,call_id,call_id};
        const bool proof=process==axv::RuntimeProcess::Running &&
                         evidence==axv::SipEvidence::VerifiedNativeApi &&
                         current==session && current!=0;
        if(axv::sip_proof_is_current(obs,current)!=proof)
            fail("sip",i,"false evidence admitted");
        const auto effective=axv::effective_sip_status(obs,current);
        if(effective!=(proof?status:axv::SipStatus::Unavailable))
            fail("sip",i,"unverified state visible");
        const auto action=static_cast<axv::CallAction>(rng.small(3));
        bool allowed=false;
        if(proof) {
            allowed=(action==axv::CallAction::Dial&&status==axv::SipStatus::Registered)||
                (action==axv::CallAction::Answer&&status==axv::SipStatus::Ringing)||
                (action==axv::CallAction::Hangup&&
                (status==axv::SipStatus::Calling||status==axv::SipStatus::Ringing||
                 status==axv::SipStatus::InCall));
        }
        if(axv::may_use_call_control(obs,current,action)!=allowed)
            fail("sip",i,"call control without trusted state");
        const auto original=obs;
        const auto kind=static_cast<axv::BaresipEventKind>(rng.small(9));
        const auto event_session=(rng.small(4)==0?session+1:current);
        const std::uint64_t event_call=(rng.small(5)==0)?0:call_id;
        const auto transition=axv::apply_baresip_event(
            obs,current,{kind,event_session,event_call});
        if(transition!=axv::EventTransition::Applied &&
           (obs.process!=original.process||obs.sip!=original.sip||
            obs.evidence!=original.evidence||obs.session!=original.session||
            obs.call_id!=original.call_id||obs.last_call_id!=original.last_call_id))
            fail("sip",i,"rejected event modified state");
        if(transition==axv::EventTransition::Applied &&
           kind==axv::BaresipEventKind::TransportLost &&
           (obs.sip!=axv::SipStatus::Disconnected||obs.call_id!=0))
            fail("sip",i,"lost transport kept active call");
        const bool stale=event_session==0||event_session!=current||
                         original.session!=current;
        if(stale&&transition!=axv::EventTransition::StaleSession)
            fail("sip",i,"stale event admitted");
        if(!stale&&!proof&&transition!=axv::EventTransition::UnverifiedSource)
            fail("sip",i,"unverified event admitted");
    }
}
void release_codec(Rng& rng) {
    for(std::uint64_t i=0;i<kPerGroup;++i) {
        const std::uint32_t m=rng.small(256),n=rng.small(64);
        const axv::EngineeringReleaseEvidence e{
            bool(m&1),bool(m&2),bool(m&4),bool(m&8),
            bool(m&16),bool(m&32),bool(m&64),bool(m&128)};
        const axv::ExternalReleaseEvidence x{
            bool(n&1),bool(n&2),bool(n&4),bool(n&8),bool(n&16),bool(n&32)};
        const auto decision=axv::evaluate_release_scope(e,x);
        if(decision.engineering_delivery_ready!=(m==255)||
           decision.production_approved!=(m==255&&n==63)||
           !decision.live_telephony_deferred_from_engineering)
            fail("release",i,"production approved with missing external gate");
        const int index=static_cast<int>(rng.small(8));
        const bool verified=(rng.next()&1)!=0;
        const auto codec=static_cast<axv::Codec>(index);
        const bool expected=index>=0&&index<=4&&(index!=4||verified);
        if(axv::can_enable_codec(codec,verified)!=expected)
            fail("release",i,"codec bypassed external validation");
    }
}
}

int main(int argc,char** argv) {
    if(argc>2) {
        std::cerr<<"Usage: axv_10m_policy_stress [results.json]\n";
        return 2;
    }
    Rng rng;
    viewport(rng);
    dpi(rng);
    dialpad(rng);
    sip(rng);
    release_codec(rng);
    if(argc==2) {
        std::ofstream out(argv[1],std::ios::trunc);
        if(!out) {std::cerr<<"Cannot create stress evidence report\n";return 3;}
        out<<"{\n"
              "  \"schema_version\": 1,\n"
              "  \"seed\": \""<<kSeed<<"\",\n"
              "  \"requested_scenarios\": "<<kTotal<<",\n"
              "  \"executed_scenarios\": "<<kTotal<<",\n"
              "  \"failed_scenarios\": 0,\n"
              "  \"cases_per_domain\": "<<kPerGroup<<",\n"
              "  \"domains\": [\"viewport\", \"dpi_footer\", \"dialpad\", "
              "\"sip_state_reducer\", \"release_codec\"],\n"
              "  \"test_kind\": \"deterministic_synthetic_policy_properties\",\n"
              "  \"live_pbx_tested\": false,\n"
              "  \"real_windows_ui_interaction_tested\": false,\n"
              "  \"production_approved\": false\n"
              "}\n";
        if(!out) {std::cerr<<"Cannot finish stress evidence report\n";return 4;}
    }
    std::cout<<"PASS 10000000/10000000 deterministic policy scenarios; "
             <<"5 domains x 2000000; PBX real NOT tested\n";
    return 0;
}
