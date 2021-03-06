#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <netdb.h>
#include "HttpException.h"
#include "InetSocketAddress.h"
#include "SocketImpl.h"
#include "NameResolver.h"
#include "InputStream.h"
#include "OutputStream.h"
#include "SocketInputStream.h"
#include "SocketOutputStream.h"
#ifndef ABSTRACTPLAINSOCKETIMPL_H
#include "AbstractPlainSocketImpl.h"
#endif
#include "Socket.h"
#include "SocketImplFactory.h"
#include "ServerSocket.h"

int AbstractPlainSocketImpl::SHUT_RD = 0;
int AbstractPlainSocketImpl::SHUT_WR = 1;
void AbstractPlainSocketImpl::create(bool stream) {
    socketCreate(stream);
    if (socket != NULL) socket->setCreated();
    if (serverSocket != NULL) serverSocket->setCreated();
}

void AbstractPlainSocketImpl::connect(std::string &host, int p) throw (IOException) {
    connect(host.c_str(), p);
}

void AbstractPlainSocketImpl::connect(const char* host, int p) throw (IOException) {
    bool connected = false;
    try {
        InetAddress *addr = InetAddress::getByName(host);
        SocketImpl::setPort(p);
        SocketImpl::setAddress(addr);
        connectToAddress(addr, p, timeout);
        if (addr != NULL) addr->unref();
        connected = true;
    } catch (...) {
        if (!connected) {
            try {
                close();
            } catch (const IOException &ex) { }
        }
    }
    if (!connected) {
        try {
            close();
        } catch (const IOException &ex) { }
    }
}

void AbstractPlainSocketImpl::connect(InetAddress* addr, int p) throw (IOException) {
    SocketImpl::setPort(p);
    SocketImpl::setAddress(addr);
    try {
        connectToAddress(addr, p, timeout);
        return;
    } catch (const IOException &ex) {
        close();
        throw;
    }
}

void AbstractPlainSocketImpl::connect(SocketAddress *addr, int p) throw (IOException) {
    bool  connected = false;
    try {
        addr = dynamic_cast<InetSocketAddress *>(addr);
        if (addr == NULL) throw IllegalArgumentException("unsupported address type");
        if (addr->isUnresolved()) throw UnknownHostException(addr->getHostName());
        InetAddress *inetaddr = addr->getAddress();
        SocketImpl::setPort(addr->getPort());
        SocketImpl::setAddress(inetaddr);
        connectToAddress(inetaddr, addr->getPort(), p);
        if (inetaddr) inetaddr->unref();
        connected = true;
    } catch (...) { }
    if (!connected) {
        try {
            close();
        } catch (const IOException &e) {
        }
    }
}

void AbstractPlainSocketImpl::connectToAddress(InetAddress* addr, int p, int t) throw (IOException) {
    try {
        if (addr->isAnyLocalAddress()) {
            InetAddress *r = InetAddress::getLocalHost();
            doConnect(r, p, t);
            r->unref();
        } else doConnect(addr, p, t);
    } catch (const IOException &ex) {
        std::cerr << ex.what() <<  addr->toString() << std::endl;
        addr->unref();
        throw;
    } catch (...) {
        std::cerr << "Unknown error when connecting to: " <<  addr->toString() << std::endl;
        addr->unref();
        throw;
    }
}

void AbstractPlainSocketImpl::doConnect(InetAddress* addr, int p, int timeout) throw (IOException) {
    {
        Lock l(fdLock);
        try {
            acquireFD();
            try {
                socketConnect(addr, p, timeout);
                if (closePending) throw SocketException ("Socket closed");
                if (socket != NULL) {
                    socket->setConnected();
                    socket->setBound();
                }
            } catch (...) {
                releaseFD();
                throw;
            }
            releaseFD();
        } catch (const IOException &e) {
            close();
            throw;
        }
    }
}

void AbstractPlainSocketImpl::bind(InetAddress *host, int p)  throw (IOException) {
    Lock l(fdLock);
    socketBind(host, p);
    if (socket != NULL) socket->setBound();
    if (serverSocket != NULL) serverSocket->setBound();
    host->unref();
}

void AbstractPlainSocketImpl::listen(int baclog) throw (IOException) {
    socketListen(baclog);
}

void AbstractPlainSocketImpl::accept(SocketImpl *s) throw (IOException) {
    acquireFD();
    try {
        socketAccept(s);
    } catch(...) {
        releaseFD();
        throw;
    }
    releaseFD();
}

InputStream *AbstractPlainSocketImpl::getInputStream() throw (IOException) {
    Lock l(fdLock);
    if (isClosedOrPending()) throw IOException("Socket Closed");
    if (shut_rd) throw IOException("Socket input is shutdown");
    if (socketInputStream == NULL) socketInputStream = new SocketInputStream(this);
    return socketInputStream;
}



OutputStream *AbstractPlainSocketImpl::getOutputStream() throw (IOException) {
    Lock l(fdLock);
    if (isClosedOrPending()) throw IOException("Socket Closed");
    if (shut_wr) throw IOException("Socket output is shutdown");
    if (socketOutputStream == NULL) socketOutputStream = new SocketOutputStream(this);
    return socketOutputStream;
}

int AbstractPlainSocketImpl::available()  throw (IOException) {
    int n = 0;
    {
        Lock l(fdLock);
        if (isClosedOrPending()) throw IOException("Stream closed.");
        if (isConnectionReset() || shut_rd) return 0;
        try {
            n = socketAvailable();
            if (n == 0 && isConnectionResetPending()) setConnectionReset();
        } catch (const ConnectionResetException& ex) {
            setConnectionResetPending();
            try {
                n = socketAvailable();
                if (n == 0) setConnectionReset();
            } catch (const ConnectionResetException& e) {
            }
        }
    }
    return n;
}

void AbstractPlainSocketImpl::close()  throw (IOException) {
    Lock l(fdLock);
    if (fd != -1) {
        if (fdUseCount == 0) {
            if (closePending) return;
            closePending = true;
            try {
                //socketPreClose();
                socketClose();
            } catch(...) {
                throw;
            }
            socketClose();
            fd = -1;
            return;
        } else {
            if (!closePending) {
                closePending = true;
                fdUseCount--;
                //socketPreClose();
            }
        }
    }
}

void AbstractPlainSocketImpl::socketClose() {
    if (fd > 0) ::close(fd);
    fd = -1;
}

void AbstractPlainSocketImpl::reset() throw (IOException) {
    if (fd != -1) socketClose();
    fd = -1;
}

AbstractPlainSocketImpl::~AbstractPlainSocketImpl() {
    if (socketInputStream) socketInputStream->unref();
    if (socketOutputStream) socketOutputStream->unref();
}

AbstractPlainSocketImpl::AbstractPlainSocketImpl() {
    socketInputStream = NULL;
    socketOutputStream = NULL;
    so_timeout = -1;
    fdUseCount = 0;
    closePending = false;
    shut_rd = false;
    shut_wr = false;
    CONNECTION_NOT_RESET = 0;
    CONNECTION_RESET_PENDING = 1;
    CONNECTION_RESET = 2;
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&fdLock, &attr);
    pthread_mutex_init(&resetLock, &attr);
}

void AbstractPlainSocketImpl::shutdownInput() {
    if (fd != -1) {
        socketShutdown(SHUT_RD);
        SocketInputStream *sis = dynamic_cast<SocketInputStream *> (socketInputStream);
        if (sis != NULL) sis->setEOF(true);
        shut_rd = true;
    }
}

void AbstractPlainSocketImpl::shutdownOutput() {
    if (fd != -1) {
        socketShutdown(SHUT_WR);
        shut_wr = true;
    }
}

void AbstractPlainSocketImpl::setOption(int opt, int val) {
    int rc = setsockopt(fd, SOL_SOCKET, opt, &val, sizeof(val));
    if (rc == 0) return;
    throw SocketException("setOption(%d, %d) failed", opt, val);
}

void AbstractPlainSocketImpl::setSoTimeout(int tout) {
    so_timeout = tout;
}

int AbstractPlainSocketImpl::getSoTimeout() {
    return so_timeout;
}

int AbstractPlainSocketImpl::getOption(int opt) {
    int option_value = -1;
    socklen_t option_len;
    int rc = getsockopt(fd, SOL_SOCKET, opt, (char *)&option_value, &option_len);
    return option_value;
}

void AbstractPlainSocketImpl::sendUrgentData (int data) throw (IOException) {
    if (fd == -1) throw IOException("Socket Closed");
    socketSendUrgentData (data);
}

int AbstractPlainSocketImpl::acquireFD() {
    Lock l(fdLock);
    fdUseCount++;
    return fd;
}

void AbstractPlainSocketImpl::releaseFD() {
    Lock l(fdLock);
    fdUseCount--;
    if (fdUseCount == -1) {
        if (fd != -1) {
            try {
                socketClose();
            } catch (const IOException e) {
            } catch (...) {
            }
            fd = -1;
        }
    }
}

int AbstractPlainSocketImpl::getTimeout() {
    return timeout;
}

bool AbstractPlainSocketImpl::isConnectionReset() {
    Lock l(resetLock);
    return (resetState == CONNECTION_RESET);
}

bool AbstractPlainSocketImpl::isConnectionResetPending() {
    Lock l(resetLock);
    return (resetState == CONNECTION_RESET_PENDING);
}

void AbstractPlainSocketImpl::setConnectionReset() {
    Lock l(resetLock);
    resetState = CONNECTION_RESET;
}
void AbstractPlainSocketImpl::setConnectionResetPending() {
    Lock l(resetLock);
    if (resetState == CONNECTION_NOT_RESET) resetState = CONNECTION_RESET_PENDING;
}

bool AbstractPlainSocketImpl::isClosedOrPending() {
    Lock l(fdLock);
    if (closePending || (fd == -1)) return true;
    else return false;
}
