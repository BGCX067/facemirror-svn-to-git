#ifndef __FACE_H__
#define __FACE_H__

#ifndef WIN32
#include <arpa/inet.h>
#include <unistd.h> // for usleep()
#endif

#include <openssl/crypto.h>
#include <openssl/x509.h>
#include <openssl/pem.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/rand.h>

#include <fcntl.h>
#include <iostream>

#include "utils.h"
#include "object.h"
#include "connection.h"
#include "packet.h"

using namespace std;

namespace FACE{

    static const int FACE_ERR__PROTOCOL_ERROR = -1;

    enum use_cipher_enum{
        USE_CIPHER__NONE,
        USE_CIPHER__SSL,
    };

    class Connection;
    
    template<typename LOGIC>
    class Face:public Object{
    public:
        Face();
        // setup all the service functions
        int  listen_to(const unsigned int port,
                       const unsigned int max_connection,
                       const unsigned int read_buffer_size,
                       use_cipher_enum    use_cipher);

        int set_connect_param(const unsigned int max_connection,
                              const unsigned int read_buffer_size,
                              use_cipher_enum    use_cipher);

        Connection* connect_to(const char* ip, unsigned short port);

        // aka heartbeat()
        void smile();

        static int set_nonblock(int fd);

        // connect as a client
        Connection* connect(const char* host,
                            unsigned short port);

        ~Face();
        unsigned int _bytes_in;
        // customed logic
        static LOGIC* _logic;

        bool use_ssl() {return (USE_CIPHER__SSL == _use_ssl?true:false);}
        SSL* _ssl;
    private:
        use_cipher_enum _use_ssl;



        bool _initialized;


        struct event_base *_base;
        int                _listen_fd;
        struct sockaddr_in _listen_addr;
        int                _reuseaddr_on;
        // The socket accept event. 
        struct event       _ev_accept;
        
        unsigned int _port;
        unsigned int _max_connection;
        
        static unsigned int _read_buffer_size;

        // callbacks
        static void on_accept(int fd, short ev, void *arg);
        static void on_write(int fd, short ev, void *arg);
        static void on_read(int fd, short ev, void *arg);

        // used when connections comes in
        Connection* find_available_connection();

    };

    template<typename LOGIC>
    Face<LOGIC>::Face(){
        _bytes_in = 0;
        _reuseaddr_on = 1;
        // init libevent
        _base         = event_init();
        if(NULL == _base){
            p(ERR, "Unable to initialize libevent, quit...\n");
            exit(1);
        }

        // set init flag
        _initialized = false;

        // comply with base class Object, here m_name is unique because
        // it's based on ip:port, which should be unique at anytime.
        char buf[32];
        bzero(buf, sizeof(buf));
        snprintf(buf, sizeof(buf),
                 "%d", m_idx);

        m_name = string(buf);
    }

    template<typename LOGIC>
    int Face<LOGIC>::listen_to(const unsigned int port,
                               const unsigned int max_connection,
                               const unsigned int read_buffer_size,
                               use_cipher_enum    use_cipher = USE_CIPHER__NONE
        ){
        _use_ssl          = use_cipher;
        _port             = port;
        _max_connection   = max_connection;
        _read_buffer_size = read_buffer_size;
    

        _listen_fd = socket(AF_INET, SOCK_STREAM, 0);

        if(0 > _listen_fd)
            p(ERR, "listen failed\n");

        if(-1 == setsockopt(_listen_fd, SOL_SOCKET, SO_REUSEADDR, &_reuseaddr_on,
                            sizeof(_reuseaddr_on)))
            p(ERR, "setsockopt failed\n");
        memset(&_listen_addr, 0, sizeof(_listen_addr));

        _listen_addr.sin_family      = AF_INET;
        _listen_addr.sin_addr.s_addr = INADDR_ANY;
        _listen_addr.sin_port        = htons(port);
    
        if(bind(_listen_fd, (struct sockaddr *)&_listen_addr,
                sizeof(_listen_addr)) < 0)
            p(ERR, "bind failed\n");
        if (listen(_listen_fd, max_connection) < 0)
            p(ERR, "listen failed\n");

        if(use_ssl()){
            // setup SSL
            SSL_library_init();
			SSLeay_add_ssl_algorithms();
			ERR_load_crypto_strings();
			SSL_load_error_strings();
        }

        if (set_nonblock(_listen_fd) < 0)
            p(ERR, "failed to set server socket to non-blocking\n");

        event_set(&_ev_accept, _listen_fd, EV_READ|EV_PERSIST, on_accept, this);
        event_base_set(_base, &_ev_accept);
        event_add(&_ev_accept, NULL);

        // init connections
        if(0 != g_connection_manager.create(m_idx, _max_connection)){
            p(ERR, "Face entity with idx %d is already there.\n");
            return -1;
        }

        _initialized = true;
        p(INF, "Face bind to port %d with %d connections capacity.\n", _port, _max_connection);

        return 0;
    }

    template<typename LOGIC>
    int Face<LOGIC>::set_connect_param(const unsigned int max_connection,
                                       const unsigned int read_buffer_size,
                                       use_cipher_enum    use_cipher){

        _use_ssl          = use_cipher;
        _max_connection   = max_connection;
        _read_buffer_size = read_buffer_size;

        // setup SSL
        if(_use_ssl){
            SSL_library_init();
            SSLeay_add_ssl_algorithms();
            ERR_load_crypto_strings();
            SSL_load_error_strings();

        }
        // init connections
        if(0 != g_connection_manager.create(m_idx, _max_connection)){
            p(ERR, "Face entity with idx %d is already there.\n");
            return -1;
        }

        _initialized = true;
        return 0;
    }

    template<typename LOGIC>
    int Face<LOGIC>::set_nonblock(int fd)
    {
        int flags;

        flags  = fcntl(fd, F_GETFL);
        if (flags < 0)
            return flags;
        flags |= O_NONBLOCK;
        if (fcntl(fd, F_SETFL, flags) < 0)
            return -1;

        return 0;
    }


    template<typename LOGIC>
    void Face<LOGIC>::smile(){
        usleep(1000);
        event_base_loop(_base, EVLOOP_ONCE);
    }

    template<typename LOGIC>
    void Face<LOGIC>::on_read(int fd, short ev, void *arg){
        Connection            *client = (Connection *)arg;
//    struct buffered_queue *bufferq;
        char                  *buf;
        int                    len;
    
        // NOTE: this malloced memory will be freed in ~buffered_queue()
        buf = (char*)malloc(_read_buffer_size);

        if (buf == NULL)
            p(ERR, "malloc failed");

        if(client->encrypted()){// SSL
            len = SSL_read(client->_ssl, buf, _read_buffer_size);
        }else
            len = read(fd, buf, _read_buffer_size);

        if (len == 0) {
            // Client disconnected, remove the read event and the
            // free the client structure. 
            // FIN received
            _logic->connection_on_disconnect(client);

            close(fd);
            event_del(&client->ev_read);
            event_del(&client->ev_write);
            // "cleanse" instead delete.
            client->cleanse();
//        delete client;
            return;
        }
        else if (len < 0) {
            // Some other error occurred, close the socket, remove
            // the event and free the client structure.
            // RST received
        
//        p(ERR, "Socket failure, disconnecting client: %s",
//               strerror(errno));
            _logic->connection_on_disconnect(client);

            close(fd);
            event_del(&client->ev_read);
            event_del(&client->ev_write);
            client->cleanse();
//        delete client;
            return;
        }

        _logic->dispatch(client, buf, len);
//    bufferq = new struct buffered_queue(buf, len);
//    if (bufferq == NULL)
//        p(ERR, "bufferq failed\n");

//    cout<<buf<<endl;

//    client->writeq.push_back(bufferq);

//    event_add(&client->ev_write, NULL);
    }

    template<typename LOGIC>
    void Face<LOGIC>::on_write(int fd, short ev, void *arg)
    {
        if(ev == EV_TIMEOUT)
            p(ERR, "WRITE TIMEOUT\n");

        Connection            *remote_connection = (Connection *)arg;
        struct buffered_queue *bufferq;
        int                    len;
    
        if(0 == remote_connection->writeq.size()){
            return;
//           p(ERR, "writeq size is 0\n");
        }
//        p(0, "q size: %d\n", remote_connection->writeq.size());
        bufferq = remote_connection->writeq.front();
        if (bufferq == NULL)
            return;

        len = bufferq->len - bufferq->offset;

        if(remote_connection->encrypted())
            len = SSL_write(remote_connection->_ssl,
                            bufferq->buf + bufferq->offset,
                            bufferq->len - bufferq->offset);
        else
            len = write(fd, bufferq->buf + bufferq->offset,
                        bufferq->len - bufferq->offset);
        if (len == -1) {
            if (errno == EINTR || errno == EAGAIN) {
                /* The write was interrupted by a signal or we
                 * were not able to write any data to it,
                 * reschedule and return. */
                event_add(&remote_connection->ev_write, NULL);
                return;
            }
            else {
                /* Some other socket error occurred, exit. */
                p(ERR, "write error\n");
            }
        }
        else if ((bufferq->offset + len) < bufferq->len) {
            /* Not all the data was written, update the offset and
             * reschedule the write event. */
            bufferq->offset += len;
            event_add(&remote_connection->ev_write, NULL);
            return;
        }
    
        // The data was completely written, remove the buffer from the
        // write queue.
        remote_connection->writeq.pop_front();
//    remote_connection->writeq.remove(bufferq);
//    TAILQ_REMOVE(&remote_connection->writeq, bufferq, entries);
        delete bufferq;// here destructor will handle bufferq->buf.
    }

    template<typename LOGIC>
    void Face<LOGIC>::on_accept(int fd, short ev, void *arg)
    {
        int                 client_fd;
        struct sockaddr_in  client_addr;
        socklen_t           client_len        = sizeof(client_addr);
        Connection         *remote_connection = NULL;
        Face<LOGIC>        *fa                = (Face<LOGIC> *)arg;
        unsigned int       *face_idx          = &(fa->m_idx);
        

        client_fd = accept(fd, (struct sockaddr *)&client_addr, &client_len);
        if (client_fd == -1) {
            p(ERR, "accept failed\n");
            return;
        }

        // Accept the new connection. 
        // if use SSL
        // ssl: 3 steps
        if(fa->use_ssl()){
			unsigned char rand[32] ;
			memset( rand, 0, sizeof( rand ) );
			snprintf( (char*)rand, sizeof( rand ), "%ld", time(NULL) );
			RAND_seed( rand, sizeof( rand ) );

            SSL_CTX* ctx = SSL_CTX_new(SSLv23_method());
            

            // load the cert
            if(1 != SSL_CTX_use_certificate_chain_file(ctx, "server.pem"))
                p(ERR, "Error loading certificate from file\n");
            SSL_CTX_set_default_passwd_cb_userdata(ctx, (void*)"password");
            if(1 != SSL_CTX_use_PrivateKey_file(ctx, "server.pem", SSL_FILETYPE_PEM))
                p(ERR, "Error loading certificate key from file\n");

            fa->_ssl = SSL_new(ctx);


            if(NULL == fa->_ssl){
                char errmsg[256];
                bzero(errmsg, sizeof(errmsg));
                ERR_error_string_n( ERR_get_error(), errmsg, sizeof( errmsg ) );
                p(ERR, "Unable to create SSL: %s\n", errmsg);
            }

            // assign SSL with fd
            int      ssl_r;
            ssl_r = SSL_set_fd(fa->_ssl, client_fd);
            ssl_r = SSL_accept(fa->_ssl);
            if(0 > ssl_r){
                char errmsg[256];
                bzero(errmsg, sizeof(errmsg));
                ERR_error_string_n( ERR_get_error(), errmsg, sizeof( errmsg ) );
                p(ERR, "Unable to create SSL: %d, %d, %s\n",
                  errno, 
                  SSL_get_error(fa->_ssl, ssl_r), errmsg);
            }
                
        }

        // Set the client socket to non-blocking mode. 
        if (set_nonblock(client_fd) < 0)
            p(ERR, "failed to set client socket non-blocking\n");

        // get a connection from the connection pool
        ObjectManager<Connection*> *t;
        try{
            t = g_connection_manager._connections.find_by_idx(*face_idx);
        }catch(...){
            p(ERR, "Unable to find the face entity for connection: %d\n",
              face_idx);
            return;
        }

        FACE_CYCLE_INDEX(Connection*, t){
            if(it->second->available())
                remote_connection = it->second;
        }
    
        if (remote_connection == NULL){
            p(ERR, "Connection pool full.\n");
//        shutdown(fd, SHUT_RDWR);
            return;
        }

        remote_connection->setup(client_fd, client_addr);
        remote_connection->set_encrypted(fa->use_ssl());
        remote_connection->_ssl = fa->_ssl;

        event_set(&remote_connection->ev_read, client_fd, EV_READ|EV_PERSIST,
                  on_read, remote_connection);

        // Setting up the event does not activate, add the event so it
        //  becomes active. 
        event_base_set(fa->_base, &remote_connection->ev_read);
        event_add(&remote_connection->ev_read, NULL);

        event_set(&remote_connection->ev_write, client_fd, EV_WRITE|EV_PERSIST,
                  on_write, remote_connection);
//    event_add(&remote_connection->ev_write, NULL);
        event_base_set(fa->_base, &remote_connection->ev_write);
    
        _logic->connection_on_connect(remote_connection);
    }

    template<typename LOGIC>
    Connection* Face<LOGIC>::find_available_connection(){

        return NULL;// pre-allocated connection pool full
    }

    template<typename LOGIC>
    Connection* Face<LOGIC>::connect_to(const char* ip, unsigned short port){
        if(!_initialized){
            p(ERR, "connection parameters not set yet, call set_connect_param()\n");
            return NULL;
        }

        Connection* remote_connection;
        ObjectManager<Connection*> *t;
        try{
            t = g_connection_manager._connections.find_by_idx(m_idx);
        }catch(...){
            p(ERR, "Unable to find the face entity for connection: %d\n",
              m_idx);
            return NULL;
        }

        FACE_CYCLE_INDEX(Connection*, t){
            if(it->second->available())
                remote_connection = it->second;
        }
//    remote_connection = find_available_connection();
    
        if (remote_connection == NULL){
            p(ERR, "Connection pool full.\n");
//        shutdown(fd, SHUT_RDWR);
            return NULL;
        }

        remote_connection->set_encrypted(use_ssl());// must be set before connect_to()
        remote_connection->_ssl = _ssl;
        remote_connection->connect_to(ip, port);

        event_set(&remote_connection->ev_read, remote_connection->_fd,
                  EV_READ|EV_PERSIST, on_read, remote_connection);
        event_base_set(_base, &remote_connection->ev_read);
        event_add(&remote_connection->ev_read, NULL);

        event_set(&remote_connection->ev_write, remote_connection->_fd,
                  EV_WRITE|EV_PERSIST, on_write, remote_connection);
        event_base_set(_base, &remote_connection->ev_write);
        event_add(&remote_connection->ev_write, NULL);

        return remote_connection;
    }

    template<typename LOGIC>
    Face<LOGIC>::~Face(){
        // PENDING: libevent1.1a does NOT provided this function, 
#ifndef WIN32
        event_base_free(_base);
#endif
    }

    template<typename LOGIC>
    unsigned int Face<LOGIC>::_read_buffer_size = 0;

    template<typename LOGIC>
    LOGIC* Face<LOGIC>::_logic = new LOGIC;


}
#endif
