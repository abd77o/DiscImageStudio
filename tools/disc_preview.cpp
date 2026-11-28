// disc_preview - renders the live-preview disc to a raw RGBA file (used to check the look
// without the GUI). Usage: disc_preview in.bmp out.rgba [brightness contrast gamma angleDeg mirror]
#include "../src/core/ImageSource.h"
#include "../src/gui/DiscPreview.h"
#include <cstdio>
#include <cstdlib>
using namespace dis;
int main(int argc, char** argv) {
    if (argc < 3) { std::printf("usage: %s in.bmp out.rgba [brightness contrast gamma angle mirror]\n", argv[0]); return 1; }
    GrayRaster img; std::string err;
    if (!loadBmpAsGrayRaster(argv[1], 1024, img, err)) { std::printf("load: %s\n", err.c_str()); return 2; }
    PreviewParams p;
    if (argc > 3) p.brightness = (float)atof(argv[3]);
    if (argc > 4) p.contrast = (float)atof(argv[4]);
    if (argc > 5) p.gamma = (float)atof(argv[5]);
    if (argc > 6) p.angleDeg = atof(argv[6]);
    if (argc > 7) p.mirror = atoi(argv[7]) != 0;
    DiscPreview prev; std::vector<uint32_t> px((size_t)DiscPreview::kSize * DiscPreview::kSize);
    prev.render(&img, p, px.data());
    FILE* f = std::fopen(argv[2], "wb");
    for (size_t i = 0; i < px.size(); ++i) {
        unsigned char c[4] = { (unsigned char)(px[i] >> 16), (unsigned char)(px[i] >> 8), (unsigned char)px[i], (unsigned char)(px[i] >> 24) };
        std::fwrite(c, 1, 4, f);
    }
    std::fclose(f);
    return 0;
}
