#include <iostream> 
#include <winsock2.h>                                                   // Windows Socket API
#include <thread>
#include <mutex>
#include <vector>

#pragma comment(lib, "ws2_32.lib")                                      // Подключаем библиотеку ws2_32

int clientCount = 0;                                                    // Счетчик подключений
std::mutex clientCountMutex;                                            // Взаимное исключение для client
std::vector<SOCKET> clients;                                            // Динамический массив из SOCKET с "общим" именем clients
std::mutex clientsMutex;                                                // Взаимное исключение для clients

void NewClient(SOCKET clientSocket) {

            char buffer[256];                                           // Буфер для приема данных 
            int bufferSize = sizeof(buffer);                            // Сколь-ко байт можно прочитать

            while (true) {
            int bytesRead = recv(clientSocket, buffer, bufferSize, 0);  // recv(сокет с которого читаем, буфер получения, его размер, 0)
            if (bytesRead == 0) {                                       // Если > 0 - данные пришли, если = 0 - клиент закрыл соединение 
                break;
            }
            else if (bytesRead == SOCKET_ERROR) {                       // Проверка на ошибку
                break;
            }
            std::lock_guard<std::mutex> lockClients(clientsMutex);      
            for (SOCKET s : clients) {                                  // C++11 вместо for (i = 0; i < clients.size(); i++) { SOCKET s = clients[i]; } 
                int bytesSend = send(s, buffer, bytesRead, 0);          // send(сокет, буфер отправки, кол-во прочитанных байт, 0)
            }
            
            }

            std::lock_guard<std::mutex> lockClientCount(clientCountMutex);// Автоматическая блокировка
            clientCount--;                                              // Счетчик подключений --
            std::cout << "Client " << clientSocket << " unconnected \n";

            std::lock_guard<std::mutex> lockClients(clientsMutex);      // Автоматическая блокировка
            // clients.erase(std::remove(clients.begin(), clients.end(), clientSocket), clients.end()); // erase - удаляет элементы равные clientSocket,  remove - перемещает равные clientSocket элементы в конец
            auto it = std::remove(clients.begin(), clients.end(), clientSocket);// Находим первый элемент clientSocket, перемещаем в конец, it указывает на начало clientSocket
            clients.erase(it, clients.end());                           // Удаляем все от it до конца вектора
            closesocket(clientSocket);                                  // Закрытие клиент-сокета
 }

int main()
{
    //SOCKET mySocket;                                                  // Пустая переменная
    WSADATA data;                                                       // Объявлена структура WSADATA и записана в data
    WSAStartup(MAKEWORD(2, 2), &data);                                  // MAKEWORD(2, 2) - версия Winstock, &data - адрес струкртуры с системной информацией
    SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);               // AF_INET - IPv4, SOCK_STREAM - потоковая передача, IPPROTO_TCP - TCP

    sockaddr_in server;                                                 // Объявление структуры для хранения адреса и порта сервера
    server.sin_family = AF_INET;                                        // AF_INET - адресс IPv4
    server.sin_port = htons(54000);                                     // Присвоенеи порта 54000. Перевод числа из байт в сетевой порядок
    server.sin_addr.s_addr = INADDR_ANY;                                // Присвоение порта ко всем локальным IP-адресам

    //int bind(SOCKET s, const sockaddr*, int addrlen);                 // Привязка 'Socket s' к адресу 'server'
    if (bind(s, (sockaddr*)&server, sizeof(server)) == SOCKET_ERROR) {  // (sockaddr*)&server - приведение sockaddr_in к sockaddr*
        std::cout << "Bind failed\n";                                   // syzeof(server) - размер структуры в байтах, ОС нужно знать сколько памяти считать за адрес
    }                                                                   // SOCKET_ERROR константа = -1
    else {
        std::cout << "Bind succeeded\n";
    }

    //int listen(SOCKET s, int backlog);                                // listen() - Принятие клиентов,  backlog - максимальное кол-во подключений, которое система держит в очереди
    if (listen(s, SOMAXCONN) == SOCKET_ERROR) {                         // SOMAXCONN - системное максимальное значение
        std::cout << "Listen failed\n";
    }
    else {
        std::cout << "listen succeedeed \n";
    }
    
    while (true) {
        sockaddr_in client;                                             // Для каждого клиента создается структура
        int clientSize = sizeof(sockaddr_in);                           // Размер структуры клиента
        SOCKET clientSocket = accept(s, (sockaddr*)&client, &clientSize);//accept - берет одно входящее сообщение. s - дескриптор серверного сокета. sockaddr - адрес клиента. clientSize - размер

        if (clientSocket == INVALID_SOCKET) {                           //INVALID_SOCKET - если создание сокета не удалось
            std::cout << "Accept failed\n";                             
        }
        std::lock_guard<std::mutex> lockClients(clientsMutex);          // Автоматическая блокировка для vector clients
        clients.push_back(clientSocket);                                // 
        std::lock_guard<std::mutex> lockClientCount(clientCountMutex);  // Автоматическая блокировка вместо lock() / unlock()
        clientCount++;                                                  // Счетчик подключений ++
        std::cout << "Client " << clientSocket << " connected " << " Client count - " << clientCount << "\n";

        std::thread threadClient (NewClient, clientSocket);             // Взаимное исключение
        threadClient.detach();                                          // Освобождение ресурсов потока

    }
    //closesocket(s);                                                   // Закрытие сокета не требуется, так как цикл while бесконечный
    WSACleanup();                                                       // Завершение работы сетевой подсистемы
}

