#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#include <sys/stat.h>

#include "include/file.h"
#include <errno.h>

typedef int socket_descriptor_t;


volatile sig_atomic_t server_running = 1;

void on_signal_interrupt(int signal) {
    server_running = 0;
    (void)signal;
}

typedef enum {
    OK,
    ERROR_OPENING_FILE,
    ERROR_READING_FILE,
    ERROR_CREATING_SOCKET,
    ERROR_SETTING_SOCKET_OPTIONS,
    ERROR_BINDING_SOCKET,
    ERROR_LISTENING_SOCKET,
    ERROR_ACCEPTING_CONNECTION,
    ERROR_READING_FROM_CLIENT,
    ERROR_WRITING_TO_CLIENT
} ErrorCode;

// Helper function to get mime type based on file extension
const char* get_mime_type(const char* filepath) {
    const char* ext = strrchr(filepath, '.');
    if (ext == NULL) return "application/octet-stream";
    
    if (strcmp(ext, ".html") == 0) return "text/html";
    if (strcmp(ext, ".css") == 0) return "text/css";
    if (strcmp(ext, ".js") == 0) return "application/javascript";
    if (strcmp(ext, ".json") == 0) return "application/json";
    if (strcmp(ext, ".png") == 0) return "image/png";
    if (strcmp(ext, ".jpg") == 0 || strcmp(ext, ".jpeg") == 0) return "image/jpeg";
    if (strcmp(ext, ".gif") == 0) return "image/gif";
    if (strcmp(ext, ".svg") == 0) return "image/svg+xml";
    if (strcmp(ext, ".txt") == 0) return "text/plain";
    
    return "application/octet-stream";
}


void fatal_error(ErrorCode code) {
    switch (code)
    {
    case ERROR_CREATING_SOCKET:
        perror("error creating socket ");
        break;
    case ERROR_BINDING_SOCKET:
        perror("error binding socket ");
        break;
    case ERROR_LISTENING_SOCKET:
        perror("error listening for connections ");
        break;
    case ERROR_ACCEPTING_CONNECTION:
        perror("error accepting connection ");
        break;
    case ERROR_READING_FROM_CLIENT:
        perror("error reading from client ");
        break;
    case ERROR_WRITING_TO_CLIENT:
        perror("error writing to client ");
        break;
    default:
        perror("unknown error ");
        break;
    }
    exit(1);
}


ErrorCode create_server_socket(socket_descriptor_t* server_descriptor) {
    //ipv4 = AF_INET
    //bidirectional communication = SOCK_STREAM
    *server_descriptor = socket(AF_INET,SOCK_STREAM,0);
    if (*server_descriptor == -1){
        return ERROR_CREATING_SOCKET;
    }
    return OK;
}

ErrorCode allow_address_reuse(socket_descriptor_t server_descriptor) {
    // allow reusing the address to avoid address already in use errors
    int reuse = 1;
    if (setsockopt(server_descriptor, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) == -1) {
        return ERROR_SETTING_SOCKET_OPTIONS;
    }
    return OK;  
}

ErrorCode bind_server_socket_ipv4(socket_descriptor_t server_descriptor, int port) {
    struct sockaddr_in address = {0};    
    address.sin_family = AF_INET; // ipv4
    address.sin_port = htons(port); // port (htons to set correct byte order)
    address.sin_addr.s_addr = INADDR_ANY; // set the in address
    
    // bind the socket to the address and port
    int bind_result = bind(server_descriptor,(struct sockaddr *)&address,sizeof(address)); 
    if (bind_result == -1){
        return ERROR_BINDING_SOCKET;
    }

    return OK;
}

ErrorCode listen_for_connections(socket_descriptor_t server_descriptor, int address_queue_length) {
    // listen for incoming connections
    int listen_result = listen(server_descriptor, address_queue_length);
    if (listen_result == -1){
        return ERROR_LISTENING_SOCKET;
    }

    return OK;
}


ErrorCode accept_client_connection(socket_descriptor_t server_descriptor, socket_descriptor_t* client_descriptor) {
    *client_descriptor = accept(server_descriptor, NULL, NULL);
    if (*client_descriptor == -1){
        return ERROR_ACCEPTING_CONNECTION;
    }
    return OK;
}


ErrorCode read_client_request(socket_descriptor_t client_descriptor, char* buffer, size_t buffer_size, i64* bytes_read) {
    *bytes_read = read(client_descriptor, buffer, buffer_size);
    if (*bytes_read == -1){
        return ERROR_READING_FROM_CLIENT;
    }
    buffer[*bytes_read] = '\0'; 
    return OK;
}


void close_server_socket(socket_descriptor_t server_descriptor) {
    if (close(server_descriptor) == -1){
        perror("error closing socket ");
        exit(1);
    }
}

void send_404_response(socket_descriptor_t client_descriptor) {
    const char* not_found = "HTTP/1.1 404 Not Found\r\nContent-Type: text/html\r\nContent-Length: 60\r\n\r\n<html><body><h1>404 Not Found</h1></body></html>";
    if (write(client_descriptor, not_found, strlen(not_found)) == -1){
        perror("error writing 404 response to client ");
    }
}

void get_requested_file_path(const char* request_buffer, char* requested_path, size_t max_length) {
    sscanf(request_buffer, "GET %511s HTTP", requested_path);
    if (strcmp(requested_path, "/") == 0 || requested_path[0] == '\0') {
        strncpy(requested_path, "/index.html", max_length - 1);
        requested_path[max_length - 1] = '\0';
    }
}

ErrorCode get_requested_file(FILE** out_file, const char* request_buffer, char* out_filepath, size_t filepath_max) {
    char requested_path[512];
    get_requested_file_path(request_buffer, requested_path, sizeof(requested_path));
    
    // construct full file path
    char filepath[520]; // 512 for requested path + 8 for "hosted/"
    snprintf(filepath, sizeof(filepath), "hosted%s", requested_path);
    
    // try to open the requested file
    struct stat file_stat;
    if (stat(filepath, &file_stat) == 0 && S_ISREG(file_stat.st_mode)) {
        *out_file = fopen(filepath, FILE_MODE_READ);
        if (*out_file == NULL) {
            return ERROR_OPENING_FILE;
        }
        strncpy(out_filepath, filepath, filepath_max - 1);
        out_filepath[filepath_max - 1] = '\0';
        return OK;
    }
    
    // if file not found and path doesn't end with slash, try .html extension
    if (requested_path[strlen(requested_path) - 1] != '/') {
        char html_filepath[529]; // 512 for requested path + hosted%s.html"
        snprintf(html_filepath, sizeof(html_filepath), "hosted%s.html", requested_path);
        if (stat(html_filepath, &file_stat) == 0 && S_ISREG(file_stat.st_mode)) {
            *out_file = fopen(html_filepath, FILE_MODE_READ);
            if (*out_file == NULL) {
                return ERROR_OPENING_FILE;
            }
            strncpy(out_filepath, html_filepath, filepath_max - 1);
            out_filepath[filepath_max - 1] = '\0';
            return OK;
        }
    }
    
    // if still not found, try /index.html
    char dir_filepath[529]; // 512 for requested path + hosted%s/index.html"
    snprintf(dir_filepath, sizeof(dir_filepath), "hosted%s/index.html", requested_path);
    if (stat(dir_filepath, &file_stat) == 0 && S_ISREG(file_stat.st_mode)) {
        *out_file = fopen(dir_filepath, FILE_MODE_READ);
        if (*out_file == NULL) {
            return ERROR_OPENING_FILE;
        }
        strncpy(out_filepath, dir_filepath, filepath_max - 1);
        out_filepath[filepath_max - 1] = '\0';
        return OK;
    }
    
    return ERROR_OPENING_FILE;
}


ErrorCode send_file_response(socket_descriptor_t client_descriptor, FILE* file, const char* filepath) {
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char* file_contents = malloc(file_size);
    if (file_contents == NULL) {
        return ERROR_READING_FILE;
    }

    if (fread(file_contents, 1, file_size, file) != (size_t)(file_size)) {
        free(file_contents);
        return ERROR_READING_FILE;
    }

    const char* mime_type = get_mime_type(filepath);
    
    char header[256];
    int header_length = snprintf(header, sizeof(header), "HTTP/1.1 200 OK\r\nContent-Type: %s\r\nContent-Length: %ld\r\n\r\n", mime_type, file_size);
    
    if (write(client_descriptor, header, header_length) == -1){
        free(file_contents);
        return ERROR_WRITING_TO_CLIENT;
    }
    
    if (write(client_descriptor, file_contents, file_size) == -1){
        free(file_contents);
        return ERROR_WRITING_TO_CLIENT;
    }

    free(file_contents);
    return OK;
}


int main() {
    signal(SIGINT, on_signal_interrupt);

    ErrorCode result;
    socket_descriptor_t server_descriptor;

    result = create_server_socket(&server_descriptor);
    if (result != OK) fatal_error(result);
    
    result = allow_address_reuse(server_descriptor);
    if (result != OK) fatal_error(result);

    result = bind_server_socket_ipv4(server_descriptor, 8080);
    if (result != OK) fatal_error(result);

    result = listen_for_connections(server_descriptor, 10);
    if (result != OK) fatal_error(result);

    printf("server running on http://localhost:8080\n");
    printf("press ctrl+c to stop the server...\n");
    
    char buffer[1024 + 1]; // +1 for null terminator


    while (server_running)
    {
        socket_descriptor_t client_descriptor;

        result = accept_client_connection(server_descriptor, &client_descriptor);
        if (result != OK){
            if (errno == EINTR) break; // if interrupted by user close the server 
            fatal_error(result);
        }

        i64 bytes_read;
        result = read_client_request(client_descriptor, buffer, sizeof(buffer), &bytes_read);
        if (result != OK) fatal_error(result);

        FILE* file = NULL;
        char filepath[520];
        result = get_requested_file(&file, buffer, filepath, sizeof(filepath));
        if (result != OK) {
            send_404_response(client_descriptor);
            close(client_descriptor);
            continue;   
        }

        result = send_file_response(client_descriptor, file, filepath);
        if (result != OK) fatal_error(result);
        
        fclose(file);
        close(client_descriptor);

    }
    printf("\nshutting down server\n");
    close(server_descriptor);
    
    return 0;
}