%{
//#define YYDEBUG 1

#ifdef YYDEBUG
#define TRACE printf("reduce at line %d\n", __LINE__);
#else
#define TRACE
#endif

#define FACE_HEAD_LEN 4
#define INDENT_LEN    4

#include <iostream>
#include <fstream>
#include <map>
#include <string>
#include <vector>
#include <cctype>
#include <stdarg.h>
#include <getopt.h>

#include "face.tab.hh"
using namespace std;

extern "C"
{
        int yyparse(void);
        extern int yylex(void);  
        int yywrap()
        {
                return 1;
        }
		void yyerror(const char *str)
		{
		    cerr<<str<<endl;
		}

}

extern FILE* yyin;

string class_name;
string tmp;
string file_content;

typedef struct {
    string       type_name;
    string       var_name;
    unsigned int len;
}param_t;

typedef struct {
    string          rpc_name;
    unsigned int    rpc_protocol_id;
    string          protocol_id_string;
    vector<param_t> param_list;
}rpc_t;

rpc_t tmp_rpc;

unsigned int protocol_id_counter = 1;
map<int, string> protocol_id_string;

vector<rpc_t> method_list_srv;
vector<rpc_t> method_list_cli;

string to_upper(string strToConvert)
{                               //change each element of the string to upper case
    for(unsigned int i=0;i<strToConvert.length();i++)
    {
        strToConvert[i] = toupper(strToConvert[i]);
    }
    return strToConvert;        //return the converted string
}

string to_lower(string strToConvert)
{                               //change each element of the string to lower case
    for(unsigned int i=0;i<strToConvert.length();i++)
    {
        strToConvert[i] = tolower(strToConvert[i]);
    }
    return strToConvert;        //return the converted string
}

int g_indent = 0;

void indent(int how_many){
    g_indent += how_many;

    if(g_indent < 0){
        cout<<"indent smashed"<<endl;
        g_indent = 0;
        return;
    }
}

void _p(char *str, va_list ap){

    char buf[1024];
    bzero(buf, sizeof(buf));

/*    va_list ap; */
/*    va_start(ap, str); */
    vsprintf(buf, str, ap);

    file_content.append(buf);
/*    va_end(ap); */
}

void println(char *str, ...){
    for(int i = 0; i < g_indent; i++)
        file_content.append(" ");

    va_list ap;
    va_start(ap, str);
    _p(str, ap);
    va_end(ap);
    return;
}

void print(char *str, ...){
    va_list ap;
    va_start(ap, str);
    _p(str, ap);
    va_end(ap);
    return;
}


%}

%token ROOT SERVER_PUSH CLIENT_PULL OPARENSES EPARENSES OBRACE EBRACE OSQUARE ESQUARE COMMA


%union 
{
        int number;
        char *string;
}

%token <number> NUMBER
%token <string> TYPE
%token <string> VAR_NAME

%type <string> param params

%%

root:
	|ROOT VAR_NAME OBRACE rpcs EBRACE
    {class_name = string($2);}
;

rpcs:
    |rpcs rpc
;

rpc:
	SERVER_PUSH VAR_NAME OPARENSES params EPARENSES
    {
        tmp_rpc.rpc_name        = string($2);
        tmp_rpc.rpc_protocol_id = protocol_id_counter++;
        tmp_rpc.protocol_id_string = to_upper(string($2));
        protocol_id_string.insert(map<int, string>::value_type(
                                      protocol_id_counter-1,
                                      to_upper(string($2))));

        method_list_srv.push_back(tmp_rpc);

        tmp      = string("");
        tmp_rpc.param_list.erase(tmp_rpc.param_list.begin(),
                                 tmp_rpc.param_list.end());
    }
	|CLIENT_PULL VAR_NAME OPARENSES params EPARENSES
    {
        tmp_rpc.rpc_name        = string($2);
        tmp_rpc.rpc_protocol_id = protocol_id_counter++;
        tmp_rpc.protocol_id_string = to_upper(string($2));
        protocol_id_string.insert(map<int, string>::value_type(
                                      protocol_id_counter-1,
                                      to_upper(string($2))));

        method_list_cli.push_back(tmp_rpc);

        tmp      = string("");
        tmp_rpc.param_list.erase(tmp_rpc.param_list.begin(),
                                 tmp_rpc.param_list.end());
    }
;

params:
    {}
	|param
    {
        $$ = (char*)(tmp.append(string((char*)$1)).append(", ").c_str());
    }
	|params COMMA param
    {
        $$ = (char*)(tmp.append(string((char*)$3)).append(", ").c_str());
    }
;

param:
    TYPE VAR_NAME OSQUARE NUMBER ESQUARE
    {
        char buf[256];
        bzero(buf, sizeof(buf));
        snprintf(buf, sizeof(buf), "%s* %s, int %s_len",
                 $1, $2, $2);

        param_t pt;

        pt.type_name = string($1);
        pt.var_name = string($2);
        pt.len = $4;

        tmp_rpc.param_list.push_back(pt);

        TRACE $$ = buf;
    }
	|TYPE VAR_NAME
	{
        param_t pt;

        pt.type_name = string($1);
        pt.var_name = string($2);
        pt.len = 1;//1 means not array

        tmp_rpc.param_list.push_back(pt);

        TRACE $$ = (char*)string($1).append(" ").append(string($2)).c_str();
    }
;
%%

void generate_header_one_side(vector<rpc_t>& _mlist_srv, vector<rpc_t>& _mlist_cli, bool is_srv){
    file_content = "";

    string cn = class_name;
    if(is_srv)
        cn.append("S");
    else
        cn.append("C");
    
    // generate server source first
    print("\n"
          "#ifndef __%s_H__\n"
          "#define __%s_H__\n\n"
          "// DO NOT EDIT THIS FILE\n"
          "// THIS FILE IS GENERATED BY face2cpp\n\n"
          "#include \"face.h\"\n\n"
          "using namespace std;\n"
          "using namespace FACE;\n\n"
          "class %s{\n"
          "public:\n\n", to_upper(cn).c_str(), to_upper(cn).c_str(), cn.c_str());

    indent(INDENT_LEN);

    for(map<int, string>::iterator it = protocol_id_string.begin();
        it != protocol_id_string.end();
        it++)
    {
        println("static const short FACE_RPC__%s_%s = %d;\n",
               to_upper(class_name).c_str(),
               it->second.c_str(),
               it->first);
    }

    println("\n");

    // dispatch function
    println("void dispatch(Connection* c, char* data, unsigned int len){\n");
    indent(INDENT_LEN);
    println("Packet *pack = new Packet(len);\n\n");
    println("if(-1 == pack->set_buf(data, len)){\n");
    indent(INDENT_LEN);
    println("p(ERR, \"%%s:%%d: buffer overflow\", __FILE__, __LINE__);\n");
    println("return;\n");
    indent(-INDENT_LEN);
    println("}\n\n");
    println("short rpc_num = pack->pop_2byte();\n\n");

    bool first_if = true;

    for(int i = 0; i < _mlist_cli.size(); i++){
        if(first_if){
            first_if = false;
            println("if(FACE_RPC__%s_%s == rpc_num){\n",
                    to_upper(class_name).c_str(),
                    to_upper(_mlist_cli[i].rpc_name).c_str());
        }else{
            println("}else if(FACE_RPC__%s_%s == rpc_num){\n",
                    to_upper(class_name).c_str(),
                    to_upper(_mlist_cli[i].rpc_name).c_str());
        }
        indent(INDENT_LEN);
        
        // fetch the paramters
        for(int j = 0; j < _mlist_cli[i].param_list.size(); j++){

            char inner_len;
            if("int" == _mlist_cli[i].param_list[j].type_name)
                inner_len = 4;
            else if("short" == _mlist_cli[i].param_list[j].type_name)
                inner_len = 2;
            else if("char" == _mlist_cli[i].param_list[j].type_name)
                inner_len = 1;

            if(1 < _mlist_cli[i].param_list[j].len){ // array
                println("int %s_actual_len = pack->pop_4byte();\n",
                        _mlist_cli[i].param_list[j].var_name.c_str());
                println("%s *%s;\n",
                        _mlist_cli[i].param_list[j].type_name.c_str(),
                        _mlist_cli[i].param_list[j].var_name.c_str());
                println("%s = (%s*)malloc(sizeof(%s) * %s_actual_len);\n",
                        _mlist_cli[i].param_list[j].var_name.c_str(),
                        _mlist_cli[i].param_list[j].type_name.c_str(),
                        _mlist_cli[i].param_list[j].type_name.c_str(),
                        _mlist_cli[i].param_list[j].var_name.c_str());
                
                println("bzero(%s, sizeof(%s) * %s_actual_len);\n",
                        _mlist_cli[i].param_list[j].var_name.c_str(),
                        _mlist_cli[i].param_list[j].type_name.c_str(),
                        _mlist_cli[i].param_list[j].var_name.c_str());
                
                println("for(int k = 0; k < %s_actual_len; k++)\n",
                        _mlist_cli[i].param_list[j].var_name.c_str());
                indent(INDENT_LEN);
                println("%s[k] = pack->pop_%dbyte();\n\n",
                        _mlist_cli[i].param_list[j].var_name.c_str(),
                        inner_len);
                indent(-INDENT_LEN);

            }else
                println("%s %s = pack->pop_%dbyte();\n\n",
                        _mlist_cli[i].param_list[j].type_name.c_str(),
                        _mlist_cli[i].param_list[j].var_name.c_str(),
                        inner_len);
        }

        println("\n");
        println("%s_on_receive(c, ", _mlist_cli[i].rpc_name.c_str());

        // append params to the RPC invoke
        for(int j = 0; j < _mlist_cli[i].param_list.size(); j++){
            if(1 == _mlist_cli[i].param_list[j].len) // non-array
                print("%s",
                       _mlist_cli[i].param_list[j].var_name.c_str());
            else                // array
                print("%s, %d",//PENDING: should be dynamic
                       _mlist_cli[i].param_list[j].var_name.c_str(),
                       _mlist_cli[i].param_list[j].len);
            if(j == _mlist_cli[i].param_list.size() - 1)
                print(");\n"); // last param
            else
                print(", ");

        }

        indent(-INDENT_LEN);
        
    }
    println("}else{\n");
    indent(INDENT_LEN);
    println("connection_on_error(c, FACE_ERR__PROTOCOL_ERROR);\n");
    indent(-INDENT_LEN);
    println("}\n\n");
    println("delete pack;\n");
    indent(-INDENT_LEN);
    println("}\n\n");
    

    println("// server side send functions\n\n");

    // auto generate functions
    for(unsigned int i = 0; i < _mlist_srv.size(); i++){
        println("int %s(Connection *c, ", _mlist_srv[i].rpc_name.c_str());
        // declare line:
        for(int j = 0; j < _mlist_srv[i].param_list.size(); j++){
            if(1 == _mlist_srv[i].param_list[j].len) // non-array
                print("%s %s",
                       _mlist_srv[i].param_list[j].type_name.c_str(),
                       _mlist_srv[i].param_list[j].var_name.c_str());
            else                // array
                print("%s *%s, int %s_len",
                       _mlist_srv[i].param_list[j].type_name.c_str(),
                       _mlist_srv[i].param_list[j].var_name.c_str(),
                       _mlist_srv[i].param_list[j].var_name.c_str()
                    );
            if(j == _mlist_srv[i].param_list.size() - 1)
                print("){\n"); // last param
            else
                print(", ");
        }
        indent(INDENT_LEN);
        // function content
        println("Packet *pack = new Packet(%d +", FACE_HEAD_LEN); // 4 bytes face header
        for(int j = 0; j < _mlist_srv[i].param_list.size(); j++){
            print("sizeof(%s) * %d",
                   _mlist_srv[i].param_list[j].type_name.c_str(),
                   _mlist_srv[i].param_list[j].len);
            
            if(j == _mlist_srv[i].param_list.size() - 1)
                print(");\n");    // last param
            else
                print(" + ");
        }
        println("\n");
        println("pack->push_2byte(FACE_RPC__%s_%s);\n",
               to_upper(class_name).c_str(),
               _mlist_srv[i].protocol_id_string.c_str());
        // push all the parameters
        for(int j = 0; j < _mlist_srv[i].param_list.size(); j++){
            // decide which push function to use
            char inner_len;
            if("int" == _mlist_srv[i].param_list[j].type_name)
                inner_len = 4;
            else if("short" == _mlist_srv[i].param_list[j].type_name)
                inner_len = 2;
            else if("char" == _mlist_srv[i].param_list[j].type_name)
                inner_len = 1;
            // array:
            if(1 < _mlist_srv[i].param_list[j].len){
                println("pack->push_4byte(sizeof(%s) * %s_len);// array len indicator\n",
                        _mlist_srv[i].param_list[j].type_name.c_str(),
                        _mlist_srv[i].param_list[j].var_name.c_str());
                        
                println("for(int k = 0; k < %s_len; k++)\n",
                        _mlist_srv[i].param_list[j].var_name.c_str());
                indent(INDENT_LEN);
                println("pack->push_%dbyte(%s[k]);\n",
                       inner_len,
                       _mlist_srv[i].param_list[j].var_name.c_str());
                indent(-INDENT_LEN);
            }else               //non-array:
                println("pack->push_%dbyte(%s);\n",
                       inner_len,
                       _mlist_srv[i].param_list[j].var_name.c_str());
        }
        println("c->push_out(pack);\n\n");
        println("delete pack;\n\n");
        println("return 0;\n");
        indent(-INDENT_LEN);
        println("}\n\n");
    }
    println("// user fill contents\n");
    println("void connection_on_connect(Connection* c);\n");
    println("void connection_on_disconnect(Connection* c);\n");
    println("void connection_on_error(Connection* c, int err_code);\n");
    println("\n");

    for(unsigned int i = 0; i < _mlist_cli.size(); i++){
        println("int %s_on_receive(Connection *c, ", _mlist_cli[i].rpc_name.c_str());
        // declare line:
        for(int j = 0; j < _mlist_cli[i].param_list.size(); j++){
            if(1 == _mlist_cli[i].param_list[j].len) // non-array
                print("%s %s",
                       _mlist_cli[i].param_list[j].type_name.c_str(),
                       _mlist_cli[i].param_list[j].var_name.c_str());
            else                // array
                print("%s *%s, int %s_len",
                       _mlist_cli[i].param_list[j].type_name.c_str(),
                       _mlist_cli[i].param_list[j].var_name.c_str(),
                       _mlist_cli[i].param_list[j].var_name.c_str()
                    );
            if(j == _mlist_cli[i].param_list.size() - 1)
                print(");\n"); // last param
            else
                print(", ");
        }
    }
    indent(-INDENT_LEN);


    println("};\n");
    println("#endif\n");

}

void generate_src_one_side(vector<rpc_t>& _mlist, bool is_srv){
    file_content = "";

    string class_name_with_suffix = class_name;
    if(is_srv){
        println("#include \"%s_srv.h\"\n\n", to_lower(class_name).c_str());
        class_name_with_suffix.append("S");
    }
    else{
        println("#include \"%s_cli.h\"\n\n", to_lower(class_name).c_str());
        class_name_with_suffix.append("C");
    }

    println("void %s::connection_on_connect(Connection* c){\n\n",
            class_name_with_suffix.c_str());
    println("}\n\n");
    println("void %s::connection_on_disconnect(Connection* c){\n\n",
            class_name_with_suffix.c_str());
    println("}\n\n");
    println("void %s::connection_on_error(Connection* c, int err_code){\n\n",
            class_name_with_suffix.c_str());
    println("}\n\n");

    // auto generate functions
    for(unsigned int i = 0; i < _mlist.size(); i++){
        println("int %s::%s_on_receive(Connection *c, ",
                class_name_with_suffix.c_str(),
                _mlist[i].rpc_name.c_str());
        // declare line:
        for(int j = 0; j < _mlist[i].param_list.size(); j++){
            if(1 == _mlist[i].param_list[j].len) // non-array
                print("%s %s",
                       _mlist[i].param_list[j].type_name.c_str(),
                       _mlist[i].param_list[j].var_name.c_str());
            else                // array
                print("%s *%s, int %s_len",
                       _mlist[i].param_list[j].type_name.c_str(),
                       _mlist[i].param_list[j].var_name.c_str(),
                       _mlist[i].param_list[j].var_name.c_str()
                    );
            if(j == _mlist[i].param_list.size() - 1)
                print("){\n"); // last param
            else
                print(", ");
        }
        indent(INDENT_LEN);
        println("return 0;\n");
        indent(-INDENT_LEN);
        println("};\n\n");
    }
        
}

int main(int argc, char** argv)
{
    int ch;
    opterr = 0;                 // in getopt.h
    
    bool outer_src = false;

    bool server_only = true;
    bool client_only = true;

    while(-1 != (ch = getopt(argc, argv, "f:sc"))){
        switch(ch){
        case 'f':
            yyin = fopen(optarg, "r");
            if(!yyin){
                cerr<<"Unable to open file '"<<argv[1]<<"'."<<endl;
                return 1;
            }
            outer_src = true;
            break;
        case 's':
            client_only = false;
            break;
        case 'c':
            server_only = false;
            break;
        default:
            ;
        }
    }

    yyparse(); // read the grammar information

	if(outer_src)
		fclose(yyin);

    char fn[256];
    bzero(fn, sizeof(fn));
    snprintf(fn, sizeof(fn), "%s_srv.cpp", to_lower(class_name).c_str());

    ofstream out_stream;
    ifstream detect_stream;  //detect   file   stream

    if(server_only){
        detect_stream.open(fn);

        if(detect_stream.fail()){// if cpp source is not generated, generate cpp first
            generate_src_one_side(method_list_cli, true);// this is tricky, but it's true
            out_stream.open(fn, ios::out|ios::trunc);
            out_stream<<file_content<<endl;
            out_stream.close();
            detect_stream.close();
        }else{                       // do not do the src file if exist
            ;
        }

        generate_header_one_side(method_list_srv, method_list_cli, true); // this is server side
        bzero(fn, sizeof(fn));
        snprintf(fn, sizeof(fn), "%s_srv.h", to_lower(class_name).c_str());

        out_stream.open(fn, ios::out|ios::trunc);
        out_stream<<file_content<<endl;
        out_stream.close();

    }
    if(client_only){
        // client
        bzero(fn, sizeof(fn));
        snprintf(fn, sizeof(fn), "%s_cli.cpp", to_lower(class_name).c_str());
        detect_stream.open(fn);

        if(detect_stream.fail()){// if cpp source is not generated, generate cpp first
            generate_src_one_side(method_list_srv, false);
            out_stream.open(fn, ios::out|ios::trunc);
            out_stream<<file_content<<endl;
            out_stream.close();
            detect_stream.close();
        }else{                       // do not do the src file if exist
            ;
        }

        generate_header_one_side(method_list_cli, method_list_srv, false);// this is cli side

        bzero(fn, sizeof(fn));
        snprintf(fn, sizeof(fn), "%s_cli.h", to_lower(class_name).c_str());

        out_stream.open(fn, ios::out|ios::trunc);
        out_stream<<file_content<<endl;
        out_stream.close();
    }
//    cout<<file_content<<endl;

	return 0;
}


