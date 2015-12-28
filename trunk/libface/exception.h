#ifndef __EXCEPTION_H__
#define __EXCEPTION_H__

#include <string>


using namespace std;

// for operator [] in DB class. Throwed when field name is bad
class Exception :public std::exception
{
public:
    Exception(const Exception& e) throw():exception(e),
                                                          _what(e._what){}

    Exception * operator = (const Exception& rhs) throw()
        {
            _what = rhs._what;
            return this;
        }
    ~Exception() throw() {}

    virtual const char* what() const throw(){
        return _what.c_str();
    }
    Exception(const char* w = "") throw():_what(w){}
    Exception(const string& w) throw():_what(w){}
protected:
    
    string _what;
};

class NoSuchElementException:public Exception{};
class DuplicateElementException:public Exception{};

#endif
