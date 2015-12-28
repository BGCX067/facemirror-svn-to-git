%{
#include <iostream>
#include <stdlib.h>
#include <string.h>
#include "face.tab.hh"

extern "C"{
int yylex(void);  
}

using namespace std;

%}

pt        int|short|char
vn             [a-zA-Z][a-zA-Z0-9|_]*
%%

rpcset	return ROOT;
^[ ]*s	return SERVER_PUSH;
^[ ]*c	return CLIENT_PULL;
\(		return OPARENSES;
\)		return EPARENSES;
\{		return OBRACE;
\}		return EBRACE;
\[		return OSQUARE;
\]		return ESQUARE;
[0-9]+  yylval.number=atoi(yytext); return NUMBER;
{pt}  	yylval.string=strdup(yytext);return TYPE;
{vn}    yylval.string=strdup(yytext);return VAR_NAME;
,		return COMMA;
\n 	   ;
[\t]+   ;
\/\/	;


%%
