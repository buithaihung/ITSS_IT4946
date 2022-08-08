#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#define MAXLINE 1024
#define PORT 3000
#define SHMSZ 4

typedef struct
{
	char name[100];
	int normalMode;
	int savingMode;
} device;

device *deviceList;
int N;
int clientSocket;
char serverResponse[MAXLINE];
int *shm;
char *shm2;
char info[1000];
char systemInfo[1000];
int threshold;
int maxThreshold;
int maxDevice;
int kbhit();
int getch();
void showMenuDevices();
void showMenu();
void getResponse();
void makeCommand(char *command, char *code, char *param1, char *param2);
void showMenuAction(int i);
void getShareMemoryPointer(char *key_from_server);
void runDevice(int i, int isSaving);
void stopDevice(char *deviceName);
void getInfo(char *key_from_server);
int countEntityNumber(char *str);
char **tokenizeString(char *str);
device parseStringToStruct(char *str);
device *parseStringToStructArray(char *str);

int main()
{
	// lấy thông tin thiết bị từ bộ nhớ dùng chung của equipInfo
	getInfo("5678");
	strcpy(info, shm2);

	N = countEntityNumber(info);
	deviceList = parseStringToStructArray(info);

	// lấy thông tin hệ thống từ bộ nhớ dùng chung của systemInfo
	getInfo("9999");
	strcpy(systemInfo, shm2);

	char *token;
	token = strtok(systemInfo, "|");
	threshold = atoi(token);
	token = strtok(NULL, "|");
	maxThreshold = atoi(token);
	token = strtok(NULL, "|");
	maxDevice = atoi(token);

	printf("Max devices: %d\nMax threshold: %d\n", maxDevice, maxThreshold);

	// khởi tạo kết nối IP
	struct sockaddr_in serverAddr;
	clientSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (clientSocket < 0)
	{
		printf("[-]Error in connection.\n");
		exit(1);
	}

	memset(&serverAddr, '\0', sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(PORT);
	serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

	if (connect(clientSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
	{
		printf("[-]Error in connection.\n");
		exit(1);
	}
	printf("[+]Connected to Server.\n");

	while (1)
	{
		showMenuDevices();
	}

	return 0;
}

void showMenuDevices()
{
	int choice;
	char c;
	while (1)
	{
		choice = 0;
		printf("-----Welcome-----\n");
		printf("Please choose type of device to connect\n");
		int i;
		for (i = 0; i < N; ++i)
		{
			printf("%d.%s(%d|%d)\n", i + 1, deviceList[i].name, deviceList[i].normalMode, deviceList[i].savingMode);
		}
		printf("%d. Quit\n", N + 1);
		printf("Your choice: \n");
		while (choice == 0)
		{
			if (scanf("%d", &choice) < 1)
			{
				choice = 0;
			}
			if (choice < 1 || choice > N + 1)
			{
				choice = 0;
				printf("Invalid choice!\n");
				printf("Enter again: ");
			}
			while ((c = getchar()) != '\n')
				;
		}
		if (1 <= choice && choice <= N)
		{
			showMenuAction(choice);
		}
		else
		{
			exit(0);
		}
	}
}

void showMenuAction(int i)
{
	int choice;
	char c;
	while (1)
	{
		choice = 0;
		printf("-----Welcome-----\n");
		printf("Please choose an action:\n");
		printf("1. Run at default mode \n");
		printf("2. Run at saving mode\n");
		printf("3. Turn off and quit\n");
		printf("Your choice: ");
		while (choice == 0)
		{
			if (scanf("%d", &choice) < 1)
			{
				choice = 0;
			}
			if (choice < 1 || choice > 4)
			{
				choice = 0;
				printf("Invalid choice!\n");
				printf("Enter again: ");
			}
			while ((c = getchar()) != '\n')
				;
		}

		switch (choice)
		{
		case 1:
			runDevice(i - 1, 0);
			break;
		case 2:
			runDevice(i - 1, 1);
			break;
		default:
			exit(0);
		}
	}
}
void getResponse()
{
	int n = recv(clientSocket, serverResponse, MAXLINE, 0);
	if (n == 0)
	{
		perror("The server terminated prematurely");
		exit(4);
	}
	serverResponse[n] = '\0';
}

int kbhit()
{
	struct timeval tv = {0L, 0L};
	fd_set fds;
	FD_ZERO(&fds);
	FD_SET(0, &fds);
	return select(1, &fds, NULL, NULL, &tv);
}

int getch()
{
	int r;
	unsigned char c;
	if ((r = read(0, &c, sizeof(c))) < 0)
	{
		return r;
	}
	else
	{
		return c;
	}
}

void makeCommand(char *command, char *code, char *param1, char *param2)
{
	strcpy(command, code);
	strcat(command, "|");
	if (param1 != NULL)
	{
		strcat(command, param1);
		strcat(command, "|");
	}
	if (param2 != NULL)
	{
		strcat(command, param2);
		strcat(command, "|");
	}
}

void getShareMemoryPointer(char *key_from_server)
{
	int shmid;
	key_t key;
	key = atoi(key_from_server);

	if ((shmid = shmget(key, SHMSZ, 0666)) < 0)
	{
		perror("shmget");
		exit(1);
	}

	if ((shm = shmat(shmid, NULL, 0)) == (int *)-1)
	{
		perror("shmat");
		exit(1);
	}
}

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

void runDevice(int i, int isSaving)
{
	char command[100];
	char response[100];
	char buffer[20];
	char param[20];
	int countDown;
	char deviceName[100];
	strcpy(deviceName, deviceList[i].name);
	int voltage;
	if (isSaving)
	{
		strcat(deviceName, "|SAVING|");
		voltage = deviceList[i].savingMode;
	}
	else
	{
		strcat(deviceName, "|NORMAL|");
		voltage = deviceList[i].normalMode;
	}
	snprintf(buffer, 10, "%d", voltage);
	makeCommand(command, "ON", deviceName, buffer);
	send(clientSocket, command, strlen(command), 0);
	getResponse();
	getShareMemoryPointer(serverResponse);
	int *currentDevice;
	currentDevice = shm + 1;

	printf("Current Device is: %d\n", *currentDevice);

	if (*currentDevice <= maxDevice)
	{
		*currentDevice++;
	}
	else
	{
		printf("Max device is %d. Please connect next time.\n", maxDevice);
		exit(0);
	}
	countDown = 10;
	int show_message = 0;
	while (1)
	{
		if (*shm <= threshold) //4500
		{
			if (show_message == 0) {
				printf("The current device is running at %d W\n Press enter to stop this device\n", voltage);
				show_message = 1;
			}
		}
		else if (*shm <= maxThreshold) //5000
		{
			if (show_message == 0) {
				printf("Waring: The threshold is exceeded. The supply currently is %d\n", *shm);
				show_message = 1;
			}
		}
		else
		{
			printf("Current Power Consumption: %d > 5000. Maximum threshold is exceeded.\nA device will be restart in %d\n", *shm, countDown);
			countDown--;
			show_message = 0;
			if (countDown < 0)
			{
				stopDevice(deviceName);
				break;
			}
		}

		if (kbhit())
		{
			stopDevice(deviceName);
			break;
		}
		sleep(1);
	}
}

void stopDevice(char *deviceName)
{
	char command[100];
	makeCommand(command, "STOP", deviceName, NULL);
	send(clientSocket, command, strlen(command), 0);
	getResponse();
}

int countEntityNumber(char *str)
{
	int i;
	int count = 0;
	for (i = 0; i < strlen(str); ++i)
	{
		if (str[i] == ',')
		{
			count++;
		}
	}
	return count;
}

char **tokenizeString(char *str)
{
	int count = countEntityNumber(str);
	char **tokenArray;
	tokenArray = (char **)malloc(count * sizeof(char *));
	char *dup = strdup(str);
	char *token;
	int i;
	token = strtok(dup, ",");
	tokenArray[0] = token;
	for (i = 1; i < count; ++i)
	{
		tokenArray[i] = strtok(NULL, ",");
	}
	return tokenArray;
}

device parseStringToStruct(char *str)
{
	device hanhvl;
	char *dup = strdup(str);
	char *token;
	int i;
	token = strtok(dup, "|");
	strcpy(hanhvl.name, token);
	token = strtok(NULL, "|");
	hanhvl.normalMode = atoi(strdup(token));
	token = strtok(NULL, "|");
	hanhvl.savingMode = atoi(strdup(token));
	return hanhvl;
}

device *parseStringToStructArray(char *str)
{
	int count = countEntityNumber(str);
	char *dup = strdup(str);
	device *hanhvl;
	hanhvl = (device *)malloc(count * sizeof(device));
	int i;
	for (i = 0; i < count; ++i)
	{
		hanhvl[i] = parseStringToStruct(tokenizeString(dup)[i]);
	}
	return hanhvl;
}
