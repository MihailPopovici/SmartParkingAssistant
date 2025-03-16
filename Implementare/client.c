#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>

/* error code returned by some function calls */
extern int errno;
#define BUFFER_SIZE 1024

/* port used for connection with the server*/
int port;

int main (int argc, char *argv[])
{
    int sd;			//socket descriptor
    struct sockaddr_in server;	// struct used for connection
    char msg_to_server[10];		
    char buffer[BUFFER_SIZE];

    /* checking if there are all the necessary arguments */
    if (argc != 3)
    {
      printf ("Usage: %s <server_adress> <port>\n", argv[0]);
      return -1;
    }

    /* selecting the port */
    port = atoi (argv[2]);

    /* creatong the socket */
    if ((sd = socket (AF_INET, SOCK_STREAM, 0)) == -1)
    {
      perror ("[Client]Error at socket().\n");
      return errno;
    }

    /* filling the struct used by the server */
    /* socket family*/
    server.sin_family = AF_INET;
    /* IP server adress */
    server.sin_addr.s_addr = inet_addr(argv[1]);
    /* connection port */
    server.sin_port = htons (port);

    /* connecting to the server */
    if (connect (sd, (struct sockaddr *) &server,sizeof (struct sockaddr)) == -1)
    {
      perror ("[client]Error at connect().\n");
      return errno;
    }

    //Printing the current parking status
    memset(buffer, 0, BUFFER_SIZE);
    if(read(sd, buffer, BUFFER_SIZE)<=0){
      perror("[Client] Error at read() from server\n");
      return errno;
    }
    printf("%s", buffer);
    fflush(stdout);

    //Selecting the type of client we are working with
    printf("Select the type of client you are:\n");
    printf("\t 1)Not parked, I want to enter the parking\n");
    printf("\t 2)Already parked, I want to leave the parking\n");
    printf("Your choice(1/2):");
    fflush(stdout);
    char choice=0;
    scanf("%c", &choice);
    while(choice!='1' && choice!='2'){
        printf("Invalid choice. Choose again!\n");
        printf("Your choice(1/2):");
        fflush(stdout);
        scanf("%c", &choice);
    }
    //we are splitting the communication with the server based on the type of client
    if(choice=='1'){
        int running=1;
        while(running){
            printf("Enter your license plate (Format : LLNNLLL) (L-letter, N-number): ");
            fflush(stdout);
            memset(buffer, 0, sizeof(buffer));
            scanf("%s", buffer);
            while(!(buffer[0]>='A' && buffer[0]<='Z') || !(buffer[1]>='A' && buffer[1]<='Z')
              || !(buffer[4]>='A' && buffer[4]<='Z') || !(buffer[5]>='A' && buffer[5]<='Z')
              || !(buffer[6]>='A' && buffer[6]<='Z') || !(buffer[2]>='0' && buffer[2]<='9')
              || !(buffer[0]>='0' && buffer[3]<='9') || strlen(buffer)!=7){

                printf("Invalid input! Try again : ");
                fflush(stdout);
                scanf("%s", buffer);
            }
            //Sending to the server the type of client and its license plate
            memset(msg_to_server, 0, sizeof(msg_to_server));
            msg_to_server[0]=choice;
            strncpy(msg_to_server+2, buffer, 7);
            msg_to_server[9]='\0';
            if (write (sd, msg_to_server, sizeof(msg_to_server)) <= 0)
            {
              perror ("[client]Eroare la write() spre server.\n");
              return errno;
            }
            //getting and printing the response from the server
            memset(buffer, 0, BUFFER_SIZE);
            if(read(sd, buffer, BUFFER_SIZE)<=0){
                perror("[Client] Error at read() from server\n");
                return errno;
            }
            printf("%s", buffer);
            fflush(stdout);
            if(strstr(buffer, "invalid")==0) running=0;
        }
    }else if(choice=='2'){
          int running=1;
          while(running){
              printf("Enter your key (6 digits number): ");
              fflush(stdout);
              memset(buffer, 0, sizeof(buffer));
              scanf("%s", buffer);
              while(!(buffer[0]>='0' && buffer[0]<='9') || !(buffer[1]>='0' && buffer[1]<='9')
                || !(buffer[2]>='0' && buffer[2]<='9') || !(buffer[3]>='0' && buffer[3]<='9')
                || !(buffer[4]>='0' && buffer[4]<='9') || !(buffer[5]>='0' && buffer[5]<='9')
                || strlen(buffer)!=6){

                  printf("Invalid input! Try again : ");
                  fflush(stdout);
                  scanf("%s", buffer);
              } 
              //Sending to the server the type of client and its license plate
              memset(msg_to_server, 0, sizeof(msg_to_server));
              msg_to_server[0]=choice;
              strncpy(msg_to_server+2, buffer, 6);
              msg_to_server[8]='\0';
              if (write (sd, msg_to_server, sizeof(msg_to_server)) <= 0)
              {
                perror ("[client]Eroare la write() spre server.\n");
                return errno;
              }
              //getting and printing the response from the server
              memset(buffer, 0, BUFFER_SIZE);
              if(read(sd, buffer, BUFFER_SIZE)<=0){
                  perror("[Client] Error at read() from server\n");
                  return errno;
              }
              printf("%s", buffer);
              fflush(stdout);
              if(strstr(buffer, "Invalid")==0) running=0;
          }
    }
    /* closing the connection since we are done */
    close (sd);
    return 0;
}