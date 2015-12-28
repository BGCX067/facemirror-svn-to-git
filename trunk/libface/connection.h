#ifndef __CONNECTION_H__
#define __CONNECTION_H__


#include <list>
#include <string>
#include "packet.h"
#include "object.h"
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/rand.h>

#ifndef WIN32
#include <unistd.h> // for usleep()
#include <fcntl.h>
#include <iostream>

#define SOCKET   int
#include <arpa/inet.h>
#include </usr/local/include/event.h>
#else

#include "./event.h"
//#define errno WSAGetLastError()
#define close(a) closesocket(a)
#define write(a, b, c) send(a, b, c, 0)
#define read(a, b, c) recv(a, b, c, 0)
#endif

using namespace std;

namespace FACE{
    struct buffered_queue{
        char *buf;
        int   len;
        int   offset;
    
        public:
        buffered_queue(char *b, const unsigned int l):len(l),offset(0){
            buf = (char*)malloc(len+1);
            bzero(buf, len+1);
            memcpy(buf, b, len);
        };
        ~buffered_queue(){free(buf);}
        private:// prevent init without proper constructor
        buffered_queue();
    };

    class Connection:public Object{
    public:
        Connection();
        ~Connection();
        
        const string get_ip() const;
        const unsigned short get_port() const;
        const bool available() const;

        Connection* connect_to(const char* ip, unsigned short port);

        // send
        int push_out(Packet* p);

        // connection pool functions
        // mark this connection as available
        void cleanse();
        // function same as the constructor
        void setup(int fd, struct sockaddr_in remote_addr);

        bool encrypted();
        void set_encrypted(bool b);
        
        SSL* _ssl;
//        index_t face_idx;
    private:
        bool _encrypted;
        int  _fd;
        
        struct event ev_read;
        struct event ev_write;

        struct sockaddr_in _remote_addr;

        list<struct buffered_queue* > writeq;
        template<class LOGIC> friend class Face;

        index_t m_idx_bak;
    };

    // this class is used to bypass the static restriction
    class ConnectionManager{
    public:
        // Connections are managed as map elements
        // key   = index of the face entity
        // value = cooridinate connection set of that face entity
//        map<index_t, ObjectManager<Connection*>* > _connections;
        ObjectManager<ObjectManager<Connection*>* > _connections;
        int create(index_t which_face, unsigned int num);
        ConnectionManager();
        ~ConnectionManager();
    };

}                   

extern FACE::ConnectionManager g_connection_manager;
#endif
