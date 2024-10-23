#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/wait.h>

#define FIFO_FROM_CLIENT "FIFO_REC"
#define FIFO_TO_CLIENT "FIFO_SEND"
#define CONFIGURATION_FILE "USERS.txt"
#define BUF_SIZE 42

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

int logged_in = 0;
int quit = 1;
char *login(char *s)
{
    char *result = "";
    if (logged_in == 1)
        return result = "User already logged in";

    FILE *file = fopen(CONFIGURATION_FILE, "r");
    if (!file)
        perror("Configuration file cannot be opened");

    char line[256];
    while (fgets(line, sizeof(line), file)) {
        line[strchr(line, '\n') - line] = '\0';
        if (strcmp(s, line) == 0)
        {
            logged_in = 1;
            return result = "Valid user. User has logged in";
        }
    }

    logged_in = 0;
    return result = "Invalid user.Try again";
}

size_t setupFIFOsOpenRead(struct FIFOs *fifos) {
    int fd_read = open(fifos->fifo_from_client, O_RDONLY);
    if (fd_read == -1)
    {
        perror("open read failed");
        exit(1);
    }
    return fd_read;
}

size_t setupFIFOsOpenWrite(struct FIFOs *fifos) {
    int fd_read = open(fifos->fifo_to_client, O_WRONLY);
    if (fd_read == -1)
    {
        perror("open read failed");
        exit(1);
    }
    return fd_read;
}

int main()
{
    struct FIFOs fifos = createFIFO();
    int fd_read = setupFIFOsOpenRead(&fifos);
    int fd_write = setupFIFOsOpenWrite(&fifos);

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

    while (42)
    {
        // Reading from the fifo
        char buf[BUF_SIZE];
        int bytes_read = read(fd_read, buf, BUF_SIZE - 1);
        if (bytes_read == -1)
        {
            perror("read failed");
            continue ; // break
        }
        buf[bytes_read] = '\0';
        printf("Server is getting the command: %s\n", buf);
        /**
         * Getting a command implies:
         *  (Child-process)
         *  1. Create child (pipes are already working, we only need to setup) DONE
         *  2. Parse command on child (wait for parent to write it on the pipe) DONE
         *  3. Execute command on child DONE
         *  4. Write Command result on the child pipe DONE
         * (Parent Process)
         *  1. Write the command to the child (dumb)
         *  2. Wait for the result of the child (waiting can be implied on the read blocking nature (not recomended), wait(NULL))
         *  3. Continue execution
         **/
        int pid = fork();
        switch (pid)
        {
            case -1:
                perror("Eroare la fork");
                break;
            case 0: // Child process
                close(p_p2c[1]);
                int num;
                char s[300];
                char *result;

                do
                {
                    if ((num = read(p_p2c[0], s, 300)) == -1)
                        perror("Eroare la citirea din pipe parent -> child");
                    else
                        s[num] = '\0';
                } while (num < 0);
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
                int n = strlen(result) + 1; // THIS WOULD NOT BE A POINTER, IT WOULD BE A NUMBER THAT I CHOOSE, IN AN ADDRESS (0x00000005)
                write(s_c2p[1], &n, sizeof(int));
                write(s_c2p[1], result, n);

                close(p_p2c[0]); // Close read end of the pipe
                close(s_c2p[1]); // Close write end of socketpair
                exit(0);         // Exit child process
            default:
                if (write(p_p2c[1], buf, strlen(buf)) == -1)
                {
                    perror("Eroare la 'write' din pipe parent -> child");
                }
                int n_buf[1];
                bytes_read = read(s_c2p[0], n_buf, sizeof(int));
                if (bytes_read == -1)
                {
                    perror("Eroare la citirea din pipe child -> parent");
                }
                char buf[BUF_SIZE];
                bytes_read = read(s_c2p[0], buf, BUF_SIZE - 1);
                if (bytes_read == -1)
                {
                    perror("Eroare la citirea din pipe child -> parent");
                }
                buf[bytes_read] = '\0';
                printf("We got the result: [%.10s]\n", buf);
                wait(NULL);
                // Send to the client
                write(fd_write, &buf, bytes_read);
                // TODO
                break;
        }
    }
    return 0;
}
