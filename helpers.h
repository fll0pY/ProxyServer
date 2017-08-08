#pragma once

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unordered_map>
#include <string>
#include <cctype>
#include <netdb.h>
#include <arpa/inet.h>

using namespace std;

#define BUFFLEN 1000000
#define HTTP_PORT 80

const unordered_map<int, string> cErrors = {
	{500, "HTTP/1.0 Internal Server Error\r\n\r\n"},
	{501, "HTTP/1.0 501 Not Implemented\r\n\r\n"},
	{502, "HTTP/1.0 502 Bad Gateway\r\n\r\n"},
	{400, "HTTP/1.0 400 Bad Request\r\n\r\n"}
};

struct Server {
	struct in_addr ipv4addr;
	char fullPath[10000];
	int port;
};

void error(const char* err);
int getHost(char* msg, Server* server);
int validRequest(char *msg);
