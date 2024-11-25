#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <errno.h>

#define pathul_fifo "fifoserver"
#define pathul_fifo_back "fifoserverback"

char comanda[1001];
char response[1001];
int fifo_server;
int fifo_server_back;

int main()
{
    fifo_server = open(pathul_fifo, O_WRONLY);
    fifo_server_back = open(pathul_fifo_back, O_RDONLY);
    while(1)
    {
        printf("Command: ");
        fgets(comanda, sizeof(comanda), stdin);
        comanda[strcspn(comanda, "\n")] = 0;
        write(fifo_server, comanda, strlen(comanda));
        if(strcmp(comanda, "quit") == 0) break;
        comanda[0] = '\0';
        int lungime;
        read(fifo_server_back, &lungime, sizeof(lungime));
        read(fifo_server_back, response, lungime);
        response[lungime] = '\0';
        printf("Server awnser: %s\n", response);
    }
    close(fifo_server);
    close(fifo_server_back);
    return 0;
}