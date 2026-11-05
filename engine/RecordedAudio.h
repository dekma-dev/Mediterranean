#pragma once
#include <windows.h>
#include <mmreg.h>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace sound {
enum class Cue;
struct MusicFormat {
 WAVEFORMATEX wfx{};
 WORD wSamplesPerBlock=0,wNumCoef=0;
 ADPCMCOEFSET aCoef[7]{};
};
static_assert(sizeof(MusicFormat)==50,"Packed Microsoft ADPCM format");
struct WaveView {
 MusicFormat format{};
 const std::uint8_t* data=nullptr;
 std::uint32_t bytes=0,frames=0;
};
// Non-owning views into executable resources; their lifetime is the module's.
bool ParseMusicWave(const void* data,std::size_t bytes,WaveView& out);
bool MusicRecording(unsigned track,WaveView& out);
bool ParseRecordingBank(const void* data,std::size_t bytes,const char* name,std::vector<std::int16_t>& out);
unsigned RecordingVariants(Cue cue);
std::vector<std::int16_t> RecordedEffect(Cue cue,unsigned variant=0);
}
