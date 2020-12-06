/*      (C)2000 FEUP  */


#include "server.h"


int main(int argc, char** argv){

	int	socketfd;
	char	buf[] = "Mensagem de teste na travessia da pilha TCP/IP\n";  
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

	if (userLogin(socketfd, "anonymous", "1234")) {
		perror("Couldn't login user");
		exit(1);
	}

	if (sendCommand(socketfd, "PASV")) {
		perror("pasv");exit(1);
	}
	if (getResponse(socketfd))  {
		perror("res");
		exit(1);
	}
	
	
	// // write> user anonymous
	// // write> pass bla
	// //write pasv

	// char * res[512];
	// bytes = read(socketfd, res, 512);
	// printf("%s \n", res);

	close(socketfd);
	exit(0);
}


