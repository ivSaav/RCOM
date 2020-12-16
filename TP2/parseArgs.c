#include "parseArgs.h"

int parseFilePath(arg_url *parsedUrl, char *url, int i) {

    char *aux = (char*) malloc(sizeof(char) * 125);
    char *path = (char*) malloc(sizeof(char) * 255);
    
    const char s[2] = "/";
    char *token;
    
    token = strtok(url, s);
    
    int cnt = 0;
    while( token != NULL ) {
        
        if (cnt > 1) {

            if (cnt > 2)
              strcat(path, "/");

            strcat(path, token);
            memset(aux, 0, 125);
            strcpy(aux, token);
        }
        
        token = strtok(NULL, s);
        cnt++;
    }

    strcpy(parsedUrl->url_path, path);
    strcpy(parsedUrl->filename, aux);
}

arg_url parseUrl(char* url){
    arg_url parsedUrl;

    parsedUrl.user = (char*) malloc(sizeof(char) * 125);
    parsedUrl.password = (char*) malloc(sizeof(char) * 125);

    char *parsedString = (char *) malloc(sizeof(char) *255);
    int stringIndex = 0;


    int urlIndex = USER_START_INDEX;

    int parsedFields = 0;

    while(url[urlIndex] != '\0'){

        if(url[urlIndex] == ':'){
            parsedFields++;
            sprintf(parsedUrl.user, "%s", parsedString);
            memset(parsedString, '\0', stringIndex);
            stringIndex = 0;
        }
        else if(url[urlIndex] == '@'){
            parsedFields++;
            sprintf(parsedUrl.password, "%s", parsedString);
            memset(parsedString, '\0', stringIndex);
            stringIndex = 0;
        }
        else if(url[urlIndex] == '/'){
            parsedFields++;
            parsedUrl.host = (char *) malloc(sizeof(char) * stringIndex + 1);
            sprintf(parsedUrl.host, "%s", parsedString);
            break;
        }
        else{
            parsedString[stringIndex++] = url[urlIndex];
        }

        urlIndex++;

    }

    parsedUrl.filename = (char *) malloc(sizeof(char) * 125);
    parsedUrl.url_path = (char *) malloc(sizeof(char) *255);

    parseFilePath(&parsedUrl, url, urlIndex);
    
    printf("PATH: %s\nFILENAME: %s\n", parsedUrl.url_path, parsedUrl.filename);


    // no username specified
    if (parsedFields == 1) {
        parsedUrl.user = "anonymous";
        parsedUrl.password = "anonymous";
    }
    else if (parsedFields != 3) {
        fprintf(stderr, "Invalid url: ftp://[<user>:<password>@]<host>/<url-path>\n");
        exit(1);
    }

    if (parsedUrl.password == "") {
        parsedUrl.password = "anonymous";
    }

    return parsedUrl;
}