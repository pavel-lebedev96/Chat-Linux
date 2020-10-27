#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <wait.h>
#include <map>
#include "ipc_tools.h"
/*сигнал для нового события*/
#define EVENT SIGUSR1 
/*сигнал для старта дочернего процесса*/
#define START SIGUSR2
#define SIZE sizeof(struct sockaddr_in)
/*номер порта*/
#define PORT_NUM 7000
/*информации о клиентах*/
struct TClient
{
	char name[20];
	int sockfd;
};
int cl_shmid, shmid, cl_semid, semid, client_sockfd;
pid_t sender_pid;
/*контейнер для хранения информации о клиентах*/
std::map<pid_t, TClient> clients;
/*обработчик сигнал о старте*/
void start_sighandler(int){}
/*обработка ошибки при send*/
void sigpipe_handler(int)
{
	TData data;
	strcpy(data.msg, "left");
	close(client_sockfd);
	p(semid);
	write_shr_data(shmid, data);
	kill(getppid(), EVENT);
	exit(1);
}
/*обработчик событий для дочернего процесса*/
void event_child_sighandler(int)
{
	TData data;
	size_t lens;
	p(cl_semid);
	read_shr_data(cl_shmid, data);
	v(cl_semid);
	lens = strlen(data.msg);
	send(client_sockfd, data.msg, lens + 1, 0);
}
/*обработчик событий для родительского процесса*/
void event_sighandler(int)
{
	TData data;
	char msg[MAXLEN + 1];
	read_shr_data(shmid, data);
	if (!strcmp(data.msg, "leave"))
	{
		sprintf(msg, "%s left the conversation", clients[data.pid].name);
		close(clients[data.pid].sockfd);
		clients.erase(data.pid);
	}
	else
		if (!strcmp(data.msg, "join"))
			sprintf(msg, "%s joined to the conversation", clients[data.pid].name);
		else
		{
			sprintf(msg, "%s: ", clients[data.pid].name);
			strcat(msg, data.msg);
		}
	strcpy(data.msg, msg);
	p(cl_semid);
	write_shr_data(cl_shmid, data);
	v(cl_semid);
	std::map<pid_t, TClient>::iterator it;
	for (it = clients.begin(); it != clients.end(); it++)
	{
		if (it->first == data.pid)
			continue;
		kill(it->first, EVENT);
	}
	v(semid);
}
/*дочерний процессс*/
void child_proccess()
{
	TData data;
	data.pid = getpid();
	while (recv(client_sockfd, data.msg, MAXLEN + 1, 0) > 0)
	{
		p(semid);
		write_shr_data(shmid, data);
		kill(getppid(), EVENT);
	}
	sigpipe_handler(0);
}
int main()
{
	int server_sockfd;
	TClient client_inf;
	TData data;
	struct sockaddr_in server = { AF_INET, PORT_NUM, INADDR_ANY };
	shmid = init_shm(get_ipc_key(1));
	cl_shmid = init_shm(get_ipc_key(2));
	semid = init_sem(get_ipc_key(3));
	cl_semid = init_sem(get_ipc_key(4));
	signal(EVENT, event_sighandler);
	/* создание сокета */
	if ((server_sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
		error("Socket error");
	/* связывание сокета с адресом*/
	if (bind(server_sockfd, (struct sockaddr *)&server, SIZE) == -1)
		error("Bind error");
	/* включение приема соединений */
	if (listen(server_sockfd, 5) == -1 )
		error("Listen error");
	for (;;)
	{
		/* прием запроса на соединение */
		if ((client_sockfd = accept(server_sockfd, NULL, NULL)) == -1)
			error("Accept error");
		client_inf.sockfd = client_sockfd;
		send(client_sockfd, "Enter your name", 16, 0);
		if (recv(client_sockfd, client_inf.name, 20, 0) == 0)
		{
			close(client_sockfd);
			continue;
		}
		if ((data.pid = fork()) == 0)
		{
			signal(EVENT, event_child_sighandler);
			signal(SIGPIPE, sigpipe_handler);
			signal(START, start_sighandler);
			/*ожидание сигнала о начале*/
			sigpause(START);
			child_proccess();
		}
		clients[data.pid] = client_inf;
		strcpy(data.msg, "join");
		p(semid);
		write_shr_data(shmid, data);
		kill(data.pid, START);
		kill(getpid(), EVENT);
	}
}