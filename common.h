#ifndef _COMMON_H
# define _COMMON_H
#include "os.h"
#include "solclient/solClient.h"
#include "solclient/solClientMsg.h"

const char* get_type_string(int type);
void check_error(solClient_returnCode_t err, const char* msg);

char* encode_bin(void*, char*, int);
void* decode_bin(char*, void*, int);

void debug(char* fmt, ...);

extern int debug_mode;

// delivery mode for solace message
extern int delivery_mode;
#endif
