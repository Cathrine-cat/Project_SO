// score_calculator.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#define MAX_PATH 256 //max size of file path
#define MAX_USERNAME 40
#define MAX_CLUE 100
#define HUNTS_DIR "hunts" //name of hunts directory
#define TREASURE_FILE "treasure.bin" //name of treasure file in each hunt

typedef struct _treasure {
    int id;
    char username[MAX_USERNAME];
    struct gps_coord {
        float latitude;
        float longitude;
    } gps;
    char clue[MAX_CLUE];
    int value;
} treasure;

typedef struct {
    char username[MAX_USERNAME];
    int score;
} user_score;

#define MAX_USERS 100

int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <hunt_id>\n", argv[0]);
        return 1;
    }

    char filepath[MAX_PATH];
    snprintf(filepath, sizeof(filepath), "%s/%s/%s", HUNTS_DIR,argv[1],TREASURE_FILE);
    int fd = open(filepath, O_RDONLY);
    if (fd < 0) {
        perror("open");
        return 1;
    }

    treasure t;
    user_score scores[MAX_USERS];
    int num_users = 0;

    while (read(fd, &t, sizeof(t)) == sizeof(t)) {
        int found = 0;
        for (int i = 0; i < num_users; ++i) {
            if (strcmp(scores[i].username, t.username) == 0) {
                scores[i].score += t.value;
                found = 1;
                break;
            }
        }
        if (!found && num_users < MAX_USERS) {
            strncpy(scores[num_users].username, t.username, MAX_USERNAME);
            scores[num_users].score = t.value;
            num_users++;
        }
    }

    close(fd);

    for (int i = 0; i < num_users; ++i) {
        printf("%s: %d\n", scores[i].username, scores[i].score);
    }

    return 0;
}
