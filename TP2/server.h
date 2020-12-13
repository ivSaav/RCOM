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
#include <sys/stat.h>
#include <fcntl.h>

typedef struct {
    int socketfd;
    int datafd;
    char ip[25];
} ftpData;

#define SERVER_PORT 21
#define SERVER_ADDR "192.168.109.136"

#define SERVER_ADDR2 "193.137.29.15"

#define BUFF_SIZE 255
#define MAX_SIZE 1024

int ftpInit(char *domain);

int ftpConnect(char *domain, int serverPort);

int ftpPassiveMode();

int ftpCommand(int socketfd, char *cmd);
int ftpRead(int socketfd, char *ret);

int ftpLogin(const char *user, const char *pass);

int parsePassiveResponse(char *ip);

int ftpDownload(const char *path);

void ftpClose();