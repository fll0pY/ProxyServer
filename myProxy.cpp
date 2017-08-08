#include "myProxy.h"

int main(int argc, char *argv[]) {
	if (argc < 2) {
		char err[100];
		sprintf(err, "Usage: %s port", argv[0]);
        error(err);
    }

    setup(atoi(argv[1]));
    start();
	
	pthread_exit(NULL);
	return 0;
}

// Deschide socket-ul tcp pentru a accepta conexiuni pe portul port
void setup(int port) {
	struct sockaddr_in serverAddr;

	serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port);

	tcpSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (tcpSocket < 0) {
		error("tcp socket open");
	}

    int opt = 1;
	setsockopt(tcpSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

	if (bind(tcpSocket, (struct sockaddr *) &serverAddr,
    						sizeof(struct sockaddr)) < 0) {
    	error("tcp bind");
    }

    listen(tcpSocket, MAX_CLIENTS);
}

// Functie din Lab11, citeste cate o linie de pe un socket
int readLine(int sockd, char *vptr, int maxlen) {
    int n, rc;
    char c, *buffer;

    buffer = vptr;

    for (n = 1; n < maxlen; n++) {
        if ((rc = read(sockd, & c, 1)) == 1) { *buffer++ = c;
            if (c == '\n')
                break;
        } else if (rc == 0) {
            if (n == 1)
                return 0;
            else
                break;
        } else {
            if (errno == EINTR)
                continue;
            return -1;
        }
    }

    *buffer = 0;
    return n;
}

// Citeste un request complet de la client
int getRequest(int sockd, char* msg, int maxlen) {
	char buff[BUFFLEN];
	memset(msg, 0, maxlen);
	int size = 0;

	while (1) {
		int rc = readLine(sockd, buff, BUFFLEN);
		if (rc == -1) {
			return -1;
		}
		size += rc;
		strcat(msg, buff);
		if (strcmp(buff, "\r\n") == 0) {
			msg[size] = 0;
            break;
		}
	}

    if (strstr(msg, "POST") != NULL) {
        int rc = read(sockd, msg + size, BUFFLEN - size);
        size += rc;    
    }

    return size;
}

// Trimite un mesaj pe socket-ul sockfd
void sendMessage(int sockfd, char* msg, int len) {
     if (send(sockfd, msg, len, 0) < 0) {
        error("send error");
    }
}

// Returneaza in head, header-ul unnui mesaj (tot de pana la /r/n)
void getHeader(char *msg, int len, char* head) {
    int i;
    for (i = 0; i < len; i++) {
        head[i] = msg[i];
        if (i > 4) {
            if (msg[i] == '\n' && msg[i-1] == '\r' && msg[i-2] == '\n') {
                break;
            }
        }
    }
    head[i + 1] = '\0';
}

// Testeaza daca raspunsul este 200
bool isOk(char *header) {
    char *saveptr;
    char *p = strtok_r(header, " ", &saveptr);
    p = strtok_r(NULL, " ", &saveptr);
    if (atoi(p) == 200) {
        return true;
    }

    return false;
}

// Testeaza daca request-ul este de tip Get
bool isGet(char *msg, int len) {
    char *saveptr;
    char buff[BUFFLEN];
    strcpy(buff, msg);
    char *p = strtok_r(buff, " ", &saveptr);
    if (strcmp(p, "GET") == 0) {
        return true;
    }

    return false;
}

// Corecteaza request-ul daca prima linie este altceva
void checkReq(char *msg, int len) {
	if (msg[0] != '[') {
		return;
	}
	int i = 0;
	while (msg[i] != '\n') {
		i++;
	}

	memcpy(msg, msg + i + 1, len - i - 1);
}

// Functie apelata pe un thread nou pentru fiecare client nou
void *newClient(void* data) {
	int clientSocket = (long)data;
	int sockfd;
	char msg[BUFFLEN];
    char recvbuf[BUFFLEN];

    // obtin request-ul de la client
    int reqSize = getRequest(clientSocket, msg, BUFFLEN);
    checkReq(recvbuf, reqSize);

    // verific daca request-ul este valid
    int r;
    if ((r = validRequest(msg)) != 0) {
        char response[BUFFLEN];
        strcpy(response, cErrors.at(r).c_str());
        sendMessage(clientSocket, response, strlen(response));
    } else {
        Server server;

        // obtin host-ul si port-ul din request
        getHost(msg, &server);

        struct sockaddr_in servaddr;
        memset(&servaddr, 0, sizeof(servaddr));
        servaddr.sin_family = AF_INET;
        servaddr.sin_port = htons(server.port);
        memcpy(&servaddr.sin_addr, &server.ipv4addr, sizeof(struct in_addr));

        // deschid socket pentru comunicarea cu server-ul http
		if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
	        error("tcp socket open");
	    }

	    char fileName[100];
        int file;
        bool cached = false;

        // daca cererea este de tip get, verific daca exista in cache
        if (isGet(msg, reqSize)) {
            if (cache.find(server.fullPath) != cache.end()) {
                strcpy(fileName, cache[server.fullPath].c_str());
                file = open(fileName, O_RDONLY);

                int bytes;
                while ((bytes = read(file, &recvbuf, BUFFLEN)) > 0) {
                    sendMessage(clientSocket, recvbuf, bytes);
                }
                cached = true;
                close(file);
            }
        } 
        if (!cached) {

        	// ma conectez la server-ul http
	        if (connect(sockfd, (struct sockaddr * ) &servaddr, sizeof(servaddr)) < 0) {
	            error("connect to http server failed");
	        }

	        // trimit request-ul
            sendMessage(sockfd, msg, reqSize);

            // primesc raspunsul si il memorez in cache daca este cazul
            int nbytes = 0;
            bool canCache = false;
            sprintf(fileName, "cache-%d.html", (int)cache.size());
            do {
                memset(recvbuf, 0, BUFFLEN);
                int bytes = read(sockfd, recvbuf, BUFFLEN);
                if (bytes < 0) {
                    error("reading response from socket");
                }
                if (bytes == 0 && nbytes != 0)
                    break;
                if (bytes == 0 && nbytes == 0) {
                	continue;
                }

                if (nbytes == 0) {
                    char header[BUFFLEN];
                    getHeader(recvbuf, bytes, header);
                    if (strstr(header, "private") == NULL 
                            && strstr(header, "no-cache") == NULL && isOk(header)) {
                        canCache = true;
                        cache[server.fullPath] = fileName;
                        file = open(fileName, O_WRONLY|O_CREAT|O_TRUNC, 0644);
                    }   
                }

                if (canCache) {
                    write(file, recvbuf, bytes);
                }

                nbytes += bytes;

                // Trimit raspunsul la client
                sendMessage(clientSocket, recvbuf, bytes);
            } while (1);

            if (canCache) {
                close(file);
            }
        }
    }

    close(sockfd);
    close(clientSocket);
	pthread_exit(NULL);
}

void start() {
	
	while (1) {
		struct sockaddr_in clientAddr;
		socklen_t len = sizeof(clientAddr);
		int newSockFd;
		newSockFd = accept(tcpSocket, (struct sockaddr *)&clientAddr, &len);
		pthread_t thread;
		pthread_create(&thread, NULL, newClient, (void*)(intptr_t)newSockFd);
	}	
}