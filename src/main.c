#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>

#include "include/file.h"
#include <errno.h>

volatile sig_atomic_t running = 1;

void on_signal_interrupt(int signal) {
    running = 0;
    (void)signal;
}
int main() {
    signal(SIGINT, on_signal_interrupt);
    // create a socket with ipv4(AF_INET),and bidirectional communication(SOCK_STREAM)
    int server_descriptor = socket(AF_INET,SOCK_STREAM,0);
    if (server_descriptor == -1){
        perror("error creating socket ");
        exit(1);
    }

    struct sockaddr_in address;    
    address.sin_family = AF_INET; // ipv4
    address.sin_port = htons(8080); // port 8080 (htons to set correct byte order)
    address.sin_addr.s_addr = INADDR_ANY; // set the in address
    
    // bind the socket to the address and port
    int bind_result = bind(server_descriptor,(struct sockaddr *)&address,sizeof(address)); 
    if (bind_result == -1){
        perror("error binding socket ");
        exit(1);
    }

    // listen for incoming connections
    int adress_queue_length = 10;
    listen(server_descriptor, adress_queue_length);
    if (listen(server_descriptor, adress_queue_length) == -1){
        perror("error listening for connections ");
        exit(1);
    }

    printf("server running on http://localhost:8080\n");
    printf("press ctrl+c to stop the server...\n");
    
    int client_descriptor;
    char buffer[1024];

    while (running)
    {
        client_descriptor = accept(server_descriptor, NULL, NULL);
        if (client_descriptor == -1){
            if (errno == EINTR){ // if interrupted break the while loop
                break;
            }
            perror("error accepting connection ");
            exit(1);
        }
        int bytes_read = read(client_descriptor, buffer, sizeof(buffer));
        if (bytes_read == -1){
            perror("error reading from client ");
            exit(1);
        }
        buffer[bytes_read] = '\0'; // null-terminate the buffer
        

        // read the contents of index.html
        u64 file_size;
        FILE* file = fopen("index.html", FILE_MODE_READ);
        char* html_contents = file_get_contents(file, &file_size);
        if (html_contents == NULL){
            perror("error reading index.html ");
            close(client_descriptor);
            close(server_descriptor);
            exit(1);
        }

        // send the HTTP response header
        char header[256];
        sprintf(
            header,
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/html\r\n"
            "Content-Length: %ld\r\n"
            "\r\n",
            file_size
        );

        // send the header
        if (write(client_descriptor, header, strlen(header)) == -1){
            perror("error writing header to client ");
            fclose(file);
            free(html_contents);
            close(client_descriptor);
            close(server_descriptor);
            exit(1);
        }

        // send the file contents
        if (write(client_descriptor, html_contents, file_size) == -1){
            perror("error writing file contents to client ");
            fclose(file);
            free(html_contents);
            close(client_descriptor);
            close(server_descriptor);
            exit(1);
        }

        fclose(file);
        free(html_contents);
        close(client_descriptor);
    }
    printf("\nshutting down server\n");
    close(server_descriptor);
    
    return 0;
}