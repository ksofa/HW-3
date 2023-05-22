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
#define MAX_OBSERVERS 100

int serverSocket;
int* fans; // внутри храним сокеты поклонников, индекс массива - индес поклонника соответственно.
int* observers;
int fansCount = 0; // Счетчик поклонников.
int N = 0;

void initFans(int N) {
    fans = malloc(N * sizeof(int)); // масив поклонников.
    
    // изначально поклонников нет.
    for (int i = 0; i < N; ++i) {
        fans[i] = -1;
    }
}

void initObservers() {
    observers = malloc(MAX_OBSERVERS * sizeof(int)); // масив поклонников.
    
    // изначально поклонников нет.
    for (int i = 0; i < MAX_OBSERVERS; ++i) {
        observers[i] = -1;
    }
}

bool isConnectionAlive(int socket) {
    // Создание набора файловых дескрипторов для проверки готовности чтения
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(socket, &readfds);

    // Установка таймаута на ноль, чтобы функция select сразу вернула результат
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;

    // Выполнение проверки готовности чтения
    int result = select(socket + 1, &readfds, NULL, NULL, &timeout);
    if (result == -1) {
        perror("Ошибка при вызове select");
        return false;  // Обработка ошибки...
    }

    return !FD_ISSET(socket, &readfds);
}

bool isThereAvaliableConnection(int N) {
    for (int i = 0; i < N; ++i) {
        if (fans[i] == -1) {
            continue;
        }
        
        if (isConnectionAlive(fans[i])) {
            return true;
        }
    }
    
    return false;
}

int chooseFan(int N) {
    printf("Студентка выбирает поклонника на встречу.\n");
    
    srand(time(NULL)); // для рандомного выбора поклонника.
    while (1) {
        int ind = rand() % N; // выбор поклонника для встречи.
        if (fans[ind] == -1) {
            continue;
        }
        
        if (isConnectionAlive(fans[ind])) {
            return ind;
        }
        
        // Все поклонники, отправившие валентинки отключились.
        if (!isThereAvaliableConnection(N)) {
            return -1;
        }
    }
}

void visit(int N) {
    int ind = chooseFan(N);
    if (ind == -1) {
        for (int i = 0; i < MAX_OBSERVERS; ++i) { // Оповещаем всех наблюдателей об изменении.
            if (observers[i] == -1) {
                continue;
            }
            
            send(observers[i], "-1", sizeof("-1"), 0);
        }
        
        printf("Все поклонники, отправившие валентинки отключились, типичные мужчины! Растроенная студентка идет гулять одна!\n");
        sleep(5); // Имитация прогулки.
        return;
    }
    
    for (int i = 0; i < MAX_OBSERVERS; ++i) { // Оповещаем всех наблюдателей об изменении.
        if (observers[i] == -1) {
            continue;
        }
        
        char buf[32];
        sprintf(buf, "%d", ind);
        send(observers[i], buf, 32, 0);
    }
    
    // Отправка ответа всем поклонникам.
    for (int i = 0; i < N; ++i) {
        if (ind == i) {
            if (send(fans[i], "1", sizeof("1"), 0) == -1) {
                printf("Поклонник c id: %d сбежал вовремя прогулки, ну и ну...\n", ind);
            } else {
                printf("Студентка пошла на встречу с поклонником: %d.\n", ind);
            }
            
            continue;
        }
        
        if (send(fans[i], "0", sizeof("0"), 0) == -1) {
            printf("Ошибка при отправке сообщения!");
            continue;
        }
    }
    
    sleep(5); // Имитация встречи.
    printf("Встреча завершена, студентка вновь готова к получению писем от поклонников!\n");
}

void clearConnections(int N) {
    for (int i = 0; i < N; ++i) {
        if (fans[i] == -1) {
            continue;
        }
        
        if (!isConnectionAlive(fans[i])) {
            fans[i] = -1;
            --fansCount;
            printf("Поклонник с id: %d отключился ранее, теперь его место свободно!.\n", i);
        }
    }
    
    // Удаляем отключившихся наблюдателей.
    for (int i = 0; i < MAX_OBSERVERS; ++i) {
        if (observers[i] == -1) {
            continue;
        }
        
        if (!isConnectionAlive(observers[i])) {
            observers[i] = -1;
        }
    }
}

void addFan(int N, int socket) {
    for (int i = 0; i < N; ++i) {
        if (fans[i] == -1) {
            printf("Получила валентинку от нового обажателя, его номер в списке: %d\n", i);
            fans[i] = socket;
            ++fansCount;
            
            char str[32];
            sprintf(str, "%d %d", fansCount, N); // Число фанатов, и ожидаемое их количество.
            for (int i = 0; i < MAX_OBSERVERS; ++i) { // Оповещаем всех наблюдателей об изменении.
                if (observers[i] == -1) {
                    continue;
                }
                
                send(observers[i], str, 32, 0);
            }
            
            return;
        }
    }
}

int findPosForObserver() {
    for (int i = 0; i < MAX_OBSERVERS; ++i) {
        if (observers[i] == -1) {
            return i;
        }
    }
    
    return -1;
}

void handleSIGPIPE(int signum) {
    // Чтобы не было ошибки при неожиданном отключении клиента, в самом коде все учитывается.
}


void handleSIGINT(int signal) {
    // Закрываем все клиентские сокеты.
    for (int i = 0; i < N; ++i) {
        if (fans[i] == -1) {
            continue;
        }
        
        close(fans[i]);
    }
    
    for (int i = 0; i < MAX_OBSERVERS; ++i) {
        if (observers[i] == -1) {
            continue;
        }
        
        close(observers[i]);
    }
    
    close(serverSocket);
    exit(0); // Завершаем программу.
}


int main(int argc, char *argv[])
{
    signal(SIGINT, handleSIGINT);
    signal(SIGPIPE, handleSIGPIPE);
    
    if (argc != 2) {
        printf("Необходимо запустить прогу со следующим параметром: количество поклонников.");
        exit(0);
    }
    
    N = atoi(argv[1]);
    if (N <= 0) {
        printf("На сегодня поклонников нет, студентка разочарованно проводит вечер одна.");
        exit(0);
    }
    
    initFans(N);
    initObservers();
    
    struct sockaddr_in serverAddr;
    
    // Создание сокета
    if ((serverSocket = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("Ошибка создания сокета");
        exit(EXIT_FAILURE);
    }
    
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(PORT);
    
    // Привязка сокета к указанному порту
    if (bind(serverSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
    {
        perror("Ошибка привязки сокета");
        exit(EXIT_FAILURE);
    }
    
    // Ожидание подключения клиентов, не более N поклонников.
    if (listen(serverSocket, N) < 0)
    {
        perror("Ошибка ожидания подключений");
        exit(EXIT_FAILURE);
    }
    
    printf("Студентка проснулась и ожидает %d валентинок!\n", N);
    
    int clientSocket;
    struct sockaddr_in clientAddr;
    socklen_t addrLen = sizeof(clientAddr);
    
    while (1) {
        clearConnections(N); // удаляем отключившихся клиентов и наблюдателей.
        // Принятие нового подключения от клиента
        if ((clientSocket = accept(serverSocket, (struct sockaddr *)&clientAddr, &addrLen)) < 0)
        {
            perror("Ошибка при принятии подключения\n");
            continue;
        }
        
        char buf[32];
        int receivedBytes = recv(clientSocket, buf, 32, 0);
        if (receivedBytes == -1) { // Ошибка при получении данных.
            close(clientSocket);
            printf("Ошибка при получении данных от только что подключившегося клиента!\n");
            continue;
        }
        
        if (receivedBytes == 0) {
            close(clientSocket);
            continue;
        }
        
        int opType = atoi(buf); // Тип клиента(наблюдатель или поклонник).
        
        if (opType == 1) {
            clearConnections(N);
            if (fansCount < N) {
                addFan(N, clientSocket);
            }
            
            if (fansCount == N) {
                visit(N); // Встреча с поклонником.
                
                for (int i = 0; i < N; ++i) {
                    if (fans[i] == -1) {
                        continue;
                    }
                    
                    fans[i] = -1;
                }
                
                fansCount = 0;
            }
            
            if (fansCount > N) {
                perror("Количество поклонников стало больше допустимого лимита!");
                break;
            }   
        } else {
            int pos = findPosForObserver();
            if (pos == -1) {
                printf("Сервер отклонил входящее подключение наблюдателя(превышен лимит на сервере).\n");
                close(clientSocket);
                continue;
            }
            
            printf("Наблюдатель успешно подключен!\n");
            char str[32];
            sprintf(str, "%d %d", fansCount, N); // Число фанатов, и ожидаемое их количество, а также индекс.
            send(clientSocket, str, 32, 0);
            observers[pos] = clientSocket;
        }
    }
    return 0;
}

