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

    return 0;
}

int sendCommand(int socketfd, char* cmd) {

    int n;
    if ((n = send(socketfd, cmd, strlen(cmd), MSG_OOB)) < 0){
        perror("Couldn't send command");
        exit(1);
    }

    if (n == 0) {
        perror("Connection closed");
        exit(2);
    }
    getResponse(socketfd);
    // printf("sent %d bytes\n", n);
    return 0;
}

int getResponse(int socketfd) {
    int n;
    char *buff[512];
    if ((n = recv(socketfd, buff, 512, 0)) < 0) {
        perror("Couldn't read response");
        exit(1);
    }

    printf("%s\n", buff);

    if (n == 0) {
        perror("Connection closed");
        exit(1);
    }
    return 0;
}

int userLogin(int socketfd, const char *user, const char *pass) {

    char userCmd[BUFF_SIZE];
    sprintf(userCmd, "user %s\n", userCmd);
    if (sendCommand(socketfd, userCmd)) {
        perror("user login");
        exit(1);
    }

    char passCmd[BUFF_SIZE];
    sprintf(passCmd, "pass %s\n", passCmd);
    if (sendCommand(socketfd, passCmd)) {
        perror("user pass");
        exit(1);
    }

    return 0;
}


