#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <netdb.h>
#include "HttpException.h"
#include "NameResolver.h"
#include "InetSocketAddress.h"
#include "SocketImpl.h"
#include "InputStream.h"
#include "OutputStream.h"
#include "AbstractPlainSocketImpl.h"
#include "SocketInputStream.h"
#include "SocketOutputStream.h"
#include "URI.h"
#include "URL.h"
#include "URLStreamHandler.h"
#include "SocketImplFactory.h"
#include "Socket.h"
#include "ServerSocket.h"

int main() {
    Socket s("10.1.40.60", 65500);
    return 0;
}
