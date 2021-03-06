#include "HttpException.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <netdb.h>
#include <pthread.h>
#include "NameResolver.h"
#include "InetSocketAddress.h"
#include "SocketImpl.h"
#include "OutputStream.h"
#include "InputStream.h"
#include "AbstractPlainSocketImpl.h"
#include "SocketInputStream.h"
#include "SocketOutputStream.h"
#include "DualStackPlainSocketImpl.h"
#ifndef SOCKET_H
#include "Socket.h"
#endif
Socket::Socket() { 
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&closeLock, &attr);
    setImpl();
}
Socket::~Socket() { 
    if (impl != NULL) impl->unref(); 
}
Socket::Socket(SocketImpl *i):shutOut(false), shutIn(false), bound(false), created(false), connected(false), closed(false), oldImpl(false)  {
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&closeLock, &attr);
    impl = i;
    if (impl != NULL) 
        impl->setSocket(this);
}

Socket::Socket(const char *host, int p) : 
    Socket(((host == NULL)?new InetSocketAddress("0.0.0.0", p):new InetSocketAddress(host, p)), NULL, true) {
    }
Socket::Socket(std::string &host, int port) :
    Socket(((host.length() != 0) ? new InetSocketAddress(host, port) : new InetSocketAddress("0.0.0.0", port)), NULL, true)  {
}
Socket::Socket(InetAddress *addr, int port) :
    Socket(((addr != NULL)? new InetSocketAddress(addr, port) : 0), NULL, true)  {
}
Socket::Socket(std::string &host, int port, InetAddress *localAddr, int localPort) :
    Socket(((host.length() != 0)?new InetSocketAddress(host, port) : new InetSocketAddress("0.0.0.0", port)), new InetSocketAddress(localAddr, localPort), true)  {
}
Socket::Socket(InetAddress* addr, int port, InetAddress *localAddr, int localPort) :
    Socket(((addr != NULL)? new InetSocketAddress(addr, port) : 0), new InetSocketAddress(localAddr, localPort), true) {
}
Socket::Socket(std::string &host, int port, bool stream) :
    Socket(((host.length() != 0)?new InetSocketAddress(host, port):new InetSocketAddress("0.0.0.0", port)), NULL, stream) {
}
Socket::Socket(InetAddress* addr, int port, bool stream) :
    Socket(((addr != NULL)? new InetSocketAddress(addr, port):0), NULL, stream) {
}

Socket::Socket(SocketAddress *addr, SocketAddress *local, bool s):shutOut(false), shutIn(false), bound(false), created(false), connected(false), closed(false), oldImpl(false) {
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&closeLock, &attr);
    setImpl();
    if (addr == NULL) throw NullPointerException();
    try {
        createImpl(s);
        if (local != NULL) bind(local);
        connect(addr);
        addr->unref();
        if (local) local->unref();
    } catch (const IOException &ex) {
        if (addr != NULL) addr->unref();
        if (local != NULL) local->unref();
        try {
            close();
        } catch (const IOException &ex) {
        }
        throw ex;
    }
}

void Socket::bind(SocketAddress *bindpoint) {
    if (isClosed()) throw SocketException("Socket is closed");
    if (isBound()) throw SocketException("Already bound");
    if (bindpoint == NULL) IllegalArgumentException("Unsupported address type");
    InetSocketAddress *epoint = dynamic_cast<InetSocketAddress *>(bindpoint);
    if (epoint != NULL && epoint->isUnresolved()) throw SocketException("Unresolved address");
    if (epoint == NULL) epoint = new InetSocketAddress("0.0.0.0", 0);
    InetAddress *addr = epoint->getAddress();
    int p = epoint->getPort();
    getImpl()->bind(addr, p);
    bound = true;
}
void Socket::createImpl(bool stream) {
    if (impl == NULL) setImpl();
    try {
        impl->create(stream);
        created = true;
    } catch (const IOException &e) {
        throw SocketException(e.what());
    }
}

void Socket::setImpl() {
    impl = new DualStackPlainSocketImpl(true);
    if (impl != NULL) impl->setSocket(this);
}
SocketImpl *Socket::getImpl() {
    if (!created) createImpl(true);
    return impl;
}
void Socket::connect(SocketAddress *endpoints) {
    connect(endpoints, 0);
}
void Socket::connect(SocketAddress *endpoint, int timeout) {
    if (endpoint == NULL) throw IllegalArgumentException("connect: The address can't be null");
    if (timeout < 0) IllegalArgumentException("connect: timeout can't be negative");
    if (isClosed()) throw SocketException("Socket is closed");
    if (isConnected()) throw SocketException("already connected");
    InetAddress *addr = endpoint->getAddress();
    int p = endpoint->getPort();
    if (!created) createImpl(true);
    try {
        if (!oldImpl) impl->connect(endpoint, timeout);
        else if (timeout == 0) {
            impl->connect(addr, p);
        } else throw UnsupportedOperationException("SocketImpl.connect(addr, timeout)");
        if(addr) addr->unref();
        connected = true;
        bound = true;
    } catch (...) {
        if (addr) addr->unref();
        throw;
    }
}

void Socket::postAccept() {
    connected = true;
    created = true;
    bound = true;
}
void Socket::setCreated() { created = true; }
void Socket::setBound() { bound = true; }
void Socket::setConnected() { connected = true; }
void Socket::setSoLinger(bool on, int linger) throw (SocketException) {
    if (isClosed()) throw SocketException("Socket is closed");
    int nativefd = getImpl()->getFileDescriptor();
    if (nativefd != -1) {
        struct linger so_linger;
        so_linger.l_onoff = on;
        so_linger.l_linger = linger;
        if (setsockopt(nativefd, SOL_SOCKET, SO_LINGER, &so_linger, sizeof(so_linger)) < 0) {
            throw SocketException("setOption(SO_LINGER, %d, %d) failed", on, linger);
        }
    }
}

int Socket::getSoLinger() throw (SocketException) {
    if (isClosed()) throw SocketException("Socket is closed");
    return -1;

}
void Socket::setSoTimeout(int timeout) throw (SocketException) {
    if (isClosed()) throw SocketException("Socket is closed");
    getImpl()->setSoTimeout(timeout);
}

int Socket::getSoTimeout() throw (SocketException) {
    if (isClosed()) throw SocketException("Socket is closed");
    int t = -1;
    t = getImpl()->getSoTimeout();
    return t;
}

void Socket::setSendBufferSize(int size) throw (SocketException) {
    if (isClosed()) throw SocketException("Socket is closed");
    getImpl()->setOption(SO_SNDBUF, size);
}

int Socket::getSendBufferSize() throw (SocketException) {
    if (isClosed()) throw SocketException("Socket is closed");
    int t = -1;
    t = getImpl()->getOption(SO_RCVTIMEO);
    return t;
}

void Socket::setReceiveBufferSize(int size)throw (SocketException) {
    if (isClosed()) throw SocketException("Socket is closed");
    getImpl()->setOption(SO_RCVBUF, size);
}

int Socket::getReceiveBufferSize() throw (SocketException) {
    if (isClosed()) throw SocketException("Socket is closed");
    int t = -1;
    t = getImpl()->getOption(SO_RCVBUF);
    return t;
}

void Socket::setKeepAlive(bool on) throw (SocketException) {
    if (isClosed()) throw SocketException("Socket is closed");
    getImpl()->setOption(SO_KEEPALIVE, on);
}

bool Socket::getKeepAlive() throw (SocketException) {
    if (isClosed()) throw SocketException("Socket is closed");
    int t = -1;
    t = getImpl()->getOption(SO_KEEPALIVE);
    return t;
}

void Socket::setReuseAddress(bool on) throw (SocketException) {
    if (isClosed()) throw SocketException("Socket is closed");
    getImpl()->setOption(SO_REUSEADDR, on);
}
bool Socket::getReuseAddress() throw (SocketException) {
    if (isClosed()) throw SocketException("Socket is closed");
    int t = -1;
    t = getImpl()->getOption(SO_REUSEADDR);
    return t;
}
int Socket::getPort() {
    if (!isConnected()) return 0;
    try {
        return getImpl()->getPort();
    } catch (const SocketException &e) {
    }
    return -1;
}
InetAddress* Socket:: getLocalAddress() {
    if (!isBound()) return InetAddress::anyLocalAddress();
    InetAddress* in = NULL;
    try {
        int nativefd = getImpl()->getFileDescriptor();
        if (nativefd > 0) {
            struct sockaddr_in localAddress;
            socklen_t addressLength = sizeof(localAddress);
            getsockname(nativefd, (struct sockaddr*)&localAddress, &addressLength);
            char hostnamestr[128];
            sprintf(hostnamestr, "%s", inet_ntoa( localAddress.sin_addr));
            in = new InetAddress(hostnamestr);
        } else throw IOException();
    } catch (...) {
        in = InetAddress::anyLocalAddress();
    }
    return in;
}
InetAddress *Socket::getInetAddress() {
    if (!isConnected()) return NULL;
    try {
        return getImpl()->getInetAddress();
    } catch (const SocketException &e) { }
    return NULL;
}
int Socket::getLocalPort() {
    if (!isBound()) return -1;
    try {
        return getImpl()->getLocalPort();
    } catch (const SocketException &e) { }
    return -1;
}
SocketAddress *Socket::getRemoteSocketAddress() {
    if (!isConnected()) return NULL;
    return new InetSocketAddress(getInetAddress(), getPort());
}
SocketAddress *Socket::getLocalSocketAddress() {
    if (!isBound()) return NULL;
    InetAddress *tempAddr = getLocalAddress();
    SocketAddress *sockAddr = new InetSocketAddress(tempAddr, getLocalPort());
    tempAddr->unref();
    return sockAddr;
}
InputStream* Socket::getInputStream() throw (IOException) {
    if (isClosed()) throw SocketException("Socket is closed");
    if (!isConnected()) throw SocketException("Socket is not connected");
    if (isInputShutdown()) throw SocketException("Socket input is shutdown");
    Socket *s = this;
    InputStream *in = NULL;
    in = impl->getInputStream();
    return in;
}
OutputStream* Socket::getOutputStream() throw (IOException){
    if (isClosed()) throw SocketException("Socket is closed");
    if (!isConnected()) throw SocketException("Socket is not connected");
    if (isOutputShutdown()) throw SocketException("Socket output is shutdown");
    Socket *s = this;
    OutputStream *on = NULL;
    on = impl->getOutputStream();
    return on;
}

void Socket::close() throw (IOException) {
    {
        Lock l(closeLock);
        if (isClosed()) return;
        if (created) impl->close();
        closed = true;
    }
}

void Socket::shutdownInput() throw (IOException) {
    if (isClosed()) throw SocketException("Socket is closed");
    if (!isConnected()) throw SocketException("Socket is not connected");
    if (isInputShutdown()) throw SocketException("Socket input is already shutdown");
    getImpl()->shutdownInput();
    shutIn = true;
}

void Socket::shutdownOutput() throw (IOException) {
    if (isClosed()) throw SocketException("Socket is closed");
    if (!isConnected()) throw SocketException("Socket is not connected");
    if (isOutputShutdown()) throw SocketException("Socket output is already shutdown");
    getImpl()->shutdownOutput();
    shutOut = true;
}

std::string Socket::toString() {
    try {
        if (isConnected()) {
            stringstream ss;
            ss << "Socket[addr=" << getImpl()->getInetAddress() << ",port=" << getImpl()->getPort() << ",localport=" << getImpl()->getLocalPort() << "]";
            return ss.str();
        }
    } catch (const SocketException &e) {
    }
    return "Socket[unconnected]";
}

bool Socket::isClosed() {
    Lock l(closeLock);
    return closed;
}

bool Socket::isInputShutdown() { return shutIn; }
bool Socket::isOutputShutdown() { return shutOut; }
bool Socket::isBound() { return bound || oldImpl; }
bool Socket::isConnected() { return connected || oldImpl; }
