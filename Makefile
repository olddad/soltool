SOLCLIENT_DIR = $(SOLCLIENTDIR)

CFLAGS += -I$(SOLCLIENT_DIR)/include -I. -g -std=gnu99
LDFLAGS += -L$(SOLCLIENT_DIR)/lib -lsolclient 
CC=gcc

all: sollisten solsend

clean:
	 rm -f sollisten solsend *.o *.tab.c *.tab.h solsend.c

%.tab.c:%.y
	bison -d -osolsend.tab.c solsend.y

%.c:%.l %.tab.c
	flex -osolsend.c solsend.l

solsend.c: solsend.tab.c msg_send.h solsend.tab.h
solsend: solsend.o solsend.tab.o msg_send.o common.o
sollisten: sollisten.o common.o
