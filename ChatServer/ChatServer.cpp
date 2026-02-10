#include <iostream> 
#include <winsock2.h>                                                   // Windows Socket API

#pragma comment(lib, "ws2_32.lib")                                      // Подключаем библиотеку ws2_32

int main()
{
    SOCKET mySocket;                                                    // Пустая переменная
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
            std::cout << "Accept failed\n";                             //
        }

        char buffer[256];                                               // Буфер для приема данных 
        int bufferSize = sizeof(buffer);                                // Сколь-ко байт можно прочитать


        while (true) {                                                  
            int bytesRead = recv(clientSocket, buffer, bufferSize, 0);  // recv(сокет с которого читаем, буфер получения, его размер, 0)
            if (bytesRead == 0) {                                       // Если > 0 - данные пришли, если = 0 - клиент закрыл соединение 
                break;
            }
            else if (bytesRead == SOCKET_ERROR) {                       // Проверка на ошибку
                break;
            }
            int bytesSend = send(clientSocket, buffer, bytesRead, 0);   // send(сокет, буфер отправки, кол-во прочитанных байт, 0)
        }
        closesocket(clientSocket);                                      // Закрытие клиент-сокета
    }
     
    closesocket(s);                                                     // Закрытие сокета
    WSACleanup();                                                       // Завершение работы сетевой подсистемы
}

