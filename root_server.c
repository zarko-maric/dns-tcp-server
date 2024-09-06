#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define LOCAL_SERVER_PORT 6000
#define ROOT_SERVER_PORT 5000
#define BUFFER_SIZE 1024
#define MAX_LINE_LENGTH 256
#define MAX 100

pthread_mutex_t mutex;

struct DNSRecord { 
    char name[50];               //Declaration of the structure with the domain name and IP address 
    char ip_address[20];
};        

struct DNSRecord dns_root[MAX]; //Declaration of dns_root used for searching up for the IP address in the dns_list file

int num_records = 0;            //Variable used to track number of domain name's with IP addresses in dns_list file

void addToRoot(const char*, const char*);

char *lookup_ip_root(const char *query){
    int num_records = sizeof(dns_root) / sizeof(dns_root[0]);
    for (int i = 0; i < num_records; ++i) {                         //Function used for searching through dns_root if it contains a domain name
        if(strcmp(query, dns_root[i].name) == 0) {                  //from query and return it's IP address
            return dns_root[i].ip_address;
        }
    }

    return NULL;
}

void readLinesFromFile(const char *filename){                       //Function used for reading information from dns_list file which
    FILE *file = fopen(filename,"r");                               //contains a bigger base of domain names and add's to dns_root
    
    if(file == NULL){
        perror("Error, file dns_list couldn't be opened!");
    }
    char line[MAX_LINE_LENGTH];

    while(fgets(line,sizeof(line),file) != NULL){
        char name[MAX_LINE_LENGTH];
        char ip_address[MAX_LINE_LENGTH];

        size_t length = strlen(line);
        if (length > 0 && line[length - 1] == '\n') {
            line[length - 1] = '\0';
        }
        
        char *token = strtok(line, " ");
        if(token != NULL){
            strcpy(name, token);
            token = strtok(NULL," ");

            if(token != NULL){
                strcpy(ip_address,token);
                addToRoot(name,ip_address);
            }
        }else
            printf("Invalid line format: %s\n",line);
    }
    fclose(file);
}



void addToRoot(const char* name, const char* ip_address){
    pthread_mutex_lock(&mutex);
    strcpy(dns_root[num_records].name, name);                           //Function used for adding domain name and IP address to dns_root
    strcpy(dns_root[num_records].ip_address, ip_address);
    num_records++;
    pthread_mutex_unlock(&mutex);
}

int main() {
    int root_server_socket,local_server_socket,l;
    struct sockaddr_in root_server_address,local_server_address;
    char server_message[MAX_LINE_LENGTH];

                                                                         // Initialize mutex
    if (pthread_mutex_init(&mutex, NULL) != 0) {
        perror("Mutex initialization failed");
        exit(EXIT_FAILURE);
    }

    const char *file = "dns_list.txt";
    
    readLinesFromFile(file);

                                                                        //Creating root server socket
    root_server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (root_server_socket == -1) {
        perror("Root Server socket creation failed");
        exit(EXIT_FAILURE);
    }

    root_server_address.sin_family = AF_INET;
    root_server_address.sin_addr.s_addr = INADDR_ANY;
    root_server_address.sin_port = htons(ROOT_SERVER_PORT);

    const int enable=1;
    setsockopt(root_server_socket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));     //Used for the socket to be enabled for use without waiting for
                                                                                        //the last used socket to close

    if (bind(root_server_socket, (struct sockaddr *)&root_server_address, sizeof(root_server_address)) < 0) {
        perror("Root Server binding failed");
        exit(EXIT_FAILURE);
    }

    listen(root_server_socket, 5);

    printf("Root Server listening on port %d\n", ROOT_SERVER_PORT);

    l=sizeof(struct sockaddr_in);

    local_server_socket=accept(root_server_socket, (struct sockaddr *)&local_server_address, (socklen_t*)&l);

    if(local_server_socket<0){
        perror("accept failed");
        return 1;
    }
    printf("Connection accepted\n");

    recv(local_server_socket , server_message , MAX_LINE_LENGTH , 0);

    printf("Message received: %s\n", server_message);
    

    char *ip_address=lookup_ip_root(server_message);
    

    if(send(local_server_socket,ip_address,strlen(ip_address),0)<0){
        printf("Send error.");
    }

    close(root_server_socket);

    return 0;
}