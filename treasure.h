#ifndef TREASURE_H
#define TREASURE_H

#define MAX_USERNAME 40
#define MAX_CLUE 100
#define MAX_PATH 256 //max size of file path
#define HUNTS_DIR "hunts" //name of hunts directory
#define TREASURE_FILE "treasure.bin" //name of treasure file in each hunt
#define HUNT_DIR_NAME "hunt"//pattern for hunt directory
#define LOG_NAME "logged_hunt"//name pattern of log-symlink


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

#endif 