#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include "treasure.h"

typedef struct {
    char username[MAX_USERNAME];
    int score;
} user_score;

#define MAX_USERS 100

int main(int argc, char* argv[]) {
   
    int fd = open(argv[1], O_RDONLY);
    if (fd < 0) {
        perror("open");
        return 1;
    }

    treasure t;
    //user_score scores[MAX_USERS];
    user_score* scores=(user_score*)malloc(sizeof(user_score));

    if(!scores){
        perror("malloc failed");
        return 1;
    }

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

            user_score* tmp=(user_score*)realloc(scores,sizeof(user_score)*(num_users+1));
            if(!tmp){
                perror("realloc failed");
                free(scores);
                return 1;
            }

            scores=tmp;

            strncpy(scores[num_users].username, t.username, MAX_USERNAME);
            scores[num_users].score = t.value;
            num_users++;
        }
    }

    close(fd);

    for (int i = 0; i < num_users; ++i) {
        printf("%s: %d\n", scores[i].username, scores[i].score);
    }

    free(scores);

    return 0;
}
