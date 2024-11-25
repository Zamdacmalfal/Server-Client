#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <utmpx.h>
#include <errno.h>
#include <sys/stat.h>

#define pathul_fifo "fifoserver"
#define pathul_fifo_back "fifoserverback"

char comanda[1001];
int fifo_server;
int fifo_server_back;
int socket_from_copil[2];
int pipe_from_parent[2];
int logged_in = 0;

int log_in(char user[30])
{
    FILE *file = fopen("users.conf", "r");
    char line[30];
    while(fgets(line, sizeof(line), file))
    {
        line[strcspn(line, "\n")] = '\0';
        if(strcmp(line, user) == 0)
        {
            fclose(file);
            return 1;
        }
    }
    fclose(file);
    return 0;
}

int main()
{
    unlink(pathul_fifo);
    unlink(pathul_fifo_back);

    pipe(pipe_from_parent);
    mkfifo(pathul_fifo, 0666);
    mkfifo(pathul_fifo_back, 0666);
    fifo_server = open(pathul_fifo, O_RDONLY);
    fifo_server_back = open(pathul_fifo_back, O_WRONLY);

    while(1)
    {
        memset(comanda, 0, sizeof(comanda));
        if(read(fifo_server, comanda, sizeof(comanda)) > 0)
        {
            comanda[strcspn(comanda, "\n")] = 0;
            if(strcmp(comanda, "quit") == 0)
            {
                break;
            }
        }
        socketpair(AF_UNIX, SOCK_STREAM, 0, socket_from_copil);

        pid_t pid = fork();
        if(pid == 0)
        {
            char response[1001];
            memset(response, 0, sizeof(response));
            char nume[21];
            read(pipe_from_parent[0], comanda, sizeof(comanda));
            read(pipe_from_parent[0], &logged_in, sizeof(logged_in));

            if(strncmp(comanda, "login : ", 8) == 0)
            {
               strcpy(nume, comanda + 8);
               if(log_in(nume))
               {
                logged_in = 1;
                strcpy(response, "Buna ");
                strcat(response, nume);
                strcat(response, " !\n");
               }
               else
               {
                strcpy(response, "Unknown user!\n");
               }

               
            }
            else if(strcmp(comanda, "get-logged-users") == 0)
            {
                if(logged_in != 0)
                {
                    struct utmpx *info;
                    struct tm *login_time;
                    setutxent();
                    while((info = getutxent()) != NULL)
                    {
                        if(info -> ut_type == USER_PROCESS)
                        {
                            strcpy(response, "Username: ");
                            strcat(response, info -> ut_user);
                            strcat(response, "\n");
                            strcat(response, "Hostname: (local login) \n");
                            login_time = localtime(&info -> ut_tv.tv_sec);
                            strcat(response, "Login time: ");
                            strcat(response, asctime(login_time));
                        }
                    }
                    endutxent();
                }
                else
                {
                    strcpy(response, "You can't see (ur not logged in)\n");
                }
            }
            else if(strcmp(comanda, "logout") == 0)
            {
                if(logged_in > 0)
                {
                    logged_in --;
                    strcpy(response, "You logged out\n");
                }
                else 
                {
                    strcpy(response, "Nobody is logged in\n");
                }
            }
            else
            {
                strcpy(response, "Unknown command \n");
            }

            int lungime = strlen(response);
            write(socket_from_copil[1], &lungime, sizeof(lungime));
            write(socket_from_copil[1], response, lungime);
            exit(EXIT_SUCCESS);
        }
        else if(pid > 0)
        {
            write(pipe_from_parent[1], comanda, sizeof(comanda));
            write(pipe_from_parent[1], &logged_in, sizeof(logged_in));
            char response[1001];
            int lungime;
            read(socket_from_copil[0], &lungime, sizeof(lungime));
            read(socket_from_copil[0], response, lungime);
            response[lungime] = '\0';
            write(fifo_server_back, &lungime, sizeof(lungime));
            write(fifo_server_back, response, strlen(response));
            if(strncmp(comanda, "login", 5) == 0 && strstr(response, "Buna") != NULL) logged_in = 1;
            else if(strcmp(comanda, "logout") == 0 && strstr(response , "You logged out") != NULL) logged_in = 0;
            wait(NULL);
        }
        else
        {
            perror("Fail la fork");
        }
    }
    close(fifo_server);
    close(fifo_server_back);
    return 0;
}