#include "os.h"
#include <getopt.h>

#include "solclient/solClient.h"
#include "solclient/solClientMsg.h"

// global vars for solaces
static const char* program_name;
char host[256];
char port[20];
char vpn[256];
char user[256];
char password[256];
int delivery_mode = SOLCLIENT_DELIVERY_MODE_DIRECT;

int verbose = 0;
int use_kerberos = 0;

solClient_opaqueContext_pt context_p;
static solClient_opaqueSession_pt session_p;

void parse_args(int argc, char** argv);

/*****************************************************************************
 * messageReceiveCallback
 *
 * The message receive callback function is mandatory for session creation.
 *****************************************************************************/
solClient_rxMsgCallback_returnCode_t
messageReceiveCallback ( solClient_opaqueSession_pt opaqueSession_p, solClient_opaqueMsg_pt msg_p, void *user_p )
{
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
}

/*****************************************************************************
 * main
 * 
 * The entry point to the application.
 *****************************************************************************/
int
initialize ( int argc, char *argv[] )
{
    parse_args(argc, argv);
  
    solClient_context_createFuncInfo_t contextFuncInfo = SOLCLIENT_CONTEXT_CREATEFUNC_INITIALIZER;
	solClient_session_createFuncInfo_t sessionFuncInfo = SOLCLIENT_SESSION_CREATEFUNC_INITIALIZER;
	
    const char     *sessionProps[20];
    int             propIndex = 0;

    /* Message */
    solClient_opaqueMsg_pt msg_p = NULL;
	solClient_opaqueMsg_pt msg2_p;
    solClient_destination_t destination;

    /* solClient needs to be initialized before any other API calls. */
    solClient_initialize ( SOLCLIENT_LOG_DEFAULT_FILTER, NULL );

    /* 
     * Create a Context, and specify that the Context thread be created 
     * automatically instead of having the application create its own
     * Context thread.
     */
    solClient_context_create ( SOLCLIENT_CONTEXT_PROPS_DEFAULT_WITH_CREATE_THREAD,
                               &context_p,
							   &contextFuncInfo,
							   sizeof ( contextFuncInfo ) );

    /*
     * Message receive callback function and the Session event function
     * are both mandatory. In this sample, default functions are used.
     */
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

    sessionProps[propIndex++] = SOLCLIENT_SESSION_PROP_GENERATE_SEND_TIMESTAMPS;
    sessionProps[propIndex++] = SOLCLIENT_PROP_ENABLE_VAL;

    if(use_kerberos){
       sessionProps[propIndex++] = SOLCLIENT_SESSION_PROP_AUTHENTICATION_SCHEME;
       sessionProps[propIndex++] = SOLCLIENT_SESSION_PROP_AUTHENTICATION_SCHEME_GSS_KRB;
    }

    sessionProps[propIndex] = NULL;

    /* Create the Session. */
    solClient_session_create ( ( char ** ) sessionProps,
                               context_p,
                               &session_p, &sessionFuncInfo, sizeof ( sessionFuncInfo ) );

    /* Connect the Session. */
    if(solClient_session_connect ( session_p ) != SOLCLIENT_OK )
       return 1;
		
    return 0;
}

int send_msg(solClient_opaqueMsg_pt msg_p){
	/* Send the message. */
    solClient_session_sendMsg ( session_p, msg_p );
}

int finalize(){
	solClient_session_disconnect ( session_p );
    solClient_cleanup (  );
}

void main_loop()
{
    solClient_returnCode_t rc = SOLCLIENT_OK;
    while ( ( rc = solClient_context_processEvents (context_p) ) != SOLCLIENT_OK );
}

void print_msg(solClient_opaqueMsg_pt msg_p){
	solClient_msg_dump ( msg_p, NULL, 0 ); 
}


void usage()
{
  fprintf(stderr, "Usage: %s options\n", program_name);
  fprintf(stderr,
		  "  -s  --host          VPN host (e.g. localhost).\n"
		  "  -P  --port          VPN port (optional, 55555 by default).\n"
		  "  -n  --vpn           VPN name (e.g. VPN_NAME).\n"
		  "  -u  --user          VPN user name (e.g. someuser).\n"
		  "  -p  --password      VPN password (e.g. somepassword).\n"
		  "  -m  --mode          Delivery Mode: DIRECT, NONPERSISTENT, PERSISTENT\n"
		  "  -h  --help          Display the help.\n"
		  "  -k  --kerberos      Use kerberos schema.\n"
		  "  -v  --verbose       Print verbose messages.\n");
}

void parse_args(int argc, char** argv){
  // Setup for getopt
  int next_option;
  const char* short_options = "s:n:u:p:m:hvP:k";
  const struct option long_options[] = {
	{ "host",		1,	NULL,	's'	},
	{ "port",		1,	NULL,	'P'	},
	{ "vpn",		1,	NULL,	'n' },
	{ "user",		1,	NULL,	'u' },
	{ "password",	1,	NULL,	'p' },
	{ "mode",   	1,	NULL,	'm' },
	{ "help",		0,	NULL,	'h'	},
	{ "verbose",	0,	NULL,	'v'	},
    { "kerberos",   0,  NULL,   'k' },
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
	case 'm':
      if(strcmp(optarg, "DIRECT")==0){
          delivery_mode = SOLCLIENT_DELIVERY_MODE_DIRECT;
      }
      else if(strcmp(optarg, "NONPERSISTENT")==0){
          delivery_mode = SOLCLIENT_DELIVERY_MODE_NONPERSISTENT; 
      }
      else if(strcmp(optarg, "PERSISTENT")==0){
          delivery_mode = SOLCLIENT_DELIVERY_MODE_PERSISTENT; 
      }
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

  // validate parrams
  if((strlen(host)==0 ||strlen(vpn)==0)
    || (strlen(user)==0 && use_kerberos == 0)){
	usage();
	exit(0);
  }	 
  if(strlen(port)==0){
    strcpy(port, "55555");
  }
}
		 
