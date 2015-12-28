#ifndef __OBJECT_H__
#define __OBJECT_H__

#include <string>
#include <map>
#include "exception.h"
#include "utils.h"

using namespace std;
using namespace FACE;

typedef unsigned int index_t;

// very base class for all, contains:
// string name;
// unsigned int idx;
// for index search
class Object{
public:
    index_t get_idx() const{return m_idx;}
    string  get_name() const{return m_name;}

    void set_name(const string& name){m_name = name;}
    void set_name(const char* name){m_name = string(name);}

    void set_idx(index_t idx){m_idx = idx;}

    Object():m_idx(counter++){}
    static unsigned int counter;
protected:

    index_t m_idx;
    string  m_name;
    
    
};

template<typename T>
class ObjectManager:public Object{
public:
    T find_by_name(const string& name);
    T find_by_idx(const index_t idx);

    int add(T& element);
    const int size() const;

    // NOTE: remove element will not destroy the element object,
    //       has to explicitly delete the element.
    int remove(T& element);
    int remove_by_name(const string& name);
    int remove_by_idx(const index_t idx);
        
    ObjectManager();
    ~ObjectManager();
    map<string, T>  index_name;
    map<index_t, T> index_idx;
    
//    typedef map<index_t, T>::iterator it_t;
#define FACE_CYCLE_INDEX(type,container) for(map<index_t, type>::iterator it = container->index_idx.begin(); \
                                               it != container->index_idx.end(); \
                                               it++)

private:

    
};

template<typename T>
ObjectManager<T>::ObjectManager(){
    char buf[32];
    bzero(buf, sizeof(buf));
    snprintf(buf, sizeof(buf),
             "%d", m_idx);

    m_name = string(buf);

}

template<typename T>
ObjectManager<T>::~ObjectManager(){
    index_name.erase(index_name.begin(), index_name.end());
    index_idx.erase(index_idx.begin(), index_idx.end());
}
    
template<typename T>
T ObjectManager<T>::find_by_name(const string& name){
    if(0 == index_name.count(name)){
        p(ERR, "Unable to find the name %s.\n", name.c_str());
        throw NoSuchElementException();
    }
    if(1 < index_name.count(name)){
        p(ERR, "Duplicated name %s.\n", name.c_str());
        throw NoSuchElementException();
    }

    return index_name[name];
}

template<typename T>
T ObjectManager<T>::find_by_idx(const index_t idx){
    if(0 == index_idx.count(idx)){
        p(ERR, "Unable to find the idx %d.\n", idx);
        throw NoSuchElementException();
    }
    if(1 < index_idx.count(idx)){
        p(ERR, "Duplicated idx %d.\n", idx);
        throw NoSuchElementException();
    }

    return index_idx[idx];
}

template<typename T>
int ObjectManager<T>::add(T& element){
    if(0 != index_idx.count(element->get_idx()))
        throw DuplicateElementException();
    if(0 != index_name.count(element->get_name()))
        throw DuplicateElementException();
    
    index_name.insert(typename map<string, T>::value_type(element->get_name(),
                                                          element));
    index_idx.insert(typename map<index_t, T>::value_type(element->get_idx(),
                                                          element));
    return 0;
}

template<typename T>
const int ObjectManager<T>::size() const{
    return index_name.size();
}

template<typename T>
int ObjectManager<T>::remove(T& element){
    if(1 != index_idx.count(element->get_idx()))
        throw Exception();
    
    index_name.erase(element->get_name());
    index_idx.erase(element->get_idx());
    return 0;
}


template<typename T>
int ObjectManager<T>::remove_by_name(const string& name){
    if(1 != index_name.count(name))
        throw DuplicateElementException();

    T tmp = index_name[name];
    
    index_name.erase(name);
    index_idx.erase(tmp->get_idx());
    return 0;
}

template<typename T>
int ObjectManager<T>::remove_by_idx(const index_t idx){
    if(1 != index_idx.count(idx))
        throw DuplicateElementException();

    T tmp = index_idx[idx];
    
    index_name.erase(tmp->get_name());
    index_idx.erase(idx);
    return 0;
}


#endif

