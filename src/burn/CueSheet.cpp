#include "CueSheet.h"

namespace dis {

void framesToMsf(uint32_t n, uint8_t& m, uint8_t& s, uint8_t& f) {
    m = (uint8_t)(n / 4500);
    s = (uint8_t)((n / 75) % 60);
    f = (uint8_t)(n % 75);
}

static void entry(std::vector<uint8_t>& v, uint8_t ctlAdr, uint8_t tno, uint8_t index,
                  uint8_t form, uint32_t absFrames) {
    uint8_t m, s, f;
    framesToMsf(absFrames, m, s, f);
    const uint8_t e[8] = { ctlAdr, tno, index, form, 0x00 /*SCMS*/, m, s, f };
    v.insert(v.end(), e, e + 8);
}

std::vector<uint8_t> buildDaoCueSheet(bool dataTrack, uint32_t userSectors) {
    const uint8_t ca   = dataTrack ? kCtlAdrData : kCtlAdrAudio;
    const uint8_t trk  = dataTrack ? kFormDataRaw : kFormAudio;
    const uint8_t lead = dataTrack ? kFormLeadData : kFormLeadAudio;
    std::vector<uint8_t> v;
    entry(v, ca, 0x00, 0x00, lead, 0);                  // lead-in
    entry(v, ca, 0x01, 0x00, trk, 0);                   // track 1, index 0 (pre-gap) at 00:00:00
    entry(v, ca, 0x01, 0x01, trk, 150);                 // track 1, index 1 at 00:02:00
    entry(v, ca, 0xAA, 0x01, lead, 150 + userSectors);  // lead-out
    return v;
}

} // namespace dis
