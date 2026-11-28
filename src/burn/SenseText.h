// SenseText.h - turns SCSI sense data / opcodes into readable text for errors and the burn log.
#pragma once
#include <cstdint>
#include <string>

namespace dis {

// e.g. "Illegal request: invalid field in parameter list (05/26/00)"
std::string senseDescription(uint8_t key, uint8_t asc, uint8_t ascq);
const char* mmcOpcodeName(uint8_t opcode);

} // namespace dis
