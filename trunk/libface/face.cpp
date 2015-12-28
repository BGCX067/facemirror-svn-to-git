#include "face.h"

using namespace FACE;
using namespace std;

#ifdef MAKEEXE
#include "sample_srv.h"
#include "sample_cli.h"

int main(int argc, char** argv){


//     f._bytes_in++;
//     cout<<f._bytes_in<<endl;
//     cout<<f2._bytes_in<<endl;

#ifdef MAKESERVER
    Face<SampleS> f;
//    Face<DummyLogicS> f1;
//    f1.listen_to(6666, 4, 1024);
//    f.listen_to(7777, 2, 1024, USE_CIPHER__NONE);
    f.listen_to(7777, 2, 1024, USE_CIPHER__SSL);
    while(1){
        f.smile();
//        f1.smile();
    }
#endif

#ifdef MAKECLI
    Face<SampleC> f;
//    Face<DummyLogicC> f1;

//    f1.set_connect_param(2, 1024);
    f.set_connect_param(2, 1024, USE_CIPHER__SSL);
//    f.set_connect_param(2, 1024, USE_CIPHER__NONE);
    Connection *c1 = f.connect_to("127.0.0.1", 7777);
    p(INF, "Connected to %s:%d\n", c1->get_ip().c_str(), c1->get_port());
//    Connection *c = f.connect_to("127.0.0.1", 7777);
//    p(INF, "Connected to %s:%d\n", c->get_ip().c_str(), c->get_port());
    if(NULL == c1)
        p(ERR, "SUCK\n");

    f._logic->ping(c1, 10, "SUCK", 4);
//    f._logic->ping2(c, 10, "SUCK", 4);

//    Packet pa(1024);
//     pa.push_2byte(2);
//     pa.push_4byte(10);
//     pa.push_4byte(4);
//     pa.push_1byte('s');
//     pa.push_1byte('u');
//     pa.push_1byte('c');
//     pa.push_1byte('k');
//    for(int i = 0 ; i< 2000; i++)
//    c->push_out(&pa);
//     c1->push_out(&pa);
//     c1->push_out(&pa);
//     c1->push_out(&pa);
//     c1->push_out(&pa);
//     c->push_out(&pa);
//     c->push_out(&pa);

    while(1){
        f.smile();
//        f1.smile();
    }
#endif
    return 0;
}

#endif // MAKEEXE
