#ifndef __CONFIG_H__
#define __CONFIG_H__

#include <map>
#include <iostream>
#include <fstream>

using namespace std;

namespace FACE{

    class Config{
    public:
        static const int MAX_LINE_LEN = 256;

        int load(const char* fn);
        int load(const string& fn );
        int reload(const char* fn);
        int reload(const string& fn);

        const char* operator[](const char* key);
        long int get_longint(const char* key);
        
        
    private:
        ifstream _config_file;

        map<string, string> _dict;

        typedef map<string, string>::value_type conf_pair;
    };
}
#endif
