#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int startup(u_short *);
int get_line(int, char *, int);
void error_die(const char *);


int main(void)
{
	int server_socket = -1, client_socket = -1;
	u_short port = 0;
	struct sockaddr_in client_name;
	int client_name_len = sizeof(client_name);

	server_socket = startup(&port);
	printf("httpd running on port %d\n", port);

	while(1) {
		client_socket = accept(server_soket, (struct sockaddr *)&client_name, &client_name_len);

		if (client_socket == -1) {
			error_die("accept");
		}

		accept_request(client_socket);
	}
	
	close(server_socket);

	return 0 ;
}

int get_line(int socket, char *buf, int size)
{
	int i = 0, n = 0;
	char c = '\0';

	while((i < size - 1) && (c != '\n')) {
		n = recv(socket, &c, 1, 0);

		if (n > 0) {
			if (c == '\r') {
				n = recv(socket, &c, 1, MSG_PEEK);
				if (n > 0 && c == '\n') {
					recv(socket, &c, 1, 0);
				} else {
					n = '\n';
				}
			}
		} else {
			c = '\n';
		}
		buf[i]= c;
		i++;
	}

	buf[i] = '\0';

	return n;
}

int startup(u_short *port)
{
	int httpd = 0;
	struct sockaddr_in address;

	httpd = socket(AF_INET, SOCK_STREAM, 0);
	if (httpd == -1) {
		error_die("socket");
	}

	memset(&address, 0, sizeof(address));
	address.sin_family = AF_INET;
	address.sin_port = htons(*port);
	address.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(httpd, (struct sockaddr*)&address, sizeof(address)) < 0) {
		error_die("bind");
	}

	if (*port == 0) {
		int addresslen = sizeof(address);

		if (getsockname(httpd, (struct sockaddr *)&address, &addresslen) == -1) {
			error_die("getsockname");
		}

		*port = ntohs(address.sin_port);
	}

	if (listen(httpd, 5) < 0) 
	{
		error_die("listen");
	}

	return httpd;
}

void error_die(const char *message) {
	perror(message);
	exit(1);
}

void accept_request(int socketfd)
{
	char buf[1024] = "";
	char method[1024] = "";
	char url[1024] = "";
	char path[512] = "";
	int numchars = 0;

	size_t i = 0, j = 0;

	struct stat st;

	int cgi = 0;
	char *query_string = NULL;

	numchars = get_line(socketfd, buf, sizeof(buf));

	while(!isspace(buf[j]) && (i < sizeof(method) - 1)){
		method[i] = buf[j];
		i++;j++;
	}
	method[i] = '\0';

	if (strcasecmp(method, "GET") && strcasecmp(method, "POST")) {
		unimplemented(socketfd);
		return;
	}

	if (strcasecmp(method, "POST")) {
		cgi = 1;
	}

	i = 0;

	while(isspace(buf[j]) && (j < sizeof(buf))){
		j++;
	}

	while(!isspace(buf[j]) && (i < sizeof(url) - 1) && (j < sizeof(buf))) {
		url[i] = buf[j];
		i++;j++;
	}
	url[i] = '\0';

	if (strcasecmp(method, "GET") == 0) {
		query_string = url;

		while((*query_string != '?') && (*query_string != '\0')) {
			query_string++;
		}

		if (*query_string == '?') {
			cgi = 1;
			*query_string = '\0';
			query_string++;
		}
	}

	sprintf(path, "htdoc%s", url);

	if (path[strlen(path) - 1] == '/') {
		strcat(path, "index.html");
	}

	if (stat(path, &st) == -1) {
		while ((numchars > 0) && strcmp("\n", buf)) {
			numchars = get_line(socketfd, buf, sizeof(buf));
		}

		not_found(socketfd);
		return;
	}

	if ((st.st_mode & S_IFMT) == S_IFDIR) {
		strcat(path, "/index.html");
	}

	if ((st.st_mode & S_IXUSR) || (st.st_mode & S_IXGRP) || (st.st_mode & S_IXOTH)) {
		cgi = 1;
	}

	if (!cgi) {
		server_file(client, path);
	} else {

	}
}
