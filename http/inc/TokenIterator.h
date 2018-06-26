#include "HttpException.h"
#ifndef TOKENITERATOR_H
#define TOKENITERATOR_H
class TokenIterator {
    public:
    virtual ~TokenIterator() { }
    virtual bool hasNext() = 0;
    virtual std::string nextToken() = 0;
};
#endif
