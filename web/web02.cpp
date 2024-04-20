#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/sendfile.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

const int port = 8888;

int main(int argc, char *argv[]) {
    // 检查命令行参数的数量
    if (argc != 1) {
        fprintf(stderr, "Usage: %s\n", argv[0]);
        return 1;
    }

    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    char buffer[1024];
    ssize_t bytes_received;

    // 创建套接字
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("socket creation failed");
        return 1;
    }

    // 设置服务器地址信息
    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    // 绑定套接字
    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind failed");
        return 1;
    }

    // 开始监听
    if (listen(server_socket, 1) < 0) {
        perror("listen failed");
        return 1;
    }

    printf("Server is listening on port %d...\n", port);
    while (1) {
        // 接受客户端连接
        client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_len);
        if (client_socket < 0) {
            perror("accept failed");
            continue;
        }

        printf("Connected to client at %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        // 接收请求
        bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
        if (bytes_received <= 0) {
            perror("recv failed");
            close(client_socket);
            continue;
        }
        buffer[bytes_received] = '\0'; // 确保字符串以空字符结尾

        printf("Received request: %s\n", buffer);

        // 发送HTTP响应
        const char *response = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nConnection: close\r\n\r\n";
        if (send(client_socket, response, strlen(response), 0) < 0) {
            perror("send failed");
        }

        // 发送文件
        int file_descriptor = open("hello.html", O_RDONLY);
        if (file_descriptor < 0) {
            perror("open failed");
            close(client_socket);
            continue;
        }

        if (sendfile(client_socket, file_descriptor, NULL, 2500) < 0) {
            perror("sendfile failed");
        }

        // 关闭文件描述符和客户端套接字
        close(file_descriptor);
        close(client_socket);
    }

    // 关闭服务器套接字
    close(server_socket);

    return 0;
}