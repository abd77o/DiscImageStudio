#ifdef _WIN32
#include "SptiDevice.h"
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <winioctl.h>
#include <ntddscsi.h>
#include <cstddef>
#include <cstring>

namespace dis {

namespace {
struct SptdWithSense {
    SCSI_PASS_THROUGH_DIRECT sptd;
    ULONG filler;
    UCHAR sense[32];
};
const uint32_t kBounceSize = 256 * 1024;
}

SptiDevice::SptiDevice() : handle_(INVALID_HANDLE_VALUE), buffer_(0), bufferSize_(0), maxTransfer_(65536) {}
SptiDevice::~SptiDevice() { close(); }
bool SptiDevice::isOpen() const { return handle_ != INVALID_HANDLE_VALUE; }

std::vector<char> SptiDevice::listCdDrives() {
    std::vector<char> out;
    DWORD mask = GetLogicalDrives();
    for (int i = 0; i < 26; ++i) {
        if (!(mask & (1u << i))) continue;
        char root[4] = { (char)('A' + i), ':', '\\', 0 };
        if (GetDriveTypeA(root) == DRIVE_CDROM) out.push_back((char)('A' + i));
    }
    return out;
}

bool SptiDevice::open(char letter, std::string& error) {
    close();
    char path[8] = { '\\', '\\', '.', '\\', letter, ':', 0 };
    HANDLE h = CreateFileA(path, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
                           NULL, OPEN_EXISTING, 0, NULL);
    if (h == INVALID_HANDLE_VALUE) {
        error = (GetLastError() == ERROR_ACCESS_DENIED) ? "access denied - run as administrator"
                                                        : "cannot open drive";
        return false;
    }
    handle_ = h;
    bufferSize_ = kBounceSize;
    buffer_ = VirtualAlloc(NULL, bufferSize_, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!buffer_) { error = "out of memory"; close(); return false; }

    // Ask the adapter for its real transfer limit (many ATAPI/USB paths allow only 64 KiB).
    STORAGE_PROPERTY_QUERY q; std::memset(&q, 0, sizeof q);
    q.PropertyId = StorageAdapterProperty; q.QueryType = PropertyStandardQuery;
    STORAGE_ADAPTER_DESCRIPTOR d; std::memset(&d, 0, sizeof d);
    DWORD ret = 0;
    if (DeviceIoControl(h, IOCTL_STORAGE_QUERY_PROPERTY, &q, sizeof q, &d, sizeof d, &ret, NULL) &&
        d.MaximumTransferLength >= 2352)
        maxTransfer_ = d.MaximumTransferLength < bufferSize_ ? d.MaximumTransferLength : bufferSize_;
    return true;
}

void SptiDevice::close() {
    if (handle_ != INVALID_HANDLE_VALUE) { CloseHandle((HANDLE)handle_); handle_ = INVALID_HANDLE_VALUE; }
    if (buffer_) { VirtualFree(buffer_, 0, MEM_RELEASE); buffer_ = 0; }
}

ScsiResult SptiDevice::execute(const mmc::Cdb& cdb, DataDir dir, void* data,
                               uint32_t length, uint32_t timeoutSec) {
    ScsiResult r;
    if (!isOpen() || length > bufferSize_) { r.osError = ERROR_INVALID_PARAMETER; return r; }

    SptdWithSense s;
    std::memset(&s, 0, sizeof s);
    s.sptd.Length             = sizeof(SCSI_PASS_THROUGH_DIRECT);
    s.sptd.CdbLength          = cdb.len;
    s.sptd.SenseInfoLength    = sizeof s.sense;
    s.sptd.DataIn             = dir == kDataIn  ? SCSI_IOCTL_DATA_IN
                              : dir == kDataOut ? SCSI_IOCTL_DATA_OUT : SCSI_IOCTL_DATA_UNSPECIFIED;
    s.sptd.DataTransferLength = length;
    s.sptd.TimeOutValue       = timeoutSec;
    s.sptd.DataBuffer         = length ? buffer_ : NULL;
    s.sptd.SenseInfoOffset    = (ULONG)offsetof(SptdWithSense, sense);
    std::memcpy(s.sptd.Cdb, cdb.b, cdb.len);
    if (dir == kDataOut && length) std::memcpy(buffer_, data, length);

    DWORD bytes = 0;
    BOOL ok = DeviceIoControl((HANDLE)handle_, IOCTL_SCSI_PASS_THROUGH_DIRECT,
                              &s, sizeof s, &s, sizeof s, &bytes, NULL);
    if (!ok) { r.osError = GetLastError(); return r; }

    r.status = s.sptd.ScsiStatus;
    if (r.status != 0) {
        if ((s.sense[0] & 0x7F) == 0x70 || (s.sense[0] & 0x7F) == 0x71) {
            r.senseKey = s.sense[2] & 0x0F; r.asc = s.sense[12]; r.ascq = s.sense[13];
        }
        return r;
    }
    if (dir == kDataIn && length) std::memcpy(data, buffer_, length);
    r.ok = true;
    return r;
}

std::string SptiDevice::inquiryString() {
    uint8_t d[36]; std::memset(d, 0, sizeof d);
    if (!execute(mmc::inquiry(sizeof d), kDataIn, d, sizeof d, 10).ok) return std::string();
    return std::string((const char*)d + 8, 28);
}

} // namespace dis
#endif
