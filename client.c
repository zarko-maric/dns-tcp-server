#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define SERVER_ADDRESS "127.0.0.1"
#define SERVER_PORT 6000
#define BUFFER_SIZE 1024

int main() {
    int client_socket;
    struct sockaddr_in server_address;
    char query[BUFFER_SIZE], response[BUFFER_SIZE];

    client_socket = socket(AF_INET, SOCK_STREAM, 0);//Creating socket

    if (client_socket == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = inet_addr(SERVER_ADDRESS);
    server_address.sin_port = htons(SERVER_PORT);

    if (connect(client_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {//Connecting client to server
        perror("Connection failed");
        exit(EXIT_FAILURE);
    }

    printf("Enter DNS query: ");
    fgets(query, BUFFER_SIZE, stdin);//Getting from user domain name

    query[strlen(query) - 1] = '\0';//Removing white space from query

    send(client_socket, query, strlen(query), 0);

    memset(response, 0, BUFFER_SIZE);
    recv(client_socket, response, BUFFER_SIZE, 0);

    printf("DNS Response: %s\n", response);//IP address received from DNS

    close(client_socket);
    return 0;
}
