#include "packet.h"

using namespace FACE;

Packet::Packet(unsigned int pack_len){
    _ofs = 0;
    _buf = (char*)malloc(pack_len+1);
    bzero(_buf, pack_len+1);
    _max_size = pack_len;
}

int Packet::push_1byte(char b){
    memcpy(_buf+_ofs, &b, 1);
    _ofs++;
    if(_ofs > _max_size)
        p(INF, "WARNING: buffer overflow\n");
    return _ofs;
}

int Packet::push_2byte(short b){
    short t = htons(b);
    memcpy(_buf+_ofs, &t, 2);
    _ofs += 2;

    if(_ofs > _max_size)
        p(INF, "WARNING: buffer overflow\n");
    return _ofs;
}

int Packet::push_4byte(int b){
    int t = htonl(b);
    memcpy(_buf+_ofs, &t, 4);
    _ofs += 4;

    if(_ofs > _max_size)
        p(INF, "WARNING: buffer overflow\n");
    return _ofs;
}

char Packet::pop_1byte(){
    char c;
    memcpy(&c, _buf+_ofs, 1);
    _ofs++;

    if(_ofs > _max_size)
        p(INF, "WARNING: buffer over poped\n");
    return c;
}

short Packet::pop_2byte(){
    short c;
    memcpy(&c, _buf+_ofs, 2);
    _ofs += 2;

    if(_ofs > _max_size)
        p(INF, "WARNING: buffer over poped\n");
    return ntohs(c);
}

int Packet::pop_4byte(){
    int c;
    memcpy(&c, _buf+_ofs, 4);
    _ofs += 4;

    if(_ofs > _max_size)
        p(INF, "WARNING: buffer over poped\n");

    return ntohl(c);
}

char* Packet::get_buf() const{
    return _buf;
}

const unsigned int Packet::get_len() const{
    return _ofs;
}

int Packet::set_buf(char* buf, unsigned int len){
    if(len > _max_size)
        return -1;// buffer overflow

    memcpy(_buf, buf, len);
    
    return 0;
}
    

Packet::~Packet(){
    free(_buf);
}
