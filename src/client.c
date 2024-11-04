#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>

#define PORT 8080
#define BUFFER_LEN 256

// Main function for client
int main(const int argc, char *argv[])
{
    int                clientSocket;
    struct sockaddr_in serverAddr;
    char               buffer[BUFFER_LEN];
    ssize_t            bytesReceived;

    const char *userFilter = NULL;
    const char *msgContent = NULL;
    int         option;

    if(argc < 3)
    {
        fprintf(stderr, "Usage: %s -f <filter> -m <message>\n", argv[0]);
        return EXIT_FAILURE;
    }

    // Parse command-line arguments
    while((option = getopt(argc, argv, "f:m:")) != -1)
    {
        switch(option)
        {
            case 'f':
                userFilter = optarg;
                break;
            case 'm':
                msgContent = optarg;
                break;
            default:
                fprintf(stderr, "Usage: %s -f <filter> -m <message>\n", argv[0]);
                return EXIT_FAILURE;
        }
    }

    if(userFilter == NULL || msgContent == NULL)
    {
        fprintf(stderr, "Error: Both filter and message are required\n");
        return EXIT_FAILURE;
    }

    // Create client socket
    clientSocket = socket(AF_INET, SOCK_STREAM, 0); //NOLINT(android-cloexec-socket)
    if(clientSocket < 0)
    {
        perror("Error creating socket");
        return EXIT_FAILURE;
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port   = htons(PORT);

    if(inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr) <= 0)
    {
        perror("Invalid address");
        close(clientSocket);
        return EXIT_FAILURE;
    }

    // Connect to the server
    if(connect(clientSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
    {
        perror("Connection failed");
        close(clientSocket);
        return EXIT_FAILURE;
    }

    snprintf(buffer, BUFFER_LEN, "%s\n%s", userFilter, msgContent);

    send(clientSocket, buffer, strlen(buffer), 0);

    // Receive the processed response
    bytesReceived = recv(clientSocket, buffer, BUFFER_LEN - 1, 0);
    if(bytesReceived > 0)
    {
        buffer[bytesReceived] = '\0';
        printf("Processed message: %s\n", buffer);
    }
    else
    {
        fprintf(stderr, "Error receiving response\n");
    }

    close(clientSocket);
    return EXIT_SUCCESS;
}
