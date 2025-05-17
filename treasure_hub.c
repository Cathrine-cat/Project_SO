
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include <sys/stat.h>
#include <termios.h>
#include <fcntl.h>
#include <dirent.h>

#define CMD_FILE "tmp/monitor_cmd.txt"
#define HUNTS_DIR "hunts" //name of hunts directory
#define TREASURE_FILE "treasure.bin" //name of treasure file in each hunt
#define HUNT_DIR_NAME "hunt"//pattern for hunt directory
#define MAX_USERNAME 40
#define MAX_CLUE 100
#define MAX_PATH 256 //max size of file path
#define MAX_SIZE_BUF 1024

pid_t monitor_pid = -1;
volatile sig_atomic_t got_command    = 0; 
volatile sig_atomic_t stop_requested = 0; 
volatile sig_atomic_t command_done   = 0;


//Treasure structure
typedef struct _treasure{
    int id;
    char username[MAX_USERNAME];
    struct gps_coord{
        float latitude;
        float longitude;
    }gps;
    char clue[MAX_CLUE];
    int value;
}treasure;


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


void ensure_compiled(char* s) {
    struct stat st;

    char comm[100];
    snprintf(comm,sizeof(comm),"gcc %s.c -o %s",s,s);

    char bin_path[100];
    snprintf(bin_path, sizeof(bin_path), "./%s", s);

    if (stat(bin_path, &st) == -1) {
        if (system(comm) != 0) {
            fprintf(stderr, "[Monitor] Failed to compile %s\n",s);
            exit(1);
        }
    }
}


//Count treasures inside a hunt 
int count_treasures(const char* hunt_id){
    int count=0;
    char treasure_path[MAX_PATH];
    snprintf(treasure_path, sizeof(treasure_path), "%s/%s/%s",HUNTS_DIR,hunt_id,TREASURE_FILE);

    int fd=open(treasure_path,O_RDONLY);
    if(fd<0){
        printf("Error opening file\n");
        exit(1);
    }

    treasure t;

    while(read(fd,&t,sizeof(treasure))==sizeof(treasure)){
        count++;
    }

    close(fd);

    return count;
   
}

void print_from_pipe(int pipefd[2]){

    static int already_nonblock = 0;
    if (!already_nonblock) {
        int flags = fcntl(pipefd[0], F_GETFL, 0);
        fcntl(pipefd[0], F_SETFL, flags | O_NONBLOCK);
        already_nonblock = 1;
    }

    char buf[MAX_SIZE_BUF];
    ssize_t bytes;
    
    while ((bytes = read(pipefd[0], buf, sizeof(buf)-1)) > 0) {
        buf[bytes] = '\0';
        printf("%s", buf);
    }
    
    fflush(stdout);
}


void parse_hunts_dir(){
    DIR* dir = opendir(HUNTS_DIR);
    if (!dir) {
        perror("opendir");
        return;
    }

    struct dirent* entry;
    while ((entry = readdir(dir))) {
        if (entry->d_type != DT_DIR || 
            strcmp(entry->d_name, ".") == 0 || 
            strcmp(entry->d_name, "..") == 0) continue;

            int pfd[2];
            if (pipe(pfd) == -1) {
                perror("pipe");
                continue;
            }
            pid_t pid = fork();
            if (pid == 0) {
                close(pfd[0]); // Child writes
                dup2(pfd[1], STDOUT_FILENO);
                close(pfd[1]);

                ensure_compiled("calculate_score");

                execlp("./calculate_score", "calculate_score", entry->d_name, NULL);
                perror("execlp");
                exit(1);
            } else if (pid > 0) {
                close(pfd[1]); // Parent reads
                waitpid(pid, NULL, 0);
                print_from_pipe(pfd);
                close(pfd[0]);
            }
        }
    closedir(dir);
}


//Parse hunts directory
void list_hunts(){
    DIR* directory=opendir(HUNTS_DIR);

    if(directory==NULL){
        printf("Error opening hunts directory\n");
        exit(2);
    }

    printf("Hunts:\n");

    struct dirent* entry;
    char file_path[MAX_PATH*2];
    struct stat info;

    while((entry=readdir(directory))){
        if(strcmp(entry->d_name,".")==0 || strcmp(entry->d_name,"..")==0){
            continue;
        }

        snprintf(file_path,sizeof(file_path),"%s/%s",HUNTS_DIR,entry->d_name);

        if(lstat(file_path,&info)<0){
            printf("Error with lstat\n");
            exit(2);
        }

        if(S_ISDIR(info.st_mode)){//in hunt directory

            //open treasure.bin + count treasures
            printf("%s with %d treasures\n",entry->d_name, count_treasures(entry->d_name));
        }
        

    }

    closedir(directory);
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

    if(strcmp(line,"-calculate_score")==0){
        printf("Scores are:\n");
        parse_hunts_dir();
    }
    else if(strcmp(line,"-list_hunts") == 0){//use function defined here
        printf("List_hunts\n");
        list_hunts();
    }
    else{ //run treasure manager 
        ensure_compiled("treasure_manager");

        //devide command to be given into arguments to use with execvp
        char* argv[10];
        int argc = 0;
    
        argv[argc++] = "./treasure_manager";
    
        char* token = strtok(line, " ");
        while (token && argc < 9) {
            argv[argc++] = token;
            token = strtok(NULL, " ");
        }
        argv[argc] = NULL;
    
        pid_t pid = fork();
        if (pid == 0) {
            
            execvp(argv[0],argv);
            perror("[Monitor] execvp");
            exit(127);
        } else if (pid > 0) {
            waitpid(pid, NULL, 0);

        } else {
            perror("[Monitor] fork");
        }
    }

    kill(getppid(), SIGUSR2);
}

void monitor_loop(int write_fd) {
    setup_monitor_signals();

    printf("[Monitor] PID %d ready\n", getpid());

    dup2(write_fd,STDOUT_FILENO);
    close(write_fd);

    setbuf(stdout, NULL);

    while (!stop_requested) {
        pause();

        if (got_command) {
            got_command = 0;
            process_command();
            fflush(stdout);
        }
    }
    
    usleep(500000);
    exit(0);
}


void print_usage(){
    printf("Commands available:\n");
    printf(">start_monitor\n>list_hunts\n>list_treasures <hunt_id>\n>view_treasure <hunt_id> <treasure_id>\n>calculate_scores\n>stop_monitor\n>exit\n\n");
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


    int pipefd[2];
    if(pipe(pipefd)==-1){
        perror("pipe");
        exit(1);
    }

    
    mkdir("tmp", 0755);

    char input[256];
    printf("Treasure Hub started. Type commands.\n");
    print_usage();

    while (1) {
        printf("> ");
        fflush(stdout);

        if (!fgets(input, sizeof(input), stdin)) break;
        input[strcspn(input, "\n")] = 0;

        if (strcmp(input, "start_monitor") == 0) {
            if (monitor_pid != -1) {
                printf("Monitor already running.\n");
            } else {
                monitor_pid = fork();
                if (monitor_pid == 0) {

                    close(pipefd[0]);//close read end
                    monitor_loop(pipefd[1]);
                }

                close(pipefd[1]);

                sleep(1);  
                tcflush(STDIN_FILENO, TCIFLUSH);
                printf("Monitor started (PID %d)\n", monitor_pid);
                
            }
        }
        else if (strncmp(input, "list_treasures", 14) == 0 && ( input[14] == ' ' || input[14] =='\0')) {
            if (monitor_pid == -1) {
                printf("Error: monitor is not running.\n");
            } else {
                char hunt[64];
                // parse out the hunt_id
                if (sscanf(input + 14, " %63s", hunt) == 1) {
                    FILE* f = fopen(CMD_FILE, "w");
                    fprintf(f, "-list %s", hunt);
                    fclose(f);
    
                    command_done = 0;
                    kill(monitor_pid, SIGUSR1);
                    while (!command_done) pause();

                    print_from_pipe(pipefd);
                } else {
                    printf("Usage: list_treasures <hunt_id>\n");
                }
            }
        }
        else if (strncmp(input, "view_treasure", 13) == 0 && ( input[13]==' ' || input[13]=='\0')) {
            if (monitor_pid == -1) {
                printf("Error: monitor is not running.\n");
            } else {
                char hunt[64], tid[64];
                
                if (sscanf(input + 13, " %63s %63s", hunt, tid) == 2) {
                    FILE* f = fopen(CMD_FILE, "w");
                    fprintf(f, "-view %s %s", hunt, tid);
                    fclose(f);
    
                    command_done = 0;
                    kill(monitor_pid, SIGUSR1);
                    while (!command_done) pause();

                   print_from_pipe(pipefd);
                } else {
                    printf("Usage: view_treasure <hunt_id> <treasure_id>\n");
                }
            }
        }
        else if(strcmp(input, "list_hunts")==0){
            if(monitor_pid == -1){
                printf("Error: monitor is not running.\n");
            } else {
                FILE* f = fopen(CMD_FILE, "w");
                    fprintf(f, "-list_hunts\n");
                    fclose(f);
    
                    command_done = 0;
                    kill(monitor_pid, SIGUSR1);
                    while (!command_done) pause();

                    print_from_pipe(pipefd);
            }
        }
        else if (strcmp(input, "stop_monitor") == 0) {
            if (monitor_pid == -1) {
                printf("No monitor running.\n");
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
                printf("Stop the monitor first.\n");
            } else {
                unlink(CMD_FILE);
                rmdir("tmp");
                break;
            }
        }
        else if(strncmp(input,"calculate_scores",16)==0){
            if(monitor_pid == -1){
                printf("Error: monitor is not running.\n");
            } else {
                FILE* f = fopen(CMD_FILE, "w");
                fprintf(f, "-calculate_score\n");
                fclose(f);
    
                command_done = 0;
                kill(monitor_pid, SIGUSR1);
                while (!command_done) pause();

                print_from_pipe(pipefd);
            }
        }
        else {
            printf("Unknown command: %s\n", input);
        }
    }
    
    return 0;
}
