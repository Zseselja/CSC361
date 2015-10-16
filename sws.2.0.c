/* A simple udp server */
// Zachary Seselja
// V00775627

#include <time.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MAXBUFLEN 256   // max request size
#define OUTMAXBUF 1024 // The max outgoing packet buffer length

struct command { // struct for the request to go into.
    char *line;   
    char *protocal; 
    char *file;
     };

 // nc -u -s  192.168.1.100 10.10.1.100 32214 
// sending the data through the router and back.


//  sending msg to client 
void send_to_client(int sockfd, char *data, struct sockaddr_in cli_addr, int cli_len ){
  int total_bytes;

  if ((total_bytes = sendto(sockfd, data, strlen(data), 0,(struct sockaddr *)&cli_addr, cli_len)) == -1) {
    perror("sws: Failed to send data to client");
    exit(-1);
  
  }

}

//  printing log to server.
void print_Log(struct sockaddr_in cli_addr, char *request_header, char *response_header, char *response_file){


  char time_stamp[32];
  time_t current_time;
  struct tm *local_time;
  // getting the current time.
  current_time = time(NULL);
  local_time = localtime(&current_time);
  if (local_time == NULL){
    perror("sws: Failed to get the current time for logging");
    return;
  }
  // formatting the time for the log.
  if((strftime(time_stamp, 32, "%b %d %H:%M:%S", local_time)) == 0){
    perror("sws: Failed to render a time string for logging");
    return;
  }
  //  setting the IP var up to be printed
  char ip_string[64];
  sprintf(ip_string, "%s:%d", inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));

  
  // Build the final log string from the gathered data
  char log_string[1024];

  
 
  request_header[strlen(request_header)] = '\0';
  request_header[strlen(request_header) - 1] = '\0';

  // removing the newline char for formatting.

   //  If a file was passed in, generate the last semicolon and file path,
   //  otherwise do not include that information
  if(response_file != NULL){
    if (strncmp(request_header , "GET / HTTP/1.0" , 14) == 0){
       request_header = "GET / HTTP/1.0";
    }
    sprintf( log_string, "%s %s %s; %s; %s", time_stamp, ip_string, request_header, response_header, response_file);
  }else{

    sprintf(log_string,"%s %s %s; %s", time_stamp, ip_string , request_header , response_header);
  }
 
  

   //  printing log stream to server terminal.
  printf("%s\n", log_string);



}



 // checking if user is looking to get into the root dir.
char * find_404(char *value){
  if (strncmp (value , "/../" , 4) == 0){
          // printf("404 Not Found\n");
          value = "404 Not Found";
          return value;
        }else{
          return value;
        }

}


char * parser(struct command request){
    // token for tokenizing.
    char *token;
    token = strtok(request.line, " ");
    // See if request is GET request 
    if ((strncmp(token , "GET" , 3) == 0) || (strncmp(token , "get" , 3) == 0)){
        
        token = strtok(NULL, " ");
              
         if (token != NULL){
       // checking that the is a request for file
        if (token[0] == '/'){
	         request.file = (char * ) malloc ( MAXBUFLEN * sizeof(char));
           strcpy(request.file , token);
            }else{
               return "400 Bad Request";
            }
        }else{
            return "400 Bad Request";
        }
        
        token = strtok(NULL, " ");
                
        if (token != NULL){
            // SWS only takes in HTTP/1.0
            if ((strncmp(token, "HTTP/1.0" , 8) == 0) || (strncmp(token, "http/1.0" , 8) == 0)){
                request.protocal = (char * ) malloc ( MAXBUFLEN * sizeof(char));
                strcpy(request.protocal, token);   
                   
            }else{
                
                return "400 Bad Request";
            }
        }else{
                
                return "400 Bad Request";
        }
        // if request.file is not empty return it.
        if (strcmp(request.file, "") !=0){
             return request.file; 
        }
         
           
    }
    // got edge case.
    if ((strcmp(token , "got") == 0) || (strcmp(token , "GOT") == 0)){
        return "400 Bad Request";
    }
    return "400 Bad Request";
    
    
}




// =======================================================
// Beginging of SWS
int simple_web_server(int port , char *dir){
     

     int sockfd; // socket var
     socklen_t cli_len; 
     char request_header[MAXBUFLEN];	//data request_header in
     char out_buffer[OUTMAXBUF]; //out buffer
     struct sockaddr_in serv_addr; // serv info
     struct sockaddr_in cli_addr;	//we need these structures to store socket info
     int numbytes; // return value of recvfrom
     FILE *fp; // filename
      

     

     //The first step: creating a socket of type of UDP
     //error checking for every function call is necessary!
     
     if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1){
	     perror("sws: Failed to open the socket");
	     return -1;
     }

// ----------------------------------------------------- Notes from example code---------------------
      // prepare the socket address information for server side:
      // (IPv4 only--see struct sockaddr_in6 for IPv6)

 //    struct sockaddr_in {
	// short int          sin_family;  // Address family, AF_INET
	// unsigned short int sin_port;    // Port number
	// struct in_addr     sin_addr;    // Internet address
	// unsigned char      sin_zero[8]; // Same size as struct sockaddr
 //     };
// ----------------------------------------------------- Notes from example code---------------------
        
     memset(&serv_addr, 0, sizeof(struct sockaddr_in));	      
     serv_addr.sin_family = AF_INET;	//Address family, for us always: AF_INET
     serv_addr.sin_addr.s_addr = inet_addr("10.10.1.100"); //INADDR_ANY; //Listen on any ip address I have
     serv_addr.sin_port = htons(port);  //byte order a                       gain 

     // *** use setsockopt() //// with level SOL SOCKET and option REUSEADDR
     int optvalue = 1;
     setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optvalue, sizeof (optvalue)); // setting the options so the socket can 

     //Binding the socket with the address information: 
     if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0){
    	  perror("sws: Failed on binding socket!");
    	  return -1;
     }

     
     

     // printing intial start statement 
      printf("sws is running on UDP port %d and serving %s\npress 'q' to quit...\n", port , dir);

     
       
       fd_set read_fds;
       char *read_boof = NULL;
       int  select_result;
       size_t len = 2;

     while(1){
     
        // SELECT 
       // --------------------------------------------------------------------------
       // Waiting on user to print "q" to exit sws sever.


       FD_ZERO( &read_fds );
       FD_SET(sockfd , &read_fds);
       FD_SET( STDIN_FILENO, &read_fds );
        
       select_result = select( sockfd + 1, &read_fds, NULL, NULL, NULL );
       if ( -1 == select_result){
        printf( "Select Error \"select_result\"\n" );
       }
       
       if(FD_ISSET(STDIN_FILENO,  &read_fds)){

         getline(&read_boof , &len , stdin);

            
            if ((strncmp(read_boof , "q" , 1)==0) && strlen(read_boof) == 2 ){
                                                   
                    sleep(1.5);    // Sleep for a clean exit
                    close(sockfd); // Closing socket before clean exit
                    exit(1);       // Leave server. 
                       
            
          }else{
            printf("press 'q' to quit.\n");
            FD_ZERO( &read_fds );
             
          }

        }
// --------------------------------------------------------------------------------------------- select statement
     

     // cli_len is the length of the client address struct
     //the sender address information goes into cli_addr
     cli_len = sizeof(cli_addr); 
     if ((numbytes = recvfrom(sockfd, request_header, MAXBUFLEN-1 , 0,(struct sockaddr *)&cli_addr, &cli_len)) == -1) {
         perror("sws: error on recvfrom()!");
         return -1;
     }

 
    // send to parser.
    // storing request data in.
    struct command request;
    char *response_header;
    request_header[numbytes] = '\0';
    int size;

    request.line = (char * ) malloc ( MAXBUFLEN * sizeof(char));
    strcpy(request.line, request_header);
    
    char *value;
    value =  (char * ) malloc ( MAXBUFLEN * sizeof(char));
    
    value = parser(request); // reads and parses input from client
    value = find_404(value); // checks for root access attempt /../
    // looking if the return is 404
    if (strncmp( value , "404" , 3) == 0){
     response_header = "HTTP/1.0 404 Not Found";
     print_Log(cli_addr, request_header, response_header, NULL);
     response_header = "HTTP/1.0 404 Not Found\n";
     send_to_client( sockfd, response_header , cli_addr, cli_len);
     
    // if value  is not 400 error
    }else if ((strncmp ( value , "400" , 3) != 0) ){
          // send index file for '/'
        if (strcmp (value , "/") == 0){
          value = "index.html";  
        }else{
          // increasing value of pointer to remove "/"
         value++;
        }
           //trying to open file 
        fp = fopen( value , "r");
        
        if (NULL == fp){
         response_header = "HTTP/1.0 404 Not Found";
         print_Log(cli_addr, request_header, response_header, NULL);
         response_header = "HTTP/1.0 404 Not Found\n";
         send_to_client( sockfd, response_header , cli_addr, cli_len);
         

        }else{
          // looking to see if we need a '/' or not
          int dir_size = strlen(dir);
          char dir_c[dir_size];
          strcpy(dir_c , dir);
          response_header = "HTTP/1.0 200 OK";
         
          // looking to see if dir has "/" on the end
          if (dir[dir_size - 1] ==  '/'){
            // cat the file name to the dir name
            value = strcat(dir , value);
         
          }else{
            // add in '/' then cat the file name to the dir name
            strcat(dir , "/");
            value = strcat(dir , value);
            
          }
          
          // print the log to the server.
          print_Log(cli_addr, request_header, response_header, value);
          //  add \n for formatting then send to client.
          response_header = "HTTP/1.0 200 OK\n";
         
         // send "HTTP/1.0 200 OK" to Client to let them know file exsists.
          int total_bytes;
          if ((total_bytes = sendto(sockfd, response_header, strlen(response_header), 0,(struct sockaddr *)&cli_addr, cli_len)) == -1) {
            perror("sws: Failed to send data to client");
            exit(-1);
          
          }
         

          strcpy(dir , dir_c); //makes it so the strcat doesn't add up.

          // do while to read in file into buffer.
          do {
                         
              size = fread(out_buffer, 1, sizeof(out_buffer), fp);
                      
             // When the buffer is no logner full, we need to manually set
             // the null terminator at the end of the input size
               
              if(size < sizeof(out_buffer))
              {
                out_buffer[size] = '\0';
              }
              // send msg to client
              send_to_client(sockfd, out_buffer, cli_addr, cli_len);
                  
            }while(size > 0);

            send_to_client(sockfd, out_buffer, cli_addr, cli_len);
                     
             // Close and free the resources that were opened for this request
             fclose(fp);
          
        }

       
    }else{
      // Bad request 
     response_header = "HTTP/1.0 400 Bad Request";
     print_Log(cli_addr, request_header, response_header, NULL);
     response_header = "HTTP/1.0 400 Bad Request\n";
     send_to_client( sockfd, response_header , cli_addr, cli_len);
     
    }

  
  }//end of while loop
  // close socket
  close(sockfd);  
  return 0;
}


//  Main to begin SWS
int main(int argc, char *argv[]){
  char *dir = argv[2];  //read the port number value from stdin
  int port = atoi(argv[1]);  //read the port number value from Argv

  
  // If user entered <dirname> then <portnumber>
   if (port == 0) {
         printf( "Usage: %s <port>\n", argv[0] );
         fprintf(stderr,"ERROR, no port provided\n");
         return -1;
    } 

   if (argc < 2) {
         printf( "Usage: %s <port>\n", argv[0] );
         fprintf(stderr,"ERROR, no port provided\n");
         return -1;
     }

    simple_web_server(port , dir);
    return 0;



}



