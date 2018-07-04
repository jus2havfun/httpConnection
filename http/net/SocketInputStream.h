#include "HttpException.h"
#ifndef SOCKETINPUTSTREAM_H
#define SOCKETINPUTSTREAM_H
class SocketInputStream : public InputStream {
    private:
        bool eof;
        AbstractPlainSocketImpl *impl;
        char *temp;
        Socket *socket;
        bool closing;
        static init();
        int socketRead(int fd, char *b, int off, int len);
    public:
        SocketInputStream(AbstractPlainSocketImpl *);
        int read(char *b, int blen, int len) throw (IOException) ;
        int read(char *b, int blen, int off, int len) throw (IOException) ;
        int read() throw (IOException) ;
        long skip(long numbytes) throw (IOException) ;
        int available() throw (IOException) ;
        void close() throw (IOException) ;
};
#endif
