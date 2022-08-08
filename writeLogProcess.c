/************ writeLogProcess: ghi quá trình giao tiếp của server-client ra hệ thống *****************/

#include <dirent.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#include <time.h>
#define SHMSZ 1024

// lấy id của thiết bị trong log riêng
int getLastId(char *filename)
{
        FILE *fp;
        char str[81];
        char *token;
        fp = fopen(filename, "r");
        while (fgets(str, sizeof(str), fp) != NULL)
                ;
        token = strtok(str, "\t");
        fclose(fp);
        return atoi(token);
}

// ghi quá trình giao tiếp server-client ra log
void writeLog(char *filename, char *param1, char *param2, char *param3)
{
        time_t rawtime;
        struct tm *timeinfo;
        time(&rawtime);
        timeinfo = localtime(&rawtime);
        char *dateTime = asctime(timeinfo);
        dateTime[strlen(dateTime) - 1] = 0;
        //append
        FILE *fp;
        fp = fopen(filename, "a");
        if (strcmp(filename, "log.txt") == 0)
        {
                fprintf(fp, "%d\t%s\t%s\t%s\t%s\n", 1 + getLastId(filename), dateTime, param1, param2, param3);
        }
        else
        {
                fprintf(fp, "%d\t%s\t%s\t%s\n", 1 + getLastId(filename), dateTime, param2, param3);
        }

        fclose(fp);
}
int main()
{
        char c;
        int shmid;
        key_t key;
        char *shm, *s;
        key = 1111;

        // tạo bộ nhớ dùng chung
        if ((shmid = shmget(key, SHMSZ, IPC_CREAT | 0666)) < 0)
        {
                perror("shmget");
                exit(1);
        }

        // lấy địa chỉ bộ nhớ dùng chung
        if ((shm = shmat(shmid, NULL, 0)) == (char *)-1)
        {
                perror("shmat");
                exit(1);
        }

        s = shm;
        char string[100] = "FALSE";
        strcpy(s, string);
        char *a[3];
        char temp[100];
        char buffer[100];

        // ghi thông tin ra log của từng thiết bị và ghi vào log chung của toàn hệ thống
        while (1)
        {
                if (strcmp(shm, "FALSE") != 0)
                {
                        strcpy(temp, shm);
                        a[0] = strtok(temp, "|");
                        a[1] = strtok(NULL, "|");
                        a[2] = strtok(NULL, "|");
                        sprintf(buffer, "log/log_%s.txt", a[0]);
                        writeLog(buffer, a[0], a[1], a[2]);
                        writeLog("log/all_log.txt", a[0], a[1], a[2]);
                        strcpy(shm, "FALSE");
                }
                usleep(60);
        }

        exit(0);
}
