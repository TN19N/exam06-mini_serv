# include <unistd.h>
# include <string.h>
# include <stdlib.h>
# include <sys/socket.h>
# include <netinet/in.h>
# include <strings.h>
# include <sys/select.h>
# include <stdio.h>

# define MAX_CLIENTS 128
# define MAX_BUFFER  200000

void error(char *msg) {
    write(2, msg, strlen(msg));
    exit(1);
}

void ft_send(char* buffer, int socket) {
    while (*buffer) {
        buffer += send(socket, buffer, strlen(buffer), 0);
    }
}

int extract_message(char **buf, char **msg) {
    int	i;

    *msg = 0;
    i = 0;
    while ((*buf)[i]) {
        if ((*buf)[i] == '\n') {
            *msg = *buf;
            (*buf)[i] = 0;
            *buf += (i + 1);
            return (1);
        }
        i++;
    }

    if (i != 0) {
        *msg = *buf;
        (*buf)[i] = 0;
        *buf += i;
        return (1);
    }
    return (0);
}

int main(int argc, char **argv) {
    if (2 != argc) {
        error("Wrong number of arguments\n");
    }

    int serverSocket;
    if ((serverSocket  = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        error("Fatal error\n");
    }

    struct sockaddr_in serverAddr;
    bzero(&serverAddr, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET; 
    serverAddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    serverAddr.sin_port = htons(atoi(argv[1]));

    if (bind(serverSocket, (const struct sockaddr*)&serverAddr, sizeof(serverAddr)) == -1) {
        close(serverSocket);
        error("Fatal error\n");
    }

    if (listen(serverSocket, 10) == -1) {
        close(serverSocket);
        error("Fatal error\n");
    }

    fd_set activeSockets, readySockets;
    FD_ZERO(&activeSockets);
    FD_SET(serverSocket, &activeSockets);
    int maxSocket = serverSocket;

    char buffer[MAX_BUFFER];
    char buffer2[MAX_BUFFER + 100];
    int  clients[MAX_CLIENTS];
    int  clients_count = 0;

    while(1) {
        readySockets = activeSockets;
        if (select(maxSocket + 1, &readySockets, NULL, NULL, NULL) == -1) {
            error("Fatal error\n");
        }

        for (int socketId = 0; socketId <= maxSocket; ++socketId) {
            if (FD_ISSET(socketId, &readySockets)) {
                if (socketId == serverSocket) {
                    int clientSocket = accept(serverSocket, NULL, NULL);
                    FD_SET(clientSocket, &activeSockets);
                    maxSocket = (clientSocket > maxSocket) ? clientSocket : maxSocket;
                    sprintf(buffer, "server: client %d just arrived\n", clients_count);
                    for (int id = 0; id < clients_count; ++id) {
                        if (-1 != clients[id]) {
                            ft_send(buffer, clients[id]);
                        }
                    }
                    clients[clients_count++] = clientSocket;
                } else {
                    int byteReads = recv(socketId, buffer, sizeof(buffer) - 1, 0);

                    int clientId = -1;
                    for (int id = 0; id < clients_count; ++id) {
                        if (clients[id] == socketId) {
                            clientId = id;
                            break;
                        }
                    }

                    if (byteReads <= 0) {
                        FD_CLR(socketId, &activeSockets);
                        close(socketId);
                        clients[clientId] = -1;
                        sprintf(buffer, "server: client %d just left\n", clientId);
                        for (int id = 0; id < clients_count; ++id) {
                            if (-1 != clients[id]) {
                                ft_send(buffer, clients[id]);
                            }
                        }
                    } else {
                        buffer[byteReads] = '\0';
                        char *buf = buffer;
                        char *msg = NULL;

                        while (extract_message(&buf, &msg)) {
                            sprintf(buffer2, "client %d: %s\n", clientId, msg);
                            for (int id = 0; id < clients_count; ++id) {
                                if (clients[id] != -1 && id != clientId) {
                                    ft_send(buffer2, clients[id]);
                                }
                            }
                        }
                    }
                }
            } 
        }
    }
}