#include <qpdf/Constants.h>
#include <qpdf/QPDF.hh>
#include <qpdf/QPDFWriter.hh>
#include <qpdf/Buffer.hh>
#include <zlib.h>
#include <array>
#include <cmath>
#include <exception>
#include <vector>
#include <string>

static double
shannon_entropy(unsigned char const* data, size_t len)
{
    if (len == 0) {
        return 0.0;
    }
    std::array<size_t, 256> counts = {0};
    for (size_t i = 0; i < len; ++i) {
        ++counts[data[i]];
    }
    double ent = 0.0;
    double f = static_cast<double>(len);
    for (auto c: counts) {
        if (c) {
            double p = c / f;
            ent -= p * std::log2(p);
        }
    }
    return ent;
}

static void
compress_stream(QPDFObjectHandle& oh, std::shared_ptr<Buffer> buf, int level)
{
    size_t inlen = buf->getSize();
    unsigned char const* in = buf->getBuffer();
    uLongf outlen = compressBound(inlen);
    std::string out;
    out.resize(outlen);
    if (compress2(reinterpret_cast<unsigned char*>(out.data()), &outlen, in, inlen, level) !=
        Z_OK) {
        throw std::runtime_error("compression failed");
    }
    out.resize(outlen);
    oh.replaceStreamData(
        out, QPDFObjectHandle::newName("/FlateDecode"), QPDFObjectHandle::newNull());
}

extern "C" int
qpdf_wasm_compress(char const* infilename, char const* outfilename, char const* level)
{
    try {
        QPDF pdf;
        pdf.processFile(infilename);

        int lvl = 6; // balanced
        std::string l = level ? level : "";
        if (l == "fast") {
            lvl = 1;
        } else if (l == "smallest") {
            lvl = 9;
        }

        for (auto& oh: pdf.getAllObjects()) {
            if (!oh.isStream()) {
                continue;
            }

            bool skip = false;
            auto dict = oh.getDict();
            auto f = dict.getKey("/Filter");
            std::vector<std::string> filters;
            if (f.isName()) {
                filters.push_back(f.getName());
            } else if (f.isArray()) {
                for (auto& item: f.getArrayAsVector()) {
                    if (item.isName()) {
                        filters.push_back(item.getName());
                    }
                }
            }

            for (auto const& name: filters) {
                if (name == "/DCTDecode" || name == "/JPXDecode" || name == "/JBIG2Decode" ||
                    name == "/CCITTFaxDecode") {
                    skip = true;
                    break;
                }
            }

            if (skip) {
                continue;
            }

            if (filters.empty()) {
                auto buf = oh.getStreamData(qpdf_dl_none);
                if (shannon_entropy(buf->getBuffer(), buf->getSize()) > 7.9) {
                    continue;
                }
                compress_stream(oh, buf, lvl);
            } else {
                auto buf = oh.getStreamData(qpdf_dl_specialized);
                compress_stream(oh, buf, lvl);
            }
        }

        QPDFWriter w(pdf, outfilename);
        w.setObjectStreamMode(qpdf_o_generate);
        w.setCompressStreams(false);
        w.setRecompressFlate(false);
        w.write();
        return 0;
    } catch (std::exception& e) {
        return 1;
    }
}
