#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>

volatile sig_atomic_t afisari = 0;
volatile sig_atomic_t ignore_sigint = 1; // 1 dacă SIGINT trebuie ignorat, 0 după 60 secunde

// Funcție de tratare pentru semnalul SIGUSR1
void handle_sigusr1(int sig) {
    FILE *file = fopen("semnal.txt", "a");
    if (file == NULL) {
        perror("Eroare la deschiderea fișierului");
        exit(1);
    }
    fprintf(file, "Am primit semnal\n");
    fclose(file);
}

// Funcție de tratare pentru semnalul SIGINT
void handle_sigint(int sig) {
    if (ignore_sigint) {
        printf("SIGINT este ignorat în primele 60 de secunde.\n");
    } else {
        // Revenim la comportamentul default pentru SIGINT
        signal(SIGINT, SIG_DFL);
        raise(SIGINT); // Retrimitem semnalul SIGINT pentru a închide programul
    }
}

// Funcție care este apelată după 60 de secunde pentru a permite semnalul SIGINT
void allow_sigint(int sig) {
    ignore_sigint = 0;
    printf("Semnalul SIGINT nu mai este ignorat.\n");
}

int main() {
    // Setăm funcția pentru gestionarea semnalelor
    signal(SIGUSR1, handle_sigusr1);
    signal(SIGINT, handle_sigint);

    // Programul va permite SIGINT după 60 de secunde
    alarm(60);
    signal(SIGALRM, allow_sigint);

    while (1) {
        afisari++;
        printf("Afisare #%d - PID: %d\n", afisari, getpid());
        sleep(3); // Așteptăm 3 secunde înainte de următoarea afişare
    }

    return 0;
}

