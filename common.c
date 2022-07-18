#include "common.h"
#include <stdlib.h>

const char* get_type_string(int type){
	static const char* type_map[20];
	type_map[0]		= "SOLCLIENT_BOOL";
	type_map[1]		= "SOLCLIENT_UINT8";
	type_map[2]		= "SOLCLIENT_INT8";
	type_map[3]		= "SOLCLIENT_UINT16";
	type_map[4]		= "SOLCLIENT_INT16";
	type_map[5]		= "SOLCLIENT_UINT32";
	type_map[6]		= "SOLCLIENT_INT32";
	type_map[7]		= "SOLCLIENT_UINT64";
	type_map[8]		= "SOLCLIENT_INT64";
	type_map[9]		= "SOLCLIENT_WCHAR";
	type_map[10]	= "SOLCLIENT_STRING";
	type_map[11]	= "SOLCLIENT_BYTEARRAY";
	type_map[12]	= "SOLCLIENT_FLOAT";
	type_map[13]	= "SOLCLIENT_DOUBLE";
	type_map[14]	= "SOLCLIENT_MAP";
	type_map[15]	= "SOLCLIENT_STREAM";
	type_map[16]	= "SOLCLIENT_NULL";
	type_map[17]	= "SOLCLIENT_DESTINATION";
	type_map[18]	= "SOLCLIENT_SMF";
	type_map[19]	= "SOLCLIENT_UNKNOWN";

	return type_map[type];
}

void check_error(solClient_returnCode_t err, const char* msg)
{
    if (err != SOLCLIENT_OK)
    {
                solClient_errorInfo_pt errorInfo_p = solClient_getLastErrorInfo();
                fprintf ( stderr, "Error: %s: subCode %s, responseCode %d, reason %s\n",
                                  msg,
                                  solClient_subCodeToString ( errorInfo_p->subCode ),
                                  errorInfo_p->responseCode,
                                  errorInfo_p->errorStr );

        exit(1);
     }
}


char* encode_bin(void* in, char* out, int in_size){
	char hex[] = {'0','1','2','3','4','5','6','7','8','9','a', 'b', 'c', 'd', 'e', 'f'};
	unsigned char* pt = (char*)in;
	unsigned char* pend = ((char*)in) + in_size;
	char* pout = out;

	while(pt != pend){
		int h = ((*pt)>>4)&0xf;
		int l = ((*pt)<<4>>4)&0xf;
		*(pout++) = hex[h];
		*(pout++) = hex[l];
		*(pout++) = ' ';

		pt++;
	}
	*(pout-1)='\0';
	
	return out;
}

void* decode_bin(char* in, void* out, int in_size){
	unsigned char* pt = (char*)in;
	unsigned char* pend = ((char*)in) + in_size*3-1;
	char* pout = out;

	while(pt < pend){		
		int h = pt[0] >= 'a'?pt[0] - 'a' + 10: pt[0] - '0';
		int l = pt[1] >= 'a'?pt[1] - 'a' + 10: pt[1] - '0';
		*(pout++) = (h <<4)|l;
		pt+=3;
	}
	*(pout)='\0';
	
	return out;
}

int debug_mode = 0;
void debug(char* fmt, ...)
{
    if(debug_mode == 0) return;

   va_list ap;
   va_start(ap, fmt);

   printf("DEBUG:  "); 
   fflush(stdout);
   vfprintf(stdout, fmt, ap);
}
