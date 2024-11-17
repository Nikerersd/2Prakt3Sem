#include <iostream>
#include <string>
#include <netinet/in.h>
#include <unistd.h>

using namespace std;

int main() {
    int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(7432);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    connect(clientSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr));

    string command;
    while (true) {
        cout << "<< ";
        getline(cin, command);

        if (command.empty()) continue;

        send(clientSocket, command.c_str(), command.size(), 0);

        if (command == "EXIT") break;

        char buffer[1024];
        int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (bytesReceived > 0) {
            cout << "Server: " << string(buffer, bytesReceived) << endl;
        }
    }
    close(clientSocket);
    return 0;
}