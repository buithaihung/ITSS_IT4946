#include <dirent.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <signal.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>

#define SHMSZ 8
#define KEY "1234"
#define MAXLINE 1024   /*max text line length*/
#define SERV_PORT 3000 /*port*/
#define LISTENQ 10     /*maximum number of client connections*/

typedef struct
{
        char code[100];
        char params[3][256];
} command;

int listenSock, connectSock, n;
pid_t pid;
char request[MAXLINE];
struct sockaddr_in serverAddr, clientAddr;
socklen_t clilen;
command cmd;
char *shm2;

// lấy địa chỉ bộ nhớ dùng chung của log
void getInfo(char *key_from_server)
{
        int shmid;
        key_t key;
        key = atoi(key_from_server);

        if ((shmid = shmget(key, 1000, 0666)) < 0)
        {
                perror("shmget");
                exit(1);
        }

        if ((shm2 = shmat(shmid, NULL, 0)) == (char *)-1)
        {
                perror("shmat");
                exit(1);
        }
}

void sig_chld(int singno)
{
        pid_t pid;
        int stat;
        while ((pid = waitpid(-1, &stat, WNOHANG)) > 0)
                printf("child %d terminated\n", pid);
        return;
}

command convertRequestToCommand(char *request)
{
        char copy[100];
        strcpy(copy, request);
        command cmd;
        char *token;
        int i = 0;
        token = strtok(copy, "|");
        strcpy(cmd.code, token);
        while (token != NULL)
        {
                token = strtok(NULL, "|");
                if (token != NULL)
                {
                        strcpy(cmd.params[i++], token);
                }
        }
        return cmd;
}

int main()
{

        int shmid;
        key_t key;
        int *shm;
        key = atoi(KEY);
        int currentVoltage = 0;

        // tạo bộ nhớ dùng chung
        if ((shmid = shmget(key, 8, IPC_CREAT | 0666)) < 0)
        {
                perror("shmget");
                exit(1);
        }

        // lấy địa chỉ bộ nhớ dùng chung
        if ((shm = shmat(shmid, NULL, 0)) == (int *)-1)
        {
                perror("shmat");
                exit(1);
        }

        *shm = 0;
        int *currentDevice;
        currentDevice = shm + 1;
        *currentDevice = 0;
        getInfo("1111"); // sử dụng log của hệ thống

        // connectMng
        // khởi tạo kết nối IP
        if ((listenSock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        {
                printf("Loi tao socket\n");
                exit(1);
        }
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
        serverAddr.sin_port = htons(SERV_PORT);

        int enable = 1;
        if (setsockopt(listenSock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
                perror("setsockopt(SO_REUSEADDR) failed");

        if (bind(listenSock, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
        {
                printf("Loi bind\n");
                exit(2);
        }

        listen(listenSock, LISTENQ);
        clilen = sizeof(clientAddr);

        // nhận kết nối từ client
        while (1)
        {
                connectSock = accept(listenSock, (struct sockaddr *)&clientAddr, &clilen);
                *currentDevice = *currentDevice + 1;

                printf("Current Device: %d\n", *currentDevice);


                // tạo tiến trình con
                if ((pid = fork()) == 0)
                {
                        close(listenSock);
                        // nhận request từ client
                        // powerSupply
                        while ((n = recv(connectSock, request, MAXLINE, 0)) > 0)
                        {
                                pid_t pid_client;
                                int stat;
                                pid_client = waitpid(-1, &stat, WNOHANG);
                                printf("Request from child %d is: %s\n", pid_client, request);

                                request[n] = '\0';
                                cmd = convertRequestToCommand(request);
                                if (strcmp(cmd.code, "STOP") == 0)
                                {
                                        *shm = *shm - currentVoltage;
                                        currentVoltage = 0;
                                        sprintf(shm2, "%s|%s|%s|", cmd.params[0], "STOP", "0");
                                }
                                else
                                {
                                        sprintf(shm2, "%s|%s|%s|", cmd.params[0], cmd.params[1], cmd.params[2]);
                                        currentVoltage = atoi(cmd.params[2]);
                                        *shm = *shm + currentVoltage;
                                }
                                printf("Current Power Consumption: %d\n", *shm);
                                send(connectSock, KEY, 4, 0);
                        }
                        if (currentVoltage != 0)
                        {
                                *shm = *shm - currentVoltage;
                                currentVoltage = 0;
                        }

                        close(connectSock);
                        exit(0);
                }
                signal(SIGCHLD, sig_chld);
                close(connectSock);
        }
}
