#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>

#define PORT 2024
#define NR_OF_PARKING_SPOTS 20
#define BUFFER_SIZE 1024

//0-available
//1-NOT available

// ANSI color codes
#define RED "\033[31m"
#define GREEN "\033[32m"
#define RESET "\033[0m"
#define YELLOW "\033[33m"

/* error code returned by some function calls */
extern int errno;

int parking_spots[NR_OF_PARKING_SPOTS];
unsigned long spots_keys[NR_OF_PARKING_SPOTS];
pthread_mutex_t lock;

void get_parking_status(char* buffer){
    strcat(buffer, "Current parking status:\n");
    for(int i=0;i<NR_OF_PARKING_SPOTS;i++){
        char text[50];
        if(parking_spots[i]==0){
            snprintf(text, sizeof(text), GREEN "Spot %d: available\n" RESET, i+1);
        }else{
            snprintf(text, sizeof(text), RED "Spot %d: NOT available\n" RESET, i+1);
        }
        strcat(buffer, text);
    }
}

int get_available_spot(){
    for(int i=0;i<NR_OF_PARKING_SPOTS;i++){
        if(parking_spots[i]==0) return i+1;
    }
    return -1;
}

int get_spot_based_on_key(unsigned long key){
    for(int i=0;i<NR_OF_PARKING_SPOTS;i++){
        if(spots_keys[i]==key) return i+1;
    }
    return -1;
}

int check_for_thesame_key(unsigned long key){
    for(int i=0;i<NR_OF_PARKING_SPOTS;i++)
        if(spots_keys[i]==key) return 1;
    return 0;
}

// Function to generate a numeric hash key using djb2
unsigned long generate_numeric_key(const char *license_plate) {
    unsigned long hash = 5381; // Initialize hash value
    int c;

    // Process each character in the license plate
    while ((c = *license_plate++)) {
        hash = ((hash << 5) + hash) + c; // hash * 33 + c
    }

    return hash % 1000000; // Return a 6-digit key
}

void* treat_client(void* arg){
    int client = *(int*)arg;
    free(arg);
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, BUFFER_SIZE);

    get_parking_status(buffer);
    printf("[Server]: writing status to client...\n");
    if(write(client, buffer, BUFFER_SIZE)<=0){
        perror("[Server]: Eroare la write() catre client\n");
        close(client);
        return NULL;
    }

    char msg_from_server[10];
    if(read(client, &msg_from_server, sizeof(msg_from_server))<=0){
        perror("[Server]: Eroare la read() de la client\n");
        close(client);
        return NULL;
    }
    char choice=msg_from_server[0];
    if(choice=='1'){
        printf("Client that wants to park connected! (Option 1)\n");
        fflush(stdout);
        int running=1;
        while(running){
            char* license_plate = msg_from_server+2;
            int selected_spot=0;
            pthread_mutex_lock(&lock);
            selected_spot=get_available_spot();
            memset(buffer, 0, BUFFER_SIZE);
            if(selected_spot==-1){
                running=0;
                snprintf(buffer, BUFFER_SIZE, RED "No available spots in the parking. Come again later!\n" RESET);
            }else{
                unsigned long key = generate_numeric_key(license_plate);
                if(check_for_thesame_key(key)==0){
                    running=0;
                    spots_keys[selected_spot-1]=key;
                    parking_spots[selected_spot-1]=1;//We are making that parking spot NOT available
                    snprintf(buffer, BUFFER_SIZE, GREEN "You've been given the spot nr %d\n" RESET YELLOW "Your key is %lu\n" RESET, selected_spot, key);
                }else{
                    snprintf(buffer, BUFFER_SIZE, RED "The license plate you've given is invalid!\nThere is already a car with that license plate in the parking\n" RESET);
                }
            }
            pthread_mutex_unlock(&lock);
            if(write(client, buffer, BUFFER_SIZE)<=0){
                perror("[Server]: Eroare la write() catre client\n");
                close(client);
                return NULL;
            }
            if(running==1){
                //the license plate given as input is invalid (it is already in the parking)
                memset(msg_from_server, 0, sizeof(msg_from_server));
                if(read(client, &msg_from_server, sizeof(msg_from_server))<=0){
                    perror("[Server]: Eroare la read() de la client\n");
                    close(client);
                    return NULL;
                }
            }
        }
    }else{
        printf("Client that wants to leave connected! (Option 2)\n");
        fflush(stdout);
        int running=1;
        while(running){
            char* key_char=msg_from_server+2;
            unsigned long key=atoi(key_char);
            int selected_spot=get_spot_based_on_key(key);
            if(selected_spot==-1){
                snprintf(buffer, BUFFER_SIZE, RED "Invalid key! Try again\n" RESET);
            }else{
                pthread_mutex_lock(&lock);
                parking_spots[selected_spot-1]=0;
                spots_keys[selected_spot-1]=0;
                snprintf(buffer, BUFFER_SIZE, GREEN "Spot nr %d successfully leaved!\n" RESET, selected_spot);
                pthread_mutex_unlock(&lock);
                running=0;
            }
            if(write(client, buffer, BUFFER_SIZE)<=0){
                perror("[Server]: Eroare la write() catre client\n");
                close(client);
                return NULL;
            }
            if(selected_spot==-1){
                //we try to read another key
                memset(msg_from_server, 0, sizeof(msg_from_server));
                if(read(client, &msg_from_server, sizeof(msg_from_server))<=0){
                    perror("[Server]: Eroare la read() de la client\n");
                    close(client);
                    return NULL;
                }
            }
        }
    }

    close(client);
    return NULL;
}

int main(){

    struct sockaddr_in server;
    struct sockaddr_in from;
    int sd;//socket descriptor
    int optval=1; //option used for setsockopt() 

    /* socket creation */
    if ((sd = socket (AF_INET, SOCK_STREAM, 0)) == -1)
    {
      perror ("[server]Error at socket().\n");
      return errno;
    }

    /*we set the SO_REUSEADDR option to the socket */ 
    setsockopt(sd, SOL_SOCKET, SO_REUSEADDR,&optval,sizeof(optval));

    /* filling the struct used by the server */
    /* socket family */
    server.sin_family = AF_INET;	
    /* accepting any adress*/
    server.sin_addr.s_addr = htonl (INADDR_ANY);
    /* setting the predefined port */
    server.sin_port = htons (PORT);

    /* binding the socket */
    if (bind (sd, (struct sockaddr *) &server, sizeof (struct sockaddr)) == -1)
    {
      perror ("[server]Error at bind().\n");
      return errno;
    }

    /* making the server listen for clients that want to connect*/
    if (listen (sd, 5) == -1)
    {
      perror ("[server]Error at listen().\n");
      return errno;
    }

    printf("We are waiting for port %d\n", PORT);
    fflush(stdout);
    while(1){
        int *client=malloc(sizeof(int));
        int length=sizeof(from);

        /* accepting a client (blocking state until the connection is done) */
        *client = accept (sd, (struct sockaddr *) &from, &length);

        /* error while accepting the connection from a client */
        if (*client < 0)
	    {
	        perror ("[server]Error at accept().\n");
            free(client);
	        continue;
	    }

        //creating the thread for the accepted client
        pthread_t thread_id;
        pthread_create(&thread_id, NULL, treat_client, client);
        pthread_detach(thread_id);
    }
    pthread_mutex_destroy(&lock);
    return 0;
}