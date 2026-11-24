// BurnJob.h - runs one burn on a worker thread; the GUI polls atomics (no locks, no callbacks
// into the UI). The result is read only after the job has finished.
#pragma once
#include "../burn/RawDaoWriter.h"
#include "../calib/CalibrationSource.h"
#include "../core/Config.h"
#include "../core/ImageSource.h"
#include <atomic>
#include <functional>
#include <memory>
#include <string>
#include <thread>

namespace dis {

struct BurnRequest {
    enum Kind { kImage = 0, kCalibLadder = 1, kCalibAlternate = 2 };
    Kind kind = kImage;
    char driveLetter = 0;
    bool testWrite = true;
    AppConfig cfg;
    std::shared_ptr<GrayRaster> image;      // kImage only
    CalibrationConfig calib;                // calibration kinds
};

class BurnJob {
public:
    // Creates the device for a drive letter (SptiDevice on Windows; a mock in tests).
    typedef std::function<bool(char, std::unique_ptr<IScsiDevice>&, std::string&)> DeviceFactory;

    struct Status {
        uint32_t done = 0, total = 0;
        int      state = 0;                 // BufferState
        double   fill = 1.0;                // drive buffer fill 0..1
    };

    explicit BurnJob(DeviceFactory f);
    ~BurnJob();

    bool start(const BurnRequest& r, std::string& error);
    void cancel() { cancel_ = true; }
    bool running() const { return running_; }
    bool takeFinished() { return finished_.exchange(false); }   // true exactly once per job
    Status status() const;
    BurnResult result() const { return res_; }                   // valid when !running()
    const BurnRequest& request() const { return req_; }

private:
    void run();
    DeviceFactory factory_;
    std::thread th_;
    BurnRequest req_;
    BurnResult  res_;
    std::atomic<bool> running_, cancel_, finished_;
    std::atomic<uint32_t> done_, total_;
    std::atomic<int> state_, fillPermille_;
};

} // namespace dis
