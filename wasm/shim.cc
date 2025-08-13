#include <qpdf/Constants.h>
#include <qpdf/Pl_Flate.hh>
#include <qpdf/QPDF.hh>
#include <qpdf/QPDFWriter.hh>
#include <exception>
#include <atomic>
#include <fstream>
#include <memory>

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
    double* rate)
{
    try {
        g_progress.store(0);
        QPDF pdf;
        pdf.processFile(infilename);
        QPDFWriter w(pdf, outfilename);
        w.setObjectStreamMode(qpdf_o_generate);
        w.setStreamDataMode(qpdf_s_compress);
        w.setRecompressFlate(true);
        Pl_Flate::setCompressionLevel(level);
        w.registerProgressReporter(std::make_shared<QPDFWriter::FunctionProgressReporter>(
            [](int p) { g_progress.store(p); }));
        w.write();
        std::ifstream in(infilename, std::ifstream::binary | std::ifstream::ate);
        std::ifstream out(outfilename, std::ifstream::binary | std::ifstream::ate);
        if (rate && in && out) {
            double in_size = static_cast<double>(in.tellg());
            double out_size = static_cast<double>(out.tellg());
            *rate = (in_size > 0) ? (out_size / in_size) : 0.0;
        }
        g_progress.store(100);
        return 0;
    } catch (std::exception& e) {
        return 1;
    }
}
