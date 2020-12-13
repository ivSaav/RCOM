#include "parseArgs.h"


arg_url parseUrl(char* url){
    arg_url parsedUrl;

    parsedUrl.user = (char*) malloc(sizeof(char) * 254);
    sprintf(parsedUrl.user, "anonymous");

    parsedUrl.password = (char*) malloc(sizeof(char) * 254);
    sprintf(parsedUrl.password, "anonymous");

    char *parsedString = (char *) malloc(sizeof(char) *254);
    int stringIndex = 0;


    int urlIndex = USER_START_INDEX;

    while(url[urlIndex] != '\0'){

        if(url[urlIndex] == ':'){
            sprintf(parsedUrl.user, "%s", parsedString);
            memset(parsedString, '\0', stringIndex);
            stringIndex = 0;
        }
        else if(url[urlIndex] == '@'){
            sprintf(parsedUrl.password, "%s", parsedString);
            memset(parsedString, '\0', stringIndex);
            stringIndex = 0;
        }
        else if(url[urlIndex] == '/'){
            parsedUrl.host = (char *) malloc(sizeof(char) * stringIndex + 1);
            sprintf(parsedUrl.host, "%s", parsedString);
            memset(parsedString, '\0', stringIndex);
            stringIndex = 0;
        }
        else{
            parsedString[stringIndex++] = url[urlIndex];
        }

        urlIndex++;

    }
    parsedUrl.url_path = parsedString;

    return parsedUrl;
}