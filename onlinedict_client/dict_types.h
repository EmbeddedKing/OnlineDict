#ifndef __DICT_TYPES_H__
#define __DICT_TYPES_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <unistd.h>
#include <sqlite3.h>
#include <time.h>

typedef enum {
    CLI_REGISTER = 0,
    CLI_LOGIN,
    CLI_EXIT,
    CLI_FIND,
    CLI_HISTORY,
    SER_SUCCESS,
    SER_PASSWDERR,
    SER_EXIST,
    SER_NOEXIST,
    SER_HISTORYEND
}com_opt_t;

typedef struct _account {
    char account[20];
    char passwd[20];
}com_account_t;

typedef union {

}com_text_t;

typedef struct _protocol {
    com_opt_t com_opt;
    char username[50];
    char asctime[50];
    com_account_t user;
    char findword[500];
    char findhistoryname[50];
    char findhistoryword[50];
    char findhistorytime[50];
}protocol_t;

#endif
