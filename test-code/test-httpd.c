#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <assert.h>
#include <netinet/in.h>
#include <string.h>

int main()
{
	int fd = socket(PF_INET, SOCK_STREAM, 0);
	assert(fd != 0);

	int x = 1;
	setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &x, sizeof(x));

	struct sockaddr_in addr;
	memset(&addr, '\0', sizeof(addr));

	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(23456);
	addr.sin_family = AF_INET;

	int err = bind(fd, (struct sockaddr *)&addr, sizeof(addr));
	assert(err == 0);

	err = listen(fd, 10);
	assert(err == 0);

	int newfd = accept(fd, NULL, NULL);

	assert(newfd > 0);

	char buffer[4096];
	memset(buffer, '\0', 4096);
	int len = read(newfd, buffer, 4096);
	printf("%s||||\n", buffer);

	char * send_buffer[3];
	send_buffer[0] = "HTTP/1.0 200 OK\r\nContent-type: plain/text\r\nContent-Length: 16\r\nConnection: close\r\nDate: Mon, 26 Jul 2010 10:55:25 GMT\r\nServer: lighttpd/1.4.23\r\n\r\n";
	send_buffer[1] = "Plain text cxx!\n";
	send_buffer[2] = "";

	write(newfd, send_buffer[0], strlen(send_buffer[0]));
	write(newfd, send_buffer[1], strlen(send_buffer[1]));

	sleep(60);

	return 0;
}

