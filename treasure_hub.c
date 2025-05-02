#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include <sys/stat.h>
#include <termios.h>

#define CMD_FILE "tmp/monitor_cmd.txt"

pid_t monitor_pid = -1;
volatile sig_atomic_t got_command    = 0; 
volatile sig_atomic_t stop_requested = 0; 
volatile sig_atomic_t command_done   = 0;



void handler_sigchild(int sig) {
    int saved = errno, status;
    while (waitpid(-1, &status, WNOHANG) > 0) {
    }
    errno = saved;
}

void handler_sigusr2(int sig) {
    command_done = 1;
}

void handle_sigusr1(int sig) {
    got_command = 1;
}

void handle_sigterm(int sig) {
    stop_requested = 1;
}



void setup_monitor_signals() {
    struct sigaction sa = {0};
    sa.sa_flags = SA_RESTART;
    sigemptyset(&sa.sa_mask);

    sa.sa_handler = handle_sigusr1;
    sigaction(SIGUSR1, &sa, NULL);

    sa.sa_handler = handle_sigterm;
    sigaction(SIGTERM, &sa, NULL);
}


void ensure_compiled() {
    struct stat st;
    if (stat("./treasure_manager", &st) == -1) {
        if (system("gcc treasure_manager.c -o treasure_manager") != 0) {
            fprintf(stderr, "[Monitor] Failed to compile treasure_manager\n");
            exit(1);
        }
    }
}

void process_command() {
    FILE* f = fopen(CMD_FILE, "r");
    if (!f) { perror("[Monitor] fopen"); return; }

    char line[256];
    if (!fgets(line, sizeof(line), f)) {
        fclose(f);
        return;
    }
    fclose(f);
    line[strcspn(line, "\n")] = 0;

    ensure_compiled();

    char* argv[10];
    int argc = 0;

    argv[argc++] = "./treasure_manager";

    char* token = strtok(line, " ");
    while (token && argc < 9) {
        argv[argc++] = token;
        token = strtok(NULL, " ");
    }
    argv[argc] = NULL;

    for(int i=0;i<argc;i++){
        printf("fff %s\n",argv[i]);
    }

    pid_t pid = fork();
    if (pid == 0) {
        
        execvp(argv[0],argv);
        perror("[Monitor] execvp");
        _exit(127);
    } else if (pid > 0) {
        waitpid(pid, NULL, 0);
    } else {
        perror("[Monitor] fork");
    }

    kill(getppid(), SIGUSR2);
}

void monitor_loop() {
    setup_monitor_signals();
    printf("[Monitor] PID %d ready\n", getpid());
    while (!stop_requested) {
        pause();                  
        if (got_command) {
            got_command = 0;
            process_command();
        }
    }
    
    usleep(500000);
    //printf("[Monitor] exiting\n");
    exit(0);
}


int main() {
   
    struct sigaction sa_chld = { .sa_handler = handler_sigchild,
                                 .sa_flags   = SA_RESTART };
    sigemptyset(&sa_chld.sa_mask);
    sigaction(SIGCHLD, &sa_chld, NULL);

    
    struct sigaction sa_usr2 = { .sa_handler = handler_sigusr2,
                                 .sa_flags   = SA_RESTART };
    sigemptyset(&sa_usr2.sa_mask);
    sigaction(SIGUSR2, &sa_usr2, NULL);

    
    mkdir("tmp", 0755);

    char input[256];
    printf("Treasure Hub started. Type commands.\n");

    while (1) {
        printf("> ");
        fflush(stdout);

        if (!fgets(input, sizeof(input), stdin)) break;
        input[strcspn(input, "\n")] = 0;

        if (strcmp(input, "start_monitor") == 0) {
            if (monitor_pid != -1) {
                puts("Monitor already running.");
            } else {
                monitor_pid = fork();
                if (monitor_pid == 0) {
                    monitor_loop();
                }
                sleep(1);  
                tcflush(STDIN_FILENO, TCIFLUSH);
                printf("Monitor started (PID %d)\n", monitor_pid);
                
            }
        }
        /*else if (strncmp(input, "list_hunts", 10) == 0
              || strncmp(input, "list_treasures", 14) == 0
              || strncmp(input, "view_treasure", 13) == 0) {
            if (monitor_pid == -1) {
                puts("Error: monitor is not running.");
            } else {

                
                char buf[256];
                if(strncmp(input, "view_treasure", 13)==0) {
                    char hunt[64], tid[64];
                    sscanf(input + 13, "%s %s", hunt, tid);
                    snprintf(buf, sizeof(buf), "-view %s %s", hunt, tid);
                }
                if(strncmp(input, "list_treasures", 14)==0){
                    char buf[256];
                    snprintf(buf, sizeof(buf), "-list %s", input + 14);
                }

                printf("Buf is %s \n",buf);
                FILE* f = fopen(CMD_FILE, "w");
                fprintf(f, "%s\n", buf);
                fclose(f);
                
                command_done = 0;
                kill(monitor_pid, SIGUSR1);

                while (!command_done) pause();
            }
        }*/
        else if (strncmp(input, "list_treasures", 14) == 0) {
            if (monitor_pid == -1) {
                puts("Error: monitor is not running.");
            } else {
                char hunt[64];
                // parse out the hunt_id
                if (sscanf(input + 14, " %63s", hunt) == 1) {
                    FILE* f = fopen(CMD_FILE, "w");
                    fprintf(f, "-list %s\n", hunt);
                    fclose(f);
    
                    command_done = 0;
                    kill(monitor_pid, SIGUSR1);
                    while (!command_done) pause();
                } else {
                    puts("Usage: list_treasures <hunt_id>");
                }
            }
        }
        else if (strncmp(input, "view_treasure", 13) == 0) {
            if (monitor_pid == -1) {
                puts("Error: monitor is not running.");
            } else {
                char hunt[64], tid[64];
                // parse hunt_id and treasure_id
                if (sscanf(input + 13, " %63s %63s", hunt, tid) == 2) {
                    FILE* f = fopen(CMD_FILE, "w");
                    fprintf(f, "-view %s %s\n", hunt, tid);
                    fclose(f);
    
                    command_done = 0;
                    kill(monitor_pid, SIGUSR1);
                    while (!command_done) pause();
                } else {
                    puts("Usage: view_treasure <hunt_id> <treasure_id>");
                }
            }
        }
        else if (strcmp(input, "stop_monitor") == 0) {
            if (monitor_pid == -1) {
                puts("No monitor running.");
            } else {
                kill(monitor_pid, SIGTERM);
                monitor_pid = -1;
                 printf("Monitor stopped.\n");
        fflush(stdout);
        continue;  
            }
        }
        else if (strcmp(input, "exit") == 0) {
            if (monitor_pid != -1) {
                puts("Stop the monitor first.");
            } else {
                unlink(CMD_FILE);
                rmdir("tmp");
                break;
            }
        }
        else {
            printf("Unknown command: %s\n", input);
        }
    }
    
    return 0;
}
