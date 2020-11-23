/*      (C)2000 FEUP  */

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

#define SERVER_PORT 6000
#define SERVER_ADDR "192.168.28.96"
#define SERVER_ADDR2 "193.137.29.15"

int main(int argc, char** argv){

	int	sockfd;
	struct	sockaddr_in server_addr;
	char	buf[] = "Mensagem de teste na travessia da pilha TCP/IP\n";  
	int	bytes;
	
	/*server address handling*/
	bzero((char*)&server_addr,sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(SERVER_ADDR);	/*32 bit Internet address network byte ordered*/
	server_addr.sin_port = htons(SERVER_PORT);		/*server TCP port must be network byte ordered */
    
	/*open a TCP socket*/
	
	if ((sockfd = socket(AF_INET,SOCK_STREAM,0)) < 0) {
    		perror("socket()");
        	exit(0);
    	}
	printf("oppened socket \n");

	printf("connecting to server...\n");
	/*connect to the server*/
	if(connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0){
        perror("connect()");/*ftp://ftp.up.pt/pub/*/
		exit(0);
	}
	printf("connection established\n");

    /*send a string to the server*/
	bytes = write(sockfd, buf, strlen(buf));
	printf("Bytes escritos %d\n", bytes);

	// write> user anonymous
	// write> pass bla
	//write pasv

	char * res[512];
	bytes = read(sockfd, res, 512);
	printf("%s \n", res);

	close(sockfd);
	exit(0);
}


