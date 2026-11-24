// CueSheet.h - SEND CUE SHEET payload for a single-track Disc-At-Once burn.
#pragma once
#include <cstdint>
#include <vector>

namespace dis {

// Data-form constants. The audio ones are the common, well-trodden values. The data-track
// ones (marked VERIFY) must be checked against the MMC table for "CUE SHEET Data Form"
// and against your drive before relying on the data layout.
const uint8_t kCtlAdrAudio   = 0x01;   // CTL=0 (audio, no pre-emphasis), ADR=1
const uint8_t kCtlAdrData    = 0x41;   // CTL=4 (data), ADR=1
const uint8_t kFormAudio     = 0x00;   // track: CD-DA 2352
const uint8_t kFormLeadAudio = 0x01;   // lead-in / lead-out, audio disc
const uint8_t kFormDataRaw   = 0x11;   // VERIFY: track: CD-ROM mode 1, 2352 raw
const uint8_t kFormLeadData  = 0x41;   // VERIFY: lead-in / lead-out, data disc

void framesToMsf(uint32_t frames, uint8_t& m, uint8_t& s, uint8_t& f);

// userSectors = number of sectors from LBA 0 (pre-gap not included).
std::vector<uint8_t> buildDaoCueSheet(bool dataTrack, uint32_t userSectors);

} // namespace dis
