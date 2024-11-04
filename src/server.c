#include <arpa/inet.h>
#include <ctype.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define PORT 8080
#define BUFFER_LEN 256
#define BACKLOG 5

void handleClient(int clientSocket);
int  processFilter(const char *filter, char *msg, size_t len);
void handleExit(int sig) __attribute__((noreturn));

int main(void)
{
    int                     serverSocket;
    struct sockaddr_in      serverAddress;
    struct sockaddr_storage clientAddress;
    socklen_t               addrLen = sizeof(clientAddress);

    // Setup signal handling for graceful exit
    signal(SIGINT, handleExit);

    // Create server socket
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if(serverSocket == -1)
    {
        perror("Error creating socket");
        exit(EXIT_FAILURE);
    }

    serverAddress.sin_family      = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port        = htons(PORT);

    // Bind socket to a port
    if(bind(serverSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0)
    {
        perror("Error binding socket");
        close(serverSocket);
        exit(EXIT_FAILURE);
    }

    if(listen(serverSocket, BACKLOG) < 0)
    {
        perror("Error listening on socket");
        close(serverSocket);
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d...\n", PORT);

    while(1)
    {
        pid_t pid;
        const int clientSocket = accept(serverSocket, (struct sockaddr *)&clientAddress, &addrLen);
        if(clientSocket < 0)
        {
            perror("Error accepting connection");
            continue;
        }

        pid = fork();
        if(pid < 0)
        {
            perror("Failed to fork");
            close(clientSocket);
            continue;
        }

        if(pid == 0)    // Child process
        {
            close(serverSocket);
            handleClient(clientSocket);
            close(clientSocket);
            exit(EXIT_SUCCESS);
        }
        else    // Parent process
        {
            close(clientSocket);
        }
    }
}

void handleClient(int clientSocket)
{
    char        buffer[BUFFER_LEN];
    const char *filterOption;
    char       *msgContent;
    char       *tokenState;

    ssize_t bytesRead = recv(clientSocket, buffer, BUFFER_LEN - 1, 0);
    if(bytesRead <= 0)
    {
        perror("Error receiving data from client");
        return;
    }
    buffer[bytesRead] = '\0';

    filterOption = strtok_r(buffer, "\n", &tokenState);
    msgContent   = strtok_r(NULL, "\n", &tokenState);

    if(processFilter(filterOption, msgContent, BUFFER_LEN) == -1)
    {
        perror("Error processing filter");
        return;
    }

    send(clientSocket, msgContent, strlen(msgContent), 0);
}

int processFilter(const char *filter, char *msg, size_t len)
{
    if(filter == NULL || msg == NULL)
    {
        fprintf(stderr, "Error: NULL filter or message content\n");
        return -1;
    }

    if(strcmp(filter, "upper") == 0)
    {
        for(size_t i = 0; i < len && msg[i] != '\0'; i++)
        {
            msg[i] = (char)toupper((unsigned char)msg[i]);
        }
        return 0;
    }

    if(strcmp(filter, "lower") == 0)
    {
        for(size_t i = 0; i < len && msg[i] != '\0'; i++)
        {
            msg[i] = (char)tolower((unsigned char)msg[i]);
        }
        return 0;
    }

    if(strcmp(filter, "none") == 0)
    {
        return 0;
    }

    return -1;
}

void handleExit(const int sig)
{
    (void)sig;
    _exit(EXIT_SUCCESS);
}
