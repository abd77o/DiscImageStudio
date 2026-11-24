#include "App.h"
#include "Dialogs.h"
#include "../burn/SptiDevice.h"
#include "../calib/CalibrationSource.h"
#include "../core/ImageLoader.h"
#include "imgui.h"
#include <cstdio>
#include <cstring>

namespace dis {

static bool makeDevice(char letter, std::unique_ptr<IScsiDevice>& out, std::string& err) {
#ifdef _WIN32
    std::unique_ptr<SptiDevice> d(new SptiDevice());
    if (!d->open(letter, err)) return false;
    out.reset(d.release());
    return true;
#else
    (void)letter; (void)out; err = "optical drive access is only available on Windows"; return false;
#endif
}

static const int kSpeeds[] = { 1, 2, 4, 8, 10, 12, 16, 24, 32, 40, 48, 52 };
static const int kCells[]  = { 98, 196, 294, 392, 588 };

App::App()
    : hwnd_(0), driveIdx_(-1), testWrite_(true), previewDirty_(false), previewTex_(0),
      job_(makeDevice), showConfirm_(false), recommendedSpeed_(0), lightSel_(-1), darkSel_(-1) {
    std::memset(preview_, 0, sizeof preview_);
}

void App::init(const std::string& configPath, void* hwnd) {
    cfgPath_ = configPath; hwnd_ = hwnd;
    loadConfig(cfgPath_, cfg_);
    refreshDrives();
}

bool App::pressure() const { return job_.running() && job_.status().state != kBufHealthy; }

void App::saveCfg() {
    cfg_.clampValues();
    if (!saveConfig(cfgPath_, cfg_)) statusMsg_ = "Could not write config.ini (is the folder read-only?)";
}

void App::refreshDrives() {
    char prev = driveIdx_ >= 0 && driveIdx_ < (int)drives_.size() ? drives_[driveIdx_] : 0;
#ifdef _WIN32
    drives_ = SptiDevice::listCdDrives();
#else
    drives_.clear();
#endif
    driveIdx_ = drives_.empty() ? -1 : 0;
    for (size_t i = 0; i < drives_.size(); ++i) if (drives_[i] == prev) driveIdx_ = (int)i;
}

void App::chooseImage() {
    std::string p = openImageDialog(hwnd_);
    if (p.empty()) return;
    std::shared_ptr<GrayRaster> g(new GrayRaster());
    std::string err;
    if (!loadImageAsGrayRaster(p, 1024, *g, err)) { loadMsg_ = "Load failed: " + err; return; }
    image_ = g; imagePath_ = p;
    char b[160];
    std::snprintf(b, sizeof b, "Loaded %dx%d, centre-cropped to a square, working size %dx%d",
                  g->srcWidth, g->srcHeight, g->size, g->size);
    loadMsg_ = b;
    buildPreview();
}

void App::buildPreview() {
    if (!image_) return;
    for (int y = 0; y < kPreview; ++y)
        for (int x = 0; x < kPreview; ++x) {
            float nx = ((x + 0.5f) / kPreview) * 2.f - 1.f, ny = 1.f - ((y + 0.5f) / kPreview) * 2.f;
            uint32_t c;
            if (nx * nx + ny * ny > 1.f) c = 0xFF303030;
            else { uint32_t g = (uint32_t)(image_->sampleDisc(nx, ny) * 255.f + 0.5f); c = 0xFF000000u | (g << 16) | (g << 8) | g; }
            preview_[y * kPreview + x] = c;
        }
    previewDirty_ = true;
}

void App::pollJob() {
    if (!job_.takeFinished()) return;
    BurnResult r = job_.result();
    const BurnRequest& q = job_.request();
    char b[200];
    if (r.ok) {
        if (q.testWrite) {
            if (!cfg_.testWritePassed) { cfg_.testWritePassed = true; saveCfg(); }
            statusMsg_ = "TEST WRITE finished OK. Real burning is now unlocked (untick the test-write box).";
        } else {
            statusMsg_ = "Burn finished OK. The disc was ejected.";
        }
        recommendedSpeed_ = (r.recommendedSpeedX > 0 && r.recommendedSpeedX < q.cfg.speedX) ? r.recommendedSpeedX : 0;
    } else {
        statusMsg_ = r.error.empty() ? "Burn failed." : r.error;
        recommendedSpeed_ = 0;
    }
    std::snprintf(b, sizeof b, "Buffer events: low=%u critical=%u, lowest fill %.0f%%",
                  r.lowEvents, r.criticalEvents, r.minFill * 100.0);
    statsMsg_ = b;
}

void App::launch(const BurnRequest& r) {
    std::string err;
    if (!job_.start(r, err)) { statusMsg_ = err; return; }
    statusMsg_.clear(); statsMsg_.clear(); recommendedSpeed_ = 0;
}

void App::beginBurn(BurnRequest::Kind kind) {
    statusMsg_.clear();
    if (driveIdx_ < 0) { statusMsg_ = "No optical drive selected."; return; }
    BurnRequest r;
    r.kind = kind; r.driveLetter = drives_[driveIdx_]; r.cfg = cfg_; r.testWrite = effectiveTestWrite();
    if (kind == BurnRequest::kImage) {
        if (!image_) { statusMsg_ = "Choose an image first."; return; }
        r.image = image_;
    } else if (kind == BurnRequest::kCalibLadder) {
        const uint32_t n = (uint32_t)cfg_.bandSectors * 16;
        r.calib = CalibrationConfig::atProgramStart(n, n);
        r.calib.mode = CalibrationConfig::kLadder;
        r.calib.blockSectors = (uint32_t)cfg_.bandSectors;
    } else {
        const uint32_t n = (uint32_t)cfg_.altMinutes * 4500;
        r.calib = CalibrationConfig::atProgramStart(n, n);
        r.calib.mode = CalibrationConfig::kAlternate;
        r.calib.blockSectors = (uint32_t)cfg_.altBlockSectors;
    }
    if (r.testWrite) launch(r);
    else { pending_ = r; showConfirm_ = true; }
}

// ------------------------------------------------------------------------------- UI

void App::drawBurnButton(const char* realLabel, const char* testLabel, BurnRequest::Kind kind) {
    const bool test = effectiveTestWrite();
    if (ImGui::Button(test ? testLabel : realLabel, ImVec2(-1, 34))) beginBurn(kind);
}

void App::drawDeviceSection() {
    ImGui::BeginDisabled(job_.running());
    ImGui::Text("Burner");
    ImGui::SameLine();
    char cur[48] = "(no optical drive found)";
    if (driveIdx_ >= 0) std::snprintf(cur, sizeof cur, "%c:  (CD/DVD drive)", drives_[driveIdx_]);
    ImGui::SetNextItemWidth(220);
    if (ImGui::BeginCombo("##drive", cur)) {
        for (size_t i = 0; i < drives_.size(); ++i) {
            char l[32]; std::snprintf(l, sizeof l, "%c:  (CD/DVD drive)", drives_[i]);
            if (ImGui::Selectable(l, (int)i == driveIdx_)) driveIdx_ = (int)i;
        }
        ImGui::EndCombo();
    }
    ImGui::SameLine();
    if (ImGui::Button("Refresh")) refreshDrives();

    ImGui::Text("Speed");
    ImGui::SameLine();
    char sp[16]; std::snprintf(sp, sizeof sp, "%dx", cfg_.speedX);
    ImGui::SetNextItemWidth(100);
    if (ImGui::BeginCombo("##speed", sp)) {
        for (size_t i = 0; i < sizeof kSpeeds / sizeof kSpeeds[0]; ++i) {
            char l[16]; std::snprintf(l, sizeof l, "%dx", kSpeeds[i]);
            if (ImGui::Selectable(l, kSpeeds[i] == cfg_.speedX)) { cfg_.speedX = kSpeeds[i]; saveCfg(); }
        }
        ImGui::EndCombo();
    }
    ImGui::SameLine();
    ImGui::TextDisabled("lower is safer on slow PCs");

    const bool locked = !cfg_.testWritePassed;
    if (locked) testWrite_ = true;
    ImGui::BeginDisabled(locked);
    ImGui::Checkbox("Test write (simulation - the laser does not burn)", &testWrite_);
    ImGui::EndDisabled();
    if (locked)
        ImGui::TextWrapped("Locked ON. Real burning unlocks after one successful test write. If your drive "
                           "does not support test write, burn a CD-RW first, then set test_write_passed=1 in config.ini.");
    ImGui::EndDisabled();
}

void App::drawImageTab() {
    ImGui::BeginDisabled(job_.running());
    if (ImGui::Button("Choose image...  (PNG / JPG / BMP)")) chooseImage();
    ImGui::EndDisabled();
    if (!imagePath_.empty()) ImGui::TextWrapped("%s", imagePath_.c_str());
    if (!loadMsg_.empty()) ImGui::TextWrapped("%s", loadMsg_.c_str());
    if (image_ && previewTex_) {
        ImGui::Text("Disc preview (outside the circle is not burned):");
        ImGui::Image((ImTextureID)previewTex_, ImVec2(176, 176));
    }
    ImGui::Spacing();
    ImGui::Text("Tones: dark 0x%02X, light 0x%02X  (set them in the Calibration tab)", cfg_.darkByte, cfg_.lightByte);
    if (cfg_.darkByte == cfg_.lightByte) ImGui::TextColored(ImVec4(1, 0.4f, 0.3f, 1), "Dark and light bytes are identical - the image will not be visible.");

    if (ImGui::CollapsingHeader("Advanced")) {
        ImGui::BeginDisabled(job_.running());
        bool ch = false;
        char cl[16]; std::snprintf(cl, sizeof cl, "%d bytes", cfg_.cellBytes);
        ImGui::SetNextItemWidth(140);
        if (ImGui::BeginCombo("Cell size", cl)) {
            for (size_t i = 0; i < sizeof kCells / sizeof kCells[0]; ++i) {
                char l[16]; std::snprintf(l, sizeof l, "%d bytes", kCells[i]);
                if (ImGui::Selectable(l, kCells[i] == cfg_.cellBytes)) { cfg_.cellBytes = kCells[i]; ch = true; }
            }
            ImGui::EndCombo();
        }
        if (ImGui::RadioButton("Floyd-Steinberg", !cfg_.jjn)) { cfg_.jjn = false; ch = true; }
        ImGui::SameLine();
        if (ImGui::RadioButton("Jarvis-Judice-Ninke", cfg_.jjn)) { cfg_.jjn = true; ch = true; }
        ch |= ImGui::SliderFloat("Brightness", &cfg_.brightness, -0.5f, 0.5f, "%.2f");
        ch |= ImGui::SliderFloat("Contrast", &cfg_.contrast, 0.2f, 3.0f, "%.2f");
        ch |= ImGui::SliderFloat("Gamma", &cfg_.gamma, 0.3f, 3.0f, "%.2f");
        if (ch) saveCfg();
        ImGui::EndDisabled();
    }
    ImGui::Spacing();
    ImGui::BeginDisabled(job_.running() || !image_);
    drawBurnButton("BURN DISC", "Start TEST write (image)", BurnRequest::kImage);
    ImGui::EndDisabled();
}

void App::drawCalibrationTab() {
    ImGui::TextWrapped("Step 1: insert a spare blank CD-R and burn the ladder. It writes 16 candidate byte patterns, "
                       "one concentric ring each, starting near the centre of the disc.");
    ImGui::TextWrapped("Step 2: tilt the disc under a lamp. Find the lightest and the darkest ring, "
                       "pick them below and save. Contrast depends on viewing angle - judge it from the angle you will use.");
    ImGui::Spacing();
    ImGui::BeginDisabled(job_.running());
    ImGui::SetNextItemWidth(140);
    if (ImGui::InputInt("Sectors per band", &cfg_.bandSectors, 500, 1000)) saveCfg();
    ImGui::EndDisabled();
    ImGui::BeginDisabled(job_.running());
    drawBurnButton("BURN CALIBRATION LADDER", "Start TEST write (ladder)", BurnRequest::kCalibLadder);
    ImGui::EndDisabled();

    const std::vector<uint8_t> ladder = CalibrationConfig::defaultLadder();
    DiscGeometry g;
    ImGui::Spacing();
    ImGui::Text("Band map (approximate radius from the disc centre):");
    if (ImGui::BeginChild("bands", ImVec2(0, 150), true)) {
        for (size_t i = 0; i < ladder.size(); ++i) {
            double r0 = g.radiusAtLength((double)i * cfg_.bandSectors * g.sectorLengthMm);
            double r1 = g.radiusAtLength((double)(i + 1) * cfg_.bandSectors * g.sectorLengthMm);
            ImGui::Text("Band %2d   byte 0x%02X   %.1f - %.1f mm", (int)i + 1, ladder[i], r0, r1);
        }
    }
    ImGui::EndChild();

    ImGui::Spacing();
    ImGui::Text("Step 3: what you saw");
    struct Pick { const char* label; int* sel; };
    Pick picks[2] = { { "Lightest ring", &lightSel_ }, { "Darkest ring", &darkSel_ } };
    for (int p = 0; p < 2; ++p) {
        char cur[40] = "(choose)";
        int s = *picks[p].sel;
        if (s >= 0) std::snprintf(cur, sizeof cur, "Band %d  (0x%02X)", s + 1, ladder[(size_t)s]);
        ImGui::SetNextItemWidth(200);
        if (ImGui::BeginCombo(picks[p].label, cur)) {
            for (size_t i = 0; i < ladder.size(); ++i) {
                char l[40]; std::snprintf(l, sizeof l, "Band %d  (0x%02X)", (int)i + 1, ladder[i]);
                if (ImGui::Selectable(l, (int)i == s)) *picks[p].sel = (int)i;
            }
            ImGui::EndCombo();
        }
    }
    const bool ready = lightSel_ >= 0 && darkSel_ >= 0 && lightSel_ != darkSel_;
    if (lightSel_ >= 0 && lightSel_ == darkSel_) ImGui::TextColored(ImVec4(1, 0.4f, 0.3f, 1), "Pick two different bands.");
    ImGui::BeginDisabled(!ready);
    if (ImGui::Button("Save to config.ini", ImVec2(-1, 28))) {
        cfg_.lightByte = ladder[(size_t)lightSel_]; cfg_.darkByte = ladder[(size_t)darkSel_];
        saveCfg();
        if (statusMsg_.empty()) statusMsg_ = "Saved. These bytes are now the default tones.";
    }
    ImGui::EndDisabled();

    ImGui::Spacing();
    if (ImGui::CollapsingHeader("Alignment test (angle / mirror)")) {
        ImGui::TextWrapped("Alternating light/dark blocks. The spiral arms you see show how sectors map to angle on this drive.");
        ImGui::BeginDisabled(job_.running());
        ImGui::SetNextItemWidth(120);
        if (ImGui::InputInt("Sectors per block", &cfg_.altBlockSectors, 1, 10)) saveCfg();
        ImGui::SetNextItemWidth(120);
        if (ImGui::InputInt("Length (minutes)", &cfg_.altMinutes, 1, 5)) saveCfg();
        drawBurnButton("BURN ALIGNMENT TEST", "Start TEST write (alignment)", BurnRequest::kCalibAlternate);
        bool ch = false;
        float ang = (float)cfg_.angleOffsetDeg;
        if (ImGui::SliderFloat("Image rotation (deg)", &ang, -180.0f, 180.0f, "%.1f")) { cfg_.angleOffsetDeg = ang; ch = true; }
        if (ImGui::Checkbox("Mirror image", &cfg_.mirror)) ch = true;
        if (ch) saveCfg();
        ImGui::EndDisabled();
    }
}

void App::drawProgress() {
    ImGui::Separator();
    if (job_.running()) {
        BurnJob::Status s = job_.status();
        float frac = s.total ? (float)s.done / (float)s.total : 0.f;
        char ov[64]; std::snprintf(ov, sizeof ov, "%.1f%%  (%u / %u sectors)", frac * 100.f, s.done, s.total);
        ImGui::Text("%s", job_.request().testWrite ? "TEST WRITE in progress" : "BURNING - do not disturb the PC");
        ImGui::ProgressBar(frac, ImVec2(-1, 22), ov);
        const char* st = s.state == kBufHealthy ? "healthy" : (s.state == kBufLow ? "LOW" : "CRITICAL");
        char bo[64]; std::snprintf(bo, sizeof bo, "Drive buffer %.0f%%  [%s]", s.fill * 100.0, st);
        ImGui::ProgressBar((float)s.fill, ImVec2(-1, 18), bo);
        if (ImGui::Button("Cancel")) job_.cancel();
        if (!job_.request().testWrite) { ImGui::SameLine(); ImGui::TextDisabled("cancelling a real burn ruins the disc"); }
    }
    if (!statusMsg_.empty()) ImGui::TextWrapped("%s", statusMsg_.c_str());
    if (!statsMsg_.empty() && !job_.running()) ImGui::TextDisabled("%s", statsMsg_.c_str());
    if (recommendedSpeed_ > 0 && !job_.running()) {
        char l[64]; std::snprintf(l, sizeof l, "Buffer ran low - use %dx next time", recommendedSpeed_);
        if (ImGui::Button(l)) { cfg_.speedX = recommendedSpeed_; saveCfg(); recommendedSpeed_ = 0; }
    }
}

void App::drawConfirmPopup() {
    if (showConfirm_) { ImGui::OpenPopup("Confirm real burn"); showConfirm_ = false; }
    if (ImGui::BeginPopupModal("Confirm real burn", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Burn to drive %c: at %dx?", pending_.driveLetter, pending_.cfg.speedX);
        ImGui::Text("The disc must be a blank CD-R. This cannot be undone.");
        ImGui::Spacing();
        if (ImGui::Button("Burn", ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); launch(pending_); }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0))) ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }
}

void App::drawUi(float w, float h) {
    pollJob();
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(w, h));
    ImGui::Begin("DiscImageStudio", NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                 ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings);
    ImGui::Text("DiscImageStudio  -  burn images onto the data side of a CD-R");
    ImGui::TextDisabled("Audio DAO raw mode. CD-R/CD-RW only. Do not play the burned disc on speakers.");
    ImGui::Separator();
    drawDeviceSection();
    ImGui::Separator();
    if (ImGui::BeginTabBar("tabs")) {
        if (ImGui::BeginTabItem("Image")) { drawImageTab(); ImGui::EndTabItem(); }
        if (ImGui::BeginTabItem("Calibration")) { drawCalibrationTab(); ImGui::EndTabItem(); }
        ImGui::EndTabBar();
    }
    drawProgress();
    drawConfirmPopup();
    ImGui::End();
}

} // namespace dis
