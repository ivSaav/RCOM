#include "app.h"

static App app;

int sendControlPacket(unsigned char controlFlag) {

  unsigned char control[255];

  int pos = 0;
  control[pos++] = controlFlag;
  control[pos++] = app.fileSize;

  //SIZE
  control[pos++] = TLV_SIZE;

  unsigned char sizeValue[10];
  sprintf(sizeValue, "%X", app.fileSize); //convert to hexadecimal
  int length = strlen(sizeValue);
  control[pos++] = length % 2; //length in oct
  
  unsigned char c;
  for (int i = length-1; i >= 0; i--){  //fetching one octect at a time
      
      int j = i-1;
      if (j >= 0) {
          unsigned char front, back;
          front = sizeValue[j] & 0x0f; //extract front of oct
          back = sizeValue[i] & 0x0f; //extract back of oct

          c = (front << 4) | back;  //construct oct
          i--;
      }
      else {
          c = sizeValue[i] & 0x0f;
      }

      control[pos++] = c; //concatenate value oct

  }

  //FILENAME
  control[pos++] = TLV_FILENAME;
  int nameSize = strlen(app.filename);
  for (int i = 0; i < nameSize; i++)
    control[pos++] = app.filename[i];

  //send command
  if (llwrite(app.port, control, pos-1)){
    perror("Couldn't send control frame\n");
    exit(-1);
  }

  return 0;
  
}


int main(int argc, char **argv) {

    // if ((strcmp("/dev/ttyS0", argv[1])!=0) && (strcmp("/dev/ttyS1", argv[1])!=0)){
    //       printf("Usage:\t serialport filename: app /dev/ttyS0 filename\n");    
    //       exit(-1);    
    // }

    if (argc == 2) {  //Receiver
      app.status = RCV_STAT;
    }
    else if (argc == 3) { //Transmitter

      //open image
      int fd = open(argv[2], O_RDONLY);
      if (fd < 0) { perror(argv[2]); exit(-1); }

      app.fd = fd;
      app.status = EMT_STAT;
      app.filename = (char *) malloc(strlen(argv[2]));
      app.filename = argv[2];


    }
    else {
        printf("Usage (Transmitter):\tserialport filename: app /dev/ttyS0 filename\n");
        printf("Usage (Receiver):\tserialport: app /dev/ttyS0 filename\n");
        exit(-1);
    }

    //set alarm handler
    if (signal(SIGALRM, signalHandler)) {
      perror("Couldn't install alarm\n");
      exit(-1);
    }

    int portfd, c, res;
    struct termios oldtio,newtio;

  /*
    Open serial port device for reading and writing and not as controlling tty
    because we don't want to get killed if linenoise sends CTRL-C.
  */

    portfd = open(argv[1], O_RDWR | O_NOCTTY);
    if (portfd <0) { perror(argv[1]); exit(-1); }

    app.port = portfd;

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
	
    if (app.status == EMT_STAT) { //Transmitter

        if (llopen(app.port, EMT_STAT)) {
            perror("Couldn't open connection to noncanonical.\n");
            exit(-1);
        }

        printf("Connection established.\n");

        int size = 6;
        unsigned char dataBuffer[6] = {0x7d, 0x21,0x7e, 0x12, 0x11, '\0'}; 
        
        if (llwrite(app.port, dataBuffer, size) < 0) {
          perror("Couldn't send data.\n");
          exit(-1);
        }


        if (llclose(app.port, EMT_STAT)) {
          perror("Couldn't close conection\n");
          exit(-1);
        }
        
        if ( tcsetattr(app.port,TCSANOW,&oldtio) == -1) {
          perror("tcsetattr");
          exit(-1);
        }

        sleep(1);

        close(app.fd);
        close(app.port);
  
    }
    else {  //RCV_STAT

        if (llopen(app.port, RCV_STAT)) {
          perror("stateMachine");
          exit(1);
        }

        char * buffer = (char*) malloc(DATA_BUFFER_MAX_SIZE * sizeof(char));

        if(llread(app.port, buffer) == -1){
          perror("Error reading data!");
          exit(2);
        }

        printf("///////\n");

        if (llclose(app.port, RCV_STAT)) {
          perror("Couldn't close connection\n");
          exit(1);
        }
        tcsetattr(app.port,TCSANOW,&oldtio);
        close(app.fd);
        close(app.port);

    }
    
   return 0;
}