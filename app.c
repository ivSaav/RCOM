#include "app.h"

static App app;

int intToHexa(unsigned char *ret, int value) {

    unsigned char sizeValue[15];

    sprintf(sizeValue, "%lx", value); //convert to hexadecimal
    int length = strlen(sizeValue);
    length = length / 2 + !(length % 2 == 0); //length is +1 for an odd length

    long unsigned hexaSize = strtoul(sizeValue, NULL, 16);  //convert string to long unsigned

    unsigned char sizeArray[25]; // Porque é que é 25?

    for (int i = 0; i < length; i++) {
      unsigned char c = (unsigned char) hexaSize;//extract first 8 bits
      sizeArray[length-1-i] = c;  //save in reverse order
      hexaSize = hexaSize >> 8; //shift out the first 2 digits
    }
  
    strcpy(ret, sizeArray);

    return length;
}

int sendControlFrame(unsigned char controlFlag) {

  unsigned char control[255];

  int pos = 0;
  control[pos++] = controlFlag;

  // Saven length into control
  control[pos++] = TLV_SIZE;

  // Converting length to hexadecimal
  unsigned char values[25];
  int length = intToHexa(values, app.fileSize);
  control[pos++] = length;

  // Concatenate size values into control
  for (int i = 0; i < length; i++){
    control[pos++] = values[i];
  }

  // Save Filename value and size into control
  control[pos++] = TLV_FILENAME;
  int nameSize = strlen(app.filename);
  control[pos++] = (unsigned char) nameSize;

  // Concatenate filename value into control
  for (int i = 0; i < nameSize; i++)
    control[pos++] = app.filename[i];

  //Number of 512 blocks 
  control[pos++] = TLV_BLOCK;
  unsigned char tmp[25];
  int length_ = intToHexa(tmp, app.numBlocks);
  control[pos++] = length_;

  //Concatenate block values into control
  for (int i = 0; i < length; i++){
    control[pos++] = tmp[i];
  }

  // Send the command through app.port
  int n = llwrite(app.port, control, pos+1);
  if (n < 0){
    perror("Couldn't send control frame\n");
    exit(-1);
  }

  printf("Wrote %d bytes\n", n-4);

  return 0;

}


int receiveControlFrame(unsigned char controlFlag){

  unsigned char * buffer = (unsigned char *) malloc(BUFFER_MAX_SIZE * sizeof(char)); // Buffer for control frame
  
  // Read the control frame
  int bufferLength = llread(app.port, buffer);
  bufferLength = bufferLength - 4;  //remove control header from buffer size

  if(bufferLength < 0){
    perror("Couldn't read control frame\n");
    exit(-1);
  }

  // Check if the control flags match
  if(buffer[0] != controlFlag){
    perror("Control Flag is different\n");
    exit(-2);
  }

  int valueLength = 0;
  char type;

  for (int i = 1; i < bufferLength; i++) {
    type = buffer[i];

    // Get the number of octets read
    valueLength = (int) buffer[++i];

    i++; // Increment i to be at the value indice

    char tmp[valueLength]; // temporary array to store the string value read

    switch (type)
    {
    case TLV_SIZE:
      // Concatenate size value into string
      for(int j= 0; j < valueLength; j++){  
          sprintf(tmp+(j*2), "%02x", buffer[i]);
          i++;
      }

      // Save the value into the App struct
      long unsigned fileSize = strtoul(tmp, NULL, 16);
      app.fileSize = (int) fileSize;
      
      break;

    case TLV_FILENAME:
      // Concatenate size value into string
        for (int j = 0; j < valueLength; j++) {
          tmp[j] = (char) buffer[i];
          i++;
        }
        tmp[valueLength] = '\0';

        // Save the filename in the app struct
        app.filename = (char*) malloc(valueLength);
        memset(app.filename, '\0', valueLength+1);
        sprintf(app.filename, "%s", tmp);
        
      break;

    case TLV_BLOCK:
      // Concatenate size value into string
      for(int j= 0; j < valueLength; j++){  
          sprintf(tmp+(j*2), "%02x", buffer[i]);
          i++;
      }

      // Save the number of blocks into the struct
      long unsigned blocks = strtoul(tmp, NULL, 16);
      app.numBlocks = (int) blocks;

      break;
    }

    i--;
  }
  return 0;
}

int sendDataFrames() {

    int sentBlocks = 0, nRead = 0, nWrite = 0;

    unsigned char buffer[BUFFER_MAX_SIZE]; // Buffer that has both data and flags
    unsigned char dataBuffer[BLOCK_SIZE]; // Buffer that only holds data blocs

    while(sentBlocks < app.numBlocks) {

        int index = 0; // For each block we must start at index 0

        buffer[index++] = C_DATA;
        buffer[index++] = sentBlocks % 255; //sequence number

        // Read block from file
        nRead = read(app.fd, dataBuffer, BLOCK_SIZE); 
        if (nRead < 0) {
          perror("read error (sendDataPackets).\n");
          return -1;
        }

        buffer[index++] = nRead / 256;  //L1
        buffer[index++] = nRead % 256;  //L2

        // Concatenate data to buffer
        for (int i = 0; i <= nRead; i++)
          buffer[index + i] = dataBuffer[i];

        // Send data through the app.port
        nWrite = llwrite(app.port, buffer, nRead+5);
        if (nWrite < 0) {
          perror("llwrite sendDataFrames");
          return -1;
        }
        printf("Sent nseq: %d  nWrite: %d\n", sentBlocks, nWrite);

        sentBlocks++;
    }

    return 0;

}

int receiveDataFrames() {

    int receivedBlocks = 0; // Number of blocks received
    char buffer[BUFFER_MAX_SIZE];

    while (receivedBlocks < app.numBlocks) {
      // Read a block from app.port
      int nRead = llread(app.port, buffer);
      if (nRead < 0) {
        perror("llread error (receiveDataFrames)\n");
        return -1;
      }

      int index = 0; // For each block we must start at index 0

      // Check if the first flag is correct
      if (buffer[index++] != C_DATA) {
        perror("Invalid frame \n");
        continue;
      }

      // Save sequence number and the number of octets
      int nseq = buffer[index++];
      unsigned char l2 = buffer[index++];
      unsigned char l1 = buffer[index++];

      // Calculate the number of octets
      int numberOctets = 256*l2+l1;  //number of octets

      unsigned char data[numberOctets+1]; // Buffer to hold the data received
      
      // Save the data received into the data buffer
      for (int i = 0; i < numberOctets; i++) {
        data[i] = buffer[index+i];
      }

      // Save the data into a file with descriptor app.fd
      int nWrite = write(app.fd, data, numberOctets);

      printf("Received nseq:%d  nread:%d nwrite:%d numberOctets:%d\n", nseq, nRead, nWrite, numberOctets);
      
      if (nseq == receivedBlocks) { //if the packet is not duplicate
       receivedBlocks++;
      }
    }

    return 0;
}



int main(int argc, char **argv) {

    if ((strcmp("/dev/ttyS0", argv[1])!=0) && (strcmp("/dev/ttyS1", argv[1])!=0)){
          printf("Usage:\t serialport filename: app /dev/ttyS0 filename\n");
          exit(-1);
    }

    if (argc == 2) {  //Receiver
      app.status = RCV_STAT;
    }
    else if (argc == 3) { //Transmitter

      // Open file to send
      int fd = open(argv[2], O_RDONLY);
      if (fd < 0) { perror(argv[2]); exit(-1); }

      // Save the information about the file to send in App Struct
      app.fd = fd;
      app.status = EMT_STAT;
      app.filename = (char *) malloc(strlen(argv[2]));
      app.filename = argv[2];

      struct stat st;

      // Get the file attributes
      if(fstat(app.fd, &st))
      {
        perror("fstat error\n");
        close(app.fd);
        exit(-1);
      }

      // Save important attributes in App Struct
      app.fileSize = st.st_size;  // Size in bytes
      app.numBlocks = app.fileSize/BLOCK_SIZE + (app.fileSize % BLOCK_SIZE != 0);

    }
    else {
        printf("Usage (Transmitter):\tserialport filename: app /dev/ttyS0 filename\n");
        printf("Usage (Receiver):\tserialport: app /dev/ttyS0 filename\n");
        exit(-1);
    }

    // Set alarm handler to handle timeouts
    if (signal(SIGALRM, signalHandler)) {
      perror("Couldn't install alarm\n");
      exit(-1);
    }

    app.port = initLinkLayer(argv[1], app.status);
    if (app.port < 0) {
      perror("initLinkLayer.");
      exit(1);
    }

    if (app.status == EMT_STAT) { //Transmitter

        // Open the connection
        if (llopen(app.port, EMT_STAT)) {
            perror("Couldn't open connection to noncanonical.\n");
            exit(-1);
        }

        printf("Connection established.\n");

        // Send a control frame to warn the initiation of the transfer
        if(sendControlFrame(C_START)){
          perror("Couldn't send control frame\n");
          exit(-1);
        }

        // Send all the data
        if (sendDataFrames()) {
            perror("sendDataFrames\n");
            exit(-1);
        }

      //sleep(1);

    }
    else {  // Receiver

        // Open the connection
        if (llopen(app.port, RCV_STAT)) {
          perror("stateMachine");
          exit(1);
        }
        
        // Receive a control frame to confirm the initiation of the transfer
        if(receiveControlFrame(C_START)){
          perror("Couldn't receive control frame\n");
          exit(-1);
        }

        printf("filename: %s size:%d blocks:%d\n", app.filename, app.fileSize, app.numBlocks);

        // Open file where we save the information received
        int fd = open(app.filename, O_CREAT|O_WRONLY | O_TRUNC, S_IRWXU);
        if (fd < 0) { perror(app.filename); exit(-1); }

        app.fd = fd;  // Assign file fd to app struct

        // Read all the data frames and save them into the file
        if (receiveDataFrames()) {
            perror("sendDataFrames\n");
            exit(-1);
        }
    }

  // Close the connection
  if (llclose(app.port)) {
    perror("Couldn't close connection\n");
    exit(1);
  }

  // Close all the open files
  close(app.fd);
  close(app.port);

  return 0;
}
