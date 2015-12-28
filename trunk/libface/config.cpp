#include "config.h"
#include "utils.h"

using namespace std;
using namespace FACE;

int Config::load(const char* fn){
    _config_file.open(fn, ios::in);
    if(_config_file.fail()){
        p(ERR, "Unable to open %s:\n", fn);
        return -1;
    }

    char buf[MAX_LINE_LEN];

    bzero(buf, sizeof(buf));

    int lineno = 0;
    while(_config_file.getline(buf, MAX_LINE_LEN)){
        string mkey;
        string mvalue;
        
        string tmp(buf);

        // wipeout the comment
        string::size_type pos = tmp.find("//");
        if(string::npos != pos)
            tmp.erase(pos);

        pos = tmp.find("=");
        if(string::npos != pos){
            mkey = tmp.substr(0, pos);
            mvalue = tmp.substr(pos+1);

            // if there're duplicated keys, only first get recorded
            if(0 == _dict.count(mkey))
                _dict.insert(conf_pair(mkey, mvalue)); 
            bzero(buf, sizeof(buf));
            lineno++;
        }// process next line if '=' not find 
    }

    _config_file.close();
    return _dict.size();
}

int Config::load(const string& fn){
    return load(fn.c_str());
}

int Config::reload(const char* fn){
    _dict.erase(_dict.begin(), _dict.end());
    return load(fn);
}

int Config::reload(const string& fn){
    _dict.erase(_dict.begin(), _dict.end());
    return load(fn);
}

const char* Config::operator [](const char* key){
    if(1 == _dict.count(string(key)))
        return _dict[key].c_str();
    return NULL;
}

long int Config::get_longint(const char* key){
    if(1 == _dict.count(string(key)))
        return strtol(_dict[key].c_str(), NULL, 10);
    return 0;
}
