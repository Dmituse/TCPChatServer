#include <iostream> 
#include <winsock2.h>                                                       // Windows Socket API
#include <thread>
#include <mutex>
#include <vector>
#include <algorithm>

#pragma comment(lib, "ws2_32.lib")                                          // Подключаем библиотеку ws2_32


struct Client {
    int id;
    SOCKET socket;
};


class Server {
private:
    std::vector<Client> clients;                                            // Динамический массив из SOCKET с "общим" именем clients
    std::mutex clientsMutex;                                                // Взаимное исключение для clients
    std::atomic<int> nextClientId{ 1 };                                     // Можем заменить две строчки на (int ClientID = 1; )

public:
    void start() {
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
                                                                                       
            Client newClient;
            newClient.id = nextClientId++;
            newClient.socket = clientSocket;
            clients.push_back(newClient);
            size_t count;
            {
                std::lock_guard<std::mutex> lockClients(clientsMutex);      // Автоматическая блокировка для vector clients
                count = clients.size();
            }
                std::cout << "Client " << clientSocket << " connected \n"
                    << "Online is " << count << " clients \n";
            
            std::thread(&Server::handleClient, this, clientSocket).detach();
        }
    }

    void handleClient(SOCKET clientSocket) {                                
         char buffer[256];                                                  // Буфер для приема данных 
         int bufferSize = sizeof(buffer);                                   // Сколь-ко байт можно прочитать
         while (true) {
             int bytesRead = recv(clientSocket, buffer, bufferSize, 0);     // recv(сокет с которого читаем, буфер получения, его размер, 0)
             if (bytesRead == 0) {                                          // Если > 0 - данные пришли, если = 0 - клиент закрыл соединение 
                 break;
             }
             broadcast(buffer, bytesRead, clientSocket);
         }
         Server::removeClient(clientSocket);
    }

    void broadcast(const char* message, int length, SOCKET sender = INVALID_SOCKET) {
        std::vector<Client> clientsCopy;                                    // Создании копии для mutex, чтобы не держать блокировку на весь цикл send
        {
            std::lock_guard<std::mutex> lock(clientsMutex);
            clientsCopy = clients;
        }
        for (Client c : clientsCopy) {                                      // C++11 вместо for (i = 0; i < clients.size(); i++) { SOCKET s = clients[i]; } 
            if (c.socket == sender) {
                continue;                               // Исключаем отправителя
            }
                int bytesSend = send(c.socket, message, length, 0);             // send(сокет, буфер отправки, кол-во прочитанных байт, 0)
            if (bytesSend == SOCKET_ERROR) {
                removeClient(c.socket);
            }
        }
    }

        void removeClient(SOCKET clientSocket) {
            std::lock_guard<std::mutex> lock(clientsMutex);
            clients.erase(
                std::remove_if(clients.begin(), clients.end(), [clientSocket](const Client& cl) {
                    return cl.socket == clientSocket;
                    }), clients.end()
                );
            closesocket(clientSocket);
        }

};

int main()
{
    Server server;
    server.start();
    //closesocket();                                                        // Закрытие сокета не требуется, так как цикл while бесконечный
    //WSACleanup();                                                         // Завершение работы сетевой подсистемы
}

