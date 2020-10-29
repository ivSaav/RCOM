#include "app.h"

static App app;

int intToHexa(unsigned char *ret, int value) {

    unsigned char sizeValue[15];

    sprintf(sizeValue, "%lx", value); //convert to hexadecimal
    int length = strlen(sizeValue);
    length = length / 2 + !(length % 2 == 0); //length is +1 for an odd length

    long unsigned hexaSize = strtoul(sizeValue, NULL, 16);  //convert string to long unsigned


    //printf("size %s %lx %d\n", sizeValue, hexaSize, length);

    long unsigned tmp = hexaSize;

    unsigned char sizeArray[25];

    for (int i = 0; i < length; i++) {
      unsigned char c = (unsigned char) tmp;//extract first 8 bits
      sizeArray[length-1-i] = c;  //save in reverse order
      tmp = tmp >> 8; //shift out the first 2 digits
    }
  
    strcpy(ret, sizeArray);

    return length;
}
int sendControlFrame(unsigned char controlFlag) {

  unsigned char control[255];

  int pos = 0;
  control[pos++] = controlFlag;

  //LENGTH
  control[pos++] = TLV_SIZE;

  unsigned char values[25];
  int length = intToHexa(values, app.fileSize);
  control[pos++] = length; //length in oct

  //concatenate size values into control
  for (int i = 0; i < length; i++){
    control[pos++] = values[i];
  }

  //FILENAME
  control[pos++] = TLV_FILENAME;
  int nameSize = strlen(app.filename);

  control[pos++] = (unsigned char) nameSize;

  for (int i = 0; i < nameSize; i++)
    control[pos++] = app.filename[i];

  //NUMBER OF 512 BLOCKS
  control[pos++] = TLV_BLOCK;
  unsigned char tmp[25];
  int length_ = intToHexa(tmp, app.numBlocks);
  control[pos++] = length_;

   //concatenate block values into control
  for (int i = 0; i < length; i++){
    control[pos++] = tmp[i];
  }

  //send command
  int n = llwrite(app.port, control, pos+1);
  if (n < 0){
    perror("Couldn't send control frame\n");
    exit(-1);
  }

  printf("Wrote %d bytes\n", n-4);

  return 0;

}
int receiveControlFrame(unsigned char controlFlag){

  unsigned char * buffer = (unsigned char *) malloc(BUFFER_MAX_SIZE * sizeof(char));

  int buffer_length = llread(app.port, buffer);
  buffer_length = buffer_length - 4;  //remove control header from buffer size

  if(buffer_length < 0){
    perror("Couldn't read control frame\n");
    exit(-1);
  }

  
  int data_length; // buffer will have data starting at indice 4

  if(buffer[0] != controlFlag){
    perror("Control Flag is different\n");
    exit(-2);
  }

  int lengthV = 0;
  for (int i = 1; i < buffer_length; i++) {

    // printf("i %d\n", i);

    if (buffer[i] == TLV_SIZE) {
      lengthV = (int) buffer[++i];

      //printf("length %d\n",lengthV);

      i++;
      char tmp[25] = {00};
      int j= 0;
      for(; j < lengthV; j++)  //concatenate size value into string
          sprintf(tmp+(j*2), "%02x", buffer[i+j]);
      i += j-1;
    
      long unsigned fileSize = strtoul(tmp, NULL, 16);
      app.fileSize = (int) fileSize;
    }
    else if (buffer[i] == TLV_FILENAME) {
        lengthV = (int) buffer[++i];

        i++;
        char tmp_[lengthV];
        int j = 0;
        for (; j < lengthV; j++) {
          tmp_[j] = (char) buffer[i+j];
          // printf("%C\n", tmp_[j]);
        }
        tmp_[lengthV] = '\0';
        i += j-1;

        app.filename = (char*) malloc(lengthV);
        memset(app.filename, '\0', lengthV+1);
        sprintf(app.filename, "%s", tmp_);

    }
    else if (buffer[i] == TLV_BLOCK) {
      lengthV = (int) buffer[++i];

      //printf("length %d\n",lengthV);

      i++;
      char tmp[25] = {00};
      int j= 0;
      for(; j < lengthV; j++)  //concatenate size value into string
          sprintf(tmp+(j*2), "%02x", buffer[i+j]);
      i += j-1;
    
      long unsigned blocks = strtoul(tmp, NULL, 16);
      app.numBlocks = (int) blocks;
    }
  }

  return 0;
}

int sendDataFrames() {

    int sentBlocks = 0;
    unsigned char buffer[BUFFER_MAX_SIZE];
    unsigned char dataBuffer[BLOCK_SIZE];
    int nRead = 0, nWrite = 0;
    while(sentBlocks < app.numBlocks) {

        int index = 0;

        buffer[index++] = C_DATA;
        buffer[index++] = sentBlocks % 255; //sequence number


        nRead = read(app.fd, dataBuffer, BLOCK_SIZE); //read block from file
        if (nRead < 0) {
          perror("read error (sendDataPackets).\n");
          exit(-1);
        }

        buffer[index++] = nRead / 256;  //L1
        buffer[index++] = nRead % 256;  //L2

        //concatenate data to buffer
        for (int i = 0; i <= nRead; i++)
          buffer[index + i] = dataBuffer[i];

        nWrite = llwrite(app.port, buffer, nRead+5);  //send block to receiver
        printf("nread: %d  nWrite: %d\n",nRead , nWrite);
        // if (nWrite < 0) {
        //   perror("Didn't send full data package\n");
        //   exit(-1);
        // }

        printf("nseq: %d  nWrite: %d\n", sentBlocks, nWrite);
        sentBlocks++;

    }

    return 0;

}

int receiveDataFrames() {

    int receivedBlocks = 0;
    char buffer[BUFFER_MAX_SIZE];

    while (receivedBlocks < app.numBlocks) {
      int nRead = llread(app.port, buffer);
      if (nRead < 0) {
        perror("llread error (receiveDataFrames)\n");
        exit(-1);
      }

      int index = 0;
      if (buffer[index++] != C_DATA) {
        perror("Invalid frame \n");
        exit(-1);
      }

      int nseq = buffer[index++];
      unsigned char l2 = buffer[index++];
      unsigned char l1 = buffer[index++];

      int k = 256*l2+l1;  //number of octets

      unsigned char data[k+1];
      for (int i = 0; i < k; i++) {
        data[i] = buffer[index+i];
        printf("data %X %c\n", data[i],data[i]);

      }


      int nWrite = write(app.fd, data, k);

      printf("Received nseq:%d  nread:%d nwrite:%d k:%d\n", nseq, nRead, nWrite, k);

      // if (nWrite < k) {
      //   perror("Didn't write full block (receiveDataFrame)\n");
      //   exit(-1);
      // }

      
      receivedBlocks++;


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

       struct stat st;

      if(fstat(app.fd, &st))
      {
        perror("fstat error\n");
        close(app.fd);
        exit(-1);
      }

      app.fileSize = st.st_size;  //size in bytes
      app.numBlocks = app.fileSize/BLOCK_SIZE + (app.fileSize % BLOCK_SIZE != 0);

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

        if(sendControlFrame(C_START)){
          perror("Couldn't send control frame\n");
          exit(-1);
        }

        if (sendDataFrames()) {
            perror("sendDataFrames\n");
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

        if(receiveControlFrame(C_START)){
          perror("Couldn't receive control frame\n");
          exit(-1);
        }

        printf("filename: %s size:%d blocks:%d\n", app.filename, app.fileSize, app.numBlocks);


        //open image
        int fd = open("p.gif", O_CREAT|O_WRONLY | O_TRUNC, S_IRWXU);
        if (fd < 0) { perror(app.filename); exit(-1); }

        app.fd = fd;  //assign imaged fd to app struct

        if (receiveDataFrames()) {
            perror("sendDataFrames\n");
            exit(-1);
        }


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
