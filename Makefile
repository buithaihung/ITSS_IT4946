all:client.c server.c systemInfo.c equipInfo.c writeLogProcess.c;gcc -o client client.c;gcc -o server server.c;gcc -o systemInfo systemInfo.c;gcc -o equipInfo equipInfo.c;gcc -o writeLogProcess writeLogProcess.c;

clean:
	rm -f client server equipInfo systemInfo writeLogProcess
