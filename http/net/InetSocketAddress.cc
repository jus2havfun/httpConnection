#include "Integer.h"
#include "Stokenizer.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <netdb.h>
#ifndef INETSOCKETADDRESS_H
#include "InetSocketAddress.h"
#endif
#include "NameResolver.h"

ostream& operator<<(ostream &os, InetAddress &rhs) {
    os << "Hostname: " << rhs.gethostname() << ", ip address: " << rhs.getipaddr() << std::endl;
    return os;
}
InetAddress::InetAddress() { }
InetAddress::InetAddress(std::string src) : InetAddress(src.c_str()) { }
InetAddress::InetAddress(const char *str) {
    Stokenizer<std::string> st(str, (const char *)".");
    try {
        while (st.hasnext()) {
            std::string ip_octet = st.getnext();
            if (Integer::parseInt(ip_octet) > 0 && Integer::parseInt(ip_octet) < 255) continue;
            else { 
                throw NumberFormatException();
            }
        }
        std::string ip_octet = st.getnext();
        if (Integer::parseInt(ip_octet) > 0 && Integer::parseInt(ip_octet) < 255) ipaddr = str;
        else {
            hostname = str;
            vector<std::string> res;
            NameResolver::resolve(str, res);
            assert(res.size() > 0);
            ipaddr = res[0];
        }
    } catch (const NumberFormatException &ex) {
        hostname = str;
        vector<std::string> res;
        NameResolver::resolve(str, res);
        assert(res.size() > 0);
        ipaddr = res[0];
    }
}
InetAddress* InetAddress::getLocalHost() {
    char name[1024];
    memset(name, 0, 1024);
    if (::gethostname(name, 1023) != 0) {
        strcpy(name, "localhost");
    }
    InetAddress *r = new InetAddress(name);
    return r;
}

InetAddress* InetAddress::getByName(std::string host) throw (UnknownHostException) {
    vector<InetAddress *> res;
    InetAddress::getAllByName(host, res);
    InetAddress *r = res[0];
    for (int i = 1; i < res.size(); i++) delete res[i];
    return r;
}

void InetAddress::getAllByName(std::string host, vector<InetAddress *> &res) throw (UnknownHostException) {
    InetAddress::getAllByName(host, res, NULL);
}
void InetAddress::getAllByName(std::string host, vector<InetAddress *> &res, InetAddress* reqAddr) throw (UnknownHostException) {
    if (host == "") {
        res.push_back(new InetAddress("127.0.0.1"));
        return;
    }
    vector<std::string> ips;
    NameResolver::resolve(host.c_str(), ips);
    assert(ips.size() > 0);
    int i = 0;
    for (int i = 0; reqAddr && i < ips.size(); i++) {
        if (reqAddr->getipaddr() == ips[i]) break;
    }
    if (reqAddr && (i < ips.size()|| i == ips.size())) {
        res.push_back(reqAddr);
    } else res.push_back(new InetAddress(ips[0]));
}

int InetAddress::compareTo(InetAddress &rhs) {
    if (this == &rhs) return 0;
    if (hostname == rhs.hostname)
        if (ipaddr > rhs.ipaddr) return 1;
        else return -1;
    if (hostname > rhs.hostname) return 1;
        else return -1;
}
InetAddress::InetAddress(const InetAddress &rhs) {
    hostname = rhs.hostname;
    ipaddr = rhs.ipaddr;
}
InetAddress& InetAddress::operator=(const InetAddress &rhs) {
    if (this != &rhs) {
        hostname = rhs.hostname;
        ipaddr = rhs.ipaddr;
    }
    return *this;
}
InetSocketAddress::InetSocketAddress(std::string host, int port) : InetSocketAddress(host.c_str(), port) {
}
InetSocketAddress::InetSocketAddress(InetAddress *addr, int port) {
    if (addr == NULL) throw IllegalArgumentException("InetAddress cannot be NULL");
    bzero(&si_addr, sizeof(si_addr));
    si_addr.sin_family = AF_INET;
    si_addr.sin_port = htons(port);
    if (addr->gethostname() == "") {
        si_addr.sin_addr.s_addr = inet_addr(addr->getipaddr().c_str());
    } else {
        vector<std::string> res;
        NameResolver::resolve(addr->gethostname().c_str(), res);
        addr->setipaddr(res[0]);
        si_addr.sin_addr.s_addr = inet_addr(res[0].c_str());
    }

}
InetSocketAddress::InetSocketAddress(const char *host, int p) : hostname(host), port(htons(p)) {
    bzero(&si_addr, sizeof(si_addr));
    si_addr.sin_family = AF_INET;
    si_addr.sin_addr.s_addr = inet_addr(host);
    si_addr.sin_port = htons(port);
}
InetSocketAddress::InetSocketAddress() { }
InetSocketAddress::InetSocketAddress(const InetSocketAddress &rhs) {
    si_addr = rhs.si_addr;
    hostname = rhs.hostname;
    port = rhs.port;
}
InetSocketAddress& InetSocketAddress::operator=(const InetSocketAddress& rhs) {
    if (this != &rhs) {
        si_addr = rhs.si_addr;
        hostname = rhs.hostname;
        port = rhs.port;
    }
    return *this;
}
