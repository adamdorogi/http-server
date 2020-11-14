#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <netinet/in.h>

const int ADDRESS = INADDR_ANY;
const int PORT = 80;

const int MAX_REQUEST_SIZE = 4096;

typedef struct
{
    char *method;
    char *file;
    char *version;
} http_request;

void parse_http_request(char *request_text, http_request *request)
{
    char *first_line = strtok(request_text, "\r\n");
    request->method = strtok(first_line, " ");
    request->file = strtok(NULL, " ");
    request->version = strtok(NULL, " ");
}

void fulfill_http_request(int *socket, http_request *request)
{
    char response[] = "HTTP/1.1 200 OK\r\n\r\n<html><body><h1>Hello, world!</h1></body></html>";
    int response_size = send(*socket, response, sizeof(response) - 1, 0);
    if (response_size < 0)
    {
        printf("Error: Couldn't send response.\n");
        exit(response_size);
    }

    printf("**** Response sent (%d bytes) ****\n", response_size);
}

int main(int argc, char const *argv[])
{
    // Create socket
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0)
    {
        printf("Error: Couldn't create socket.\n");
        exit(server_socket);
    }

    printf("Server socket descriptor: %d\n", server_socket);

    // Define server address
    struct sockaddr_in server_address = {.sin_family = AF_INET,
                                         .sin_addr.s_addr = ADDRESS, // Config IP address
                                         .sin_port = htons(PORT)};   // Config port number (?)

    // Bind the socket to the server address
    int bind_status = bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address));
    if (bind_status < 0)
    {
        printf("Error: Couldn't bind socket to server address.\n");
        exit(bind_status);
    }

    printf("Bind status: %d\n", bind_status);
    printf("Server address: %d\n", server_address.sin_addr.s_addr);
    printf("Server port: %d\n", ntohs(server_address.sin_port));

    // Listen to incoming connections
    int listen_status = listen(server_socket, 5); /** TODO: number of connections? */
    if (listen_status < 0)
    {
        printf("Error: Couldn't listen to incoming connections.\n");
        exit(listen_status);
    }

    printf("Listen status: %d\n", listen_status);

    while (1)
    {
        printf("Waiting for connection...\n");

        // Accept incoming connection
        struct sockaddr_in client_address;
        socklen_t client_address_length = sizeof(client_address);
        int client_socket = accept(server_socket, (struct sockaddr *)&client_address, &client_address_length);
        if (client_socket < 0)
        {
            printf("Error: Couldn't accept incoming connection.\n");
            exit(client_socket);
        }

        int child_pid = fork();

        printf("============= CHILD PID: %d, PID: %d =============\n", child_pid, getpid());

        if (child_pid < 0) // Error
        {
            printf("Error: Couldn't create new process.\n"); /** TODO: continue? */
            exit(child_pid);
        }
        else if (child_pid == 0) // Child process
        {
            // Separate process to handle communication with established client on `client_socket`.
            // No longer need to listen to incoming connections.
            close(server_socket);

            printf("******** New connection ********\n");
            printf("Client socket: %d\n", client_socket);
            printf("Client address: %u\n", client_address.sin_addr.s_addr);
            printf("Client port: %d\n", ntohs(client_address.sin_port));

            // Receive client message
            char request_text[MAX_REQUEST_SIZE + 1];
            int request_size = recv(client_socket, &request_text, MAX_REQUEST_SIZE, 0);
            if (request_size < 0)
            {
                printf("Error: Couldn't receive request.\n");
                exit(request_size);
            }
            request_text[request_size] = '\0'; // Terminate request string

            printf("**** Data received (%d bytes) ****\n", request_size);
            printf("%s\n", request_text);
            http_request request;
            parse_http_request(request_text, &request);
            printf("Requested method: %s\n", request.method);
            printf("Requested file: %s\n", request.file);
            printf("Requested version: %s\n", request.version);
            printf("****************************\n");

            // Send a response back to the client
            fulfill_http_request(&client_socket, &request);

            close(client_socket);
            exit(0);
        }
        else // Main process
        {
            waitpid(-1, NULL, WNOHANG);
            close(client_socket);
        }
    }

    return 0;
}
