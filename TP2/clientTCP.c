/*      (C)2000 FEUP  */


#include "server.h"
#include "parseArgs.h"

int main(int argc, char** argv){

	int	socketfd; 
	int	bytes;
	
	arg_url parsedUrl = parseUrl(argv[1]);

	if (ftpInit(parsedUrl.host)) {
		perror("ftpInit");
		exit(1);
	}

    /*send a string to the server*/

	if (ftpLogin(parsedUrl.user, parsedUrl.password)) {
		perror("Couldn't login user");
		exit(1);
	}


	if (ftpPassiveMode()) {
		perror("passive mode");
		exit(1);
	}
	
	if (ftpDownload(parsedUrl.filename, parsedUrl.url_path)) {
		perror("ftpDownload");
		exit(1);
	}
	
	ftpClose();

	exit(0);
}


