#include "../src/audio_preview_levels.h"
#include <array>
#include <cmath>
#include <cstdint>
int main() {
    using namespace axv;
    if(preview_level_for_index(0,75)!=25 || preview_level_for_index(1,75)!=50 ||
       preview_level_for_index(2,75)!=75 || preview_level_for_index(3,75)!=100 ||
       preview_level_for_index(-1,75)!=75 || preview_level_for_index(4,75)!=75 ||
       preview_level_index(25)!=0 || preview_level_index(75)!=2 ||
       preview_level_index(13)!=-1) return 1;
    std::array<float,6> samples{{0.11f,-0.11f,0.05f,0.07f,-0.05f,0.0f}};
    if(!scale_preview_buffer(samples.data(),3,2,ToneSampleEncoding::Float32,25))
        return 2;
    if(std::fabs(samples[0]-0.0275f)>0.00001f ||
       std::fabs(samples[1]+0.0275f)>0.00001f ||
       std::fabs(samples[2]-0.0125f)>0.00001f) return 3;
    if(!scale_preview_buffer(samples.data(),3,2,ToneSampleEncoding::Float32,0))
        return 4;
    for(auto s:samples) if(s!=0.0f) return 5;
    std::array<std::int16_t,4> pcm{{3600,-3600,2600,-2600}};
    if(!scale_preview_buffer(pcm.data(),2,2,ToneSampleEncoding::Signed16,50))
        return 6;
    if(pcm[0]!=1800 || pcm[1]!=-1800 || pcm[2]!=1300 || pcm[3]!=-1300) return 7;
    if(scale_preview_buffer(pcm.data(),2,2,ToneSampleEncoding::Signed16,101) ||
       scale_preview_buffer(nullptr,2,2,ToneSampleEncoding::Float32,75) ||
       scale_preview_buffer(samples.data(),0,2,ToneSampleEncoding::Float32,75) ||
       scale_preview_buffer(samples.data(),3,0,ToneSampleEncoding::Float32,75) ||
       scale_preview_buffer(samples.data(),384001,2,ToneSampleEncoding::Float32,75))
        return 8;
    return 0;
}
