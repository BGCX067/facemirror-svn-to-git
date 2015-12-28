#ifndef __PACKET_H__
#define __PACKET_H__

#ifndef WIN32
#include <arpa/inet.h>
#else
#include <winsock2.h>
#endif
#include <string>
#include "utils.h"

using namespace std;
//using namespace FACE;

class Packet{
public:
    Packet(unsigned int pack_len);
    int push_1byte(char b);
    int push_2byte(short b);
    int push_4byte(int b);

    char  pop_1byte();
    short pop_2byte();
    int   pop_4byte();

    char* get_buf() const;
    const unsigned int get_len() const;
    
    int set_buf(char* data, unsigned int len);

    ~Packet();
    
private:
    Packet();
    char*        _buf;
    unsigned int _ofs;
    unsigned int _max_size;
    
};
#endif
