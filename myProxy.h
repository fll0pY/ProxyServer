#pragma once

#include <iostream>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <pthread.h>
#include <stdint.h>
#include <cstring>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>


#include "helpers.h"

#define MAX_CLIENTS 10000

using namespace std;

int tcpSocket;

unordered_map<string, string> cache;

void setup(int port);
void start();
bool isOk(char *header);
void sendMessage(int sockfd, char* msg, int len);
int readLine(int sockd, char *vptr, int maxlen);
int getRequest(int sockfd, char* msg, int maxLen);