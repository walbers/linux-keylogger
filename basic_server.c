/**
This better work.

 */
#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>




int main(int argc, char **argv) {
	int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (sock_fd == -1) {
		fprintf(stderr, "Socket error!");
	}


	// for reusing port
	int opt = 1;
	setsockopt(sock_fd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));

	struct addrinfo hints, *result;
	// setting up the hints
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	// getting result
	int s = getaddrinfo(NULL, "1234", &hints, &result);
	if (s != 0) {
		printf("return val: %d\n", s);
		fprintf(stderr, "yikes!\n");
		exit(1);
	}

	if (bind(sock_fd, result->ai_addr, result->ai_addrlen) != 0) {
		perror("bind()");
		exit(1);
	}

	if (listen(sock_fd, 1) != 0) {
		perror("listen()");
		exit(1);
	}

	freeaddrinfo(result);

	int client_fd = accept(sock_fd, NULL, NULL);
	printf("Connection made: fd=%d\n", client_fd);

	printf("Reading:\n");
	char* buffer = calloc(1024, 1);
	while (1) {
		int bytes_read = read(client_fd, buffer, 1);
		if (bytes_read == -1) {
			printf("error with read: %s\n", strerror(errno));
		}
		if (bytes_read == 0) {
			break;
		}
		printf("%s", buffer);
		memset(buffer, 0, 1024);
	}

	free(buffer);
	printf("read 0 bytes\n")
}
