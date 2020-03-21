#include <pthread.h>
#include <stdio.h>
#include <errno.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <pthread.h>

#define PORT 8888
#define SIZE 8096
#define BACKLOG 10
#define MIMETYPE "mime-types.tsv"


void report(struct sockaddr_in *serverAddress);


//get mime type searches the file name for the extension then searches the mime types
//file and matches the extension to the file type. It returns the type of the file
char * get_mime_type(char *name) {
   // printf("GETTING MIME TYPE\n");
    char *ext = strrchr(name, '.');
    char delimiters[] = " ";
    char *mime_type = NULL;
    mime_type = malloc (128 * sizeof(char)) ;
    bzero(mime_type, 128);
    char line[128];
    bzero(line, 128);
    char *token;
    int line_counter = 1;
    ext++;
    FILE *mime_type_file = fopen(MIMETYPE, "r");
    //printf("EXTENTION IS: %s\n", ext);
    char *saveptr;
    if (mime_type_file != NULL) {
        while(fgets(line, sizeof line, mime_type_file) != NULL) {
            if (line_counter > 1){
                 if((token = strtok_r(line, delimiters, &saveptr)) != NULL) {
                      if(strcmp(token,ext) == 0) {
			  printf("TYPE FOUND\n");
                          token = strtok_r(NULL, delimiters, &saveptr);
			  if(token != NULL) {
			    strcpy(mime_type, token);
			    break;
			  } else {
				  printf(">>>>>>>>>>>>>>>>>>>>>>>>>>The token is NULL for extention: %s\n", ext);
			  }
		      }	
		 }

	    }
	    line_counter++;
            bzero(line, 128);
	}
	fclose( mime_type_file );
    }
    else{
        perror("in open");
    }
    return mime_type;
}


void sendFileBack(int socket, char*rURI){

    //check filepath
    if(strcmp(rURI, "./")==0){
        printf("HOME\n");
	strcpy(rURI, "./index.html");
    }

}
//opens and reads file, forms header, and sends the response
void sendResponse(int socket, char*buf){
    char newline[] = "\n";
    char *saveptr;
    char *ptr = strtok_r(buf, newline, &saveptr);
    char space[] = " ";
    char *saveptr2;
    char *rptr = strtok_r(ptr, space, &saveptr2);
    char rmethod[200];
    bzero(rmethod, 200);
    strcpy(rmethod, rptr);
    printf("Method is: %s< in thread: %ld\n", rmethod, pthread_self());
    rptr = strtok_r(NULL, space, &saveptr2);
    char rURI[2048];//255
    bzero(rURI, 2048);
    strcpy(rURI, ".");
    strcat(rURI, rptr);
    //printf("URI is: %s\n", rURI);
    if(strcmp(rmethod, "GET") == 0){
       //printf("method is GET\n");
       sendFileBack(socket, rURI);
    }
    FILE *fileData = fopen(rURI, "rb");
    long fileSize;
    char *ftype;
    char responseData[SIZE];
    if(fileData == NULL){
        printf("Cannot retrieve file\n");

        char *em = "File not found";
        strcpy(responseData, "HTTP/1.1 404 Not Found\r\n"); //this should be 404: File not found.
        strcat(responseData, "Connection: keep-alive\r\n");
        strcat(responseData, "Keep-Alive: timeout=10\r\n");
	strcat(responseData, "Content-Type: ");
	strcat(responseData, "text/plain\r\n");
	strcat(responseData, "Content-Length: 14\r\n\r\n");
	strcat(responseData, em);
        
        send(socket, responseData, strlen(responseData), 0);
	printf("Sent: %s\n", responseData);
    } else {
        printf("File opened successfully!\n");
        struct stat finfo;
	stat(rURI, &finfo);
	fileSize = finfo.st_size;
	//printf("File size is: %ld\n", fileSize);
        char fSize[255];
	sprintf(fSize, "%ld", fileSize);
       // printf ("file size: %ld \n", fileSize);
	//getting file type
	char *name = rURI;
	ftype = get_mime_type(name);
	char ftypes[255];
	if(ftype != NULL){
	strcpy(ftypes,ftype);
	} else {
		strcpy(ftypes, "application/octet-stream");
	}
	printf("File type is: %s\n", ftypes);
        free(ftype);	

	//build header
        strcpy(responseData, "HTTP/1.1 200 Document Follows\r\n");
        strcat(responseData, "Connection: keep-alive\r\n");
        strcat(responseData, "Keep-Alive: timeout=10\r\n");
	strcat(responseData, "Content-Type: ");
	strcat(responseData, ftypes);
	strcat(responseData, "Content-Length: ");
	strcat(responseData, fSize);
	
	//KEEP ALIVE INFO HERE
	//*****************************************************************************
	strcat(responseData, "\r\n\r\n");
	printf("Response Header: %s\n", responseData);
        
        int readS = 0;
        int totalRead = 0;
	//send Header
	send(socket, responseData, strlen(responseData), 0);
        //send file
	bzero(responseData, SIZE);
        fseek(fileData, 0, SEEK_SET);
	do{
            readS = fread(responseData, 1, SIZE, fileData);
	    send(socket, responseData, readS, 0);
            if (readS >= 0){
	       totalRead = totalRead + readS;
            }
	    bzero(responseData, SIZE);

	}while(totalRead < fileSize); 
	printf("Sent %d \n", totalRead);
        fclose(fileData);
    }

}
//receives requests from socket and sends them to sendResponse
void *setPage(void * input){
    int clientS = *((int *)input);
    unsigned char responseData[SIZE];
    bzero(responseData, SIZE);
    int requestLen = 0;
    //char *buf;
    char buf[SIZE];// = malloc(sizeof(char)*SIZE);
    int numRequestsServedFromKeepAlive = 0;
    do{
      printf("client socket is: %d in thread:%ld\n", clientS, pthread_self());
    //char *bptr = buf;
    bzero(buf, SIZE);
    struct timeval tv = {10,0}; // ten second timeout
    setsockopt( clientS, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv) ) ;
    int valread = recv(clientS, buf, sizeof(buf)-1, 0);
    
      requestLen = strlen(buf);
      printf("requestLen: %d in thread %ld\n", requestLen, pthread_self());
      if(requestLen > 0){
          numRequestsServedFromKeepAlive++;
          printf("request in thread: %ld:\n%s\n", pthread_self(), buf);
          sendResponse(clientS, buf);
      }
      //free(buf);
    } while(requestLen > 0);

    close(clientS);
    free(input);
    printf(">>>>>>>>>>>>>>> Total number of requests served from one thread: %d\n", numRequestsServedFromKeepAlive);
    return NULL;
}
//main function sets up socket and threads
int main(void){
    int serverSocket = socket( AF_INET, SOCK_STREAM, 0);
    //local addr struct
    struct sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(PORT);
    serverAddress.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    //bind socket
    bind(serverSocket, (struct sockaddr *) &serverAddress, sizeof(serverAddress));
    //listen
    int listening = listen(serverSocket, BACKLOG);
    if(listening < 0){
        printf("Server not listening\n");
	return 1;

    }
    report(&serverAddress);
    struct timeval tv = {10,0}; // ten second timeout
    
    while(1){
        int clientSocket;
        char buf[SIZE];
	bzero(buf, SIZE);
        printf("waiting for connection\n");
	clientSocket = accept(serverSocket, NULL, NULL);
	printf("Socket Accepted, client socket:  %d \n", clientSocket);
        int *arg = malloc(sizeof(int));
        *arg = clientSocket;
        
        pthread_t tid;
        pthread_create(&tid, NULL, setPage, (void *) arg);
	
    }
    close(serverSocket);
    return 0;

}

void report(struct sockaddr_in *serverAddress){
    char hostBuffer[INET6_ADDRSTRLEN];
    char serviceBuffer[NI_MAXSERV];
    socklen_t addr_len = sizeof(*serverAddress);
    int err = getnameinfo((struct sockaddr *) serverAddress, addr_len, hostBuffer, sizeof(hostBuffer), serviceBuffer, sizeof(serviceBuffer), NI_NUMERICHOST);
    if(err != 0){
	    printf("Not working :/\n");
    }
    printf("\n\n\t Server listening on http://%s:%d\n", hostBuffer, PORT);
}

