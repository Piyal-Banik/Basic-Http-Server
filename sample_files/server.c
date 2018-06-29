
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h> 
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>

#define BYTES 1024
//Number of Clients
#define NUMCLIENTS 100
char *ROOT;
//used to connect multiple clients at the same time
int clients[NUMCLIENTS];

//Function sending response to the client
void respond(int slot);

int main(int argc, char **argv)
{
  	int clientNumber = 0;
	int sockfd, listenfd, portno; // clilen;
	struct sockaddr_in serv_addr, cli_addr;
	socklen_t clilen;
	if (argc < 2) 
	{
		fprintf(stderr,"ERROR, no port provided\n");
		exit(1);
	}
	
	ROOT = argv[2];

	listenfd = socket(AF_INET, SOCK_STREAM, 0);

	if(listenfd < 0) 
	{
		perror("ERROR opening socket");
		exit(1);
	}
  
	bzero((char *) &serv_addr, sizeof(serv_addr));

	portno = atoi(argv[1]);
	
	/* Create address we're going to listen on (given port number)
	 - converted to network byte order & any IP address for 
	 this machine */
	
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(portno);  // store in machine-neutral format

	 /* Bind address to the socket */
	
	if (bind(listenfd, (struct sockaddr *) &serv_addr,
			sizeof(serv_addr)) < 0) 
	{
		perror("ERROR on binding");
		exit(1);
	}
	if( listen(listenfd,64) <0){
		perror("ERROR on listening");
		exit(1);
	}

    clilen = sizeof(cli_addr);
	
	while (1){
		clients[clientNumber] = accept(listenfd, (struct sockaddr *) &cli_addr,&clilen);
		if(clients[clientNumber] < 0) 
		{
			perror("ERROR on accept");
			exit(1);
	
		}
		else{
			//if forked successfully then respond to the client
            if(fork() == 0){
            	respond(clientNumber);
            }
          	(void)close(clients[clientNumber]);
        }
        
        if(clientNumber < NUMCLIENTS) 
        	clientNumber++;
	}

	return 0;
}


void respond(int clientNumber){
	int bytes_read;
	int rcvd;
	char *reqline[3];
	char buffer[BYTES], path[BYTES];
  	char data_to_send[BYTES];
	char *content_type;
	
	bzero(buffer,BYTES);
	rcvd = read(clients[clientNumber],buffer,sizeof(buffer));

	if (rcvd<0)    // receive error
		perror("recv() error\n");
	else if (rcvd==0)   // receive socket closed
		perror("Client disconnected upexpectedly.\n");
	else{
		reqline[0] = strtok(buffer, " ");
		if(strcmp(reqline[0],"GET")==0){
			//used to check validity of the request
			reqline[1] = strtok (NULL, " \t");
			reqline[2] = strtok (NULL, " \t\n");
			if ( strncmp( reqline[2], "HTTP/1.0", 8)!=0 && strncmp( reqline[2], "HTTP/1.1", 8)!=0 )
			{
				write(clients[clientNumber], "HTTP/1.0 404\r\n", 25);
			}

			//if request is correct then open the file and read it and send it to the client
			else{
        		if(strstr(reqline[1], "html") != NULL){
        			content_type = "text/html";
          		} 
          		else if(strstr(reqline[1], "css")){
              		content_type = "text/css";
          		} 
          		else if(strstr(reqline[1], "js")){
              		content_type = "text/javascript";
          		} 
          		else if(strstr(reqline[1], "jpg")){
              		content_type = "image/jpeg";
          		}

				strcpy(path,ROOT);
				strcpy(&path[strlen(ROOT)],reqline[1]);
        		

        		if((open(path, O_RDONLY) != -1)){
          			char buffer[BYTES];
          			int bytesRead = 0;

          			FILE *file = fopen(path, "r");   
          			snprintf(data_to_send,sizeof(data_to_send), "HTTP/1.0 200 OK\r\n"
                    			                                "Content-Type: %s\r\n\r\n"
                                                      			,content_type);
					send(clients[clientNumber],data_to_send,strlen(data_to_send),0);
          			
          			//reads chunks of the file and stores it to buffer and then send it to the client
	          		if (file != NULL)    
	          		{
	              		while ((bytesRead = fread(buffer, 1, sizeof(buffer), file)) > 0)
	              		{
	                		send(clients[clientNumber],buffer,bytesRead,0);
	              		}
	          		}
        		} 

        		//file not found
        		else{
          			write(clients[clientNumber], "HTTP/1.0 404\r\n", 13);
        		}
			}
		}
		else{
			write(clients[clientNumber], "HTTP/1.0 404\r\n", 13);
		}
	}
}
	
		

