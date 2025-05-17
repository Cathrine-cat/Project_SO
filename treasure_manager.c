#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <dirent.h>
#include "treasure.h"

#define MAX_MSG 250
#define MAX_STR_LEN 100 //max size of read input from keyboard
#define MAX_LINE 100 //max length of line in log

/*
====================================================================================================================================================
                                CREATE HUNTS DIRECTORY
====================================================================================================================================================
*/

void create_hunts(){
    struct stat st = {0};
    if (stat(HUNTS_DIR, &st) == -1) {//if hunts directory doesn't exist
        if(mkdir(HUNTS_DIR, 0700)){//try to create it 
            printf("Error creating hunts dir\n");
            exit(9);
        }
    }
}


void not_valid_hunt_id(){
    printf("That is not a valid hunt_id\nA valid hunt_id is of the form: %s[natural number]\n",HUNT_DIR_NAME);
    printf("Please enter a valid hunt_id or an existing one\n");
}


//Check if hunt_id is valid. 
//Decided form of hide_id : hunt[number]. Example:hunt1, hunt2...
int valid_hunt_id(const char* hunt_id){
    const char* prefix=HUNT_DIR_NAME;
    
    if(strncmp(hunt_id,prefix,strlen(HUNT_DIR_NAME)!=0)){
        not_valid_hunt_id();
        return 0;
    }

    const char* digits=hunt_id+strlen(HUNT_DIR_NAME);
    if(*digits=='\0'){
        not_valid_hunt_id();
        return 0;
    }

    while(*digits){
        if(!isdigit((unsigned char)*digits)){
            not_valid_hunt_id();
            return 0;
        }
        digits++;
    }

    return 1;//valid hunt_id
}


//Continue or cancel action
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





/*
====================================================================================================================================================
                                LOG OPERATION
====================================================================================================================================================
*/

void log_operation(const char* hunt_id, const char* message){
    char path[256];
    if(hunt_id!=NULL){
        snprintf(path, sizeof(path), "%s/%s/%s.logs",HUNTS_DIR,hunt_id,hunt_id);
    }
    else{
        snprintf(path, sizeof(path), "%s/general.logs",HUNTS_DIR);
    }
    int fd = open(path, O_WRONLY | O_APPEND | O_CREAT, 0644);//open log file
    if (fd < 0) {
        printf("Error opening log file\n");
        exit(1);
    }

    time_t now = time(NULL);
    char *timestamp = ctime(&now);
    dprintf(fd, "[%.*s] %s\n", (int)(strlen(timestamp) - 1), timestamp, message);
    close(fd);


    if(hunt_id!=NULL){
        //create symlink
        char symlink_name[MAX_PATH];
        snprintf(symlink_name, sizeof(symlink_name), "%s-%s",LOG_NAME,hunt_id);

        struct stat st={0};
        if(lstat(symlink_name,&st)==-1){//symlink does not yet exist 
            if(symlink(path, symlink_name)!=0){//create->if fail 
                printf("Error creating symbolic link\n");//fail message
                exit(9);
            }
        }
    }
   
}





/*
====================================================================================================================================================
                                CREATE HUNT FROM GIVEN HUNT_ID
====================================================================================================================================================
*/

void create_hunt(const char* hunt_id) {
    create_hunts();//create hunts directory if not exists
    if(valid_hunt_id(hunt_id)){
        char hunt_path[MAX_PATH];
        snprintf(hunt_path, sizeof(hunt_path), "%s/%s",HUNTS_DIR,hunt_id);

        struct stat st={0};
        if(stat(hunt_path,&st)==-1){//if hunt directory doesnt exist
            printf("A hunt with that id doesn't exist yet. Would you like to create it?\n");
            if(proceed()){
                if (mkdir(hunt_path, 0700)){
                    printf("Error creating hunt directory\n");
                    exit(9);
                }
                else{
                    char log_msg[MAX_MSG];
                    snprintf(log_msg, sizeof(log_msg), "Created hunt %s", hunt_id);
                    log_operation(NULL,log_msg);//general log
                }
            }
            else{
                exit(0);//user decided to not create the directory
            }
        }
    }
    else exit(0);//not a valid hunt_id
 
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


//get the ID of the next treasure to be added to file
int get_treasure_id(int fd){

    off_t file_size=lseek(fd,0,SEEK_END);
    if(file_size==0){
        //empty file
        return 1;//first treasure id should be 1
    }

    off_t offset=file_size-sizeof(treasure);//get to the last treasure

    if(lseek(fd,offset,SEEK_SET)==-1){
        printf("Error with searching file\n");
        close(fd);
        exit(8);
    }

    treasure last;
    if(read(fd,&last,sizeof(treasure))!=sizeof(treasure)){
        printf("Error with reading file\n");
        close(fd);
        exit(6);
    }

    //retunr last treasure ID+1 so that treasures have consecutive IDs
    return last.id+1;
}

int get_treasure(treasure* t,int fd){

    char input[MAX_STR_LEN];
    int valid = 0;

    t->id=get_treasure_id(fd);

    printf("Treasure ID: %d\n",t->id);

    printf("Please enter the following data or press Q to quit\n");

   
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






/*
====================================================================================================================================================
                                ADD  TREASURE
====================================================================================================================================================
*/

void add_treasure(const char* hunt_id) {
    create_hunt(hunt_id);//create hunt to add treasure to if not exists

    char treasure_path[MAX_PATH];
    snprintf(treasure_path, sizeof(treasure_path), "%s/%s/%s",HUNTS_DIR,hunt_id,TREASURE_FILE);


    int fd = open(treasure_path, O_CREAT | O_RDWR | O_APPEND, 0644);
    if (fd<0) {
        printf("Error opening treasure file\n");
        exit(1);
    }

    
    treasure* t=(treasure*)malloc(sizeof(treasure));

    if(get_treasure(t,fd)){
        printf("Aborted action\n");
        free(t);
        close(fd);
        return;
    }

    printf("\nThe entered data is:\n");
    printf("ID: %d\n",t->id);
    printf("Username: %s\n",t->username);
    printf("Gps coordinates: %f latitude; %f longitude\n",t->gps.latitude,t->gps.longitude);
    printf("Clue: %s\n",t->clue);
    printf("Value: %d\n",t->value);

    if(proceed()!=1){
        free(t);
        close(fd);
        printf("Aboarted action\n");
        return;
    }
    
    int id=t->id;

    if(write(fd,t,sizeof(treasure))!=sizeof(treasure)){
        free(t);
        close(fd);
        printf("Error writing in file\n");
        exit(5);
    }
    else{
        printf("Successfully added treasure\n");
        free(t);
    }

    close(fd);

    char log_msg[MAX_MSG];
    snprintf(log_msg, sizeof(log_msg), "Added treasure ID %d to hunt %s", id, hunt_id);
    log_operation(hunt_id, log_msg);//normal log
    log_operation(NULL,log_msg);//general log

}





/*
====================================================================================================================================================
                                LIST  TREASURE
====================================================================================================================================================
*/


void list_treasures(const char* hunt_id,int index){
    char treasure_path[MAX_PATH];
    snprintf(treasure_path, sizeof(treasure_path), "%s/%s/%s",HUNTS_DIR,hunt_id,TREASURE_FILE);

    int fd=open(treasure_path,O_RDONLY);
    if(fd<0){
        printf("Error opening file\n");
        exit(1);
    }

    treasure t;

    while(read(fd,&t,sizeof(treasure))==sizeof(treasure)){

        printf("%*s", index, "");
        printf("Treasure ID: %d\n",t.id);

    }
   
}



/*
====================================================================================================================================================
                                LIST  HUNT
====================================================================================================================================================
*/

long int calculate_size(const char* hunt_path){

    long int total_size=0;

    DIR* directory=opendir(hunt_path);

    if(directory==NULL){
        printf("Error opening hunt directory\n");
        exit(2);
    }

    struct dirent* entry;
    char file_path[MAX_PATH*2];
    struct stat info;

    while((entry=readdir(directory))){
        if(strcmp(entry->d_name,".")==0 || strcmp(entry->d_name,"..")==0){
            continue;
        }

        snprintf(file_path,sizeof(file_path),"%s/%s",hunt_path,entry->d_name);

        if(lstat(file_path,&info)<0){
            printf("Error with lstat\n");
            exit(10);
        }

        if(S_ISDIR(info.st_mode)){
            total_size=total_size+calculate_size(file_path);
        }
        else{
            total_size=total_size+info.st_size;
        }

    }

    closedir(directory);
    return total_size;
}

void list_hunt(const char* hunt_id){
    char hunt_path[MAX_PATH];
    snprintf(hunt_path, sizeof(hunt_path), "%s/%s",HUNTS_DIR,hunt_id);

    struct stat st={0};
    if(stat(hunt_path,&st)==-1){
        printf("No such directory\n");
        exit(0);
    }

    char *timestamp = ctime(&st.st_mtim.tv_sec);
    if (timestamp == NULL) {
        printf("Error converting time\n");
        exit(7);
    }

    char output_line[MAX_LINE];

    long int dir_size=calculate_size(hunt_path);

    snprintf(output_line, sizeof(output_line), "%s  %ld bytes  [%.*s]  ", hunt_id,dir_size,(int)(strlen(timestamp)-1),timestamp);

    printf("%sTreasures:\n",output_line);
    list_treasures(hunt_id,strlen(output_line));

    char log_msg[MAX_MSG];
    snprintf(log_msg, sizeof(log_msg), "Listed  hunt %s",hunt_id);
    log_operation(hunt_id, log_msg);//normal log
    log_operation(NULL,log_msg);//general log
}






/*
====================================================================================================================================================
                                VIEW TREASURE
====================================================================================================================================================
*/

void view_treasure(const char* hunt_id, const char* id){
    char hunt_path[MAX_PATH];
    snprintf(hunt_path, sizeof(hunt_path), "%s/%s",HUNTS_DIR,hunt_id);

    struct stat st={0};
    if(stat(hunt_path,&st)==-1){
        printf("No such directory\n");
        exit(0);
    }

    char treasure_path[MAX_PATH];
    snprintf(treasure_path, sizeof(treasure_path), "%s/%s/%s",HUNTS_DIR,hunt_id,TREASURE_FILE);

    int fd=open(treasure_path,O_RDONLY);
    if(fd<0){
        printf("Error opening file\n");
        exit(1);
    }

    int ID;
    if(is_valid_natural_number((char*)id)){
        ID=atoi(id);//convert to number
    }
    else{
        printf("The treasure id entered is not valid.\nPlease enter a valid treasure id (a natural number)\n");
        exit(0);
    }

    treasure t;
    int view=0;
    while(read(fd,&t,sizeof(treasure))==sizeof(treasure)){
        if(t.id==ID){
            printf("Treasure ID: %d; username: %s; GPS coordinates: latitude: %f, longitude %f; Clue: %s; Value: %d\n",t.id,t.username,t.gps.latitude,t.gps.longitude,t.clue,t.value);
            view=1;
            break;
        }   
    }

    char log_msg[MAX_MSG];
    if(view){//found treasure with that ID and viewed it
        snprintf(log_msg, sizeof(log_msg), "Viewed  treasure ID %s from hunt %s",id,hunt_id);
        log_operation(hunt_id, log_msg);
       
    }
    else{
        printf("Treasure ID %s does not exist in hunt %s\n",id,hunt_id);
    }

   
}







/*
====================================================================================================================================================
                                REMOVE  HUNT
====================================================================================================================================================
*/

void remove_hunt(const char* hunt_id){
    char hunt_path[MAX_PATH];
    snprintf(hunt_path, sizeof(hunt_path), "%s/%s",HUNTS_DIR,hunt_id);

    struct stat st={0};
    if(stat(hunt_path,&st)==-1){
        printf("No such directory\n");
        return;
    }

    DIR* directory=opendir(hunt_path);

    if(directory==NULL){
        printf("Error opening hunt directory\n");
        exit(2);
    }

    struct dirent* entry;
    char file_path[MAX_PATH*2];

    while((entry=readdir(directory))){
        if(strcmp(entry->d_name,".")==0 || strcmp(entry->d_name,"..")==0){
            continue;
        }

        snprintf(file_path,sizeof(file_path),"%s/%s",hunt_path,entry->d_name);

        if(unlink(file_path)){
            printf("Failed to remove file from hunt\n");
            printf("%s\n",file_path);
            closedir(directory);
            exit(3);
        }
    }

    closedir(directory);

    if(rmdir(hunt_path)){
        printf("Failed to remove hunt\n");
        exit(4);
    }

    char sym_link[MAX_PATH];
    snprintf(sym_link,sizeof(sym_link),"%s-%s",LOG_NAME,hunt_id);

    if (lstat(sym_link, &st) != -1) {
        if (S_ISLNK(st.st_mode)) { 
        if(unlink(sym_link)){
            printf("Failed to remove symbolic link\n");
            exit(3);
        }
    }
       
    }
    char log_msg[MAX_MSG];
    snprintf(log_msg, sizeof(log_msg), "Removed hunt %s",hunt_id);
    log_operation(NULL,log_msg);//general log
    printf("Hunt %s removed successfully\n",hunt_id);
    
}







/*
====================================================================================================================================================
                                REMOVE  TREASURE
====================================================================================================================================================
*/


void remove_treasure(const char* hunt_id, char* treasure_id){
    int id;
    //Check if typed ID is valid
    //Treasure IDs are natural numbers starting from 1
   if(is_valid_natural_number(treasure_id)){
        id=atoi(treasure_id);//convert to number
   }
   else{
        printf("The treasure id entered is not valid.\nPlease enter a valid treasure id (a natural number)\n");
        exit(0);
   }
   char treasure_path[MAX_PATH];
   snprintf(treasure_path,sizeof(treasure_path),"%s/%s/%s",HUNTS_DIR,hunt_id,TREASURE_FILE);

   int fd=open(treasure_path,O_RDWR);
   if(fd<0){
        printf("Error opening treasure file\n");
        exit(1);
   }

   treasure t;
   off_t read_pos=0;
   off_t write_pos=0;
   int found=0;
   int new_id=1;

   while(read(fd,&t,sizeof(treasure))==sizeof(treasure)){
        if(t.id==id && !found){//found treasure
            found=1;
            write_pos=read_pos;
        }
        else{
            if (found) {
                t.id = new_id++;
                lseek(fd, write_pos, SEEK_SET);
                write(fd, &t, sizeof(treasure));
                write_pos += sizeof(treasure);
            } 
            else{
                t.id=new_id++;
            }
        }
        read_pos=read_pos+sizeof(treasure);
        lseek(fd,read_pos,SEEK_SET);
   }

   if(found){
        ftruncate(fd,write_pos);//shorten file
        char log_msg[MAX_MSG];
        snprintf(log_msg, sizeof(log_msg), "Removed treasure with ID %d from hunt %s", id, hunt_id);
        //log operarion
        log_operation(hunt_id,log_msg);
        log_operation(NULL,log_msg);
        printf("Removed treasure successfully\n");
   }
   else{
        printf("Treasure with id %d not found\n",id);
   }

   close(fd);

}







/*
====================================================================================================================================================
                                MAIN
====================================================================================================================================================
*/

//Usage o program
void print_usage(const char* prg_name){
    printf("Usage:\n");
    printf("%s -add <hunt_id>\n",prg_name);
    printf("%s -list <hunt_id>\n",prg_name);
    printf("%s -view <hunt_id> <id>\n",prg_name);
    printf("%s -remove_treasure <hunt_id> <id>\n",prg_name);
    printf("%s -remove_hunt <hunt_id>\n",prg_name);
}

//Handle options of program
int main(int argc, char *argv[]) {

    if (argc < 3) {
        print_usage(argv[0]);
        return 1;
    }

    if (strcmp(argv[1], "-add") == 0) {
        if(argc==3){
            add_treasure(argv[2]);
        }
        else{
            print_usage(argv[0]);
            return 1;
        }
     
    } else if(strcmp(argv[1],"-list")==0){
                if(argc==3)
                    list_hunt(argv[2]);
                else {
                    print_usage(argv[0]);
                    return 1;
                }
            }else if(strcmp(argv[1],"-view")==0)
                    {
                        if (argc == 4) {
                            view_treasure(argv[2],argv[3]);  
                        } else {
                            print_usage(argv[0]);
                            return 1;
                        }
                    }
                    else if(strcmp(argv[1],"-remove_treasure")==0)
                        {
                        if (argc == 4) {
                            remove_treasure(argv[2],argv[3]);  
                        } else {
                            print_usage(argv[0]);
                           return 1;
                        }
                        }
                        else if(strcmp(argv[1],"-remove_hunt")==0){
                            if(argc==3)
                                remove_hunt(argv[2]);
                            else{
                                print_usage(argv[0]);
                                return 1;
                            }
                        }
                        else
                        {
                            printf("Unknown option '%s'.\n", argv[1]);
                            print_usage(argv[0]);
                            return 1;
                        }
    return 0;
}