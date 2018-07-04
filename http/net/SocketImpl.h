#ifndef "HttpException.h"
#ifndef SOCKETIMPL_H
#define SOCKETIMPL_H
class SocketImpl {
    private:
    protected:
        Socket *socket;
        ServerSocket *serverSocket;
        int port;
        int localport;
        int fd;
        InetAddress *address;
        virtual void create(bool stream) = 0;
        virtual void connect(std::string host, int port) = 0
        virtual void bind(InetAddress *host, int port) = 0;
        virtual void listen(int baclog) = 0;
        virtual void accept(SocketImpl *s) = 0;
        virtual InputStream *getInputStream() = 0;
        virtual OutputStream *getOutputStream() = 0;
        virtual int available() = 0;
        virtual void close() = 0;
        virtual void sendUrgentData (int data) = 0;
    public:
        void setSocket(Socket &s);
        virtual ~SocketImpl() { }
        virtual void shutdownInput() = 0;
        virtual void shutdownOutput() = 0;
        virtual int getFileDescriptor() = 0;
        virtual int getPort() = 0;
        virtual InetAddress *getInetAddress() = 0;
        virtual int getLocalPort() = 0;
        virtual void setOption(int opt, int val) = 0;
        virtual int getOption(int opt) = 0;
};
#endif
