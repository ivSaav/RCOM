#include "utils.h"

static int attempts = 0;
static bool tryToSend = true, timeout = false;
static unsigned char stuffed[512];

void signalHandler(){
  printf("Timeout.\n");
  timeout = true;

  if(attempts == MAX_ATTEMPTS){
    tryToSend = false;
    printf("Exceded max tries.\n");
  }
  else{
    attempts++;
  }
}

//send supervision/numbered frame
int sendAcknowledgement(int fd, unsigned char flag, unsigned char expectedControl){
  unsigned char buffer[5] = {DELIM, flag, expectedControl,  flag^expectedControl, DELIM};

  //printf("expected %X %X\n", flag, expectedControl);

  int res = write(fd,buffer,BUF_SIZE);

  return res;
}

//receive supervision/numbered frame
int receiveFrame(int fd, unsigned char expectedFlag, unsigned char expectedControl){

  static enum state st = START;

  //printf("expected %X %X\n", expectedFlag, expectedControl);

  //get acknowledgement
  int n = 0;
  unsigned char buf[6];

  int i = 0;
  bool end = false;
  timeout = false;

  unsigned char bcc = 0;
  while (!(end || timeout)) { 
     
    //read field sent by writenoncanonical
    unsigned char byte;
    int res = read(fd,&byte,1);
    buf[i] = byte;

    printf("st: %d  buf: %X\n", st, buf[i]); 

    switch (st) {

      case START:

        if (buf[i] == DELIM) {
          st = FLAG_RCV;
          i++;
        }
        break;

      case FLAG_RCV:  //TODO: define other control commands

        if (buf[i] == expectedFlag) {   //A_EM | A_RC
          st = A_RCV;
          i++;
        }
        else if (buf[i] == DELIM) {
          continue;
        }
        else {
          i = 0;
          st = START;
        }
        break;

      case A_RCV:

        if (buf[i] == expectedControl) {
          st = C_RCV;
          i++;
        }
        else if (buf[i] == RJ) {
          st = START;
          return 1;
        }
        else if (buf[i] == DELIM) {
            st = FLAG_RCV;
            i = 1;
        }
        else {
            st = START;
            i = 0;
        }
        break;

      case C_RCV:

        bcc = buf[1]^buf[2];

        if (buf[i] == bcc) {
            st = BCC_OK;
            i++;
        }
        else if (buf[i] == DELIM) {
          st = FLAG_RCV;
          i = 1;
        }
        else {
          st = START;
          i = 0;
        }
        break;

      case BCC_OK:

        if (buf[i] == DELIM) {
          st = START; //for further use
          return 0;
        }
        else {
          st = START;
          i = 0;
        }

      break;

    }
  }

  return 1;

}

int llopen(int fd, int status) {

    switch (status) {
        case EMT_STAT: //emissor
            if (EmtSetupConnection(fd)) {
                perror("Couldn't setup connection.\n");
                exit(1);
            }
            break;
        case RCV_STAT:  //receiver
            if (RcvSetupConnection(fd)){
                perror("Couldn't setup connection.\n");
                exit(2);
            }
            break;
        default:
            perror("Invalid flag.\n");
            exit(3);
    }
 
    return 0;
}

//setup connection from writer
int EmtSetupConnection(int fd) {

     while(tryToSend){
    
        int res = 0;
        //send set frame
        unsigned char buffer[5] = {DELIM, A_EM, SET,  A_EM^SET, DELIM};
        res = write(fd,buffer,BUF_SIZE); 

        alarm(3);

        if (!receiveFrame(fd, A_RC, UA)) { //receive acknowledgement from receiver
            tryToSend = false;
            return 0; //success
        }
    else {
        printf("Invalid acknowledgement.\n");
    }

  }
    
    return 1; //failure
}

//setup connection from receiver
int RcvSetupConnection(int fd) {
    
    int n = 0;
    unsigned char buf[6];

    int i = 0;
    bool end = false;

    if (receiveFrame(fd, A_EM, SET)) { //receive SET frame from writer
        perror("Received invalid frame.\n");
        exit(1);
    }
        
    if(sendAcknowledgement(fd, A_RC, UA) == -1){
        perror("Error sending acknowledgement!");
        exit(2);
    }

    return 0;
}

int llclose(int fd, int status) {

  tryToSend = true;
  attempts = 0;

  if (status == EMT_STAT) {
      return EmtCloseConnection(fd);
  }
  else {
    return RcvCloseConnection(fd);
  }
  
}

int EmtCloseConnection(int fd) {

  while(tryToSend && (attempts < 3)){
      
      int res = 0;
      //send set frame
      unsigned char buffer[5] = {DELIM, A_EM, DISC,  A_EM^DISC, DELIM};
      res = write(fd,buffer,BUF_SIZE); 

      alarm(3);

      if (receiveFrame(fd, A_RC, DISC)) { //receive DISC from receiver
        printf("Invalid acknowledgement.\n");
      }

      if (sendAcknowledgement(fd, A_RC,  UA) <= 0) { 
        printf("Couldn't send acknowledgment.\n");
      }
      else {
        printf("Connection closed\n");
        return 0;
      }

  }

  return 1;

}

int RcvCloseConnection(int fd) {

  timeout = false;
  attempts = 0;

  while(tryToSend){

    alarm(3);

    if (receiveFrame(fd, A_EM, DISC)) { //receive DISC from emissor
        printf("Invalid frame (llclose).\n");
    }

    int res = 0;
    //send DISC frame
    unsigned char buffer[5] = {DELIM, A_RC, DISC,  A_RC^DISC, DELIM};
    res = write(fd,buffer,BUF_SIZE); 
    if (res <= 0) {
      perror("write error\n");
      exit(1);
    }

    alarm(3);

    if (!receiveFrame(fd, A_RC, UA)) { //receive UA from emissor
        printf("Connection closed\n");
        return 0; //success
    }
    else {
      printf("Invalid frame (llclose).\n");
    }
  
  
  }

  return 1;

}


unsigned char RcvCalcBcc2(unsigned char *buffer, int i, unsigned char first, int last_data_index) {

  if (i >= last_data_index-1) {  //reached last element
    return first;
  }
  
  first = first^buffer[i+1];
  return RcvCalcBcc2(buffer, ++i, first, last_data_index);
}

unsigned char  calcBcc2(unsigned char *buffer, int i, unsigned char first) {

  if (buffer[i] == '\0') {  //reached last element
    return first;
  }
  
  first = first^buffer[i+1];
  return calcBcc2(buffer, ++i, first);
}

int stuffBytes(unsigned char *buffer, int size) {
  
  int j = 0;
  for (int i = 0; i < size; i++) { 

    if (buffer[i] == DELIM) {
      stuffed[j] = ESC_OCT;
      stuffed[++j] = buffer[i]^ESC_MASK;
    }
    else if (buffer[i] == ESC_OCT) {
      stuffed[j] = ESC_OCT;
      stuffed[++j] = buffer[i]^ESC_MASK;
    }
    else {
      stuffed[j] = buffer[i];
    }
    j++;
  }

  return j-1;
}

int DestuffBytes(unsigned char *buffer, int size) {
  return 0;
}

int llwrite(int fd, unsigned char *data, int size) {	
  
  bool rcvRR = false;
  int numTries = 0;
  static bool ns_set = false; //S starts at 0

  bool sentData = false;

	do {
    int size = 6;
    unsigned char bcc2 = calcBcc2(data, 0, data[0]);

    int ndata = stuffBytes(data, size);

    //intialize data frame header
    unsigned char buffer[ndata + 6]; 
    buffer[0] = DELIM;
    buffer[1] = A_EM;
    buffer[2] = ns_set ? 0x40 : 0x00; //Control flag: S=1 --> 0x40 ; S=0 --> 0x00
    buffer[3] = buffer[1]^buffer[2];

    int buffPos = 4;
    for (int i = 0; i < ndata; i++) {  //concatenating stuffed data buffer
        buffer[i+buffPos] = stuffed[i];
    }

    buffPos += ndata;
    buffer[buffPos] = bcc2;
    buffer[++buffPos] = DELIM;

    //send data
    int nbytes = ndata + 6;
    int n = write(fd, buffer, nbytes);

    alarm(3);

    //receive acknowledgement from receiver
    if (!receiveFrame(fd, A_EM, ns_set ? (0x0F & RR) : RR)) { //if S=0 expect to receive S=1
        ns_set = !ns_set;
        tryToSend = false;
        return 0; //success
    }
    else {
      numTries++;
    }
  
  } while (!sentData && (numTries < 3));

   return 1;
}

int llread(int fd, unsigned char * buffer){
  enum state {START, FLAG_RCV, A_RCV, C_RCV, BCC_OK, DATA, BCC2_OK};

  enum state st = START;

  unsigned char byte;
  int i = 0, res, data_received = 0;
  unsigned char bcc = 0;

  static bool nr_set = 1;

  while (true) { 
     
    //read field sent by writenoncanonical
    res = read(fd,&byte,1);
    buffer[i] = byte;
    printf("st: %d  buf: %X\n", st, buffer[i]);  

    switch (st) {

      case START: //validate header

        if (buffer[i] == DELIM) {
          st = FLAG_RCV;
          i++;
        }
        break;

      case FLAG_RCV:  

        if (buffer[i] == A_EM) {
          st = A_RCV;
          i++;
        }
        else if (buffer[i] == DELIM) {
          continue;
        }
        else {
          i = 0;
          st = START;
        }
        break;

      case A_RCV:

        if (buffer[i] == C_0 || buffer[i] == C_1) {
          nr_set = (buffer[i] == C_1); //alternate between 1 and 0
          st = C_RCV;
          i++;
        }
        else if (buffer[i] == DELIM) {
            st = FLAG_RCV;
            i = 1;
        }
        else {
            st = START;
            i = 0;
        }
        break;

      case C_RCV:

        bcc = buffer[1]^buffer[2];

        if (buffer[i] == bcc) {
            st = BCC_OK;
            i++;
        }
        else if (buffer[i] == DELIM) {
          st = FLAG_RCV;
          i = 1;
        }
        else {
          st = START;
          i = 0;
        }
        break;

      case BCC_OK:  //start reading data 
      
        if (buffer[i] == DELIM) { //reached end of frame
          data_received--;

          unsigned char bcc2 = RcvCalcBcc2(buffer, 4, buffer[4], 4 + data_received);   //TODO 


          if(true/*buffer[i-1] == bcc2*/){    //accepted frame  TODO destuffbytes
            
            if (sendAcknowledgement(fd, A_EM,  nr_set ? (0x0F & RR) : RR) <= 0) { //if S=1 send S=0
              perror("Couldn't send acknowledgement.\n");
              exit(-1);
            }
  
            st = START; //for further use
            return i;
          }
          else{   //rejected frame
      
            if (sendAcknowledgement(fd, A_EM, nr_set ? (0x0F & RJ) : RJ) <= 0) {
                perror("Couldn't send acknowledgement.\n");
                exit(-1);
              }

            printf("Rejected data packet.\n");

            st = START;
            i = 0;
            data_received = 0;
          }
        }
        else {              //add byte to buffer
          data_received++;
          i++;

          buffer = (char*) realloc(buffer, (6 + data_received)*sizeof(char));
        }

      break;
      }
    }
  
    return -1;
}
