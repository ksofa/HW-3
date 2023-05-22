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
    // Чтобы прога не падала при отключении сервера.
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
    if ((clientSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Ошибка создания сокета");
        exit(EXIT_FAILURE);
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);

    // Преобразование IP-адреса из текстового в бинарный формат
    if (inet_pton(AF_INET, SERVER_IP, &(serverAddr.sin_addr)) <= 0)
    {
        perror("Ошибка преобразования адреса");
        exit(EXIT_FAILURE);
    }

    // Подключение к серверу
    if (connect(clientSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
    {
        perror("Ошибка подключения");
        exit(EXIT_FAILURE);
    }
    
    if (send(clientSocket, "0", strlen("0"), 0) < 0) {
        close(clientSocket);
        perror("Ошибка при отправке данных о созданном поле серверу!\n");
        exit(EXIT_FAILURE);
    }
    
    bool firstTimeConnected = true;
    while (1) {
        char buf[32];
        int receivedBytes = recv(clientSocket, buf, 32, 0);
        if (receivedBytes == -1) { // Ошибка при получении данных.
            close(clientSocket);
            printf("Ошибка при получении данных от сервера!\n");
            exit(EXIT_FAILURE);
        }
        
        if (receivedBytes == 0) {
            printf("Соединение с сервером было разорвано(сервер был закрыт)!!!");
            close(clientSocket);
            break;
        }
        
        if (firstTimeConnected) {
            printf("Наблюдатель: успешно подключен\n");
            firstTimeConnected = false;
        }
        
        int first, second;
        int n = sscanf(buf, "%d %d", &first, &second);
        
        if (n == 1) {
            if (first == -1) {
                printf("Студентка выбрала поклонника, а он сбежал со встречи, студентка идет гулять одна!\n");
                continue;
            }
            
            printf("Студентка идет гулять с поклонников с id: %d\n", first);
            continue;
        }
        
        printf("Текущее количество поклонников %d/%d\n", first, second);
    }

    return 0;
}

