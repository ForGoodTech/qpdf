#include <qpdf/Constants.h>
#include <qpdf/QPDF.hh>
#include <qpdf/QPDFWriter.hh>
#include <exception>

extern "C" int
qpdf_wasm_compress(char const* infilename, char const* outfilename)
{
    try {
        QPDF pdf;
        pdf.processFile(infilename);
        QPDFWriter w(pdf, outfilename);
        w.setObjectStreamMode(qpdf_o_generate);
        w.setStreamDataMode(qpdf_s_compress);
        w.setRecompressFlate(true);
        w.write();
        return 0;
    } catch (std::exception& e) {
        return 1;
    }
}
