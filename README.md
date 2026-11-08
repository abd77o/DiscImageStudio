# DiscImageStudio (working title)

src/core/    DiscGeometry  ImageSource(BMP stream)  ImageLoader(PNG/JPG via stb)  PolarDitherEngine
             Config(config.ini)  PlatformFile(UTF-8 paths)
src/burn/    Scrambler  SectorComposer  SectorSource  Mmc  CueSheet  SafeBufferMonitor
             RawDaoWriter  ScsiDevice  SptiDevice(Windows)
src/calib/   CalibrationSource (ladder / alternating blocks)
src/gui/     BurnJob(worker thread)  App(ImGui UI)  main_win(Win32+D3D9)  Dialogs
tests/       mock-drive tests (run on any OS) + shims used only for syntax checks
third_party/ stb_image.h and Dear ImGui go here - see third_party/README.md

Build (Windows 7 target, one static exe):  build_vs2017.bat
Tests (any OS):  cmake -S . -B b && cmake --build b && b/test_burn_logic && b/test_gui_logic && b/test_image_burn <file.bmp>

Run as Administrator. First hardware test: CD-RW or a spare CD-R with the test-write box ticked.
