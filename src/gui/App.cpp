#include "App.h"
#include "Dialogs.h"
#include "Theme.h"
#include "Widgets.h"
#include "../burn/SptiDevice.h"
#include "../calib/CalibrationSource.h"
#include "../core/ImageLoader.h"
#include "imgui.h"
#include <algorithm>
#include <cmath>
#include <cstdio>

namespace dis {

using namespace ui;

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
static const float kMargin = 16.f, kLeftW = 352.f, kGap = 16.f, kBarH = 64.f;

App::App()
    : hwnd_(0), driveIdx_(-1), testWrite_(true), preview_((size_t)kPreview * kPreview, 0u), needRender_(true),
      previewTex_(0), job_(makeDevice), openConfirm_(false), openCalib_(false), openAdvanced_(false), openUnlock_(false), testWriteRejected_(false),
      statusKind_(kStatusInfo), recommendedSpeed_(0), lightSel_(-1), darkSel_(-1) {}

void App::init(const std::string& configPath, void* hwnd) {
    cfgPath_ = configPath; hwnd_ = hwnd;
    size_t slash = cfgPath_.find_last_of("\\/");
    logPath_ = (slash == std::string::npos ? std::string() : cfgPath_.substr(0, slash + 1)) + "burn_log.txt";
    loadConfig(cfgPath_, cfg_);
    refreshDrives();
    needRender_ = true;
}

bool App::pressure() const { return job_.running() && job_.status().state != kBufHealthy; }

PreviewParams App::previewParams() const {
    PreviewParams p;
    p.brightness = cfg_.brightness; p.contrast = cfg_.contrast; p.gamma = cfg_.gamma;
    p.angleDeg = cfg_.angleOffsetDeg; p.mirror = cfg_.mirror; p.imageRadiusMm = cfg_.imageRadiusMm;
    return p;
}

bool App::refreshPreview() {
    if (!needRender_) return false;
    needRender_ = false;
    disc_.render(image_.get(), previewParams(), preview_.data());
    return true;
}

void App::setStatus(int kind, const std::string& msg) { statusKind_ = kind; statusMsg_ = msg; }

void App::saveCfg() {
    cfg_.clampValues();
    if (!saveConfig(cfgPath_, cfg_)) setStatus(kStatusError, "Could not write config.ini (read-only folder?)");
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
    if (!p.empty()) loadImagePath(p);
}

void App::loadImagePath(const std::string& p) {
    if (job_.running()) { setStatus(kStatusError, "Wait for the current job to finish before changing the image."); return; }
    std::shared_ptr<GrayRaster> g(new GrayRaster());
    std::string err;
    if (!loadImageAsGrayRaster(p, 1024, *g, err)) { setStatus(kStatusError, "Could not load the image: " + err); return; }
    image_ = g; imagePath_ = p;
    char b[96];
    std::snprintf(b, sizeof b, "%d x %d px  -  cropped to a square", g->srcWidth, g->srcHeight);
    imageInfo_ = b;
    needRender_ = true;
    setStatus(kStatusInfo, "");
}

void App::pollJob() {
    if (!job_.takeFinished()) return;
    BurnResult r = job_.result();
    const BurnRequest& q = job_.request();
    char b[200];
    testWriteRejected_ = false;
    if (r.ok) {
        if (q.testWrite) {
            const bool first = !cfg_.testWritePassed;
            if (first) { cfg_.testWritePassed = true; saveCfg(); testWrite_ = false; }
            setStatus(kStatusOk, first ? "Test write OK - nothing was burned. Real burning is ON: press Burn disc."
                                       : "Test write OK - nothing was burned.");
        } else {
            setStatus(kStatusOk, "Burn finished OK. The disc was ejected.");
        }
        recommendedSpeed_ = (r.recommendedSpeedX > 0 && r.recommendedSpeedX < q.cfg.speedX) ? r.recommendedSpeedX : 0;
    } else {
        if (q.testWrite && r.rejectedTestWrite() && !cfg_.testWritePassed) {
            testWriteRejected_ = true;
            setStatus(kStatusError, "This drive does not support test write. You can unlock real burning.");
        } else {
            setStatus(kStatusError, r.error.empty() ? "Burn failed." : r.error);
        }
        recommendedSpeed_ = 0;
    }
    std::snprintf(b, sizeof b, "Buffer events: low=%u critical=%u, lowest fill %.0f%%", r.lowEvents, r.criticalEvents, r.minFill * 100.0);
    statsMsg_ = b;
}

void App::launch(const BurnRequest& r) {
    std::string err;
    if (!job_.start(r, err)) { setStatus(kStatusError, err); return; }
    setStatus(kStatusInfo, ""); statsMsg_.clear(); recommendedSpeed_ = 0;
}

void App::beginBurn(BurnRequest::Kind kind) {
    if (driveIdx_ < 0) { setStatus(kStatusError, "No optical drive selected."); return; }
    BurnRequest r;
    r.kind = kind; r.driveLetter = drives_[driveIdx_]; r.cfg = cfg_; r.testWrite = effectiveTestWrite();
    r.logPath = logPath_;
    if (kind == BurnRequest::kImage) {
        if (!image_) { setStatus(kStatusError, "Choose an image first."); return; }
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
    else { pending_ = r; openConfirm_ = true; }
}

// ================================================================================ layout

void App::drawUi(float w, float h) {
    pollJob();
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(w, h));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::Begin("##root", NULL, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                 ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus);
    ImGui::PopStyleVar();
    ImGui::PushFont(fonts.regular);

    const float contentH = h - kMargin - 12.f - kBarH - kMargin;
    const float rightW = w - kMargin - kLeftW - kGap - kMargin;

    ImGui::SetCursorScreenPos(ImVec2(kMargin, kMargin));
    drawLeft(kLeftW, contentH);
    ImGui::SetCursorScreenPos(ImVec2(kMargin + kLeftW + kGap, kMargin));
    drawRight(rightW, contentH);
    ImGui::SetCursorScreenPos(ImVec2(kMargin, h - kMargin - kBarH));
    drawBottomBar(w - 2 * kMargin);

    drawCalibrationModal(w, h);
    drawAdvancedModal(w, h);
    drawConfirmModal(w, h);
    drawUnlockModal(w, h);

    ImGui::PopFont();
    ImGui::End();
}

void App::drawLeft(float W, float H) {
    ImGui::PushStyleColor(ImGuiCol_ChildBg, IM_COL32(0, 0, 0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 10));
    ImGui::BeginChild("##left", ImVec2(W, H), false, ImGuiWindowFlags_NoScrollbar);
    drawHeader(W);
    drawBurnerCard(W);
    drawImageCard(W);
    drawTonesCard(W);
    drawStartButton(W);
    ImGui::EndChild();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();
}

void App::drawHeader(float W) {
    ImVec2 p = ImGui::GetCursorScreenPos();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    drawIcon(dl, kIconDisc, ImVec2(p.x + 18, p.y + 22), 30, cAccent());
    ImGui::SetCursorScreenPos(ImVec2(p.x + 42, p.y + 1));
    ImGui::PushFont(fonts.bold);
    ImGui::TextUnformatted("DiscImageStudio");
    ImGui::PopFont();
    ImGui::SetCursorScreenPos(ImVec2(p.x + 42, p.y + 22));
    ImGui::PushFont(fonts.small);
    ImGui::PushStyleColor(ImGuiCol_Text, cDim());
    ImGui::TextUnformatted("CD-R image burner");
    ImGui::PopStyleColor();
    ImGui::PopFont();

    ImGui::SetCursorScreenPos(ImVec2(p.x + W - 36 - 34, p.y + 8));
    iconButton("##about", kIconInfo, 30, "CD-R / CD-RW only. The burned disc contains noise-like audio - do not play it on speakers. "
                                         "Run this program as administrator.");
    ImGui::SetCursorScreenPos(ImVec2(p.x + W - 34, p.y + 8));
    if (iconButton("##gear", kIconGear, 30, "Advanced settings")) openAdvanced_ = true;
    ImGui::SetCursorScreenPos(ImVec2(p.x, p.y + 50));
}

void App::drawBurnerCard(float W) {
    const bool running = job_.running();
    beginCard("##burner", ImVec2(W, 120), "Burner", kIconDisc);
    const float inner = ImGui::GetContentRegionAvail().x;
    ImGui::BeginDisabled(running);
    char cur[48] = "No optical drive found";
    if (driveIdx_ >= 0) std::snprintf(cur, sizeof cur, "%c:   CD/DVD drive", drives_[driveIdx_]);
    ImGui::SetNextItemWidth(inner - 36);
    if (ImGui::BeginCombo("##drive", cur)) {
        for (size_t i = 0; i < drives_.size(); ++i) {
            char l[40]; std::snprintf(l, sizeof l, "%c:   CD/DVD drive", drives_[i]);
            if (ImGui::Selectable(l, (int)i == driveIdx_)) driveIdx_ = (int)i;
        }
        ImGui::EndCombo();
    }
    ImGui::SameLine();
    if (iconButton("##refresh", kIconRefresh, 28, "Rescan optical drives")) refreshDrives();

    char sp[16]; std::snprintf(sp, sizeof sp, "%dx", cfg_.speedX);
    ImGui::SetNextItemWidth(84);
    if (ImGui::BeginCombo("##speed", sp)) {
        for (size_t i = 0; i < sizeof kSpeeds / sizeof kSpeeds[0]; ++i) {
            char l[16]; std::snprintf(l, sizeof l, "%dx", kSpeeds[i]);
            if (ImGui::Selectable(l, kSpeeds[i] == cfg_.speedX)) { cfg_.speedX = kSpeeds[i]; saveCfg(); }
        }
        ImGui::EndCombo();
    }
    if (ImGui::IsItemHovered(0)) tooltipFor("Write speed. Lower is safer on slow computers.");
    ImGui::EndDisabled();

    ImGui::SameLine(0, 18);
    const bool locked = !cfg_.testWritePassed;
    if (locked) testWrite_ = true;
    ImGui::SetCursorScreenPos(ImVec2(ImGui::GetCursorScreenPos().x, ImGui::GetCursorScreenPos().y + 4));
    toggle("##testwrite", &testWrite_, locked || running);
    ImGui::SameLine();
    ImGui::SetCursorScreenPos(ImVec2(ImGui::GetCursorScreenPos().x, ImGui::GetCursorScreenPos().y + 3));
    ImGui::TextUnformatted("Test write");
    ImGui::SameLine();
    ImGui::SetCursorScreenPos(ImVec2(ImGui::GetCursorScreenPos().x, ImGui::GetCursorScreenPos().y + 2));
    infoTip("##twtip", "Simulates the whole burn without firing the laser. It is locked ON until one test write succeeds "
                       "on your drive. If your drive does not support test write, burn a CD-RW first, then set "
                       "test_write_passed=1 in config.ini.");
    endCard();
}

void App::drawImageCard(float W) {
    beginCard("##image", ImVec2(W, 92), "Image", kIconImage);
    ImVec2 start = ImGui::GetCursorScreenPos(), wp = ImGui::GetWindowPos();
    ImGui::SetCursorScreenPos(ImVec2(wp.x + W - 14 - 86, wp.y + 8));
    ImGui::BeginDisabled(job_.running());
    if (secondaryButton("Browse...", ImVec2(86, 26))) chooseImage();
    ImGui::EndDisabled();
    ImGui::SetCursorScreenPos(start);

    const float inner = W - 28;
    if (image_) {
        size_t slash = imagePath_.find_last_of("\\/");
        std::string name = slash == std::string::npos ? imagePath_ : imagePath_.substr(slash + 1);
        ImGui::TextUnformatted(ellipsize(name, inner).c_str());
        ImGui::PushFont(fonts.small);
        ImGui::PushStyleColor(ImGuiCol_Text, cDim());
        ImGui::TextUnformatted(imageInfo_.c_str());
        ImGui::PopStyleColor();
        ImGui::PopFont();
    } else {
        ImGui::PushStyleColor(ImGuiCol_Text, cDim());
        ImGui::TextUnformatted("PNG, JPG or BMP");
        ImGui::PopStyleColor();
        ImGui::PushFont(fonts.small);
        ImGui::PushStyleColor(ImGuiCol_Text, cDim());
        ImGui::TextUnformatted("or drop a file onto the disc");
        ImGui::PopStyleColor();
        ImGui::PopFont();
    }
    endCard();
}

void App::drawTonesCard(float W) {
    beginCard("##tones", ImVec2(W, 92), "Tones", kIconSliders);
    ImVec2 start = ImGui::GetCursorScreenPos(), wp = ImGui::GetWindowPos();
    ImGui::SetCursorScreenPos(ImVec2(wp.x + W - 14 - 104, wp.y + 8));
    ImGui::BeginDisabled(job_.running());
    if (secondaryButton("Calibrate...", ImVec2(104, 26))) openCalib_ = true;
    ImGui::EndDisabled();
    if (ImGui::IsItemHovered(0)) tooltipFor("Burn a test ladder on a spare disc to find the byte patterns that give the strongest contrast on your drive and media.");
    ImGui::SetCursorScreenPos(start);

    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 p = ImGui::GetCursorScreenPos();
    char a[40], b[40];
    std::snprintf(a, sizeof a, "Dark  0x%02X", cfg_.darkByte);
    std::snprintf(b, sizeof b, "Light  0x%02X", cfg_.lightByte);
    dl->AddRectFilled(ImVec2(p.x, p.y + 2), ImVec2(p.x + 18, p.y + 20), IM_COL32(28, 34, 46, 255), 5.f);
    dl->AddRect(ImVec2(p.x, p.y + 2), ImVec2(p.x + 18, p.y + 20), IM_COL32(90, 98, 116, 255), 5.f, 0, 1.f);
    ImGui::SetCursorScreenPos(ImVec2(p.x + 26, p.y));
    ImGui::TextUnformatted(a);
    ImGui::SetCursorScreenPos(ImVec2(p.x + 150, p.y));
    ImVec2 q = ImGui::GetCursorScreenPos();
    dl->AddRectFilled(ImVec2(q.x, q.y + 2), ImVec2(q.x + 18, q.y + 20), IM_COL32(222, 228, 238, 255), 5.f);
    ImGui::SetCursorScreenPos(ImVec2(q.x + 26, q.y));
    ImGui::TextUnformatted(b);
    if (cfg_.darkByte == cfg_.lightByte) {
        ImGui::SetCursorScreenPos(ImVec2(p.x, p.y + 28));
        ImGui::PushStyleColor(ImGuiCol_Text, cBad());
        ImGui::TextUnformatted("Same byte for both tones - calibrate.");
        ImGui::PopStyleColor();
    }
    endCard();
}

void App::startButtonFor(BurnRequest::Kind kind, const char* realLabel, const char* testLabel, float width) {
    const bool test = effectiveTestWrite();
    const char* reason = 0;
    if (job_.running()) reason = "A job is already running.";
    else if (driveIdx_ < 0) reason = "No optical drive found.";
    else if (kind == BurnRequest::kImage && !image_) reason = "Choose an image first.";
    ImGui::BeginDisabled(reason != 0);
    if (primaryButton(test ? testLabel : realLabel, ImVec2(width, 52), kIconPlay)) beginBurn(kind);
    ImGui::EndDisabled();
    if (reason && ImGui::IsItemHovered(0)) tooltipFor(reason);
}

void App::drawStartButton(float W) {
    startButtonFor(BurnRequest::kImage, "Burn disc", "Start test write (nothing is burned)", W);
}

void App::drawRight(float W, float H) {
    beginCard("##preview", ImVec2(W, H), "Live preview", kIconImage);
    const bool running = job_.running();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 wp = ImGui::GetWindowPos();

    // chips on the title row
    {
        char chip[48]; float x = wp.x + W - 14;
        std::vector<std::string> chips;
        chips.push_back("2 tones");
        if (cfg_.angleOffsetDeg != 0.0) { std::snprintf(chip, sizeof chip, "rot %.0f deg", cfg_.angleOffsetDeg); chips.push_back(chip); }
        if (cfg_.mirror) chips.push_back("mirrored");
        ImGui::PushFont(fonts.small);
        for (size_t i = chips.size(); i-- > 0;) {
            ImVec2 ts = ImGui::CalcTextSize(chips[i].c_str());
            float w = ts.x + 18;
            dl->AddRectFilled(ImVec2(x - w, wp.y + 9), ImVec2(x, wp.y + 29), cField(), 10.f);
            dl->AddText(ImVec2(x - w + 9, wp.y + 11), cDim(), chips[i].c_str());
            x -= w + 6;
        }
        ImGui::PopFont();
    }

    ImVec2 avail = ImGui::GetContentRegionAvail();
    const float toneRowH = 58.f, captionH = 22.f;
    const float D = std::max(120.f, std::min(avail.x, avail.y - toneRowH - captionH));
    ImVec2 p0 = ImGui::GetCursorScreenPos();
    ImVec2 dmin(p0.x + (avail.x - D) * 0.5f, p0.y), dmax(dmin.x + D, dmin.y + D);
    ImVec2 ctr((dmin.x + dmax.x) * 0.5f, (dmin.y + dmax.y) * 0.5f);

    ImGui::InvisibleButton("##disc", ImVec2(avail.x, D));
    const bool hover = ImGui::IsItemHovered(0);
    if (ImGui::IsItemClicked(0) && !running) chooseImage();
    if (hover && !running) { ImGui::SetMouseCursor(ImGuiMouseCursor_Hand); }

    if (previewTex_) dl->AddImage((ImTextureID)previewTex_, dmin, dmax, ImVec2(0, 0), ImVec2(1, 1), IM_COL32(255, 255, 255, 255));
    else dl->AddCircle(ctr, D * 0.48f, cField(), 64, 2.f);
    if (hover && !running) dl->AddCircle(ctr, D * 0.5f, IM_COL32(0x3B, 0x82, 0xF6, 110), 96, 3.f);

    // geometry helpers: preview shows 60 mm outer radius at (190/192) of the half size
    const float pxPerMm = (D * 0.5f) * (190.f / 192.f) / 60.f;

    if (!image_ && !running) {
        const char* l1 = "Drop an image here";
        const char* l2 = "or click to browse";
        ImGui::PushFont(fonts.bold);
        ImVec2 s1 = ImGui::CalcTextSize(l1);
        ImGui::PopFont();
        ImGui::PushFont(fonts.small);
        ImVec2 s2 = ImGui::CalcTextSize(l2);
        ImGui::PopFont();
        float bw = std::max(s1.x, s2.x) + 32, bh = 52;
        float by = ctr.y + 20 * pxPerMm * 0.55f + 24.f;       // sits on the blank ring, below the hub
        dl->AddRectFilled(ImVec2(ctr.x - bw * 0.5f, by), ImVec2(ctr.x + bw * 0.5f, by + bh), IM_COL32(0x18, 0x1A, 0x20, 220), 12.f);
        ImGui::PushFont(fonts.bold);
        dl->AddText(ImVec2(ctr.x - s1.x * 0.5f, by + 8), cText(), l1);
        ImGui::PopFont();
        ImGui::PushFont(fonts.small);
        dl->AddText(ImVec2(ctr.x - s2.x * 0.5f, by + 30), cDim(), l2);
        ImGui::PopFont();
    }

    // while burning: dim the part of the disc that is not written yet, accent ring at the laser
    if (running) {
        BurnJob::Status st = job_.status();
        float f = st.total ? std::min(1.f, (float)st.done / (float)st.total) : 0.f;
        float rEnd = 58.f, rStart = 25.f;
        float rNow = std::sqrt(rStart * rStart + f * (rEnd * rEnd - rStart * rStart));
        float rMid = (rNow + rEnd) * 0.5f;
        dl->AddCircle(ctr, rMid * pxPerMm, IM_COL32(0x18, 0x1A, 0x20, 175), 128, (rEnd - rNow) * pxPerMm);
        dl->AddCircle(ctr, rNow * pxPerMm, cAccent(), 128, 2.5f);
    }

    // caption
    ImGui::SetCursorScreenPos(ImVec2(p0.x, p0.y + D + 4));
    ImGui::PushFont(fonts.small);
    ImGui::PushStyleColor(ImGuiCol_Text, cDim());
    const char* cap = "Burned ring 25-58 mm. The centre stays blank. Preview is a simulation.";
    ImVec2 cs = ImGui::CalcTextSize(cap);
    ImGui::SetCursorScreenPos(ImVec2(p0.x + (avail.x - cs.x) * 0.5f, p0.y + D + 4));
    ImGui::TextUnformatted(cap);
    ImGui::PopStyleColor();
    ImGui::PopFont();

    // live tone controls
    ImGui::SetCursorScreenPos(ImVec2(p0.x, p0.y + D + captionH + 6));
    const float colW = (avail.x - 2 * 12) / 3.f;
    bool ch = false;
    struct Slider { const char* id; const char* label; float* v; float lo, hi; };
    Slider sl[3] = { { "##br", "Brightness", &cfg_.brightness, -0.5f, 0.5f },
                     { "##co", "Contrast",   &cfg_.contrast,    0.2f, 3.0f },
                     { "##ga", "Gamma",      &cfg_.gamma,       0.3f, 3.0f } };
    ImGui::BeginDisabled(running);
    for (int i = 0; i < 3; ++i) {
        ImVec2 q(p0.x + i * (colW + 12), p0.y + D + captionH + 6);
        ImGui::SetCursorScreenPos(q);
        ImGui::PushFont(fonts.small);
        ImGui::PushStyleColor(ImGuiCol_Text, cDim());
        ImGui::TextUnformatted(sl[i].label);
        ImGui::PopStyleColor();
        ImGui::PopFont();
        ImGui::SetCursorScreenPos(ImVec2(q.x, q.y + 18));
        ImGui::SetNextItemWidth(colW);
        if (ImGui::SliderFloat(sl[i].id, sl[i].v, sl[i].lo, sl[i].hi, "%.2f")) { ch = true; }
        if (ImGui::IsItemDeactivatedAfterEdit()) saveCfg();
    }
    ImGui::EndDisabled();
    if (ch) needRender_ = true;
    endCard();
}

void App::drawBottomBar(float W) {
    ImGui::PushStyleColor(ImGuiCol_ChildBg, cCard());
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 14.f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(16, 0));
    ImGui::BeginChild("##bar", ImVec2(W, kBarH), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 wp = ImGui::GetWindowPos();
    const bool running = job_.running();
    BurnJob::Status st = job_.status();
    const float midY = wp.y + kBarH * 0.5f;

    // status (left)
    const float statusW = 250.f;
    ImU32 dotCol = running ? cAccent() : (statusKind_ == kStatusOk ? cGood() : (statusKind_ == kStatusError ? cBad() : cDim()));
    dl->AddCircleFilled(ImVec2(wp.x + 24, midY), 5.f, dotCol, 12);
    std::string msg = running ? (job_.request().testWrite ? "Test write in progress..." : "Burning - do not disturb the PC")
                              : (statusMsg_.empty() ? std::string("Ready") : statusMsg_);
    ImGui::SetCursorScreenPos(ImVec2(wp.x + 40, midY - 9));
    ImGui::TextUnformatted(ellipsize(msg, statusW - 24).c_str());
    if (ImGui::IsItemHovered(0) && !msg.empty() && ImGui::CalcTextSize(msg.c_str()).x > statusW - 24) tooltipFor(msg.c_str());

    // right-hand controls
    enum { kSlotNone, kSlotUnlock, kSlotLog, kSlotSpeed } slot = kSlotNone;
    if (!running) {
        if (testWriteRejected_) slot = kSlotUnlock;
        else if (statusKind_ == kStatusError) slot = kSlotLog;
        else if (recommendedSpeed_ > 0) slot = kSlotSpeed;
    }
    const float rightW = running ? 232.f : (slot != kSlotNone ? 200.f : 0.f);
    const float barX = wp.x + 40 + statusW + 8, barW = W - (barX - wp.x) - 16 - rightW - (rightW > 0 ? 12.f : 0.f);
    float frac = running ? (st.total ? (float)st.done / (float)st.total : 0.f) : (statusKind_ == kStatusOk ? 1.f : 0.f);
    char lab[48];
    if (running) std::snprintf(lab, sizeof lab, "%.0f%%", frac * 100.f); else lab[0] = 0;
    ImGui::SetCursorScreenPos(ImVec2(barX, midY - 9));
    progressBar(frac, ImVec2(barW, 18), lab);

    if (running) {
        float x = barX + barW + 12;
        const char* sname = st.state == kBufHealthy ? "OK" : (st.state == kBufLow ? "LOW" : "CRITICAL");
        ImU32 pc = st.state == kBufHealthy ? cGood() : (st.state == kBufLow ? cWarn() : cBad());
        dl->AddRectFilled(ImVec2(x, midY - 14), ImVec2(x + 140, midY + 14), cField(), 14.f);
        dl->AddCircleFilled(ImVec2(x + 16, midY), 5.f, pc, 12);
        char pb[48]; std::snprintf(pb, sizeof pb, "Buffer %.0f%% %s", st.fill * 100.0, sname);
        ImGui::PushFont(fonts.small);
        dl->AddText(ImVec2(x + 28, midY - 8), cText(), pb);
        ImGui::PopFont();
        ImGui::SetCursorScreenPos(ImVec2(x + 150, midY - 14));
        if (secondaryButton("Cancel", ImVec2(82, 28))) job_.cancel();
        if (ImGui::IsItemHovered(0) && !job_.request().testWrite) tooltipFor("Cancelling a real burn ruins the disc.");
    } else if (slot == kSlotUnlock) {
        ImGui::SetCursorScreenPos(ImVec2(barX + barW + 12, midY - 14));
        if (primaryButton("Unlock real burn", ImVec2(200, 28), kIconNone)) openUnlock_ = true;
    } else if (slot == kSlotLog) {
        ImGui::SetCursorScreenPos(ImVec2(barX + barW + 12, midY - 14));
        if (secondaryButton("Open burn log", ImVec2(200, 28))) openInDefaultApp(hwnd_, logPath_);
        if (ImGui::IsItemHovered(0)) tooltipFor("Every command sent to the drive and its answer. Send this file when asking for help.");
    } else if (slot == kSlotSpeed) {
        char l[64]; std::snprintf(l, sizeof l, "Buffer ran low - use %dx", recommendedSpeed_);
        ImGui::SetCursorScreenPos(ImVec2(barX + barW + 12, midY - 14));
        if (secondaryButton(l, ImVec2(200, 28))) { cfg_.speedX = recommendedSpeed_; saveCfg(); recommendedSpeed_ = 0; }
        if (ImGui::IsItemHovered(0) && !statsMsg_.empty()) tooltipFor(statsMsg_.c_str());
    }
    ImGui::EndChild();
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor();
}

// ================================================================================ modals

static void centerNext(float w, float h, float mw, float mh) {
    ImGui::SetNextWindowPos(ImVec2(w * 0.5f, h * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(mw, mh), ImGuiCond_Always);
}

void App::drawConfirmModal(float w, float h) {
    if (openConfirm_) { ImGui::OpenPopup("Burn this disc?"); openConfirm_ = false; }
    centerNext(w, h, 400, 170);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20, 18));
    if (ImGui::BeginPopupModal("Burn this disc?", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove)) {
        ImGui::Text("Drive %c: at %dx", pending_.driveLetter, pending_.cfg.speedX);
        ImGui::PushStyleColor(ImGuiCol_Text, cDim());
        ImGui::TextUnformatted("Use a blank CD-R. This cannot be undone.");
        ImGui::PopStyleColor();
        ImGui::SetCursorScreenPos(ImVec2(ImGui::GetWindowPos().x + 20, ImGui::GetWindowPos().y + 112));
        if (primaryButton("Burn", ImVec2(170, 38), kIconNone)) { ImGui::CloseCurrentPopup(); launch(pending_); }
        ImGui::SameLine();
        if (secondaryButton("Cancel", ImVec2(170, 38))) ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }
    ImGui::PopStyleVar();
}

void App::drawUnlockModal(float w, float h) {
    if (openUnlock_) { ImGui::OpenPopup("Unlock real burning?"); openUnlock_ = false; }
    centerNext(w, h, 460, 210);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20, 18));
    if (ImGui::BeginPopupModal("Unlock real burning?", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove)) {
        ImGui::PushTextWrapPos(420.f);
        ImGui::TextUnformatted("Use this only if your drive cannot do a test write (many newer drives cannot).");
        ImGui::PushStyleColor(ImGuiCol_Text, cDim());
        ImGui::TextUnformatted("After unlocking, the Burn button writes to the disc for real. Use a spare blank CD-R "
                               "for the first try; a failed burn cannot be undone.");
        ImGui::PopStyleColor();
        ImGui::PopTextWrapPos();
        ImGui::SetCursorScreenPos(ImVec2(ImGui::GetWindowPos().x + 20, ImGui::GetWindowPos().y + 152));
        if (primaryButton("Unlock", ImVec2(200, 38), kIconNone)) {
            cfg_.testWritePassed = true; testWrite_ = false; testWriteRejected_ = false; saveCfg();
            setStatus(kStatusOk, "Real burning unlocked. Press Burn disc.");
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (secondaryButton("Cancel", ImVec2(200, 38))) ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }
    ImGui::PopStyleVar();
}

void App::drawAdvancedModal(float w, float h) {
    if (openAdvanced_) { ImGui::OpenPopup("Advanced"); openAdvanced_ = false; }
    centerNext(w, h, 440, 410);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20, 18));
    if (ImGui::BeginPopupModal("Advanced", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove)) {
        bool ch = false, render = false;
        ImGui::BeginDisabled(job_.running());
        char cl[16]; std::snprintf(cl, sizeof cl, "%d bytes", cfg_.cellBytes);
        ImGui::TextUnformatted("Cell size");
        ImGui::SetNextItemWidth(160);
        if (ImGui::BeginCombo("##cell", cl)) {
            for (size_t i = 0; i < sizeof kCells / sizeof kCells[0]; ++i) {
                char l[16]; std::snprintf(l, sizeof l, "%d bytes", kCells[i]);
                if (ImGui::Selectable(l, kCells[i] == cfg_.cellBytes)) { cfg_.cellBytes = kCells[i]; ch = true; }
            }
            ImGui::EndCombo();
        }
        ImGui::TextUnformatted("Dithering");
        if (ImGui::RadioButton("Floyd-Steinberg", !cfg_.jjn)) { cfg_.jjn = false; ch = true; }
        ImGui::SameLine();
        if (ImGui::RadioButton("Jarvis-Judice-Ninke", cfg_.jjn)) { cfg_.jjn = true; ch = true; }
        ImGui::TextUnformatted("Image rotation");
        float ang = (float)cfg_.angleOffsetDeg;
        ImGui::SetNextItemWidth(-1);
        if (ImGui::SliderFloat("##rot", &ang, -180.f, 180.f, "%.1f deg")) { cfg_.angleOffsetDeg = ang; render = true; }
        if (ImGui::IsItemDeactivatedAfterEdit()) ch = true;
        bool mir = cfg_.mirror;
        ImGui::TextUnformatted("Mirror image");
        ImGui::SameLine(0, 12);
        if (toggle("##mirror", &mir, false)) { cfg_.mirror = mir; render = true; ch = true; }
        ImGui::EndDisabled();
        if (render) needRender_ = true;
        if (ch) saveCfg();
        ImGui::Separator();
        ImGui::TextUnformatted("Safety and diagnostics");
        if (!cfg_.testWritePassed && secondaryButton("Skip the test-write requirement...", ImVec2(400, 30))) openUnlock_ = true;
        if (secondaryButton("Open burn log", ImVec2(400, 30))) openInDefaultApp(hwnd_, logPath_);
        ImGui::SetCursorScreenPos(ImVec2(ImGui::GetWindowPos().x + 20, ImGui::GetWindowPos().y + 352));
        if (secondaryButton("Close", ImVec2(400, 36))) ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }
    ImGui::PopStyleVar();
}

void App::drawCalibrationModal(float w, float h) {
    if (openCalib_) { ImGui::OpenPopup("Calibration"); openCalib_ = false; }
    centerNext(w, h, 580, 560);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20, 18));
    if (ImGui::BeginPopupModal("Calibration", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove)) {
        const bool running = job_.running();
        ImGui::PushFont(fonts.small);
        ImGui::PushStyleColor(ImGuiCol_Text, cDim());
        ImGui::PushTextWrapPos(540.f);
        ImGui::TextUnformatted("1. Insert a spare blank CD-R and burn the ladder: 16 candidate byte patterns, one ring each, "
                               "starting near the centre.");
        ImGui::TextUnformatted("2. Tilt the disc under a lamp. Find the lightest and the darkest ring and pick them below.");
        ImGui::PopTextWrapPos();
        ImGui::PopStyleColor();
        ImGui::PopFont();

        ImGui::BeginDisabled(running);
        ImGui::TextUnformatted("Sectors per band");
        ImGui::SameLine(0, 12);
        ImGui::SetNextItemWidth(130);
        if (ImGui::InputInt("##band", &cfg_.bandSectors, 500, 1000)) saveCfg();
        ImGui::EndDisabled();
        startButtonFor(BurnRequest::kCalibLadder, "Burn calibration ladder", "Start test write (nothing is burned)", 540);

        const std::vector<uint8_t> ladder = CalibrationConfig::defaultLadder();
        DiscGeometry g;
        ImGui::TextUnformatted("Band map (approx. radius from the centre)");
        if (ImGui::BeginChild("##bands", ImVec2(540, 118), true)) {
            ImGui::PushFont(fonts.small);
            for (size_t i = 0; i < ladder.size(); ++i) {
                double r0 = g.radiusAtLength((double)i * cfg_.bandSectors * g.sectorLengthMm);
                double r1 = g.radiusAtLength((double)(i + 1) * cfg_.bandSectors * g.sectorLengthMm);
                ImGui::Text("Band %2d     byte 0x%02X     %.1f - %.1f mm", (int)i + 1, ladder[i], r0, r1);
            }
            ImGui::PopFont();
        }
        ImGui::EndChild();

        struct Pick { const char* label; const char* id; int* sel; };
        Pick picks[2] = { { "Lightest ring", "##light", &lightSel_ }, { "Darkest ring", "##dark", &darkSel_ } };
        for (int p = 0; p < 2; ++p) {
            char cur[40] = "Choose a band";
            int s = *picks[p].sel;
            if (s >= 0) std::snprintf(cur, sizeof cur, "Band %d   (0x%02X)", s + 1, ladder[(size_t)s]);
            ImGui::TextUnformatted(picks[p].label);
            ImGui::SameLine(150);
            ImGui::SetNextItemWidth(220);
            if (ImGui::BeginCombo(picks[p].id, cur)) {
                for (size_t i = 0; i < ladder.size(); ++i) {
                    char l[40]; std::snprintf(l, sizeof l, "Band %d   (0x%02X)", (int)i + 1, ladder[i]);
                    if (ImGui::Selectable(l, (int)i == s)) *picks[p].sel = (int)i;
                }
                ImGui::EndCombo();
            }
        }
        const bool ready = lightSel_ >= 0 && darkSel_ >= 0 && lightSel_ != darkSel_;
        ImGui::BeginDisabled(!ready);
        if (primaryButton("Save to config.ini", ImVec2(540, 38), kIconCheck)) {
            cfg_.lightByte = ladder[(size_t)lightSel_]; cfg_.darkByte = ladder[(size_t)darkSel_];
            saveCfg();
            setStatus(kStatusOk, "Saved. These bytes are now the default tones.");
        }
        ImGui::EndDisabled();
        if (lightSel_ >= 0 && lightSel_ == darkSel_) {
            ImGui::PushStyleColor(ImGuiCol_Text, cBad());
            ImGui::TextUnformatted("Pick two different bands.");
            ImGui::PopStyleColor();
        }
        if (ImGui::CollapsingHeader("Alignment test (angle / mirror)")) {
            ImGui::BeginDisabled(running);
            ImGui::SetNextItemWidth(120);
            if (ImGui::InputInt("Sectors per block", &cfg_.altBlockSectors, 1, 10)) saveCfg();
            ImGui::SetNextItemWidth(120);
            if (ImGui::InputInt("Length (minutes)", &cfg_.altMinutes, 1, 5)) saveCfg();
            ImGui::EndDisabled();
            startButtonFor(BurnRequest::kCalibAlternate, "Burn alignment test", "Start test write (nothing is burned)", 540);
        }
        if (secondaryButton("Close", ImVec2(540, 34))) ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }
    ImGui::PopStyleVar();
}

} // namespace dis
