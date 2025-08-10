#include <qpdf/QPDF.hh>
#include <qpdf/QPDFWriter.hh>
#include <exception>

extern "C" int qpdf_wasm_compress(char const* infilename, char const* outfilename)
{
    try {
        QPDF pdf;
        pdf.processFile(infilename);
        QPDFWriter w(pdf, outfilename);
        w.setCompressStreams(true);
        w.write();
        return 0;
    } catch (std::exception& e) {
        return 1;
    }
}
