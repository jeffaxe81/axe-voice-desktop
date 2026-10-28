#pragma once
#include "sip_runtime_observation.h"
#include "sip_address.h"
#include <array>
#include <limits>
#include <string_view>
#include <type_traits>
#include <utility>
namespace axv {
struct RecentDestination {std::uint64_t token=0;std::wstring destination;};
// GUI-thread-owned. Entries describe accepted requests, never remote outcomes.
class RecentDestinations {
public:
    static constexpr std::size_t capacity=10;
    void synchronize(const SipRuntimeObservation& observed) noexcept {
        if(!valid_context(observed)){clear();return;}
        if(session_!=observed.session){clear();session_=observed.session;}
    }
    bool remember(const SipRuntimeObservation& observed,const std::wstring& destination,bool accepted) noexcept {
        synchronize(observed);
        if(!accepted || !valid_context(observed) || observed.sip!=SipStatus::Registered || observed.call_id ||
           destination.empty() || destination.size()>128 || next_token_==std::numeric_limits<std::uint64_t>::max())return false;
        try {
            // Reject unsupported native user grammar and parameters even when a
            // caller incorrectly claims acceptance. Keep the original typed text.
            for(wchar_t c:destination)if(c<=32 || c>=127)return false;
            if(!validate_address(destination))return false;
            std::wstring_view user=destination;
            if(user.substr(0,5)==L"sips:")user.remove_prefix(5);
            else if(user.substr(0,4)==L"sip:")user.remove_prefix(4);
            user=user.substr(0,user.find(L'@'));
            for(wchar_t c:user)if(!((c>=L'a'&&c<=L'z') || (c>=L'A'&&c<=L'Z') ||
                (c>=L'0'&&c<=L'9') || c==L'_' || c==L'.' || c==L'-' || c==L'+' || c==L'*' || c==L'#'))return false;
            RecentDestination next{next_token_,destination}; // All allocations precede mutation.
            std::size_t position=0;
            while(position<size_ && entries_[position].destination!=destination)++position;
            if(position==size_){if(size_<capacity)++size_;position=size_-1;}
            erase(entries_[position]);
            for(std::size_t i=position;i>0;--i)entries_[i]=std::move(entries_[i-1]);
            entries_[0]=std::move(next);
            ++next_token_;
            return true;
        }catch(...){return false;}
    }
    std::size_t size() const noexcept {return size_;}
    const RecentDestination* at(std::size_t index) const noexcept {return index<size_?&entries_[index]:nullptr;}
    const RecentDestination* find(std::uint64_t token,const SipRuntimeObservation& observed) const noexcept {
        if(!token || !valid_context(observed) || observed.session!=session_)return nullptr;
        for(std::size_t i=0;i<size_;++i)if(entries_[i].token==token)return &entries_[i];
        return nullptr;
    }
    void clear() noexcept {
        for(auto& entry:entries_)erase(entry);
        size_=0;session_=0; // Token sequence deliberately survives clearing.
    }
    ~RecentDestinations(){clear();}
private:
    static bool valid_context(const SipRuntimeObservation& observed) noexcept {
        return observed.process==RuntimeProcess::Running && observed.evidence==SipEvidence::VerifiedNativeApi && observed.session &&
            (observed.sip==SipStatus::Registered || observed.sip==SipStatus::Calling ||
             observed.sip==SipStatus::Ringing || observed.sip==SipStatus::InCall);
    }
    static void erase(RecentDestination& entry) noexcept {
        volatile wchar_t* p=entry.destination.empty()?nullptr:&entry.destination[0];
        for(std::size_t i=0;i<entry.destination.size();++i)p[i]=0;
        entry.destination.clear();entry.token=0;
    }
    static_assert(std::is_nothrow_move_assignable<RecentDestination>::value,"MRU commit must not allocate");
    std::array<RecentDestination,capacity> entries_{};
    std::size_t size_=0;
    std::uint64_t session_=0,next_token_=1;
};
}
