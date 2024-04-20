#include <cstdio>
#include <cstdlib>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <unistd.h>
#include <cstring>
#include <errno.h>
#include <dirent.h>
#include <map>

const int port = 8888;
const char* guestbook_path = "guestbook.txt"; // 留言板文件路径
const char* html_file_path = "hel.html"; // hello.html文件路径

// 简化的函数，用于删除文件
void remove_file(const char* path) {
    unlink(path); // 删除文件
}

// 发送文件内容给客户端
void send_file(int client_socket, const char* file_path) {
    FILE *file = fopen(file_path, "rb");
    if (file) {
        fseek(file, 0, SEEK_END);
        size_t file_size = ftell(file);
        fseek(file, 0, SEEK_SET);

        char *file_content = (char *)malloc(file_size);
        if (fread(file_content, 1, file_size, file) != file_size) {
            perror("fread failed");
            free(file_content);
            fclose(file);
            return;
        }

        const char *response = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nConnection: close\r\n\r\n";
        if (send(client_socket, response, strlen(response), 0) < 0) {
            perror("send failed");
        }

        if (send(client_socket, file_content, file_size, 0) < 0) {
            perror("send failed");
        }

        free(file_content);
        fclose(file);
    } else {
        perror("fopen failed");
        const char *response = "HTTP/1.1 404 Not Found\r\nContent-Type: text/html\r\nConnection: close\r\n\r\n";
        if (send(client_socket, response, strlen(response), 0) < 0) {
            perror("send failed");
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc != 1) {
        fprintf(stderr, "Usage: %s\n", argv[0]);
        return 1;
    }

    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    char buffer[1024];
    ssize_t bytes_received;

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("socket creation failed");
        return 1;
    }

    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind failed");
        return 1;
    }

    if (listen(server_socket, 1) < 0) {
        perror("listen failed");
        return 1;
    }

    printf("Server is listening on port %d...\n", port);
    while (1) {
        client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_len);
        if (client_socket < 0) {
            perror("accept failed");
            continue;
        }

        printf("Connected to client at %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
        if (bytes_received <= 0) {
            perror("recv failed");
            close(client_socket);
            continue;
        }
        buffer[bytes_received] = '\0'; // 确保字符串以空字符结尾

        printf("Received request: %s\n", buffer);

        // 检查是否是GET请求
        if (strncmp(buffer, "GET /", 5) == 0) {
            send_file(client_socket, html_file_path);
        } else if (strncmp(buffer, "DELETE /", 8) == 0) {
            remove_file(guestbook_path);
            const char *response = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nConnection: close\r\n\r\n";
            if (send(client_socket, response, strlen(response), 0) < 0) {
                perror("send failed");
            }
        } else {
            // 其他请求类型处理
            const char *response = "HTTP/1.1 400 Bad Request\r\nContent-Type: text/html\r\nConnection: close\r\n\r\n";
            if (send(client_socket, response, strlen(response), 0) < 0) {
                perror("send failed");
            }
        }

        close(client_socket);
    }

    close(server_socket);
    return 0;
}