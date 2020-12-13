#include <stdio.h>  
#include <stdlib.h>
#include <string.h>

#define USER_START_INDEX 6

typedef struct {
	char *user;
    char *password;
    char *host;
    char* url_path;
} arg_url;

arg_url parseUrl(char* url);