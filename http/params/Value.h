#include "HttpException.h"
#include "RefCount.h"
#ifndef VALUE_H
#define VALUE_H
class ValueBase : public RefCount {
    public:
    ValueBase() {}
    virtual ~ValueBase() { }
    virtual void output(std::ostream &os) = 0;
};

template<class T>
class Value : public ValueBase {
    private:
    T m_value;
    public:
    Value(T val) : m_value(val) { 
    }
    ~Value() { }
    T& value() { return m_value; }
    void output(std::ostream &os) {
        os << m_value;
    }
};
template<class T>
T& type_cast(ValueBase *object, T& d) {
    if (object == NULL) return d;
    Value<T> *obj = dynamic_cast<Value<T> *>(object);
    if (obj) return obj->value();
    return d;
}
ostream& operator<<(ostream &os, ValueBase &rhs);
#endif
