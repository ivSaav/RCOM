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


int readResponse(int socketfd, char *ret) {
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

int userLogin(int socketfd, const char *user, const char *pass) {

    char cmd[BUFF_SIZE];
    sprintf(cmd, "user %s\r\n", user);
    if (sendCommand(socketfd, cmd)) {
        perror("user login");
        exit(1);
    }

	readResponse(socketfd, NULL);

    memset(cmd, 0, BUFF_SIZE);

    sprintf(cmd, "pass %s\r\n", pass);
    if (sendCommand(socketfd, cmd)) {
        perror("user pass");
        exit(1);
    }
	readResponse(socketfd, NULL);

    return 0;
}


int parsePassiveResponse(int socketfd, char *ip) {

    char *res = (char*) malloc(sizeof(char)*512);
	readResponse(socketfd, res);

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

int downloadFile(int socketfd, int datafd, const char *path) {

    char cmd[255];
    // sprintf(cmd, "retr pub.txt\r\n", path);
    sendCommand(socketfd, "retr pub.txt\r\n");
    readResponse(socketfd, NULL);

    printf("opening file\n");

    int fd = open("file.txt", O_WRONLY | O_CREAT, S_IRWXU);

    if (fd < 0) {
        perror("open save file");
        exit(1);
    }
        printf("reading\n");

    int nread = 0, nwrite = 0;
    char buff[MAX_SIZE];
    while(1) {
        nread = read(datafd, buff, MAX_SIZE);

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

        printf("cenas\n");
    }

}


