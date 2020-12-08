#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#define SIZE sizeof(struct sockaddr_in)
#define MAXLEN 50
#define PORT 7000
int sockfd;
void error(char *s)
{
	perror(s);
	getchar();
	exit(1);
}
void child_proccess()
{
	char msg[MAXLEN + 1];
	while (recv(sockfd, msg, MAXLEN, 0) > 0)
		printf("%s\n", msg);
}
main()
{
	char msg[MAXLEN + 1];
	int port;
	pid_t pid;;
	struct sockaddr_in server = { AF_INET, PORT};
	/* Преобразовывает и сохраняет IP адрес сервера */
	server.sin_addr.s_addr = inet_addr("127.0.0.1");
	/* Создает сокет */
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
		error("socket error");
	/* Соединяет сокет с сервером */
	if (connect(sockfd, (struct sockaddr *)&server, SIZE) == -1)
		error("connect error");
	/*имя пользователя*/
	if (recv(sockfd, msg, MAXLEN, 0) == 0)
		error("recv error");
	printf("%s\n", msg);
	scanf("%s", msg, MAXLEN);
	send(sockfd, msg, strlen(msg) + 1, 0);
	if ((pid = fork()) == 0)
		child_proccess();
	for (;;)
	{
		scanf("%s", msg, MAXLEN);
		if (!strcmp(msg, "leave"))
		{
			send(sockfd, msg, strlen(msg) + 1, 0);
			close(sockfd);
			exit(0);
		}
		send(sockfd, msg, strlen(msg) + 1, 0);
	}
}