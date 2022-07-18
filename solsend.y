%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "msg_send.h"

// ADD_FIELD is for map attachment
// MACRO: add field to message.
// type: suffix for solClient_container_add* functions.
// msg: the msg object to add field to.
// name: field name
// value: field value to add
#define ADD_FIELD(type, msg, name, value) { debug("add field %s %s %x\n", name, #type, msg); check_error(solClient_container_add##type(msg, value, name), "ERROR: add message to field failed"); }

typedef struct Msgs {
    struct Msgs* prev;
    solClient_opaqueContainer_pt val;  // SDT map obj in msg
} Msgs;

Msgs* cur_msg = NULL;
solClient_opaqueMsg_pt msg_p = NULL; // only one message object for each rec

char msgid[255];

Msgs* push_map(const char* name){
	debug("new sub msg to %x\n", cur_msg->val);
	Msgs* stack = (Msgs*)malloc(sizeof(Msgs));
	stack->prev = cur_msg;
	solClient_container_openSubMap(cur_msg->val, &stack->val, name);

	cur_msg = stack;
	debug("prev: %x  cur: %x\n", cur_msg->prev->val, cur_msg->val);
	return cur_msg->prev;
}

void pop_map(){
	if(cur_msg == NULL) return;

	if(cur_msg->prev == NULL) {
		debug("free cur_msg\n");
		free(cur_msg);
		cur_msg = NULL;
		return;
    }

	// close the stream
	solClient_container_closeMapStream(&cur_msg->val);

	debug("go to upper level %x.\n", cur_msg->prev->val);
	
	Msgs* pop = cur_msg;
	cur_msg = cur_msg->prev;

	free(pop);
}

Msgs* push_stream(const char* name){
	debug("new sub stream msg to %x\n", cur_msg->val);
	Msgs* stack = (Msgs*)malloc(sizeof(Msgs));
	stack->prev = cur_msg;

	solClient_container_openSubStream( cur_msg->val, &stack->val ,name);

	solClient_opaqueContainer_pt stream_p = stack->val;

	// add op flag in stream. ?? not very sure how to use this
	ADD_FIELD(Int32, stream_p, NULL, 1);

	cur_msg = stack;

	// push container map
	push_map(NULL);
	
	debug("prev: %x  cur: %x\n", cur_msg->prev->val, cur_msg->val);
	return cur_msg->prev;
}

void pop_stream(){
	if(cur_msg == NULL) return;
	debug("go to upper level %x.\n", cur_msg->prev->val);

	// popup container map
	pop_map();
	
	// close the stream
	solClient_container_closeMapStream(&cur_msg->val);
	
	if(cur_msg->prev == NULL) {
		debug("free cur_msg\n");
		free(cur_msg);
		cur_msg = NULL;
		return;
    }
	
	Msgs* pop = cur_msg;
	cur_msg = cur_msg->prev;

	free(pop);
}

Msgs* push_user_map(const char* name){
	debug("new user map %x\n", cur_msg->val);
	Msgs* stack = (Msgs*)malloc(sizeof(Msgs));
	stack->prev = cur_msg;
	cur_msg = stack;
	
	char            map[1024];
	solClient_container_createMap ( &cur_msg->val, map, sizeof ( map ) );
	debug("new user prop map %x.\n", cur_msg->val);
	
	return cur_msg->prev;
}
void pop_user_map(){
	if(cur_msg == NULL) return;
	debug("done user map. go to upper level %x.\n", cur_msg->prev->val);

	solClient_msg_setUserPropertyMap(msg_p, cur_msg->val);
	solClient_container_closeMapStream (&cur_msg->val );
	
	if(cur_msg->prev == NULL)
		return;
	
	Msgs* pop = cur_msg;
	cur_msg = cur_msg->prev;
		
	free(pop);
}

void init_map(){
    cur_msg = (Msgs*)malloc(sizeof(Msgs));
	cur_msg->prev=NULL;
    cur_msg->val = NULL;
	
    debug("add root Msgs node %x\n", cur_msg);
}

void init()
{
    debug("new trade. init... \n");

	while(cur_msg != NULL) pop_map(); 
	init_map();
	
    if(msg_p != NULL) solClient_msg_free ( &msg_p );

    solClient_msg_alloc ( &msg_p );
    solClient_msg_setDeliveryMode ( msg_p, delivery_mode);

	// create default map
	solClient_msg_createBinaryAttachmentMap(msg_p, &cur_msg->val, 1024);
	debug("new default map  %x.\n", cur_msg->val);
}

char* get_msg_id(){
    char host[32];
    gethostname(host, 32);

    sprintf(msgid, "solsend:%s.%d.%ld", host, getpid(), time(NULL));
    return msgid;
}

int send_trade(char* send_subj, char* reply_subj)
{
	solClient_returnCode_t err;
	
     /* extern char* replaced_subj; */
     /* if(replaced_subj != NULL && replaced_subj[0] != '\0') */
     /* { */
	 /*  send_subj = replaced_subj; */
     /* } */

     printf("Subject: %s \tReplySubject: %s\n", send_subj, reply_subj == NULL?"":reply_subj);
	
     solClient_destination_t destination;
	 destination.destType = SOLCLIENT_TOPIC_DESTINATION;
     destination.dest = send_subj;
     err = solClient_msg_setDestination ( msg_p, &destination, sizeof ( destination ) );

     // other settings
     err = solClient_msg_setApplicationMessageId( msg_p, get_msg_id());

     check_error(err, "send_trade: Failed to set the send subject in the message");

     print_msg(msg_p);
     
     send_msg(msg_p);
     printf("\n");

	 solClient_msg_free ( &msg_p );
     return 0;
}

%}
%union 
{
     char str[10240];
}

%token EOL 
%token <str> TIMESTAMP 
%token <str> TYPE_BEGIN TYPE_MSG TYPE_I8 TYPE_I16 TYPE_I32 TYPE_I64 TYPE_STRING  TYPE_F32 TYPE_F64 TYPE_U8 TYPE_U16 TYPE_U32 TYPE_U64 TYPE_BOOL TYPE_DATETIME TYPE_WCHAR TYPE_XML TYPE_BIN TYPE_USER TYPE_USERPROP TYPE_MSGBEGIN TYPE_MSGEND TYPE_STREAM_MSG TYPE_BYTEARRAY TYPE_END
%token <str> STRING

%type <str> fields trade 
%%
trade: {} 
| trade TIMESTAMP {init();} EOL fields {
    char* sep_pos = strchr($2, '');
    *sep_pos='\0'; 
    send_trade($2, sep_pos+1);
}
;
fields: {}
| fields STRING TYPE_I8     STRING STRING EOL { ADD_FIELD(Int8, cur_msg->val, $2, atoi($5));}
| fields STRING TYPE_I16    STRING STRING EOL { ADD_FIELD(Int16,cur_msg->val, $2, atoi($5));}
| fields STRING TYPE_I32    STRING STRING EOL { ADD_FIELD(Int32,cur_msg->val, $2, atoi($5));}
| fields STRING TYPE_I64    STRING STRING EOL { ADD_FIELD(Int64,cur_msg->val, $2, atol($5));}
| fields STRING TYPE_U8     STRING STRING EOL { ADD_FIELD(Uint8, cur_msg->val, $2, atoi($5));}
| fields STRING TYPE_U16    STRING STRING EOL { ADD_FIELD(Uint16,cur_msg->val, $2, atoi($5));}
| fields STRING TYPE_U32    STRING STRING EOL { ADD_FIELD(Uint32,cur_msg->val, $2, atoi($5));}
| fields STRING TYPE_U64    STRING STRING EOL { ADD_FIELD(Uint64,cur_msg->val, $2, atol($5));}
| fields STRING TYPE_F32    STRING STRING EOL { ADD_FIELD(Float,cur_msg->val, $2, atof($5));}
| fields STRING TYPE_F64    STRING STRING EOL { ADD_FIELD(Double,cur_msg->val, $2, atof($5));}
| fields STRING TYPE_WCHAR  STRING STRING EOL { ADD_FIELD(Char,cur_msg->val, $2, $5[0]);}
| fields STRING TYPE_BOOL   STRING STRING EOL { ADD_FIELD(Boolean, cur_msg->val, $2,strcmp($5, "false"));}
| fields STRING TYPE_STRING STRING EOL        { ADD_FIELD(String, cur_msg->val, $2, "");}
| fields STRING TYPE_STRING STRING STRING EOL { ADD_FIELD(String, cur_msg->val, $2, $5);}

| fields STRING TYPE_BIN STRING STRING EOL {
	solClient_uint32_t size = atol($4);
    char* bin_buf = (char*)malloc(sizeof(char)*(size+1));

    solClient_msg_setBinaryAttachment(msg_p, decode_bin($5, bin_buf, size), size);
	
	free(bin_buf);
}
| fields STRING TYPE_BYTEARRAY STRING STRING EOL {
	solClient_uint8_t size = atol($4);
    char* bin_buf = (char*)malloc(sizeof(char)*(size+1));

    solClient_container_addByteArray(cur_msg->val, decode_bin($5, bin_buf, size), size, $5);
	
	free(bin_buf);
}				
| fields STRING TYPE_XML STRING STRING EOL {
	solClient_uint32_t size = atol($4);
    solClient_msg_setXml(msg_p, $5, size);
}
| fields STRING TYPE_USER STRING STRING EOL {
	solClient_uint32_t size = atol($4);
    char* bin_buf = (char*)malloc(sizeof(char*)*(size+1));
    solClient_msg_setUserData(msg_p, decode_bin($5, bin_buf, size), size);
	free(bin_buf);
}
| fields STRING TYPE_USERPROP STRING  STRING EOL {push_user_map($2);} TYPE_MSGBEGIN EOL fields TYPE_MSGEND {pop_user_map();} EOL 

| fields STRING TYPE_MSG STRING STRING EOL {push_map($2);} TYPE_MSGBEGIN EOL fields TYPE_MSGEND {pop_map();} EOL 
| fields STRING TYPE_MSG {push_map($2);} EOL TYPE_MSGBEGIN EOL fields TYPE_MSGEND {pop_map();} EOL
| fields STRING TYPE_STREAM_MSG STRING STRING EOL {push_stream($2);} TYPE_MSGBEGIN EOL fields TYPE_MSGEND {pop_stream();} EOL
;
