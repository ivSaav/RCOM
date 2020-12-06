/*      (C)2000 FEUP  */


#include "server.h"


int main(int argc, char** argv){

	int	socketfd; 
	int	bytes;
	
	
	/*open a TCP socket*/
	if ((socketfd = socket(AF_INET,SOCK_STREAM,0)) < 0) {
    		perror("socket()");
        	exit(0);
    	}
	printf("oppened socket \n");

	if (initConnection(SERVER_ADDR, socketfd, SERVER_PORT)) {
		perror("Couldn't connect to server");
		exit(1);
	}

    /*send a string to the server*/

	if (userLogin(socketfd, "anonymous", "anonymous")) {
		perror("Couldn't login user");
		exit(1);
	}

	if (sendCommand(socketfd, "PASV\r\n")) {
		perror("pasv");exit(1);
	}
	// readResponse(socketfd, NULL);

	char *ip = (char*) malloc(sizeof(char)*20);
	int port = parsePassiveResponse(socketfd, ip);

	printf("ip %s %d \n", ip, port);
	
	if (initConnection(ip, socketfd, port)) {
		perror("Couldn't connect to server");
		exit(1);
	}

	close(socketfd);
	exit(0);
}


