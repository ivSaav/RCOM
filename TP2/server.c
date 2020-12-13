#include "server.h"

static ftpData ftp;

int ftpInit(const char *hostname) {

    struct hostent *h;

    if ((h=gethostbyname(hostname)) == NULL) {  
            herror("gethostbyname");
            exit(1);
    }

    printf("Host name  : %s\n", h->h_name);
    printf("IP Address : %s\n",inet_ntoa(*((struct in_addr *)h->h_addr)));

    char *ip = inet_ntoa(*((struct in_addr *)h->h_addr));
    
    if ((ftp.socketfd = ftpConnect(ip, FTP_PORT)) < 0) {
        perror("ftpConnect");
        exit(1);
    }

    if (ftpRead(ftp.socketfd, NULL)) {
        perror("ftpRead");
        exit(1);
    }

    return 0;
}



int ftpConnect(char *ip, int serverPort) {
    
    struct	sockaddr_in server_addr;

    int socketfd;

    /*server address handling*/
	bzero((char*)&server_addr,sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(ip);	/*32 bit Internet address network byte ordered*/
	server_addr.sin_port = htons(serverPort);		/*server TCP port must be network byte ordered */

    /*open a TCP socket*/
	if ((socketfd = socket(AF_INET,SOCK_STREAM,0)) < 0) {
    		perror("socket()");
        	exit(0);
    	}
	printf("oppened socket \n");
    
    printf("connecting to server...\n");
	/*connect to the server*/
	if(connect(socketfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0){
        perror("connect()");        /*ftp://ftp.up.pt/pub/*/
		exit(1);
	}
	printf("connection established\n");

    return socketfd;
}

int ftpPassiveMode() {

	if (ftpCommand(ftp.socketfd, "pasv\r\n")) {
		perror("pasv");exit(1);
	}

    char *ip = (char*) malloc(sizeof(char)*20);
	int port = parsePassiveResponse(ip);

	printf("ip %s %d \n", ip, port);

    if ((ftp.datafd = ftpConnect(ip, port)) < 0) {
        perror("ftpConnect");
        exit(1);
    }

    return 0;

}

int parsePassiveResponse( char *ip) {

    char *res = (char*) malloc(sizeof(char)*512);
	if (ftpRead(ftp.socketfd, res)) {
        perror("ftpRead");
        exit(1);
    }

    int ip1, ip2, ip3, ip4;
    int port1, port2;
    char *first;
    first = strrchr(res, '(');
	if (sscanf(first, "(%d,%d,%d,%d,%d,%d)", &ip1, &ip2, &ip3, &ip4, &port1, &port2) < 0) {
        perror("scanf");
        exit(1);
    }
	sprintf(ip, "%d.%d.%d.%d", ip1, ip2, ip3, ip4);

	return port1 * 256 + port2;

}

int ftpCommand(int socketfd, char* cmd) {

    int n;
    if ((n = send(socketfd, cmd, strlen(cmd), 0)) < 0){
        perror("Couldn't send command");
        exit(1);
    }

    if (n == 0) {
        perror("Connection closed");
        exit(2);
    }

    // printf("> %s\n", cmd);
    return 0;
}


int ftpRead(int socketfd, char *ret) {

    FILE* fp = fdopen(socketfd, "r");
	size_t n = 0;
    char *buff;
    while (1){
        getline(&buff, &n, fp);
            printf("%s", buff);

        if (buff[3] == ' ')
            break;
    }
    
    if (ret != NULL)
        strcpy(ret, buff);

    return 0;
}

int ftpLogin(const char *user, const char *pass) {

    char cmd[BUFF_SIZE];
    sprintf(cmd, "user %s\r\n", user);
    if (ftpCommand(ftp.socketfd, cmd)) {
        perror("user login");
        exit(1);
    }

	if (ftpRead(ftp.socketfd, NULL)) {
        perror("ftpRead");
        exit(1);
    }

    memset(cmd, 0, BUFF_SIZE);

    sprintf(cmd, "pass %s\r\n", pass);
    if (ftpCommand(ftp.socketfd, cmd)) {
        perror("user pass");
        exit(1);
    }
	if(ftpRead(ftp.socketfd, NULL)) {
        perror("ftpRead");
        exit(1);
    }

    return 0;
}

int ftpDownload(const char *path) {

    char cmd[255];
    sprintf(cmd, "retr %s\r\n", path);
    ftpCommand(ftp.socketfd, cmd);
    ftpRead(ftp.socketfd, NULL);

    int fd = open("file.txt", O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU);

    if (fd < 0) {
        perror("open save file");
        exit(1);
    }

    int nread = 0, nwrite = 0;
    char buff[MAX_SIZE];
    while(1) {
        nread = read(ftp.datafd, buff, MAX_SIZE);

        if (nread == 0)
            break;
        
        if (nread < 0) {
            perror("Couldn't read from server");
            exit(1);
        }

        nwrite = write(fd, buff, nread);

        if (nwrite < 0){
            perror("Couldn't write to file");
            exit(1);
        }
    }

    ftpRead(ftp.socketfd, NULL);
    return 0;

}

void ftpClose() {
    close(ftp.socketfd);
	close(ftp.datafd);
}


