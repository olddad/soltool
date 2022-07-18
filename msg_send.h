#ifndef _MSG_SEND_H
# define _MSG_SEND_H
#include "os.h"
#include "solclient/solClient.h"
#include "solclient/solClientMsg.h"
#include "common.h"

int send_msg(solClient_opaqueMsg_pt  msg);
int initialize(int argc, char **argv);
void main_loop();
int finalize();

void print_msg(solClient_opaqueMsg_pt msg);
     
int yyparse();
int yylex();
void yyerror(char*);

#endif
