#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <ctype.h>

#define PORT 8080
#define MAX_MSG_LEN 1024
#define MAX_ID_LEN 100
#define MAX_STORED_MSGS 500
#define MAX_CLIENTS 50

int sockfd;

// --- Data Structures ---

typedef struct {
    char msg[MAX_MSG_LEN];
    char sender_id[MAX_ID_LEN];
} ChatMessage;

typedef struct {
    char client_id[MAX_ID_LEN];
    int last_seen_index;
} ClientSession;

// Global Storage
ChatMessage message_store[MAX_STORED_MSGS];
int total_messages = 0;

ClientSession clients[MAX_CLIENTS];
int total_clients = 0;

// --- Helper Functions ---

// Decodes URL-encoded strings (e.g., "Hello%20World" -> "Hello World") 
void url_decode(char *src, char *dest) {
    char *p = src;
    char code[3] = {0};
    while (*p) {
        if (*p == '%') {
            memcpy(code, ++p, 2);
            *dest++ = (char)strtoul(code, NULL, 16);
            p += 2;
        } else if (*p == '+') {
            *dest++ = ' ';
            p++;
        } else {
            *dest++ = *p++;
        }
    }
    *dest = '\0';
}

// simple helper to get value of a specific key from a query string or body
// e.g., find "client_id" in "message=hi&client_id=123"
void get_param_value(char *data, const char *key, char *dest) {
    char query_key[100];
    sprintf(query_key, "%s=", key);
    
    char *start = strstr(data, query_key);
    if (start) {
        start += strlen(query_key);
        char *end = strchr(start, '&');
        if (end) {
            int len = end - start;
            strncpy(dest, start, len);
            dest[len] = '\0';
        } else {
            strcpy(dest, start);
        }
    } else {
        dest[0] = '\0'; // Not found
    }
}

// Finds or creates a session for a client ID to track what they have read
int get_client_index(char *client_id) {
    for (int i = 0; i < total_clients; i++) {
        if (strcmp(clients[i].client_id, client_id) == 0) {
            return i;
        }
    }
    // New client
    if (total_clients < MAX_CLIENTS) {
        strncpy(clients[total_clients].client_id, client_id, MAX_ID_LEN);
        clients[total_clients].last_seen_index = 0; // Start from beginning
        return total_clients++;
    }
    return 0; // Fallback to 0 if full
}

// --- Handlers ---

void handle_post(int clientfd, char *body) {
    char raw_msg[MAX_MSG_LEN], raw_id[MAX_ID_LEN];
    
    // Extract raw values
    get_param_value(body, "message", raw_msg);
    get_param_value(body, "client_id", raw_id);

    if (strlen(raw_msg) > 0 && total_messages < MAX_STORED_MSGS) {
        // Decode and store
        url_decode(raw_msg, message_store[total_messages].msg);
        url_decode(raw_id, message_store[total_messages].sender_id);
        
        printf("[NEW MSG] From: %s | Content: %s\n", message_store[total_messages].sender_id, message_store[total_messages].msg);
        
        total_messages++;
    }

    char response[] = 
        "HTTP/1.1 200 OK\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Connection: close\r\n\r\n"
        "OK";
    send(clientfd, response, strlen(response), 0);
}

void handle_get(int clientfd, char *url) {
    char raw_id[MAX_ID_LEN];
    char clean_id[MAX_ID_LEN];
    
    // Extract ID from URL: /receive?client_id=...
    char *query_start = strchr(url, '?');
    if (query_start) {
        get_param_value(query_start + 1, "client_id", raw_id);
        url_decode(raw_id, clean_id);
    } else {
        strcpy(clean_id, "unknown");
    }

    int client_idx = get_client_index(clean_id);
    int start_idx = clients[client_idx].last_seen_index;
    
    char buffer[4096] = "";
    int has_content = 0;

    // Collect messages this client hasn't seen yet
    for (int i = start_idx; i < total_messages; i++) {
        // Don't echo back my own messages (optional, but chat.html handles 'me' class locally)
        // But chat.html logic appends everything received. 
        // The standard logic is: Server sends everything, Client ignores own ID. 
        // However, to keep it simple with the provided HTML, we will send everything 
        // EXCEPT what the sender just sent, IF we wanted to filter.
        // But usually, for polling, we send everything new.
        
        if (strcmp(message_store[i].sender_id, clean_id) != 0) {
             strcat(buffer, message_store[i].msg);
             strcat(buffer, "\n");
             has_content = 1;
        }
    }

    // Update client's read cursor
    clients[client_idx].last_seen_index = total_messages;

    if (has_content) {
        char header[512];
        sprintf(header, 
            "HTTP/1.1 200 OK\r\n"
            "Access-Control-Allow-Origin: *\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: %lu\r\n"
            "Connection: close\r\n\r\n", strlen(buffer));
        
        send(clientfd, header, strlen(header), 0);
        send(clientfd, buffer, strlen(buffer), 0);
    } else {
        char response[] = 
            "HTTP/1.1 200 OK\r\n" // 200 OK with empty body is better for JS fetch than 204 sometimes
            "Access-Control-Allow-Origin: *\r\n"
            "Content-Length: 0\r\n"
            "Connection: close\r\n\r\n";
        send(clientfd, response, strlen(response), 0);
    }
}

void cleanup_handler(int sig) {
    printf("\nShutting down server...\n");
    close(sockfd);
    exit(0);
}

int main() {
    signal(SIGINT, cleanup_handler);
    signal(SIGTERM, cleanup_handler);

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) { perror("Socket failed"); return -1; }

    int opt = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt failed");
        return -1;
    }

    struct sockaddr_in servaddr;
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(PORT);

    if (bind(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
        perror("Bind failed");
        return -1;
    }

    if (listen(sockfd, 10) < 0) {
        perror("Listen failed");
        return -1;
    }

    printf("Chat Server started on port %d\n", PORT);

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t len = sizeof(client_addr);
        int clientfd = accept(sockfd, (struct sockaddr *)&client_addr, &len);

        if (clientfd < 0) {
            perror("Accept failed");
            continue;
        }

        char buffer[2048] = {0};
        int n = recv(clientfd, buffer, sizeof(buffer) - 1, 0);
        
        if (n > 0) {
            char method[10], url[200];
            sscanf(buffer, "%s %s", method, url);

            if (strcmp(method, "POST") == 0) {
                // Find start of body (after \r\n\r\n)
                char *body = strstr(buffer, "\r\n\r\n");
                if (body) {
                    handle_post(clientfd, body + 4);
                }
            } else if (strcmp(method, "GET") == 0) {
                handle_get(clientfd, url);
            }
        }
        
        close(clientfd);
    }
    return 0;
}



