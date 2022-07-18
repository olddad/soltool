%option noyywrap nodefault

%{
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "solsend.tab.h"
#include "msg_send.h"
#include <regex.h>

extern int debug_mode;

/* this is for type. yytext is like "<TYPE>" */
/* indeed it's fine to remove this*/
void update_yylval()
{
    yytext[strlen(yytext)-1] = '\0';
    debug("FIELD_TYPE=%s\n", yytext+1);

    strcpy(yylval.str, yytext);
}

void update_subjects()
{
  regex_t regex;
  regcomp (&regex, "(SUBJECT|\\[[0-9]{8}[ ][0-9]{2}:[0-9]{2}:[0-9]{2}\\])[ \t]+([A-Z_/0-9a-z]+)[ \t]*([A-Z_/0-9a-z]*)", REG_EXTENDED);

  regmatch_t groups[4];
  regexec (&regex, yytext, 4, groups, REG_NOTBOL|REG_NOTEOL);

  regfree (&regex);
 
  char subj[1024];
  char reply[1024];

  char* source=yytext;
  int len = strlen(source);

  memset(subj, '\0', 1024);
  memset(reply, '\0', 1024);
  
  if(groups[2].rm_so != -1 && groups[2].rm_so != len) 
     strncpy(subj, source+groups[2].rm_so, groups[2].rm_eo-groups[2].rm_so);
  if(groups[3].rm_so != -1 && groups[3].rm_so != len) 
     strncpy(reply, source+groups[3].rm_so, groups[3].rm_eo-groups[3].rm_so);

  sprintf(yylval.str, "%s%s", subj, reply);
    
  debug("From=%s To=%s\n", subj, reply);
}
%}

%%

^[ \t]*\n {debug("empty line. ignore.\n");}
^#.*\n {debug("comment. ignore.\n");}

"{" {
    debug("msg begin %s\n", yytext);
    return TYPE_MSGBEGIN;
}


"}" {
    debug("msg end %s\n", yytext);
    return TYPE_MSGEND;
}

\n {debug("CR\n"); return EOL;}

("["[0-9]{8}[ ][0-9]{2}:[0-9]{2}:[0-9]{2}"]"|"SUBJECT")[^\n]+ { 
  update_subjects();
  return TIMESTAMP ;
  }

"<"SOLCLIENT_STRING">" {
    update_yylval();
    return TYPE_STRING;
 }

"<"SOLCLIENT_MAP">" {
    update_yylval();
    return TYPE_MSG;
 }

"<"SOLCLIENT_UINT8">" {
    update_yylval();
    return TYPE_U8;
 }

"<"SOLCLIENT_UINT16">" {
    update_yylval();
    return TYPE_U16;
 }

"<"SOLCLIENT_UINT32">" {
    update_yylval();
    return TYPE_U32;
 }

"<"SOLCLIENT_UINT64">" {
    update_yylval();
    return TYPE_U64;
 }

"<"SOLCLIENT_INT8">" {
    update_yylval();
    return TYPE_I8;
 }

"<"SOLCLIENT_INT16">" {
    update_yylval();
    return TYPE_I16;
 }

"<"SOLCLIENT_INT32">" {
    update_yylval();
    return TYPE_I32;
 }

"<"SOLCLIENT_INT64">" {
    update_yylval();
    return TYPE_I32;
 }

"<"SOLCLIENT_FLOAT">" {
    update_yylval();
    return TYPE_F32;
 }

"<"SOLCLIENT_DOUBLE">" {
    update_yylval();
    return TYPE_F64;
 }

"<"SOLCLIENT_BOOL">" {
    update_yylval();
    return TYPE_BOOL;
 }

"<"SOLCLIENT_WCHAR">" {
    update_yylval();
    return TYPE_WCHAR;
 }

"<"SOLCLIENT_BYTEARRAY">" {
    update_yylval();
    return TYPE_BYTEARRAY;
 }

"<"SOLCLIENT_STREAM">" {
    update_yylval();
    return TYPE_STREAM_MSG;
 }

"<"SOLCLIENT_NULL">" {
// not support
 }

"<"SOLCLIENT_DESTINATION">" {
// not support
 }
"<"SOLCLIENT_UNKNOWN">" {
// not support
 }

"<"XML">" {
update_yylval();
return TYPE_XML;
 }

"<"BIN">" {
update_yylval();
return TYPE_BIN;
 }

"<"USER">" {
update_yylval();
return TYPE_USER;
 }

"<"USERPROP">" {
update_yylval();
return TYPE_USERPROP;
 }


"/("[^)]+ { /*this is the value length*/
    debug("STRING=%s\n", yytext+2); 
    strcpy(yylval.str, yytext+2);
    return STRING;
 }

")/"[^\n]* { /*this is for the field value */
    debug("STRING=%s\n", yytext+2); 
    strcpy(yylval.str, yytext+2);
    return STRING;
 }

[^ \t<>()=\/\n]*  { /* this string is for field name subject etc.*/
    debug("STRING=%s\n", yytext); 

    strcpy(yylval.str, yytext);
    return STRING;
  }

. { /*debug("ignore: %d[%c]\n", yytext[0], yytext[0]);*/}

%%

int main(int argc, char **argv)
{
    const char* debug_mode_str = getenv("DEBUG");
    if(debug_mode_str == NULL || strcmp(debug_mode_str, "0") == 0)
       debug_mode=0;
    else
       debug_mode=1;
 
    if(initialize(argc, argv) != 0) {
	printf("error initialize.\n");
	//exit(1);
    }

    yyparse();
    main_loop(); // not need for non-cm msg send

    finalize();

    return 0;
}

void yyerror(char* s)
{
    printf("Error: %s\n", s);
}
