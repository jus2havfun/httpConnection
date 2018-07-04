#include "HttpException.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <netdb.h>
#ifndef NAMERESOLVER_H
#include "NameResolver.h"
#endif
void resolve(std::string &str, vector<std::string> &v) {
    NameResolver::resolve(str.c_str(), v);
}
void NameResolver::resolve(const char *str, vector<std::string> &v) {
    if (str == NULL) return;
    struct hostent *h = gethostbyname(str);
    if (h) {
        for (int i = 0; (i < h->h_length && h->h_addr_list[i]); i++) {
            char *s = h->h_addr_list[i];
            stringstream ss;
            ss << (int)(s[0] &0xff) << "." << ((int)(s[1]&0xff)) << "." << ((int)(s[2]&0xff)) << "." << ((int)(s[3]&0xff));
            v.push_back(ss.str());
        }
    }
    return;
}