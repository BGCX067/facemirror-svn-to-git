#include "connection.h"

using namespace FACE;

ConnectionManager g_connection_manager;

Connection::Connection(){
    _encrypted = false;
    _fd = -1;
    // comply with base class Object, here m_name is unique because
    // it's based on ip:port, which should be unique at anytime.
    char buf[32];
    bzero(buf, sizeof(buf));
    snprintf(buf, sizeof(buf),
             "%d", m_idx);

    m_name = string(buf);

    _ssl = NULL;
}

const string Connection::get_ip() const {
    return string(inet_ntoa(_remote_addr.sin_addr));
}

const unsigned short Connection::get_port() const{
    return ntohs(_remote_addr.sin_port);
}

const bool Connection::available() const{
    return (-1 == _fd?true:false);
}

void Connection::set_encrypted(bool b){
    _encrypted = b;
}

bool Connection::encrypted(){
    return _encrypted;
}

void Connection::cleanse(){
    close(_fd);
    _encrypted = false;
    _ssl = NULL;

    _fd = -1;
    m_idx = m_idx_bak;

    for(list<struct buffered_queue* >::iterator it = writeq.begin();
        it != writeq.end();
        it++){
        delete *it;
    }
    writeq.erase(writeq.begin(), writeq.end());
}

// used by server
void Connection::setup(int fd, struct sockaddr_in remote_addr){
    m_idx_bak    = m_idx;
    m_idx        = _fd          = fd;
    _remote_addr = remote_addr;
    
    // comply with base class Object, here m_name is unique because
    // it's based on ip:port, which should be unique at anytime.
    char buf[32];
    bzero(buf, sizeof(buf));
    snprintf(buf, sizeof(buf),
             "%s:%d",
             inet_ntoa(_remote_addr.sin_addr),
             _remote_addr.sin_port);
    m_name = string(buf);
    
}
    
Connection* Connection::connect_to(const char* ip, unsigned short port){
    if(-1 != _fd)
        p(INF, "Warning: this is an unavailabe fd: %d, but called for connect.\n",
          _fd);

#ifdef WIN32
    {
        WORD    wVersionRequested;
        WSADATA wsaData;
        int     err;

        wVersionRequested = MAKEWORD( 2, 0 );

        err = WSAStartup( wVersionRequested, &wsaData );
        if ( err != 0 ) {
            /* Tell the user that we couldn't find a usable */
            /* WinSock DLL.                               */
            p(ERR, "can't initialize socket library\n");
            exit(0);
        }
    }
#endif    

    _fd = socket(AF_INET, SOCK_STREAM, 0);

    if(0 > _fd){
        p(ERR, "connect() failed\n");
        _fd = -1;
        return NULL;
    }

    bzero(&_remote_addr, sizeof(_remote_addr));
    
    _remote_addr.sin_port   = htons(port);
    _remote_addr.sin_family = AF_INET;
    

#ifndef WIN32
    if(0 >= inet_pton(AF_INET, ip, &_remote_addr.sin_addr)){
        p(ERR, "IP address error: %s\n", ip);
        close(_fd);
        _fd = -1;
        return NULL;
    }
#else
    struct hostent *phe;
    if ( phe = gethostbyname(ip) )
        memcpy(&_remote_addr.sin_addr, phe->h_addr, phe->h_length);
    else if ( (_remote_addr.sin_addr.s_addr = inet_addr(ip)) == INADDR_NONE )
        p(ERR, "%s is not a valid address\n");
#endif

            
    if(0 > connect(_fd, (struct sockaddr*)&_remote_addr, sizeof(_remote_addr))){
        p(ERR, "Connect error: %s\n", ip);
        close(_fd);
        _fd = -1;
        return NULL;
    }

    if(encrypted()){            // SSL
        // setup SSL context
        unsigned char rand[32] ;
        memset( rand, 0, sizeof( rand ) );
        snprintf( (char*)rand, sizeof( rand ), "%ld", time(NULL) );
        RAND_seed( rand, sizeof( rand ) );
            
        SSL_CTX* ctx = SSL_CTX_new(SSLv23_method());


        if(NULL == ctx){
            char errmsg[256];
            bzero(errmsg, sizeof(errmsg));
            ERR_error_string_n( ERR_get_error(), errmsg, sizeof( errmsg ) );
            p(ERR, "Unable to create CTX: %s\n", errmsg);
        }

//         if(1 != SSL_CTX_use_certificate_chain_file(ctx, "/home/yulair/client.pem"))
//             p(ERR, "Error loading certificate from file\n");
//         if(1 != SSL_CTX_use_PrivateKey_file(ctx, "/home/yulair/client.pem", SSL_FILETYPE_PEM))
//             p(ERR, "Error loading certificate key from file\n");

        _ssl = SSL_new(ctx);

        if(NULL == _ssl){
            char errmsg[256];
            bzero(errmsg, sizeof(errmsg));
            ERR_error_string_n( ERR_get_error(), errmsg, sizeof( errmsg ) );
            p(ERR, "Unable to create SSL: %s\n", errmsg);
        }

        // handshake
        int ret = SSL_set_fd(_ssl, _fd);
        if(0 > ret){
            char errmsg[256];
            bzero(errmsg, sizeof(errmsg));
            ERR_error_string_n( ERR_get_error(), errmsg, sizeof( errmsg ) );
            p(ERR, "Unable to set SSL fd: %s\n", errmsg);
            close(_fd);
            _fd = -1;
            return NULL;
        }
        
        ret = SSL_connect(_ssl);
        if(0 > ret){
            int errco = SSL_get_error(_ssl, ret);
            char errmsg[256];
            bzero(errmsg, sizeof(errmsg));
            ERR_error_string_n( ERR_get_error(), errmsg, sizeof( errmsg ) );
            p(ERR, "Unable to connect via SSL: %d, %d, %s\n", errno, errco, errmsg);
            close(_fd);
            _fd = -1;
            return NULL;
        }
    }

    m_idx_bak = m_idx;
    m_idx     = _fd;
    // comply with base class Object, here m_name is unique because
    // it's based on ip:port, which should be unique at anytime.
    char buf[32];
    bzero(buf, sizeof(buf));
    snprintf(buf, sizeof(buf),
             "%s:%d",
             inet_ntoa(_remote_addr.sin_addr),
             _remote_addr.sin_port);
    m_name = string(buf);
    
    return this;
}

int Connection::push_out(Packet* pack){
    
    struct buffered_queue* bufferq = new struct buffered_queue(pack->get_buf(),
                                                               pack->get_len());

    if (bufferq == NULL){
        p(ERR, "bufferq init failed\n");
        return -1;
    }

    writeq.push_back(bufferq);
    event_add(&ev_write, NULL);

    return 0;// pack will be destructed in caller
}

Connection::~Connection(){
    for(list<struct buffered_queue* >::iterator it = writeq.begin();
        it != writeq.end();
        it++){
        delete *it;
    }
}

ConnectionManager::ConnectionManager(){
    // comply with base class Object, here m_name is unique because
    // it's based on ip:port, which should be unique at anytime.
//     char buf[32];
//     bzero(buf, sizeof(buf));
//     snprintf(buf, sizeof(buf),
//              "%d", m_idx);

//     m_name = string(buf);
}

ConnectionManager::~ConnectionManager(){
}

int ConnectionManager::create(index_t which_face, unsigned int num){
    if(0 != _connections.index_idx.count(which_face)){
        return -1;
    }

    ObjectManager<Connection*> *t = new ObjectManager<Connection*>;

    for(unsigned int i = 0; i < num; i++){
        Connection *c = new Connection;
        if(!c)
            return -2;
        t->add(c);
    }
    t->set_idx(which_face);
//    _connections.insert(map<index_t, ObjectManager<Connection*>* >::value_type(which_face, t));
    _connections.add(t);
    return 0;
}
    
