#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <winsock2.h>
#include <stdbool.h>

#define PORT 8888
#define MAX_CLIENTS 5
#define MAX_MSG_LEN 1024
#define MAX_USERS 10

#pragma comment(lib, "ws2_32.lib")

struct User {
    char username[50];
    char name[50];
    char phoneNumber[20];
    char surname[32];
};

struct Message {
    char sender[50];
    char receiver[50];
    char content[MAX_MSG_LEN];
};

struct ServerData {
    struct User userList[MAX_USERS];
    struct Message messages[MAX_USERS][MAX_USERS];
};

struct ServerData globalServerData;
SOCKET clientSockets[MAX_CLIENTS];

void checkMessages(SOCKET clientSocket, char* user, struct ServerData* serverData) {
    // Kullanıcının mesajlarını kontrol edip, varsa client'a gönderir.
    char messages[MAX_MSG_LEN * MAX_USERS] = {0};
    int messageFound = 0;

    int i, j;
    for (i = 0; i < MAX_USERS; ++i) {
        if (strcmp(serverData->userList[i].username, user) == 0) {
            for (j = 0; j < MAX_USERS; ++j) {
                if (strcmp(serverData->messages[i][j].receiver, user) == 0) {
                    strcat(messages, "Sender: ");
                    strcat(messages, serverData->messages[i][j].sender);
                    strcat(messages, "\nContent: ");
                    strcat(messages, serverData->messages[i][j].content);
                    strcat(messages, "\n\n");
                    serverData->messages[i][j] = (struct Message){0};
                    messageFound = 1;
                }
            }
        }
    }

    if (messageFound) {
        send(clientSocket, messages, strlen(messages), 0);
    } else {
        char errMsg[] = "No messages found!";
        send(clientSocket, errMsg, strlen(errMsg), 0);
    }
}

void sendMessage(SOCKET clientSocket, char* sender, char* receiver, char* message, struct ServerData* serverData) {
    // Mesaj gönderme işlemini gerçekleştirir.
    int senderIndex = -1, receiverIndex = -1;

    int i;
    for (i = 0; i < MAX_USERS; ++i) {
        if (strcmp(serverData->userList[i].username, sender) == 0) {
            senderIndex = i;
        }
        if (strcmp(serverData->userList[i].username, receiver) == 0) {
            receiverIndex = i;
        }
    }

    if (senderIndex != -1 && receiverIndex != -1) {
        struct Message newMsg;
        strcpy(newMsg.sender, sender);
        strcpy(newMsg.receiver, receiver);
        strcpy(newMsg.content, message);

        int messageSaved = 0;
        for (i = 0; i < MAX_USERS; ++i) {
            if (strlen(serverData->messages[receiverIndex][i].content) == 0) {
                serverData->messages[receiverIndex][i] = newMsg;
                messageSaved = 1;
                break;
            }
        }
        if (messageSaved) {
            char ackMsg[] = "Message sent successfully!";
            send(clientSocket, ackMsg, strlen(ackMsg), 0);

            char notifyMsg[MAX_MSG_LEN];
            sprintf(notifyMsg, "New message from %s", sender);
            send(clientSockets[receiverIndex], notifyMsg, strlen(notifyMsg), 0);

            SOCKET receiverSocket = clientSockets[receiverIndex];
            if (receiverSocket != 0 && receiverSocket != clientSocket) {
                char msgToOtherTerminal[MAX_MSG_LEN + 50];
                sprintf(msgToOtherTerminal, "Message from %s: %s", sender, message);
                send(receiverSocket, msgToOtherTerminal, strlen(msgToOtherTerminal), 0);
            }
        } else {
            char errMsg[] = "Receiver's message box full!";
            send(clientSocket, errMsg, strlen(errMsg), 0);
        }
    } else {
        char errMsg[] = "Sender or receiver not found!";
        send(clientSocket, errMsg, strlen(errMsg), 0);
    }
}

void *handleClient(void *data) {
    // Bağlanan her client için bir thread oluşturur ve bu thread ile client'ı handle eder.
    SOCKET clientSocket = *((SOCKET *)data);
    int index = -1;
    int i;
    for (i = 0; i < MAX_CLIENTS; ++i) {
        if (clientSockets[i] == 0) {
            clientSockets[i] = clientSocket;
            index = i;
            break;
        }
    }
    struct ServerData *serverData = &globalServerData;
    int valread;
    int *clientAction = (int *)malloc(sizeof(int));
    *clientAction = 0;  
    char username[50] = {0};
    bool loginFlag = false;

    while ((valread = recv(clientSocket, (char*)clientAction, sizeof(int), 0)) > 0) {
        if (*clientAction == 4) {
            if(loginFlag){
                char msgBuffer[MAX_USERS * 50] = {0}; 
                for (i = 0; i < MAX_USERS; ++i) {
                    if (strcmp(serverData->userList[i].username, username) != 0) {
                        strcat(msgBuffer, serverData->userList[i].username);
                        strcat(msgBuffer, "\n");
                    }
                }
                send(clientSocket, msgBuffer, strlen(msgBuffer), 0);
            } else {
                char loginMsg[] = "Please login first !";
                send(clientSocket, loginMsg, strlen(loginMsg), 0);
            }
        } else if (*clientAction == 1) {
            // Kullanıcı giriş/üye kaydı işlemleri
            if (!loginFlag) {
                char name[50], phoneNumber[20], surname[50];
                recv(clientSocket, username, sizeof(username), 0);
                printf("username received is: %s\n",username);
                recv(clientSocket, name, sizeof(name), 0);
                printf("Name received is: %s\n",name);
                recv(clientSocket, phoneNumber, sizeof(phoneNumber), 0);
                printf("phone received is: %s\n",phoneNumber);
                recv(clientSocket, surname, sizeof(surname), 0);
                printf("surname received is: %s\n",surname);
                int userExists = 0;
                for (i = 0; i < MAX_USERS; ++i) {
                    if (strcmp(serverData->userList[i].username, username) == 0) {
                        userExists = 1;
                        break;
                    }
                }
                if (userExists==0) {
                    FILE *userFile;
                    char filename[50];
                    strcpy(filename, username);
                    strcat(filename, ".txt");

                    userFile = fopen(filename, "w");
                    fprintf(userFile, "Unique ID: %s\nName: %s\nPhone Number: %s\nSurname: %s\n",
                            username, name, phoneNumber, surname);
                    fclose(userFile);

                    for (i = 0; i < MAX_USERS; ++i) {
                        if (strlen(serverData->userList[i].username) == 0) {
                            strcpy(serverData->userList[i].username, username);
                            strcpy(serverData->userList[i].name, name);
                            strcpy(serverData->userList[i].phoneNumber, phoneNumber);
                            strcpy(serverData->userList[i].surname, surname);
                            break;
                        }
                    }
                    char successMsg[] = "Login successful!";
                    send(clientSocket, successMsg, strlen(successMsg), 0);
                    loginFlag = true;
                } else {
                    char failureMsg[] = "User already exists!";
                    send(clientSocket, failureMsg, strlen(failureMsg), 0);
                }
            } else {
                char alreadyLoggedInMsg[] = "Already logged in!";
                send(clientSocket, alreadyLoggedInMsg, strlen(alreadyLoggedInMsg), 0);
            }
        } else if (*clientAction == 2) {
            // Mesaj gönderme işlemi
            if(loginFlag){
                char sender[50], receiver[50], message[MAX_MSG_LEN];
                recv(clientSocket, sender, sizeof(sender), 0);
                printf("username received is: %s\n",sender);
                recv(clientSocket, receiver, sizeof(receiver), 0);
                printf("ReceiverID received is: %s\n",receiver);
                recv(clientSocket, message, sizeof(message), 0);
                printf("Msg received is: %s\n",message);

                sendMessage(clientSocket, sender, receiver, message, serverData);
            } else {
                char loginMsg[] = "Please login first to send msg!";
                send(clientSocket, loginMsg, strlen(loginMsg), 0);
            }
        } else if (*clientAction == 3) {
            // Mesajları kontrol etme işlemi
            if(loginFlag){
                char user[50];
                recv(clientSocket, user, sizeof(user), 0);
                printf("username received is: %s\n",user);
                checkMessages(clientSocket, user, serverData);
            } else {
                char loginMsg[] = "Please login first to get new msg!";
                send(clientSocket, loginMsg, strlen(loginMsg), 0);
            }
        } else if (*clientAction == 5) {
            // Kullanıcı verilerini alma işlemi
            if (loginFlag) {
                FILE *userFile;
                char filename[50];
                char user[50];
                recv(clientSocket, user, sizeof(user), 0);
                printf("username received is: %s\n",user);

                strcpy(filename, user);
                strcat(filename, ".txt");
                userFile = fopen(filename, "r");
                if (userFile != NULL) {
                    char buffer[MAX_MSG_LEN * MAX_USERS];
                    size_t bytesRead = fread(buffer, sizeof(char), sizeof(buffer), userFile);
                    fclose(userFile);

                    if (bytesRead > 0) {
                        send(clientSocket, buffer, bytesRead, 0);
                    } else {
                        char errMsg[] = "No data found for the user!";
                        send(clientSocket, errMsg, strlen(errMsg), 0);
                    }
                } else {
                    char errMsg[] = "Error opening user file!";
                    send(clientSocket, errMsg, strlen(errMsg), 0);
                }
            } else {
                char loginMsg[] = "Please login first!";
                send(clientSocket, loginMsg, strlen(loginMsg), 0);
            }
        } else if (*clientAction == 0) {
            // Kullanıcı bağlantısını sonlandırma
            printf("%s is terminated\n",username); 
            break;
        }
    }

    free(clientAction);
    closesocket(clientSocket);
    pthread_exit(NULL);
}

int main() {
    // Sunucu başlatma ve client bağlantılarını dinleme
    WSADATA wsa;
    SOCKET serverSocket, clientSocket;
    struct sockaddr_in serverAddr, clientAddr;
    pthread_t threadId;
    int i, addrLen;

    printf("\nInitialising Winsock...\n");
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("Failed. Error Code : %d\n", WSAGetLastError());
        return 1;
    }

    printf("Initialised.\n");

    for (i = 0; i < MAX_CLIENTS; ++i) {
        clientSockets[i] = 0; 
    }

    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == INVALID_SOCKET) {
        printf("Could not create socket : %d\n", WSAGetLastError());
        return 1;
    }

    memset(&serverAddr, '0', sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(PORT);

    if (bind(serverSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        printf("Bind failed with error code : %d\n", WSAGetLastError());
        return 1;
    }

    listen(serverSocket, MAX_CLIENTS);
    memset(&globalServerData, 0, sizeof(struct ServerData));
    printf("server is on listening port\n");

    while (1) {
        addrLen = sizeof(clientAddr);
        clientSocket = accept(serverSocket, (struct sockaddr *)&clientAddr, &addrLen);
        if (clientSocket == INVALID_SOCKET) {
            printf("Accept failed with error code : %d\n", WSAGetLastError());
            return 1;
        }

        printf("Connection accepted from %s:%d\n", inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port));

        if (pthread_create(&threadId, NULL, handleClient, (void *)&clientSocket) != 0) {
            perror("Thread creation failed");
            exit(EXIT_FAILURE);
        }

        pthread_detach(threadId);
    }

    closesocket(serverSocket);
    WSACleanup();
    return 0;
}
