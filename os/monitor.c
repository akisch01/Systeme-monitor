#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

#define HISTORY_SIZE 30
#define MEM_THRESHOLD (700 * 1024) // Seuil pour déclencher l'alerte

// Variables globales
pid_t child_pid = -1;
int cpu_history[HISTORY_SIZE];
int mem_history[HISTORY_SIZE];
long mem_total = 0;

void print_bar(float percentage, int length);
void print_ascii_graph(int *history, char *label, int start_index);
void send_alert_email(char *message);

void print_bar(float percentage, int length) {
    int filled = (percentage * length) / 100;
    printf("[");
    for (int i = 0; i < filled; i++) printf("#");
    for (int i = filled; i < length; i++) printf(" ");
    printf("] %.1f%%", percentage);
}

void print_ascii_graph(int *history, char *label, int start_index) {
    printf("\n%s :\n", label);
    for (int i = start_index; i < HISTORY_SIZE; i++) {
        printf("%3d%%|", history[i]);
        for (int j = 0; j < history[i] / 5; j++) printf("█");
        printf("\n");
    }
}

void send_alert_email(char *message) {
    char command[512];
    snprintf(command, sizeof(command),
             "echo \"%s\" | mail -s \"Alerte Système\" akpoakisch@gmail.com",
            message);
    system(command);
}

void monitor_cpu_memory() {
    FILE *log_file = fopen("log.txt", "w");
    FILE *data_file = fopen("data.txt", "a+");
    
    memset(cpu_history, -1, sizeof(cpu_history));
    memset(mem_history, -1, sizeof(mem_history));
    
    FILE *meminfo = fopen("/proc/meminfo", "r");
    char line[256];
    while (fgets(line, sizeof(line), meminfo)) {
        if (strstr(line, "MemTotal:")) {
            sscanf(line, "MemTotal: %ld kB", &mem_total);
            break;
        }
    }
    fclose(meminfo);

    unsigned long long last_total = 0, last_idle = 0;
    while (1) {
        system("clear");

        long mem_free = 0;
        meminfo = fopen("/proc/meminfo", "r");
        while (fgets(line, sizeof(line), meminfo)) {
            if (strstr(line, "MemFree:")) sscanf(line, "MemFree: %ld kB", &mem_free);
        }
        fclose(meminfo);

        FILE *stat = fopen("/proc/stat", "r");
        unsigned long long total = 0, idle = 0;
        while (fgets(line, sizeof(line), stat)) {
            if (strstr(line, "cpu ")) {
                sscanf(line + 3, "%llu %llu %llu %llu", &total, &idle, &total, &total);
                break;
            }
        }
        fclose(stat);

        float cpu_usage = last_total > 0 ? 100.0 - (100.0 * (idle - last_idle)) / (total - last_total) : 0;
        last_total = total;
        last_idle = idle;

        int mem_usage = 100 - (int)((mem_free * 100) / mem_total);

        memmove(cpu_history, cpu_history + 1, (HISTORY_SIZE - 1) * sizeof(int));
        cpu_history[HISTORY_SIZE - 1] = (int)cpu_usage;

        memmove(mem_history, mem_history + 1, (HISTORY_SIZE - 1) * sizeof(int));
        mem_history[HISTORY_SIZE - 1] = mem_usage;

        printf("\n=== MONITEUR SYSTÈME DÉMARRÉ ===\n");
        printf("Mise à jour des données toutes les 10 secondes.\n");
        printf("Pour arrêter, faites CTRL+C ou créez un fichier 'stop' dans le même répertoire.\n\n");

        printf("CPU : "); print_bar(cpu_usage, 20);
        printf("\nMEM : "); print_bar(mem_usage, 20);

        int start_index = 0;
        while (cpu_history[start_index] == -1) start_index++;
        print_ascii_graph(cpu_history, "Historique CPU", start_index);

        start_index = 0;
        while (mem_history[start_index] == -1) start_index++;
        print_ascii_graph(mem_history, "Historique Mémoire", start_index);
        
        // Écriture données
        fprintf(data_file, "%d %d\n", (int)cpu_usage, 100 - (int)((mem_free * 100) / mem_total));
        fflush(data_file);
        
        fprintf(log_file, "CPU Utiliser: %.1f%%, Memory Utiliser: %ld kB free\n", cpu_usage, mem_free);
        fflush(log_file);

        if (mem_free < MEM_THRESHOLD) {
            system("paplay beep.wav && notify-send 'Alerte Mémoire' 'Mémoire faible !'");
            send_alert_email("Alerte : Mémoire insuffisante !");
        }

        if (access("stop", F_OK) != -1) {
            printf("Fichier 'stop' détecté. Arrêt du programme.\n");
            remove("stop");
            exit(EXIT_SUCCESS);
        }

        sleep(10);
    }
}

void handle_sigint(int sig) {
    char response;
    while (1) {
        printf("\nVoulez-vous arrêter ? (o/n) : ");
        scanf(" %c", &response);
        if (response == 'o' || response == 'O') {
            printf("Arrêt du programme.\n");
            if (child_pid > 0) kill(child_pid, SIGTERM);
            exit(EXIT_SUCCESS);
        } else if (response == 'n' || response == 'N') {
            printf("Reprise du programme.\n");
            break;
        } else {
            printf("Réponse invalide. Veuillez entrer 'o' pour oui ou 'n' pour non.\n");
        }
    }
}

int main() {
    signal(SIGINT, handle_sigint);
    child_pid = fork();

    if (child_pid < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (child_pid == 0) {
        signal(SIGINT, SIG_IGN);
        monitor_cpu_memory();
        exit(EXIT_SUCCESS);
    } else {
        wait(NULL);
        printf("Processus fils terminé.\n");
    }

    return 0;
}