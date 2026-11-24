// App.h - all UI logic (Dear ImGui calls) with no Win32/D3D code, so it stays small and testable.
#pragma once
#include "BurnJob.h"
#include "../core/Config.h"
#include <memory>
#include <string>
#include <vector>

namespace dis {

class App {
public:
    static const int kPreview = 256;

    App();
    void init(const std::string& configPathUtf8, void* ownerHwnd);
    void drawUi(float width, float height);          // call between ImGui::NewFrame() and Render()

    bool busy() const { return job_.running(); }
    bool busyRealBurn() const { return job_.running() && !job_.request().testWrite; }
    bool pressure() const;                           // drive buffer low: UI should redraw rarely
    bool takePreviewDirty() { bool d = previewDirty_; previewDirty_ = false; return d; }
    const uint32_t* previewPixels() const { return preview_; }
    void setPreviewTexture(void* tex) { previewTex_ = tex; }

private:
    void pollJob();
    void refreshDrives();
    void chooseImage();
    void buildPreview();
    void saveCfg();
    void beginBurn(BurnRequest::Kind kind);
    void launch(const BurnRequest& r);
    bool effectiveTestWrite() const { return testWrite_ || !cfg_.testWritePassed; }

    void drawDeviceSection();
    void drawImageTab();
    void drawCalibrationTab();
    void drawProgress();
    void drawConfirmPopup();
    void drawBurnButton(const char* realLabel, const char* testLabel, BurnRequest::Kind kind);

    AppConfig cfg_;
    std::string cfgPath_;
    void* hwnd_;
    std::vector<char> drives_;
    int driveIdx_;
    bool testWrite_;

    std::shared_ptr<GrayRaster> image_;
    std::string imagePath_, loadMsg_;
    uint32_t preview_[kPreview * kPreview];
    bool previewDirty_;
    void* previewTex_;

    BurnJob job_;
    BurnRequest pending_;
    bool showConfirm_;
    std::string statusMsg_, statsMsg_;
    int recommendedSpeed_;
    int lightSel_, darkSel_;
};

} // namespace dis
