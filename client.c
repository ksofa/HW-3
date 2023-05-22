#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <pthread.h>
#include <stdbool.h>
#include <semaphore.h>
#include <string.h>
#include <errno.h>
#include <sys/select.h>
#include <poll.h>
#include <fcntl.h>
#include <stdlib.h>
#include <signal.h>
#include <arpa/inet.h>

#define PORT 16432
#define SERVER_IP "127.0.0.1"

int clientSocket;

void handleSIGPIPE(int signum) {
    printf("Соединение с сервером было разорвано!\n");
}

void handleSIGINT(int signal) {
    close(clientSocket);
    exit(0); // Завершаем программу.
}

int main()
{
    signal(SIGINT, handleSIGINT);
    signal(SIGPIPE, handleSIGPIPE);
    
    struct sockaddr_in serverAddr;
    
    // Создание сокета
    if ((clientSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Ошибка создания сокета");
        exit(EXIT_FAILURE);
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);

    // Преобразование IP-адреса из текстового в бинарный формат
    if (inet_pton(AF_INET, SERVER_IP, &(serverAddr.sin_addr)) <= 0) {
        perror("Ошибка преобразования адреса");
        exit(EXIT_FAILURE);
    }

    // Подключение к серверу
    if (connect(clientSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
        perror("Ошибка подключения");
        exit(EXIT_FAILURE);
    }
    
    if (send(clientSocket, "1", strlen("1"), 0) < 0) {
        close(clientSocket);
        perror("Ошибка при отправке данных о созданном поле серверу!\n");
        exit(EXIT_FAILURE);
    }
    
    printf("Валентинка успешно отправлена студентке, жду ответа.\n");
    
    char buf[32];
    int receivedBytes = recv(clientSocket, buf, 32, 0);
    if (receivedBytes == -1) { // Ошибка при получении данных.
        close(clientSocket);
        printf("Ошибка при получении данных от сервера!\n");
        exit(EXIT_FAILURE);
    }
    
    if (receivedBytes == 0) {
        printf("Соединение с сервером было разорвано(сервер был закрыт)!!!\n");
        close(clientSocket);
        exit(0);
    }
    
    if (buf == "0") {
        printf("Студентка отказалась пойти на встречу:(");
        exit(0);
    }
    
    if (buf[0] == '1') {
        printf("Студентка согласилась! Иду на встречу!!!");
        sleep(5); // имитация встречи.
        printf("Встреча завершена!!!\n");
    } else {
        printf("Студентка отказалась от встречи:(\n");
    }
    close(clientSocket);
    exit(0);
}

