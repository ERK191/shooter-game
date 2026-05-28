#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <winsock2.h>

#pragma comment(lib, "ws2_32.lib")

std::vector<std::string> matchQueue;

void handleAdvancedRequest(std::string data, SOCKET clientSocket) {
    size_t firstComma = data.find(',');
    if (firstComma == std::string::npos) return;
    
    std::string tag = data.substr(0, firstComma);
    std::string body = data.substr(firstComma + 1);

    if (tag == "LOGIN") {
        size_t nextComma = body.find(',');
        std::string user = body.substr(0, nextComma);
        std::string pass = body.substr(nextComma + 1);
        
        std::ifstream db("database.txt");
        std::string u, p; bool found = false;
        while (db >> u >> p) { if (u == user && p == pass) { found = true; break; } }
        db.close();
        
        std::string reply = found ? "SUCCESS" : "FAIL";
        send(clientSocket, reply.c_str(), (int)reply.size() + 1, 0);
    }
    else if (tag == "LOGUP") {
        size_t nextComma = body.find(',');
        std::string user = body.substr(0, nextComma);
        std::string pass = body.substr(nextComma + 1);

        std::ifstream dbRead("database.txt");
        std::string existingUser, existingPass;
        bool userExists = false;

        while (dbRead >> existingUser >> existingPass) {
            if (existingUser == user) { userExists = true; break; }
        }
        dbRead.close();

        if (userExists) {
            std::string reply = "FAILED_REGISTER";
            send(clientSocket, reply.c_str(), (int)reply.size() + 1, 0);
            std::cout << "[SERVER] Blocked duplicate username registration: " << user << std::endl;
        } 
        else {
            std::ofstream dbAppend("database.txt", std::ios::app);
            dbAppend << user << " " << pass << "\n";
            dbAppend.close();

            std::string reply = "SUCCESS_REGISTER";
            send(clientSocket, reply.c_str(), (int)reply.size() + 1, 0);
            std::cout << "[SERVER] Generated new unique user profile: " << user << std::endl;
        }
    }
    else if (tag == "ADDFRIEND") {
        size_t nextComma = body.find(',');
        std::string originalUser = body.substr(0, nextComma);
        std::string friendTarget = body.substr(nextComma + 1);

        std::ofstream fdb("friends.txt", std::ios::app);
        fdb << originalUser << " " << friendTarget << "\n";
        fdb.close();

        std::string reply = "FRIEND_ADDED";
        send(clientSocket, reply.c_str(), (int)reply.size() + 1, 0);
    }
    else if (tag == "GETFRIENDS") {
        std::ifstream fdb("friends.txt");
        std::string u, f, combinedList = "";
        while (fdb >> u >> f) { if (u == body) combinedList += f + "\n"; }
        fdb.close();
        if (combinedList.empty()) combinedList = "No friends added yet.";
        send(clientSocket, combinedList.c_str(), (int)combinedList.size() + 1, 0);
    }
    else if (tag == "FINDMATCH") {
        matchQueue.push_back(body);
        std::string reply = "LOBBY_WAITING";
        if (matchQueue.size() >= 2) {
            reply = "MATCH_FOUND: " + matchQueue[0] + " VS " + matchQueue[1];
            matchQueue.clear();
        }
        send(clientSocket, reply.c_str(), (int)reply.size() + 1, 0);
    }
}

int main() {
    WSADATA wsaData; WSAStartup(MAKEWORD(2, 2), &wsaData);
    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in service; service.sin_family = AF_INET;
    service.sin_addr.s_addr = inet_addr("127.0.0.1"); service.sin_port = htons(50001);

    bind(serverSocket, (SOCKADDR*)&service, sizeof(service));
    listen(serverSocket, 5);

    std::cout << "====================================================\n";
    std::cout << "  3D ONLINE SHOOTER MATCH ENGINE RUNNING (Port 50001) \n";
    std::cout << "====================================================\n";

    while (true) {
        SOCKET acceptSocket = accept(serverSocket, NULL, NULL);
        if (acceptSocket != INVALID_SOCKET) {
            char buffer[1024] = {0};
            recv(acceptSocket, buffer, 1024, 0);
            handleAdvancedRequest(std::string(buffer), acceptSocket);
            closesocket(acceptSocket);
        }
    }
    closesocket(serverSocket); WSACleanup();
    return 0;
}
