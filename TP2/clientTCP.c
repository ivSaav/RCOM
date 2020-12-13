/*      (C)2000 FEUP  */


#include "server.h"


int main(int argc, char** argv){

	int	socketfd; 
	int	bytes;
	

	ftpInit(SERVER_ADDR);

    /*send a string to the server*/

	if (ftpLogin("rcom", "rcom")) {
		perror("Couldn't login user");
		exit(1);
	}


	if (ftpPassiveMode()) {
		perror("passive mode");
		exit(1);
	}
	
	// readResponse(datafd, NULL); FIX blocking not connecting?
	ftpDownload( "pipe.txt");
	
	ftpClose();
	exit(0);
}


