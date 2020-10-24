#include "app.h"


App app;

int main(int argc, char **argv) {

    if (argc < 4 || (atoi(argv[3]) != EMT_STAT && atoi(argv[3]) != RCV_STAT) ) {
        printf("Usage:\tfilename serialport status: filename /dev/ttyS0 0\n");
        exit(-1);
    } 
    // else if ((strcmp("/dev/ttyS0", argv[2])!=0) && (strcmp("/dev/ttyS1", argv[2])!=0)){
    //       printf("Usage:\tfilename status: filename 0|1\n");    
    //       exit(-1);    
    // }

    int fd = open(argv[1], O_RDONLY);
    if (fd < 0) {
        perror(argv[1]); 
        exit(-1);
    }

    app.fd = fd;
    app.status = atoi(argv[3]);

    (void) signal(SIGALRM, signalHandler);

    int portfd, c, res;
    struct termios oldtio,newtio;
    
    (void) signal(SIGALRM, signalHandler);

  /*
    Open serial port device for reading and writing and not as controlling tty
    because we don't want to get killed if linenoise sends CTRL-C.
  */

    portfd = open(argv[2], O_RDWR | O_NOCTTY);
    if (portfd <0) { perror(argv[1]); exit(-1); }

    if ( tcgetattr(portfd,&oldtio) == -1) { /* save current port settings */
      perror("tcgetattr");
      exit(-1);
    }

    bzero(&newtio, sizeof(newtio));
    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    /* set input mode (non-canonical, no echo,...) */
    newtio.c_lflag = 0;

    newtio.c_cc[VTIME]    = 3;   /* inter-character timer unused */
    newtio.c_cc[VMIN]     = 0;   /* blocking read until 5 chars received */

    tcflush(portfd, TCIOFLUSH);

    if ( tcsetattr(portfd,TCSANOW,&newtio) == -1) {
      perror("tcsetattr");
      exit(-1);
    }
   
    printf("New termios structure set\n");
	
    if (app.status == EMT_STAT) {

        if (llopen(portfd, EMT_STAT)) {
            perror("Couldn't open connection to noncanonical.\n");
            exit(-1);
        }

        printf("Connection established.\n");

        int size = 6;
        unsigned char dataBuffer[6] = {0x7d, 0x21,0x7e, 0x12, 0x11, '\0'}; 
        
        if (llwrite(portfd, dataBuffer, size) < 0) {
        perror("Couldn't send data.\n");
        exit(-1);
        }

        printf("Successfully sent package.\n");

        if (llclose(portfd, EMT_STAT)) {
        perror("Couldn't close conection\n");
        exit(-1);
        }
        
        if ( tcsetattr(portfd,TCSANOW,&oldtio) == -1) {
        perror("tcsetattr");
        exit(-1);
        }

        sleep(1);

        close(fd);
  
    }
    else {  //RCV_STAT

        newtio.c_cc[VTIME]    = 0;   /* inter-character timer unused */ 
    newtio.c_cc[VMIN]     = 5;   /* blocking read until 5 chars received */

        if (llopen(portfd, RCV_STAT)) {
        perror("stateMachine");
        exit(1);
        }

        char * buffer = (char*) malloc(6 * sizeof(char));

        if(llread(portfd, buffer) == -1){
        perror("Error reading data!");
        exit(2);
        }

        if (llclose(portfd, RCV_STAT)) {
        perror("Couldn't close connection\n");
        exit(1);
        }
        tcsetattr(portfd,TCSANOW,&oldtio);
        close(fd);

    }
    
   return 0;
}