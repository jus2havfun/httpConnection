#include "HttpException.h"
#ifndef BASICLINEFORMATTER_H
#define BASICLINEFORMATTER_H
class BasicLineFormatter : public LineFormatter {
    protected:
        CharArrayBuffer& initBuffer(CharArrayBuffer&);
        int estimateProtocolVersionLen(ProtocolVersion*);
        void doFormatRequestLine(CharArrayBuffer&, RequestLine*);
        void doFormatStatusLine(CharArrayBuffer&, StatusLine*);
        void doFormatHeader(CharArrayBuffer&, Header*);
    public:
        ~BasicLineFormatter()  { }
        static BasicLineFormatter DEFAULT;
        std::string formatProtocolVersion(ProtocolVersion *, LineFormatter*);
        CharArrayBuffer appendProtocolVersion(CharArrayBuffer&, ProtocolVersion*);
        std::string formatRequestLine(RequestLine *,LineFormatter*);
        CharArrayBuffer formatRequestLine(CharArrayBuffer&, RequestLine *);
        std::string formatStatusLine(StatusLine*, LineFormatter*);
        CharArrayBuffer formatStatusLine(CharArrayBuffer&, StatusLine*);
        std::string formatHeader(Header*, LineFormatter*);
        CharArrayBuffer formatHeader(CharArrayBuffer&, Header*);
};
#endif
