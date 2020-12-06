#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <netdb.h>
#include <string.h>

#define SERVER_PORT 21
#define SERVER_ADDR "193.137.29.15"

#define BUFF_SIZE 255

int initConnection(char *domain, int socketfd,  int serverPort);

int sendCommand(int socketfd, char *cmd);

int getResponse(int socketfd, char *res);
int readResponse(int socketfd, char *ret);

int userLogin(int socketfd, const char *user, const char *pass);

int parsePassiveResponse(int socketfd, char *ip);