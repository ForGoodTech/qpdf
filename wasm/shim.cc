#include <qpdf/QPDFJob.hh>
#include <atomic>
#include <exception>
#include <fstream>
#include <string>

static std::atomic<int> g_progress{0};

extern "C" int
qpdf_wasm_get_progress()
{
    return g_progress.load();
}

extern "C" int
qpdf_wasm_compress(
    char const* infilename,
    char const* outfilename,
    int level,
    int jpeg_quality,
    double* rate)
{
    try {
        g_progress.store(0);
        QPDFJob j;
        j.registerProgressReporter([](int p) { g_progress.store(p); });
        auto cfg = j.config();
        cfg->inputFile(infilename)
            ->outputFile(outfilename)
            ->progress()
            ->compressStreams("y")
            ->objectStreams("generate")
            ->optimizeImages()
            ->jpegQuality(std::to_string(jpeg_quality))
            ->compressionLevel(std::to_string(level));
        switch (level) {
        case 3:
            cfg->decodeLevel("generalized");
            break;
        case 6:
            cfg->decodeLevel("specialized");
            break;
        case 9:
            cfg->decodeLevel("generalized");
            cfg->recompressFlate();
            break;
        default:
            break;
        }
        cfg->checkConfiguration();
        j.run();
        std::ifstream in(infilename, std::ifstream::binary | std::ifstream::ate);
        std::ifstream out(outfilename, std::ifstream::binary | std::ifstream::ate);
        if (rate && in && out) {
            double in_size = static_cast<double>(in.tellg());
            double out_size = static_cast<double>(out.tellg());
            *rate = (out_size >= in_size) ? 0.0 : ((in_size - out_size) / in_size);
        }
        g_progress.store(100);
        return 0;
    } catch (std::exception const&) {
        if (rate) {
            *rate = -1.0;
        }
        return 1;
    }
}
