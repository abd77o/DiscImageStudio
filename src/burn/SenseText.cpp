#include "SenseText.h"
#include <cstdio>

namespace dis {

namespace {
struct Asc { uint8_t asc, ascq; const char* text; };
const Asc kAsc[] = {
    {0x04,0x00,"logical unit not ready"}, {0x04,0x01,"drive is becoming ready"}, {0x04,0x07,"operation in progress"},
    {0x04,0x08,"long write in progress"}, {0x0C,0x00,"write error"}, {0x0C,0x07,"write error - recovery needed"},
    {0x0C,0x09,"write error - loss of streaming (buffer underrun)"}, {0x0C,0x0A,"write error - padding blocks added"},
    {0x20,0x00,"invalid command operation code"}, {0x21,0x00,"logical block address out of range"},
    {0x24,0x00,"invalid field in CDB"}, {0x26,0x00,"invalid field in parameter list"},
    {0x26,0x01,"parameter not supported"}, {0x26,0x02,"parameter value invalid"}, {0x27,0x00,"write protected"},
    {0x28,0x00,"medium may have changed"}, {0x29,0x00,"power on / reset occurred"},
    {0x30,0x00,"incompatible medium installed"}, {0x30,0x01,"cannot read medium - unknown format"},
    {0x30,0x02,"cannot read medium - incompatible format"}, {0x30,0x05,"cannot write medium - incompatible format"},
    {0x3A,0x00,"medium not present"}, {0x3A,0x01,"medium not present - tray closed"}, {0x3A,0x02,"medium not present - tray open"},
    {0x57,0x00,"unable to recover table of contents"}, {0x63,0x00,"end of user area encountered on this track"},
    {0x64,0x00,"illegal mode for this track"}, {0x72,0x00,"session fixation error"},
    {0x72,0x01,"session fixation error writing lead-in"}, {0x72,0x02,"session fixation error writing lead-out"},
    {0x73,0x00,"CD control error"}, {0x73,0x02,"power calibration area is full"},
    {0x73,0x03,"power calibration area error"}, {0x73,0x06,"RMA/PMA is almost full"},
};
const char* kKey[16] = { "No sense", "Recovered error", "Not ready", "Medium error", "Hardware error", "Illegal request",
                         "Unit attention", "Data protect", "Blank check", "Vendor specific", "Copy aborted",
                         "Aborted command", "Equal", "Volume overflow", "Miscompare", "Reserved" };
}

std::string senseDescription(uint8_t key, uint8_t asc, uint8_t ascq) {
    const char* detail = 0;
    for (size_t i = 0; i < sizeof kAsc / sizeof kAsc[0]; ++i)
        if (kAsc[i].asc == asc && kAsc[i].ascq == ascq) { detail = kAsc[i].text; break; }
    char buf[160];
    if (key == 0 && asc == 0 && ascq == 0) return "no sense data (the command was rejected or timed out)";
    std::snprintf(buf, sizeof buf, "%s: %s (%02X/%02X/%02X)", kKey[key & 15], detail ? detail : "unknown code", key, asc, ascq);
    return buf;
}

const char* mmcOpcodeName(uint8_t op) {
    switch (op) {
    case 0x00: return "TEST UNIT READY";      case 0x12: return "INQUIRY";
    case 0x1B: return "START STOP UNIT";      case 0x2A: return "WRITE(10)";
    case 0x35: return "SYNCHRONIZE CACHE";    case 0x46: return "GET CONFIGURATION";
    case 0x51: return "READ DISC INFORMATION";case 0x52: return "READ TRACK INFORMATION";
    case 0x55: return "MODE SELECT(10)";      case 0x5C: return "READ BUFFER CAPACITY";
    case 0x5D: return "SEND CUE SHEET";       case 0xBB: return "SET CD SPEED";
    default:   return "COMMAND";
    }
}

} // namespace dis
