#include "os.h"
#include <time.h>
#include "common.h"
#include <ctype.h>
#include <signal.h>
#include <getopt.h>

#include "solclient/solClient.h"
#include "solclient/solClientMsg.h"

static const char* program_name;
char host[256];
char port[10];
char vpn[256];
char user[256];
char password[256];

int verbose = 0;
int use_kerberos = 0;

int topic_count = 0;
int topic_ind=0;

// running flag
int running = 1;

const char sep = '/';
const int PRECISION = 6;
const int PREFIX_COUNT = 8;

void dump_msg(solClient_opaqueMsg_pt msg_p, int prefix_level);
void dump_map(solClient_opaqueContainer_pt, int);

/**
 * Signal handler.
 */
void sig_handler(int signo)
{
  fprintf(stderr, "signal received\n");
  running = 0;
}

const char* get_prefix_str(int level){
  static char prefix[1024];
  sprintf(prefix, "%*s", level*PREFIX_COUNT, "");
  return prefix;
}

void dump_unknown(solClient_opaqueMsg_pt msg_p){
  fprintf(stderr, "#: ");
  solClient_msg_dump(msg_p, NULL, 0);
}

void dump_bytearray(solClient_uint8_t* barray, int size){
  char* tmp = (char*)malloc(sizeof(char) *(size +1)*3+1);
  printf("%s\n", encode_bin(barray, tmp, size));
  free(tmp);

  printf("#");
  for(int i=0;i<size;i++){
  	printf("%c ", isprint(barray[i])? barray[i]: ' ');
  }
  printf("\n");
}

void dump_stream(solClient_opaqueContainer_pt stream_p, int level){

  solClient_returnCode_t rc = SOLCLIENT_OK;
  /* Get the operation, operand1 and operand2 from the stream. */
  solClient_int32_t operation = -1;
  if ( ( rc = solClient_container_getInt32 ( stream_p, &operation, NULL ) ) != SOLCLIENT_OK ) {
	// discard it
	return;
  }

  // get map content
  solClient_field_t field_p;
  size_t size = sizeof(field_p);
  if((rc = solClient_container_getField(stream_p, &field_p, size, NULL))!=SOLCLIENT_OK)
	return;
  
  dump_map(field_p.value.map, level);
}
void dump_bin(solClient_opaqueMsg_pt msg_p){
  void* data;
  solClient_uint32_t size;
  solClient_returnCode_t rtn = solClient_msg_getBinaryAttachmentPtr(msg_p, &data, &size);

  // not a bin message
  if(rtn != SOLCLIENT_OK){
	dump_unknown(msg_p);
	return;
  }

  char* tmp = (char*)malloc(sizeof(char) *(size +1)*3+1);
  printf("Data = <BIN>/(%d)/%s\n", size, encode_bin(data, tmp, size));

  //print char text for reading, this is comment in output message
  char* p = (char*)data;
  printf("#Data: ");
  for(int i=0;i<size;i++){
	if(isprint(*(p+i)))
	  printf("%c",*(p+i));
  }
  printf("\n");
	
  free(tmp);
}

void dump_xml(solClient_opaqueMsg_pt msg_p){
  solClient_returnCode_t rtn;
	
  void* bufPtr_p;
  solClient_uint32_t size;
  rtn = solClient_msg_getXmlPtr(msg_p, &bufPtr_p, &size);

  // no xml data
  if(rtn != SOLCLIENT_OK){
	return;
  }
	
  // there's no '\0' suffix
  printf("Data = <XML>/(%d)/", size);
  for(int i=0;i<size;i++)
	printf("%c", ((char*)bufPtr_p)[i]);
  printf("\n");
}

// user data
void dump_user(solClient_opaqueMsg_pt msg_p){
  solClient_returnCode_t rtn;
	
  void* bufPtr_p;
  solClient_uint32_t size;
  rtn = solClient_msg_getUserDataPtr(msg_p, &bufPtr_p, &size);

  // not a text message
  if(rtn != SOLCLIENT_OK){
	return;
  }

  char* tmp = (char*)malloc((size+1)*3*sizeof(char));
  printf("Data = <USER>/(%d)/%s\n", size, encode_bin(bufPtr_p, tmp, size));
  free(tmp);
}

// user data
void dump_user_prop(solClient_opaqueMsg_pt msg_p){
  solClient_returnCode_t rtn;

  solClient_opaqueContainer_pt map_p ;
  rtn = solClient_msg_getUserPropertyMap(msg_p, &map_p);
	
  // no user prop map
  if(rtn != SOLCLIENT_OK){
	return;
  }

  size_t size;
  solClient_container_getSize(map_p, &size);
	
  printf("Data = <USERPROP>/(%d)/\n{\n", size);
  dump_map(map_p, 1);
  printf("}\n");
}

void dump_msg(solClient_opaqueMsg_pt msg_p, int prefix_level){
  solClient_destination_t dest_p, dest_reply_p;

  solClient_returnCode_t rtn = solClient_msg_getReplyTo(msg_p, &dest_reply_p, sizeof(dest_reply_p));
  const char* reply = "";
  if(rtn != SOLCLIENT_NOT_FOUND)
	reply = dest_reply_p.dest;		
	
  rtn = solClient_msg_getDestination(msg_p, &dest_p, sizeof(dest_p));
  if(rtn != SOLCLIENT_OK){
	printf("cannot get dest\n");
	return;
  }
	
  if(prefix_level == 0) {
	char        timestamp[30];
	time_t tp = time(0);
	struct tm *tstr = localtime(&tp); 
	strftime(timestamp, 30,  "[%Y%m%d %H:%M:%S]", tstr );
		
	printf("%s %s %s\n",timestamp, dest_p.dest, reply);
  }

  // dump appid
  if(getenv("SOLLISTEN_SHOWMSGID") != NULL){
     const char* appid;
     solClient_msg_getApplicationMessageId(msg_p, &appid);
     printf("# MessageID: %s\n", appid);
  }

  // dump timestamp
  if(getenv("SOLLISTEN_SHOWSENDTIME") != NULL){
     solClient_int64_t sendtime;
     solClient_msg_getSenderTimestamp(msg_p, &sendtime);

	 time_t tp = (time_t)(sendtime/1000);
	 struct tm *tstr = localtime(&tp); 
	 printf("# Send Time: %s\n", asctime(tstr));
  }

  ////////////////////////////////////////////////////////////////////////////////
  // check user prop map
  dump_user_prop(msg_p);
	
  ////////////////////////////////////////////////////////////////////////////////
  // check user data
  dump_user(msg_p);
	
  ////////////////////////////////////////////////////////////////////////////////
  // check XML msg
  dump_xml(msg_p);
	
  ////////////////////////////////////////////////////////////////////////////////
  // check binary attachment msg. Only one binary attachment in the message.
	
  //solClient_msg_dump ( msg_p, NULL, 0 );							 
  solClient_opaqueContainer_pt map_p;
  rtn = solClient_msg_getBinaryAttachmentMap(msg_p, &map_p);

  // not a map msg, try binary attachment
  if(rtn != SOLCLIENT_OK) {
	// try to check text msg
	dump_bin(msg_p);
	return;
  }
	
  dump_map(map_p, prefix_level);

  fflush(stdout);
}

void dump_map(solClient_opaqueContainer_pt map_p, int prefix_level){
  if(map_p == NULL)
	return;
	
  const char* name;
  solClient_field_t field_p;
  size_t size = sizeof(field_p);
  solClient_returnCode_t rtn;
	
  while(solClient_container_hasNextField(map_p) != 0){
	rtn = solClient_container_getNextField(map_p, &field_p, size, &name);		
	if(rtn != SOLCLIENT_OK){
	  break;
	}

	printf("%s%-25s = <%s>%c(%d)%c", get_prefix_str(prefix_level),
		   (name==NULL?"Map":name), get_type_string(field_p.type), sep, field_p.length , sep);
	switch(field_p.type){
	case SOLCLIENT_BOOL       ://0
	  //solClient_container_getBoolean(map_p, 
	  printf("%s\n", field_p.value.boolean == 0?"false":"true");
	  break;
	case SOLCLIENT_UINT8      ://1
	  printf("%u\n", field_p.value.uint8);
	  break;
	case SOLCLIENT_INT8       ://2
	  printf("%d\n", field_p.value.int8);
	  break;
	case SOLCLIENT_UINT16     ://3
	  printf("%u\n", field_p.value.uint16);
	  break;
	case SOLCLIENT_INT16      ://4
	  printf("%d\n", field_p.value.int16);
	  break;
	case SOLCLIENT_UINT32     ://5
	  printf("%u\n", field_p.value.uint32);
	  break;
	case SOLCLIENT_INT32      ://6
	  printf("%d\n", field_p.value.int32);
	  break;
	case SOLCLIENT_UINT64     ://7
	  printf("%ld\n", field_p.value.uint64);
	  break;
	case SOLCLIENT_INT64      ://8
	  printf("%ld\n", field_p.value.int64);
	  break;
	case SOLCLIENT_WCHAR      ://9
	  printf("%c\n", field_p.value.wchar);
	  break;
	case SOLCLIENT_STRING     ://10
	  printf("%s\n", field_p.value.string);
	  break;
	case SOLCLIENT_BYTEARRAY  ://11
	  {
		dump_bytearray(field_p.value.bytearray, field_p.length);
		break;
	  }
	  break;
	case SOLCLIENT_FLOAT      ://12
	  printf("%.*f\n", PRECISION, field_p.value.float32);
	  break;
	case SOLCLIENT_DOUBLE     ://13
	  printf("%.*lf\n", PRECISION, field_p.value.float64);
	  break;
	case SOLCLIENT_MAP        ://14
	  {
		printf("\n%s{\n", get_prefix_str(prefix_level) );
				
		dump_map(field_p.value.map, prefix_level+1);
				
		printf("%s}\n", get_prefix_str(prefix_level));
				
		break;
	  }
	case SOLCLIENT_STREAM     ://15
	  {
		printf("\n%s{\n", get_prefix_str(prefix_level) );
				
		dump_stream(field_p.value.stream, prefix_level+1);
				
		printf("%s}\n", get_prefix_str(prefix_level));
				
		break;
	  }
	case SOLCLIENT_NULL       ://16
	  printf("null\n");
	  break;
	case SOLCLIENT_DESTINATION://17
	  printf("%s\n", field_p.value.dest.dest);
	  break;
	case SOLCLIENT_SMF        ://18
	  dump_msg(field_p.value.smf, prefix_level+1);
	  break;
	case SOLCLIENT_UNKNOWN : //-1
	  break;
	} 
  }
}

solClient_rxMsgCallback_returnCode_t
messageReceiveCallback ( solClient_opaqueSession_pt opaqueSession_p, solClient_opaqueMsg_pt msg_p, void *user_p )
{	
  dump_msg(msg_p, 0);
  printf ( "\n" );

  return SOLCLIENT_CALLBACK_OK;
}

/*****************************************************************************
 * eventCallback
 *
 * The event callback function is mandatory for session creation.
 *****************************************************************************/
void
eventCallback ( solClient_opaqueSession_pt opaqueSession_p,
                solClient_session_eventCallbackInfo_pt eventInfo_p, void *user_p )
{
  if(verbose)
	fprintf(stderr, "Session event_callback() called:  %s\n", solClient_session_eventToString(eventInfo_p->sessionEvent));
}

void usage()
{
  fprintf(stderr, "Usage: %s options topics\n", program_name);
  fprintf(stderr,
		  "  -s  --host          VPN host (e.g. localhost).\n"
		  "  -P  --port          VPN port (optional, 55555 by default ).\n"
		  "  -n  --vpn           VPN name (e.g. VPN_NAME).\n"
		  "  -u  --user          VPN user name (e.g. someuser).\n"
		  "  -p  --password      VPN password (e.g. somepassword).\n"
		  "  -h  --help          Display the help.\n"
          "  -k  --kerberos      Use kerberos schema.\n"
		  "  -v  --verbose       Print verbose messages.\n"
		  "  topics         Topic to subscribe (e.g. \"TEST/>\").\n");  
}

void parse_args(int argc, char** argv){
  // Setup for getopt
  int next_option;
  const char* short_options = "s:n:u:p:hvkP:";
  const struct option long_options[] = {
	{ "host",		1,	NULL,	's'	},
	{ "port",		1,	NULL,	'P'	},
	{ "vpn",		1,	NULL,	'n' },
	{ "user",		1,	NULL,	'u' },
	{ "password",	1,	NULL,	'p' },
	{ "help",		0,	NULL,	'h'	},
	{ "verbose",	0,	NULL,	'v'	},
	{ "kerberos",	0,	NULL,	'k'	},
	{ NULL,			0,	NULL,	0	} /* End */
  };
  
  // Get the program name first
  program_name = argv[0];

  // Parse the options using getopt
  int option_index = 0;
  do {
	next_option = getopt_long(argc, argv, short_options, long_options, &option_index);

	switch(next_option) {
	case 's':
	  strncpy(host, optarg, sizeof(host));
	  break;
	case 'P':
	  strncpy(port, optarg, sizeof(port));
	  break;
	case 'n':
	  strncpy(vpn, optarg, sizeof(vpn));
	  break;
	case 'u':
	  strncpy(user, optarg, sizeof(user));
	  break;
	case 'p':
	  strncpy(password, optarg, sizeof(password));
	  break;
	case 'v':
	  verbose = 1;
	  break;
	case 'k':
	  use_kerberos = 1;
	  break;
	case 'h':
	  usage();
	  exit(0);
	}
  } while(next_option != -1);

  topic_ind = optind;
  topic_count=argc - topic_ind;

  // validate parrams
  if((strlen(host)==0 ||strlen(vpn)==0 ||topic_count < 1)
     || (strlen(user)==0 && use_kerberos ==0)) {
	usage();
	exit(0);
  }
  if(strlen(port) == 0){
    strcpy(port, "55555");
  }
}
		 
/*****************************************************************************
 * main
 * 
 * The entry point to the application.
 *****************************************************************************/
int
main ( int argc, char *argv[] )
{
  // Setup signal handler first
  if (signal(SIGINT, sig_handler) == SIG_ERR) {
	fprintf(stderr, "Couldn't catch SIGINT\n");
  }
  if (signal(SIGTERM, sig_handler) == SIG_ERR) {
	fprintf(stderr, "Couldn't catch SIGTERM\n");
  }

  parse_args(argc, argv);
    
  /* Context */
  solClient_opaqueContext_pt context_p;
  solClient_context_createFuncInfo_t contextFuncInfo = SOLCLIENT_CONTEXT_CREATEFUNC_INITIALIZER;

  /* Session */
  solClient_opaqueSession_pt session_p;
  solClient_session_createFuncInfo_t sessionFuncInfo = SOLCLIENT_SESSION_CREATEFUNC_INITIALIZER;

  /* Session Properties */
  const char     *sessionProps[20];
  int             propIndex = 0;

  /* solClient needs to be initialized before any other API calls. */
  solClient_initialize ( SOLCLIENT_LOG_DEFAULT_FILTER, NULL );

  // Create a Context
  solClient_context_create ( SOLCLIENT_CONTEXT_PROPS_DEFAULT_WITH_CREATE_THREAD,
							 &context_p, &contextFuncInfo, sizeof ( contextFuncInfo ) );

  /* Configure the Session function information. */
  sessionFuncInfo.rxMsgInfo.callback_p = messageReceiveCallback;
  sessionFuncInfo.rxMsgInfo.user_p = NULL;
  sessionFuncInfo.eventInfo.callback_p = eventCallback;
  sessionFuncInfo.eventInfo.user_p = NULL;

  /* Configure the Session properties. */
  propIndex = 0;
  sessionProps[propIndex++] = SOLCLIENT_SESSION_PROP_HOST;
  sessionProps[propIndex++] = host;

  sessionProps[propIndex++] = SOLCLIENT_SESSION_PROP_PORT;
  sessionProps[propIndex++] = port;

  sessionProps[propIndex++] = SOLCLIENT_SESSION_PROP_VPN_NAME;
  sessionProps[propIndex++] = vpn;


  if(!use_kerberos){
    sessionProps[propIndex++] = SOLCLIENT_SESSION_PROP_USERNAME;
    sessionProps[propIndex++] = user;
  }

  if(strlen(password) != 0){
	sessionProps[propIndex++] = SOLCLIENT_SESSION_PROP_PASSWORD;
	sessionProps[propIndex++] = password;
  }
 
  if(use_kerberos){
     sessionProps[propIndex++] = SOLCLIENT_SESSION_PROP_AUTHENTICATION_SCHEME;
     sessionProps[propIndex++] = SOLCLIENT_SESSION_PROP_AUTHENTICATION_SCHEME_GSS_KRB;
  } 
  sessionProps[propIndex] = NULL;

  /* Create the Session. */
  solClient_session_create ( ( char ** ) sessionProps,
							 context_p,
							 &session_p, &sessionFuncInfo, sizeof ( sessionFuncInfo ) );

  if(solClient_session_connect ( session_p ) != SOLCLIENT_OK)
    goto END;

  //subscription
  for(int i=0; i<topic_count; i++){
	fprintf(stderr, "listening %s\n", argv[topic_ind + i]);
	solClient_session_topicSubscribeExt ( session_p,
										  SOLCLIENT_SUBSCRIBE_FLAGS_WAITFORCONFIRM,
										  argv[topic_ind + i]);
  }

  fflush ( stdout );
  while (running) {
	SLEEP ( 1 );
  }

  //cleanup
  for(int i=topic_ind; i<topic_count; i++)
	solClient_session_topicUnsubscribeExt ( session_p,
											SOLCLIENT_SUBSCRIBE_FLAGS_WAITFORCONFIRM,
											argv[topic_ind + i] );
END:
  solClient_session_disconnect ( session_p );
  solClient_cleanup (  );

  return 0;
}
