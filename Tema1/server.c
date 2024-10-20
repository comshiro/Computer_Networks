#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/socket.h>

#define FIFO_FROM_CLIENT "FIFO_REC"
#define FIFO_TO_CLIENT "FIFO_SEND"
#define CONFIGURATION_FILE "USERS"

struct FIFOs
{
    const char *fifo_from_client;
    const char *fifo_to_client;
};

struct FIFOs createFIFO()
{
    const char *fifo_from_client = "/tmp/" FIFO_FROM_CLIENT;
    const char *fifo_to_client = "/tmp/" FIFO_TO_CLIENT;

    if (mkfifo(fifo_from_client, 0666) == -1)
    {
        perror("mkfifo failed for client-to-server");
    }

    if (mkfifo(fifo_to_client, 0666) == -1)
    {
        perror("mkfifo failed for server-to-client");
    }

    struct FIFOs fifos = {fifo_from_client, fifo_to_client};
    return fifos;
}

int logged_in;
int quit = 1;
char *login(char *s)
{
    char *result = "";
    if (logged_in == 1)
        return result = "User already logged in";

    FILE *file = fopen(CONFIGURATION_FILE, "r");

    char line[256];
    while (fgets(line, sizeof(line), file))
        if (strcmp(s, line) == 0)
        {
            logged_in = 1;
            return result = "Valid user. User has logged in";
        }

    logged_in = 0;
    return result = "Invalid user.Try again";
}

int main()
{

    struct FIFOs fifos = createFIFO();

    int p_p2c[2]; /// Pipe pentru comunicare parinte->copil
    int s_c2p[2]; // Socketpair pentru comunicare copil->parinte

    if (pipe(p_p2c) == -1)
    {
        perror("pipe failed");
    }
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, s_c2p) == -1)
    {
        perror("socketpair failed");
    }

    int pid = fork();
    switch (pid)
    {

    case -1:
        perror("Eroare la fork");
        break;

    case 0: ////PROCESARE COMANDA///EFECTUATA DE COPIL
    {
        close(p_p2c[1]);
        int num;
        char s[300];
        char *result;

        while (quit)
        {
            do
            {
                if ((num = read(p_p2c[0], s, 300)) == -1)
                    perror("Eroare la citirea din pipe parent -> child");
                else
                    s[num] = '\0';
            } while (num > 0);

            char *p = strtok(s, " ");
            while (p)
            {
                if (strcmp(p, "login") == 0)
                {
                    p = strtok(NULL, " ");
                    result = login(p);
                }

                if (strcmp(p, "quit") == 0)
                {
                    logged_in = 0;
                    quit = 0;
                    result = "Ati fost deloggat";
                    break;
                }
                p = strtok(NULL, " ");
            }

            /// COPILUL INCARCA REZULTATUL IN SOCKEPAIR
            int n = strlen(result) + 1;
            write(s_c2p[1], &n, sizeof(int));
            write(s_c2p[1], result, n);
        }

        close(p_p2c[0]); // Close read end of the pipe
        close(s_c2p[1]); // Close write end of socketpair
        exit(0);         // Exit child process
    }

    default:
        ////CREARE SI DESCHIDERE FD PT FIFOS////////

        int fd_read = open(fifos.fifo_from_client, O_RDONLY);
        if (fd_read == -1)
        {
            perror("open read failed");
            return 1;
        }

        int fd_write = open(fifos.fifo_to_client, O_WRONLY);
        if (fd_write == -1)
        {
            perror("open write failed");
            return 1;
        }

        while (quit)
        {
            char s[300];
            int num;

            do /// Tatal citeste commanda si o stocheaza in s
            {
                if ((num = read(fd_read, s, 300)) == -1)
                    perror("Eroare la citirea din FIFO!");
                else
                    s[num] = '\0';
            } while (num > 0);

            /// Tatal incarca s in pipe
            write(p_p2c[1], s, num);
            char *result;
            /// Tatal citeste rezultatul din socketpair si il scrie in fifo to client
            do
            {
                if ((num = read(s_c2p[0], result, 300)) == -1)
                    perror("Eroare la citirea din socketpair!");
                else
                    result[num] = '\0';
            } while (num > 0);

            write(fd_write, result, num);
        }
        close(fd_read);
        close(fd_write);
    }

    return 0;
}
