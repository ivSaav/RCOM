/*      (C)2000 FEUP  */


#include "server.h"
#include "parseArgs.h"

int main(int argc, char** argv){

	int	socketfd; 
	int	bytes;
	
	arg_url parsedUrl = parseUrl(argv[1]);

	ftpInit(SERVER_ADDR);

    /*send a string to the server*/

	if (ftpLogin(parsedUrl.user, parsedUrl.password)) {
		perror("Couldn't login user");
		exit(1);
	}


	if (ftpPassiveMode()) {
		perror("passive mode");
		exit(1);
	}
	
	printf("%s\n", parsedUrl.url_path);
	ftpDownload(parsedUrl.url_path);
	
	ftpClose();
	exit(0);
}


