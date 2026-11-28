// App.h - UI logic for the split-layout window (900x600): settings cards on the left, live disc
// preview on the right, progress bar at the bottom. Only ImGui calls here - no Win32/D3D.
#pragma once
#include "BurnJob.h"
#include "DiscPreview.h"
#include "../core/Config.h"
#include <memory>
#include <string>
#include <vector>

namespace dis {

class App {
public:
    static const int kPreview = DiscPreview::kSize;

    App();
    void init(const std::string& configPathUtf8, void* ownerHwnd);
    void drawUi(float width, float height);          // between ImGui::NewFrame() and Render()
    void loadImagePath(const std::string& pathUtf8); // browse button and drag & drop

    bool busy() const { return job_.running(); }
    bool busyRealBurn() const { return job_.running() && !job_.request().testWrite; }
    bool pressure() const;                           // drive buffer low: redraw rarely
    bool refreshPreview();                           // re-renders the disc if params changed; true = upload texture
    const uint32_t* previewPixels() const { return preview_.data(); }
    void setPreviewTexture(void* tex) { previewTex_ = tex; }

private:
    enum StatusKind { kStatusInfo = 0, kStatusOk = 1, kStatusError = 2 };

    void pollJob();
    void refreshDrives();
    void chooseImage();
    void saveCfg();
    void setStatus(int kind, const std::string& msg);
    void beginBurn(BurnRequest::Kind kind);
    void launch(const BurnRequest& r);
    bool effectiveTestWrite() const { return testWrite_ || !cfg_.testWritePassed; }
    PreviewParams previewParams() const;

    void drawLeft(float w, float h);
    void drawHeader(float w);
    void drawBurnerCard(float w);
    void drawImageCard(float w);
    void drawTonesCard(float w);
    void drawStartButton(float w);
    void drawRight(float w, float h);
    void drawBottomBar(float w);
    void drawCalibrationModal(float w, float h);
    void drawAdvancedModal(float w, float h);
    void drawConfirmModal(float w, float h);
    void drawUnlockModal(float w, float h);
    void startButtonFor(BurnRequest::Kind kind, const char* realLabel, const char* testLabel, float width);

    AppConfig cfg_;
    std::string cfgPath_;
    void* hwnd_;
    std::vector<char> drives_;
    int driveIdx_;
    bool testWrite_;

    std::shared_ptr<GrayRaster> image_;
    std::string imagePath_, imageInfo_;
    DiscPreview disc_;
    std::vector<uint32_t> preview_;
    bool needRender_;
    void* previewTex_;

    BurnJob job_;
    BurnRequest pending_;
    bool openConfirm_, openCalib_, openAdvanced_, openUnlock_, testWriteRejected_;
    std::string logPath_;
    int statusKind_;
    std::string statusMsg_, statsMsg_;
    int recommendedSpeed_;
    int lightSel_, darkSel_;
};

} // namespace dis
