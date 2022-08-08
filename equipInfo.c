/***** đọc thông tin về các thiết bị hiện có và lưu vào bộ nhớ dùng chung *******/

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
#define SHMSZ 1024

// đọc thông tin các thiết bị hiện có
char *readFileIntoString()
{
  char *buffer = 0;
  long length;
  FILE *f = fopen("info/equipInfo.txt", "rb");

  if (f)
  {
    fseek(f, 0, SEEK_END);
    length = ftell(f);
    fseek(f, 0, SEEK_SET);
    buffer = malloc(length);
    if (buffer)
    {
      fread(buffer, 1, length, f);
    }
    fclose(f);
  }

  if (buffer)
  {
    return buffer;
  }
  return 0;
}

int main()
{
  int shmid;
  key_t key;
  char *shm;
  key = 5678;
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

  // ghi danh sách các thiết bị có sẵn vào bộ nhớ dùng chung
  strcpy(shm, readFileIntoString());
  exit(0);
}
