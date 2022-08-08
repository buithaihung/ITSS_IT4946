/***** systemInfo: lấy thông tin về hệ thống và lưu vào bộ nhớ dùng chung **********/

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
#define SHMSZ    1024

// lấy thông tin về hệ thống
char* readFileIntoString() {
        char * buffer = 0;
        long length;
        FILE * f = fopen ("info/systemInfo.txt", "rb");

        if (f)
        {
          fseek (f, 0, SEEK_END);
          length = ftell (f);
          fseek (f, 0, SEEK_SET);
          buffer = malloc (length);
          if (buffer)
          {
            fread (buffer, 1, length, f);
          }
          fclose (f);
        }

        if (buffer)
        {
          return buffer;
        }
}
int main()
{
    char c;
    int shmid;
    key_t key;
    char *shm, *s;
    key = 9999;

    // tạo bộ nhớ dùng chung
    if ((shmid = shmget(key, SHMSZ, IPC_CREAT | 0666)) < 0) {
        perror("shmget");
        exit(1);
    }

    // lấy địa chỉ bộ nhớ dùng chung
    if ((shm = shmat(shmid, NULL, 0)) == (char *) -1) {
        perror("shmat");
        exit(1);
    }

    // ghi thông tin hệ thống vào bộ nhớ dùng chung
    s = shm;
    strcpy(s,readFileIntoString());
    exit(0);
}
