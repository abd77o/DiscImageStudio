# third_party

Not committed to the repo. Run `scripts\fetch_deps.bat` (Windows) or `scripts/fetch_deps.sh` to download:

* **Dear ImGui** v1.90.9 (MIT) - the GUI (needs >= 1.84 for `BeginDisabled`)
* **stb_image.h** (public domain / MIT) - PNG + JPEG decoding

Without stb the project still builds (BMP only); without ImGui the GUI target is skipped.
If the `v141_xp` toolset cannot find `d3d9.h`, install the DirectX SDK (June 2010) and add its Include and Lib\x86 folders.
