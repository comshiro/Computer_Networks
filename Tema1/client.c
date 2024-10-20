#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#define FIFO_TO_SERVER "FIFO_REC"
#define FIFO_FROM_SERVER "FIFO_SEND"

int char_to_int(char *s)
{
    int x = 0;
    ;
    for (int i = 0; i < strlen(s); i++)
    {
        x = x * 10 + (s[i] - '0');
    }
    return x;
}

int main()
{
    int quit = 1;
    char input[300];
    char response[300];
    int num;

    /// DESCHIDEM FIFO-URILE
    printf("Connecting to server...\n");
    int fd_write = open("/tmp/" FIFO_TO_SERVER, O_WRONLY);
    if (fd_write == -1)
    {
        perror("Failed to open FIFO for writing");
        exit(1);
    }

    int fd_read = open("/tmp/" FIFO_FROM_SERVER, O_RDONLY);
    if (fd_read == -1)
    {
        perror("Failed to open FIFO for reading response");
        exit(1);
    }
    printf("Enter a message to send to the server: ");

    /// INCEPEM LOOP-UL DE SCRIERE CITIRE
    do
    {
        fgets(input, sizeof(input), stdin); /// CITIM COMANDA DE LA TASTATURA

        if (write(fd_write, input, strlen(input)) == -1) /// CLIENTUL SCRIE COMANDA IN FIFO
        {
            perror("Error writing to the FIFO");
            exit(1);
        }

        printf("Message sent to server: \"%s\"\n", input);

        if (strcmp(input, "quit"))
        {
            quit=0;
            close(fd_read);
            close(fd_write);
            return 0;
        }

        char *res_size;
        int size;
        do /// CITIRE RESULTAT TRIMIS DE SERVER
        {
            if ((read(fd_read, res_size, 4)) == -1)
            {
                perror("Error reading response from FIFO");
            }
            else
            {
                size = char_to_int(res_size);

                if ((num = read(fd_read, response, size)) == -1)
                    response[num] = '\0';
                printf("Server's response: \"%s\"\n", response);
            }
        } while (num > 0);

    } while (quit);
}