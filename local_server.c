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
    char name[50];          //Declaration of a structure with information used for converting domain name into IP address
    char ip_address[20];
};

struct ServerData {
    int local_server_socket;
    int root_server_socket;     //A structure used for storing needed information about local server sockets and 
    char buffer[BUFFER_SIZE];   //a buffer used for storing information needed for converting name to address
};

struct ServerData data;

struct DNSRecord dns_table[] = {
    {"www.example.com", "192.168.1.1"},
    {"mail.example.com", "192.168.1.2"},    //A sample of domain names that local server has,
    {"webserver.local", "192.168.1.3"},     //it is used for testing
    {"www.google.com", "142.251.40.100"}
};

void *handle_client(void *);
void *handle_server(void *);

char *lookup_ip(const char *query) {

    int num_records = sizeof(dns_table) / sizeof(dns_table[0]);     //Function lookup_ip is used for checking if dns_table holds
    for (int i = 0; i < num_records; ++i) {                         //a domain name that matches queary and then return its IP address
        if(strcmp(query, dns_table[i].name) == 0) { 
            return dns_table[i].ip_address;
        }
    }

    return NULL;
}

void *handle_root(void *arg){
    char root_msg[BUFFER_SIZE];

    if(send(data.root_server_socket, data.buffer, strlen(data.buffer), 0)<0){   //Function handle_root send data to root server if local server
        puts("Send failed handle_root");                                        //doesn't have the requested IP address
    }                                                                           //and receives the IP address from root server 

    if(recv(data.root_server_socket, root_msg, BUFFER_SIZE, 0)<0){
        puts("Recv failed handle_root");
    }
  
    strcpy(data.buffer, root_msg);

    return NULL;
}

void *handle_client(void *arg) {                                            //This function handle's client request's
    int client_socket = *((int *)arg);
    int flag=0;

    memset(data.buffer, 0, BUFFER_SIZE);
    if(recv(client_socket, data.buffer, BUFFER_SIZE, 0)<0){                    
        puts("Recv failed handle_client 1");
    }
    
    char *ip_address = lookup_ip(data.buffer);                              //After receiving query from client the local server uses lookup_ip
    if (ip_address != NULL) {                                               //to see if it has the IP address of the requested domain name.
        printf("Resolved %s to %s\n", data.buffer, ip_address);             //If found it sends the address to the client
        if(send(client_socket, ip_address, strlen(ip_address), 0)<0){
            puts("Send failed handle_client 1");
        }
        flag=1;
    } else {
        printf("Query for %s forwarded to root server\n", data.buffer);     //In case it hasn't found the IP address for the requested domain name
        handle_root(&data);                                                 //It sends the querry to the root server
    } 

    send(client_socket, data.buffer, strlen(data.buffer), 0);               //When resloved the local server sends the client

    if(flag==1) close(client_socket);

    return NULL;
}

void *local_server_handler(void *arg) {
    int local_server_socket = data.local_server_socket;

    while (1) {
        int client_socket = accept(local_server_socket, NULL, NULL);
        if (client_socket == -1) {
            perror("Error accepting connection from client");
            continue; // Continue to the next iteration
        }

        // Handle the client
        handle_client(&client_socket);
    }
}

int main() {
    int local_server_socket,root_server_socket;
    struct sockaddr_in local_server_address,root_server_address;

    root_server_socket = socket(AF_INET, SOCK_STREAM, 0);                       //In main we are creating sockets for communicaiton with both
    if (root_server_socket == -1) {                                             //clinet and root server
        perror("Local Server socket creation failed");
        exit(EXIT_FAILURE);
    }

    root_server_address.sin_family = AF_INET;
    root_server_address.sin_addr.s_addr = INADDR_ANY;
    root_server_address.sin_port = htons(ROOT_SERVER_PORT);

    connect(root_server_socket,(struct sockaddr*)&root_server_address,sizeof(root_server_address));

    // Setup local server socket
    local_server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (local_server_socket == -1) {
        perror("Local Server socket creation failed");
        exit(EXIT_FAILURE);
    }

    local_server_address.sin_family = AF_INET;
    local_server_address.sin_addr.s_addr = INADDR_ANY;
    local_server_address.sin_port = htons(LOCAL_SERVER_PORT);

    const int enable=1;
    setsockopt(local_server_socket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));

    if (bind(local_server_socket, (struct sockaddr *)&local_server_address, sizeof(local_server_address)) < 0) {
        perror("Local Server binding failed");
        exit(EXIT_FAILURE);
    }

    listen(local_server_socket, 5);
    printf("Local Server listening on port %d\n", LOCAL_SERVER_PORT);

    data.local_server_socket = local_server_socket;
    data.root_server_socket = root_server_socket;

    pthread_t local_thread;
    pthread_create(&local_thread, NULL, local_server_handler, (void *)&data);
       
    pthread_join(local_thread, NULL);

    close(local_server_socket);

    return 0;
}