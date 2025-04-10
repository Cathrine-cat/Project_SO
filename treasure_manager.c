#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>

#define MAX_PATH 250
#define MAX_USERNAME 40
#define MAX_CLUE 100
#define MAX_MSG 250
#define MAX_STR_LEN 100
#define MAX_LINE 100

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

//create hunts directory if it doesnt exist
void create_hunts(){
    struct stat st = {0};
    if (stat("hunts", &st) == -1) {
        if(mkdir("hunts", 0700)){
            printf("Error creating hunts dir\n");
            exit(2);
        }
    }
}

//create directory for specific hunt
void create_hunt(const char* hunt_id) {
    create_hunts();//create hunts directory if not exists

    char hunt_path[MAX_PATH];
    snprintf(hunt_path, sizeof(hunt_path), "hunts/%s",hunt_id);

    struct stat st={0};
    if(stat(hunt_path,&st)==-1){
        if (mkdir(hunt_path, 0700)){
            printf("Error creating hunt directory\n");
            exit(3);
        }
    }
}

//log performed operation and make a symlink
void log_operation( const char* hunt_id, const char* message){
    char path[256];
    snprintf(path, sizeof(path), "hunts/%s/%s.logs", hunt_id, hunt_id);
    int fd = open(path, O_WRONLY | O_APPEND | O_CREAT, 0644);//open log file
    if (fd < 0) {
        printf("Error opening log file\n");
        exit(5);
    }

    time_t now = time(NULL);
    char *timestamp = ctime(&now);
    dprintf(fd, "[%.*s] %s\n", (int)(strlen(timestamp) - 1), timestamp, message);
    close(fd);

    //create symlink
    char symlink_name[MAX_PATH];
    snprintf(symlink_name, sizeof(symlink_name), "logged_hunt-%s", hunt_id);

    struct stat st={0};
    if(lstat(symlink_name,&st)==-1){//symlink does not yet exist 
        if(symlink(path, symlink_name)!=0){//create->if fail 
            printf("Error creating symbolic link\n");//fail message
            exit(5);
        }
    }
   
}

int proceed()
{
   printf("\nPress ENTER to proceed. Press anything else to cancel\n");
   int c=getchar();
   if(c!='\n')
   {
    while(c!='\n')
    {
        c=getchar();
    }
    return 0;
   }
    return 1;
}


int is_valid_natural_number(char *input) {
    if (!isdigit(input[0])) return 0;
    for (int i = 0; input[i]; i++) {
        if (!isdigit(input[i])) return 0;
    }
    return atoi(input) > 0;
}

int get_input(char *prompt, char *buffer, int size) {
    printf("%s", prompt);
    if (fgets(buffer, size, stdin) == NULL) return 0;
    buffer[strcspn(buffer, "\n")] = '\0';
    if (strlen(buffer) == 1 && (buffer[0] == 'q' || buffer[0] == 'Q')) {
        return -1;
    }
    return 1;
}

int get_treasure(treasure* t){

    char input[MAX_STR_LEN];
    int valid = 0;

    printf("Please enter the following data or press Q to quit\n");

    while (!valid) {
        int res = get_input("Enter ID (natural number): ", input, MAX_STR_LEN);
        if (res == -1) {
            printf("Quitting...\n");
            return 1;
        }
        if (is_valid_natural_number(input)) {
            t->id = atoi(input);
            valid = 1;
        } else {
            printf("Invalid ID. Please enter a natural number.\n");
        }
    }

   
    valid = 0;
    while (!valid) {
        int res = get_input("Enter username: ", input, MAX_STR_LEN);
        if (res == -1) {
            printf("Quitting...\n");
            return 1;
        }
        if (strlen(input) > 0) {
            strncpy(t->username, input, MAX_USERNAME);
            valid = 1;
        } else {
            printf("Username cannot be empty.\n");
        }
    }

    valid = 0;
    while (!valid) {
        int res = get_input("Enter latitude (float): ", input, MAX_STR_LEN);
        if (res == -1) {
            printf("Quitting...\n");
            return 1;
        }
        if (sscanf(input, "%f", &(t->gps.latitude)) == 1) {
            valid = 1;
        } else {
            printf("Invalid latitude. Please enter a valid float.\n");
        }
    }

    valid = 0;
    while (!valid) {
        int res = get_input("Enter longitude (float): ", input, MAX_STR_LEN);
        if (res == -1) {
            printf("Quitting...\n");
            return 1;
        }
        if (sscanf(input, "%f", &(t->gps.longitude)) == 1) {
            valid = 1;
        } else {
            printf("Invalid longitude. Please enter a valid float.\n");
        }
    }

    valid = 0;
    while (!valid) {
        int res = get_input("Enter clue: ", input, MAX_STR_LEN);
        if (res == -1) {
            printf("Quitting...\n");
            return 1;
        }
        if (strlen(input) > 0) {
            strncpy(t->clue, input, MAX_CLUE);
            valid = 1;
        } else {
            printf("Clue cannot be empty.\n");
        }
    }

    valid = 0;
    while (!valid) {
        int res = get_input("Enter value (integer): ", input, MAX_STR_LEN);
        if (res == -1) {
            printf("Quitting...\n");
            return 1;
        }
        if (sscanf(input, "%d", &(t->value)) == 1) {
            valid = 1;
        } else {
            printf("Invalid value. Please enter a valid integer.\n");
        }
    }

    return 0;
}


//add treasure to a hunt
void add_treasure(const char* hunt_id) {
    create_hunt(hunt_id);//create hunt to add treasure to if not exists

    char treasure_path[MAX_PATH];
    snprintf(treasure_path, sizeof(treasure_path), "hunts/%s/%s", hunt_id,"treasure.bin");


    int fd = open(treasure_path, O_CREAT | O_WRONLY | O_APPEND, 0644);
    if (fd<0) {
        printf("Error adding treasure\n");
        exit(4);
    }

    //add treasure
    treasure* t=(treasure*)malloc(sizeof(treasure));

    if(get_treasure(t)){
        printf("Aborded action\n");
        free(t);
        close(fd);
        return;
    }

    printf("\nThe entered data is:\n");
    printf("ID: %d\n",t->id);
    printf("Username: %s\n",t->username);
    printf("Gps coordinates: %lf latitude; %lf longitude\n",t->gps.latitude,t->gps.longitude);
    printf("Clue: %s\n",t->clue);
    printf("Value: %d\n",t->value);

    if(proceed()!=1){
        free(t);
        close(fd);
        printf("Aboarded\n");
        return;
    }
    
    int id=t->id;

    if(write(fd,t,sizeof(treasure))<0){
        free(t);
        close(fd);
        printf("Error writing in file\n");
        exit(4);
    }
    else{
        printf("Successfully added treasure\n");
        free(t);
    }

    close(fd);

    char log_msg[MAX_MSG];
    snprintf(log_msg, sizeof(log_msg), "Added treasure ID %d to hunt %s", id, hunt_id);
    log_operation(hunt_id, log_msg);

}


void list_treasures(const char* hunt_id,int index){
    char treasure_path[MAX_PATH];
    snprintf(treasure_path, sizeof(treasure_path), "hunts/%s/treasure.bin", hunt_id);

    int fd=open(treasure_path,O_RDONLY);
    if(fd<0){
        printf("Error opening file\n");
        exit(6);
    }

    struct stat treasure_stat = {0};
    off_t treasure_size = 0;
    if (stat(treasure_path, &treasure_stat) == 0) {
        treasure_size = treasure_stat.st_size;
    }

    treasure t;

    int* ids=NULL;
    int size_ids=0;
    while(read(fd,&t,sizeof(treasure_size))==sizeof(treasure_size)){
        int duplicate=0;

        for(int i=0;i<size_ids;i++){
            if(ids[i]==t.id){
                duplicate=1;
                break;
            }
        }

        if(duplicate==0){//not duplicate
            printf("%*s", index, "");
            printf("Treasure ID: %d\n",t.id);
            int* tmp = realloc(ids, (size_ids + 1) * sizeof(int));
        if (!tmp) {
            printf("Memory allocation failed\n");
            free(ids);
            close(fd);
            return;
        }
            ids[size_ids++]=t.id;
        }
    }
    free(ids);
}

void list_hunt(const char* hunt_id){
    char hunt_path[MAX_PATH];
    snprintf(hunt_path, sizeof(hunt_path), "hunts/%s", hunt_id);

    struct stat st={0};
    if(stat(hunt_path,&st)==-1){
        printf("No such directory\n");
        return;
    }

    char *timestamp = ctime(&st.st_mtim.tv_sec);
    if (timestamp == NULL) {
        printf("Error converting time\n");
        return;
    }

    char output_line[MAX_LINE];

    snprintf(output_line, sizeof(output_line), "%s  %ld bytes  [%.*s]  ", hunt_id,st.st_size,(int)(strlen(timestamp)-1),timestamp);

    //printf("%s  %ld bytes  ", hunt_id, st.st_size);
    //printf("[%.*s]  ", (int)(strlen(timestamp) - 1), timestamp);
    //printf("Treasures: ");

    printf("%sTreasures:\n",output_line);
    list_treasures(hunt_id,strlen(output_line));
    
}


int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Usage:\n");
        printf("%s add <hunt_id>  -> add treasure to hunt\n", argv[0]);
        return 1;
    }

    if (strcmp(argv[1], "--add") == 0) {
        if (argc == 3) {
            // Add treasure
            add_treasure(argv[2]);
            
        } else {
            printf("Invalid number of arguments for the 'add' option.\n");
            return 1;
        }
    } else{ if(strcmp(argv[1],"--list")==0){
                list_hunt(argv[2]);
            }
            else
            {
                printf("Unknown option '%s'.\n", argv[1]);
                return 1;
            }
    }

    return 0;
}