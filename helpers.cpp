#include "helpers.h"

using namespace std;

void error(const char* err) {
	fprintf(stderr, "Error: %s\n", err);
	exit(-1);
}

// Testeaza daca un request de la client este valid (HEAD, GET sau POST)
// si daca se pot obtine din el host-ul si portul la care trebuie trimis
// request-ul
int validRequest(char *msg) {
	char delim[4] = " \r\n";

	char buff[BUFFLEN];
	strcpy(buff, msg);
	char *token = strtok(buff, delim);

	if ((strcmp(token, "GET") != 0) && (strcmp(token, "HEAD") != 0)
											& (strcmp(token, "POST") != 0)) {
		return 501;
	}

	Server server;
	return getHost(msg, &server);
}

// Testeaza daca un sir de caractere are cel putin o litera
bool hasLetter(char *name) {
	for (unsigned int i = 0; i < strlen(name); i++) {
		if (!isdigit(name[i]) && name[i] != '.') {
			return true;
		}
	}

	return false;
}

// Parseaza request-ul si returneaza intr-o structura de tip Server
// host-ul si portul si calea completa catre resursa pentru cache
int getHost(char* msg, Server* server) {
	char delim[4] = " \r\n";
	char buff[BUFFLEN];
	char hostName[10000];
	char path[10000];
	strcpy(buff, msg);

	hostName[0] = 0;
	path[0] = 0;
	server->fullPath[0] = 0;

	char *saveptr1 = NULL;
	char *token = strtok_r(buff, delim, &saveptr1);
	server->port = HTTP_PORT;
	
	token = strtok_r(NULL, delim, &saveptr1);
	char url[10000];
	strcpy(url, token);

	char *saveptr2 = NULL;
	char *p = strtok_r(token, ": ", &saveptr2);

	// daca am de a face cu o cale abosulta
	if (strcmp(p, "http") == 0) {
		p = strtok_r(NULL, "/", &saveptr2);

		char *saveptr3 = NULL;
		char *t = strtok_r(p, ":", &saveptr3);
		
		if (hasLetter(t)) {
			struct hostent *host;
			host = gethostbyname(t);
			if (host == NULL) {
				return 400;
			}
			struct in_addr **addr_list = (struct in_addr **)host->h_addr_list;

			memcpy(&server->ipv4addr, addr_list[0], sizeof(struct in_addr));
		} else {
			if (inet_aton(t, &server->ipv4addr) <= 0) {
		        return 400;
		    }
		}
		t = strtok_r(NULL, ": \n", &saveptr3);
		if (t != NULL) {
			server->port = atoi(t);
		}

		strcpy(url, url + 7);

		int i = 0;
		while (url[i] != '/') {
			i++;
		}

		strncpy(server->fullPath, url, i);
		server->fullPath[i] = '\0';
		strcat(server->fullPath, ":");
		strcat(server->fullPath, to_string(server->port).c_str());
		strcat(server->fullPath, url + i);

	} else { // am o cale relativa dupa get si trebuie sa iau hostul din header
		strcpy(path, p);
		token = strtok_r(NULL, delim, &saveptr1);
		while (strcmp(token, "Host:") != 0) {
			token = strtok_r(NULL, delim, &saveptr1);
		}
		token = strtok_r(NULL, delim, &saveptr1);
		struct hostent *host;
		strcpy(hostName, token);
		host = gethostbyname(token);
		if (host == NULL) {
			return 400;
		}
		struct in_addr **addr_list = (struct in_addr **)host->h_addr_list;

		memcpy(&server->ipv4addr, addr_list[0], sizeof(struct in_addr));
		sprintf(server->fullPath, "%s:%d%s", hostName, server->port, path);
	}

	
	
	return 0;
}