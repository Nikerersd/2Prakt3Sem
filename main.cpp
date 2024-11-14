#include <iostream>
#include <string>
#include <thread>
#include <mutex>
#include <netinet/in.h>
#include <unistd.h>
#include <sstream>
#include "insert.h"
#include "delete.h"
#include "parser.h"
#include "select.h"

using namespace std;

mutex dbMutex; // Мьютекс для синхронизации доступа к таблице
mutex clientMutex; // Мьютекс для синхронизации доступа к счетчику клиентов
int activeClients = 0; // Счетчик активных клиентов

void handleClient(int clientSocket, JsonTable& jstab) {

    char buffer[1024];
    while (true) {
        int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (bytesReceived <= 0) break; // Прерываем цикл, если клиент отключился или ошибка

        string command(buffer, bytesReceived);
        istringstream iss(command);
        string firstMessage;
        iss >> firstMessage;

        // Блокировка на время выполнения команды
        dbMutex.lock();
        if (firstMessage == "INSERT") {
            insert(command, jstab);
        } else if (firstMessage == "DELETE") {
            deleteRows(command, jstab);
        } else if (firstMessage == "SELECT") {
            select(command, jstab);
        } else if (firstMessage == "EXIT") {
            dbMutex.unlock();
            break;  // Если команда EXIT, выходим из цикла
        }
        dbMutex.unlock();

        send(clientSocket, "Комманда отправлена\n", 38, 0); // Отправляем подтверждение выполнения
    }

    // Закрытие сокета клиента
    close(clientSocket);

    // Уменьшаем количество активных клиентов после завершения работы с клиентом
    {
        lock_guard<mutex> lock(clientMutex);
        activeClients--; // Уменьшаем количество активных клиентов
    }

    // Если больше нет активных клиентов, завершаем сервер
    if (activeClients == 0) {
        cout << "Все клиенты отключены. Завершение работы сервера.\n";
        exit(0);  // Завершаем сервер
    }
}


int main() {
    JsonTable jstab;
    parser(jstab); // Инициализация таблицы при запуске

    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(7432);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
    listen(serverSocket, 3);

    cout << "Сервер запущен и слушает порт 7432...\n";

    while (true) {
        int clientSocket = accept(serverSocket, nullptr, nullptr);
        
        // Увеличиваем количество активных клиентов в главном потоке
        {
            lock_guard<mutex> lock(clientMutex);
            activeClients++; // Увеличиваем количество активных клиентов
        }

        // Запуск потока для обработки запроса клиента
        thread clientThread(handleClient, clientSocket, ref(jstab));
        clientThread.detach(); // Поток для обработки запроса клиента

    }

    close(serverSocket); // Закрытие серверного сокета
    return 0;
}