#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

#define SIZE 100
typedef struct urlStruct {
    char hostName[SIZE];
    int port;
    char path[SIZE];
} urlStruct;

urlStruct* parsing(char* str)
{
    urlStruct *URL=(urlStruct*)malloc(sizeof (urlStruct));
    str=str+7;
    char *token ;
/* get the first token */
    token = strtok(str, "/");
    strcpy(URL->hostName, token);
    /* walk through other tokens */
    while( token != NULL ) {
        token = strtok(NULL, "/");
        if(token!=NULL){
            strcat(URL->path, "/");
            strcat(URL->path, token);
        }
           }
    if(strlen(URL->path)==0)
    {strcpy(URL->path,"/");}

    token = strtok(URL->hostName, ":");
    strcpy(URL->hostName, token);
    token = strtok(NULL, ":");
    if(token!=NULL){

        URL->port= atoi(token);
    }
    else{ URL->port= 80;}

   /**printf( " %s\n", URL->hostName );
    printf("%d\n", URL->port);
    printf( " %s\n", URL->path );**/
    return URL;
}

void creatDir(char *filename) {
    char *withoutFile = (char*) malloc(strlen(filename) + 1);
    strcpy(withoutFile, filename);
    char* endOfString = strrchr(withoutFile, '/');
    withoutFile[endOfString - withoutFile] = '\0';

    char* token = strtok(withoutFile, "/");
    char dirToCreate[1000];
    memset(dirToCreate, '\0', 1000);
    while (token != NULL) {
        strcat(dirToCreate, token);
        strcat(dirToCreate, "/");
        mkdir(dirToCreate, 0700);
        token = strtok(NULL, "/");
    }

    free(withoutFile);
}

int connectToServer(urlStruct *URL){
    //socket
    int sd;
    if((sd=socket(PF_INET, SOCK_STREAM, 0))<0)
    {
        perror("socket");
        exit(1);
    }

    struct hostent *hp; /*ptr to host info for remote*/ //for DNS uses
    struct sockaddr_in peeraddr;// for socket

    peeraddr.sin_family = AF_INET;//IPv4
    hp = gethostbyname(URL->hostName); //
    if(hp==NULL){
        herror("getHostByName");
        exit(1);
    }
    peeraddr.sin_addr.s_addr = ((struct in_addr*)(hp->h_addr))->s_addr;//ip into peeraddr
    peeraddr.sin_port= htons(URL->port);//htons convert port to available num
    //ip
    //connect
    if(connect(sd, (struct sockaddr*) &peeraddr, sizeof(peeraddr)) < 0) {
        perror("connect");
        exit(1);
    }

        return sd;
}
    void HTTPRequest(urlStruct *URL,char* request)
    {

        strcat(request,"GET " );
        strcat(request,URL->path );
        strcat(request," HTTP/1.0\r\nHost: ");
        //strcat(request,"Host: ");
        strcat(request,URL->hostName);
        strcat(request,"\r\n\r\n");
       // "GET /index.html HTTP/1.0\r\nHost: www.jce.ac.il\r\n\r\n".
    }

    int main(int argc, char *argv[]) {
        if (argc != 2)
        {
            printf("Usage: proxy1 <URL>\n");
            exit(1);
        }
        unsigned char buff[50];
        int sd=0;
        urlStruct *URL = parsing(argv[1]);
        char filename[200];
        strcpy(filename, URL->hostName);
        strcat(filename, URL->path);
        if (filename[strlen(filename)-1] == '/') {
            strcat(filename, "index.html");
        }

        FILE *fp;

        if ((fp = fopen(filename, "r")) == NULL) { //the file isn't exist
            //printf("error with open file\n");
         sd= connectToServer(URL);
         int responseLength=0;
            char request[200];
            memset(request, '\0', 200);
            HTTPRequest(URL,request );
            printf("HTTP request =\n%s\nLEN = %d\n", request, (int)strlen(request));
         //printf("%s",request);// format dont forget

            if( write(sd,request, sizeof(request)) < 0) {
                perror("write");
                exit(1);
            }
//HTTP req// only if 200// read in lupe
//res
            memset(buff, 0,50);
            int count=0;
            FILE* fileToCreate;
            int responseOk=0, foundHeader=0;
            while((count=read(sd,buff, sizeof(buff))) > 0) {
               responseLength+=count;
               printf("%s", buff);
               //move to file only body as unsighed char
               if(!responseOk && strstr((char*)buff, "200 OK") != NULL)
               {
                   responseOk=1;
                   //create dir
                   creatDir(filename);
                   fileToCreate = fopen(filename, "w");
                   if (fileToCreate == NULL)
                   {
                       perror("fopen");
                       exit(1);
                   }
               }
               if (responseOk) {
                   if (!foundHeader) {
                       char* headerEnd = strstr((char*)buff, "\r\n\r\n");
                       if (headerEnd != NULL) {
                           foundHeader=1;
                           int diff= ((unsigned char*)headerEnd) - buff;
                           fwrite(buff + diff+4, 1, 50-diff-4, fileToCreate);
                       }
                    } else {
                       fwrite(buff, 1, count, fileToCreate);
                   }
               }
                memset(buff, 0,50);
            }
            if (responseOk)
            {
                fclose(fileToCreate);
            }
            printf("\n Total response bytes: %d\n",responseLength);
            //finish response
           // exit(1);
          close(sd);
        }//end if open
        else {
            printf("File is given from local filesystem\n");
            int totalCounter=0;
            fseek(fp, 0, SEEK_END);
            int size = ftell(fp);// size of file
            rewind(fp);
            char buffer[size];//get the file
            memset(buffer, '\0',size);
            char print[1000];
            memset(print, '\0',1000);
            strcpy(print,"HTTP/1.0 200 OK\r\nContent-Length:");
            ///-- convert size from int to char----///
            char sizeInChar [50];
            memset(sizeInChar, '\0',50);
            sprintf(sizeInChar, "%d", size);
            ///-----------///
            strcat(print,sizeInChar);
            strcat(print,"\r\n\r\n");
            printf("%s", print);
            ///--- read print file---///
            fread(buffer, size+1, 1, fp);
            printf("%s\n", buffer);
            ///----total response length ----
            totalCounter=strlen(print);
            totalCounter+=size;
            printf("\r\nTotal response:%d",totalCounter);
            fclose(fp);
        }

        free(URL);

        return 0;
    }
