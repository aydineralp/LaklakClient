#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>

#define PORT 8888
#define MAX_MSG_LEN 1024

#pragma comment(lib, "ws2_32.lib")

void printMenu() {
    // Kullanıcı menüsünü ekrana yazdırır.
    printf("\n===== MENU =====\n");
    printf("1. Login/Register\n");
    printf("2. Send Message\n");
    printf("3. Check Messages\n");
    printf("4. List Users\n");
    printf("5. Retrieve User Data\n");
    printf("0. Exit\n");
    printf("================\n");
}

int main() {
    // Client tarafı işlemleri
    WSADATA wsa;
    SOCKET clientSocket;
    struct sockaddr_in serverAddr;
    char buffer[MAX_MSG_LEN] = {0};

    printf("\nInitialising Winsock...\n");
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("Failed. Error Code : %d\n", WSAGetLastError());
        return 1;
    }

    printf("Initialised.\n");

    if ((clientSocket = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
        printf("Could not create socket : %d\n", WSAGetLastError());
        return 1;
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(clientSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
        printf("Connect error: %d\n", WSAGetLastError());
        return 1;
    }

    while (1) {
        printMenu();
        printf("Enter your choice: ");
        int choice;
        scanf("%d", &choice);
        send(clientSocket, (char*)&choice, sizeof(choice), 0);

        switch (choice) {
            case 1: {
                // Kullanıcı giriş/kayıt işlemleri
                char username[50], name[50], phoneNumber[20], surname[50];
                printf("Enter username: ");
                scanf("%s", username);
                printf("Enter name: ");
                scanf("%s", name);
                printf("Enter phone number: ");
                scanf("%s", phoneNumber);
                printf("Enter surname: ");
                scanf("%s", surname);

                send(clientSocket, username, sizeof(username), 0);
                send(clientSocket, name, sizeof(name), 0);
                send(clientSocket, phoneNumber, sizeof(phoneNumber), 0);
                send(clientSocket, surname, sizeof(surname), 0);

                char loginMsg[MAX_MSG_LEN] = {0};
                recv(clientSocket, loginMsg, sizeof(loginMsg), 0);
                printf("%s\n", loginMsg);
                break;
            }
            case 2: {
                // Mesaj gönderme işlemi
                char sender[50], receiver[50], message[MAX_MSG_LEN];
                printf("Enter sender username: ");
                scanf("%s", sender);
                printf("Enter receiver username: ");
                scanf("%s", receiver);
                printf("Enter message: ");
                scanf(" %[^\n]s", message);

                send(clientSocket, sender, sizeof(sender), 0);
                send(clientSocket, receiver, sizeof(receiver), 0);
                send(clientSocket, message, sizeof(message), 0);

                char sendMsg[MAX_MSG_LEN] = {0};
                recv(clientSocket, sendMsg, sizeof(sendMsg), 0);
                printf("%s\n", sendMsg);
                break;
            }
            case 3: {
                // Mesajları kontrol etme işlemi
                char user[50];
                printf("Enter your username: ");
                scanf("%s", user);

                send(clientSocket, user, sizeof(user), 0);

                char msgBuffer[MAX_MSG_LEN * 10] = {0};
                recv(clientSocket, msgBuffer, sizeof(msgBuffer), 0);
                printf("%s\n", msgBuffer);
                break;
            }
            case 4: {
                // Kullanıcıları listeleme işlemi
                char userList[MAX_MSG_LEN * 10] = {0};

                recv(clientSocket, userList, sizeof(userList), 0);
                printf("%s\n", userList);
                break;
            }
            case 5: {
                // Kullanıcı verilerini alma işlemi
                char user[50];
                printf("Enter username to retrieve data: ");
                scanf("%s", user);

                send(clientSocket, user, sizeof(user), 0);

                char userData[MAX_MSG_LEN * 10] = {0};
                recv(clientSocket, userData, sizeof(userData), 0);
                printf("%s\n", userData);
                break;
            }
            case 0: {
                // Bağlantıyı sonlandırma
                printf("Exiting...\n");
                closesocket(clientSocket);
                WSACleanup();
                exit(0);
            }
            default: {
                printf("Invalid choice. Please try again.\n");
                break;
            }
        }
    }
    return 0;
}
