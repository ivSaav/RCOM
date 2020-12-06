#include "server.h"

int initConnection(char *ip, int socketfd,  int serverPort) {
    
    struct	sockaddr_in server_addr;

    /*server address handling*/
	bzero((char*)&server_addr,sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(ip);	/*32 bit Internet address network byte ordered*/
	server_addr.sin_port = htons(serverPort);		/*server TCP port must be network byte ordered */
    
    printf("connecting to server...\n");
	/*connect to the server*/
	if(connect(socketfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0){
        perror("connect()");        /*ftp://ftp.up.pt/pub/*/
		exit(1);
	}
	printf("connection established\n");

    if(getResponse(socketfd)) {
        exit(1);
    }

    return 0;
}

int sendCommand(int socketfd, char* cmd) {

    int n;
    if ((n = send(socketfd, cmd, strlen(cmd), 0)) < 0){
        perror("Couldn't send command");
        exit(1);
    }

    if (n == 0) {
        perror("Connection closed");
        exit(2);
    }

    // printf("sent %d bytes\n", n);
    return 0;
}

int getResponse(int socketfd) {
    
    FILE* fp = fdopen(socketfd, "r");
    char * buff;
	size_t n = 0;

    while (1){
        getline(&buff, &n, fp);
        printf("%s", buff);

        if (buff[3] == ' ')
            break;

    }

    fflush(stdout);

    return 0;
}

int userLogin(int socketfd, const char *user, const char *pass) {

    char cmd[BUFF_SIZE];
    sprintf(cmd, "user %s\r\n", user);
    if (sendCommand(socketfd, cmd)) {
        perror("user login");
        exit(1);
    }

    getResponse(socketfd);

    memset(cmd, 0, BUFF_SIZE);

    sprintf(cmd, "pass %s\r\n", pass);
    if (sendCommand(socketfd, cmd)) {
        perror("user pass");
        exit(1);
    }
    getResponse(socketfd);

    return 0;
}


