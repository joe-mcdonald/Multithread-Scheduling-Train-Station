#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/time.h>

#define MAX_TRAINS 99
#define DECISECOND_CONVERSION 100000
#define PRI_LEN 5
#define DIR_LEN 5
#define STA_LEN 9
#define TIM_LEN 11
#define _POSIX_C_SOURCE 200809L
#define LAST_TRAINS_COUNT 3
int last_train_directions[LAST_TRAINS_COUNT] = {-1, -1, -1};

typedef enum Priority { LOW, HIGH } Priority;
typedef enum Direction { WEST, EAST } Direction;
typedef enum State { UNLOADED, LOADING, READY, CROSSING, GONE } State;

typedef struct Train {
    pthread_t thread;
    pthread_cond_t granted;
    int id;
    int load_time;
    int cross_time;
    Priority priority;
    Direction direction;
    State state;
} Train;

Train trains[MAX_TRAINS];
int train_count = 0;
Direction last_direction = WEST;
int track_occupied = 0;
int consecutive_same_direction = 0;
pthread_mutex_t track_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t track_available = PTHREAD_COND_INITIALIZER;
struct timeval start_time;

char* current_time_str() {
    static char time_str[12];
    struct timeval now, elapsed;
    gettimeofday(&now, NULL);
    timersub(&now, &start_time, &elapsed);
    int deciseconds = elapsed.tv_usec / 100000;
    int seconds = elapsed.tv_sec % 60;
    int minutes = (elapsed.tv_sec / 60) % 60;
    int hours = (elapsed.tv_sec / 3600) % 24;
    sprintf(time_str, "%02d:%02d:%02d.%1d", hours, minutes, seconds, deciseconds);
    return time_str;
}

Train* select_next_train() {
    Train *ready_trains[MAX_TRAINS];
    int count_ready = 0;

    for (int i = 0; i < train_count; i++) {
        if (trains[i].state == READY) {
            ready_trains[count_ready++] = &trains[i];
        }
    }

    if (count_ready == 0) {
        printf("NO READY TRAINS\n");
        return NULL;
    }

    Train *high_priority_train = NULL;
    Train *second_high_priority_train = NULL;
    Train *selected_train = NULL;

    for (int i = 0; i < count_ready; i++) {
        Train *current_train = ready_trains[i];
        if (current_train->priority == HIGH) {
            if (!high_priority_train) {
                high_priority_train = current_train;
            } else if (!second_high_priority_train) {
                second_high_priority_train = current_train;
                break;
            }
        }
    }

    if (high_priority_train && !second_high_priority_train) {
        selected_train = high_priority_train;
    } else if (high_priority_train && second_high_priority_train) {
        if (high_priority_train->direction == second_high_priority_train->direction) {
            selected_train = (high_priority_train->load_time < second_high_priority_train->load_time) ? high_priority_train : second_high_priority_train;
            if (high_priority_train->load_time == second_high_priority_train->load_time) {
                selected_train = (high_priority_train->id < second_high_priority_train->id) ? high_priority_train : second_high_priority_train;
            }
        } else {
            selected_train = (last_direction != high_priority_train->direction) ? high_priority_train : second_high_priority_train;
            if (last_direction == -1) {
                selected_train = (high_priority_train->direction == WEST) ? high_priority_train : second_high_priority_train;
            }
        }
    }

    if (last_train_directions[0] == last_train_directions[1] && last_train_directions[1] == last_train_directions[2] && selected_train && selected_train->direction == last_train_directions[0]) {
        for (int i = 0; i < count_ready; i++) {
            if (ready_trains[i]->direction != last_train_directions[0]) {
                selected_train = ready_trains[i];
                break;
            }
        }
    }

    if (!selected_train) {
        Train *low_priority_train = NULL;
        Train *second_low_priority_train = NULL;
        for (int i = 0; i < count_ready; i++) {
            Train *current_train = ready_trains[i];
            if (current_train->priority == LOW) {
                if (!low_priority_train) {
                    low_priority_train = current_train;
                } else if (!second_low_priority_train) {
                    second_low_priority_train = current_train;
                    break;
                }
            }
        }

        if (low_priority_train && !second_low_priority_train) {
            selected_train = low_priority_train;
        } else if (low_priority_train && second_low_priority_train) {
            if (low_priority_train->direction == second_low_priority_train->direction) {
                selected_train = (low_priority_train->load_time < second_low_priority_train->load_time) ? low_priority_train : second_low_priority_train;
                if (low_priority_train->load_time == second_low_priority_train->load_time) {
                    selected_train = (low_priority_train->id < second_low_priority_train->id) ? low_priority_train : second_low_priority_train;
                }
            } else {
                selected_train = (last_direction != low_priority_train->direction) ? low_priority_train : second_low_priority_train;
                if (last_direction == -1) {
                    selected_train = (low_priority_train->direction == WEST) ? low_priority_train : second_low_priority_train;
                }
            }
        }
    }
    return selected_train;
}

void *train_behavior(void *arg) {
    Train *train = (Train *)arg;
    train->state = LOADING;
    usleep(train->load_time * DECISECOND_CONVERSION);
    train->state = READY;
    printf("%s Train %2d is ready to go %s\n", current_time_str(), train->id, train->direction == EAST ? "East" : "West");

    int ret;
    pthread_mutex_lock(&track_mutex);
    // ret = pthread_mutex_lock(&track_mutex);
    // if (ret != 0) {
    //     fprintf(stderr, "Error locking mutex: %s\n", strerror(ret));
    //     exit(EXIT_FAILURE);
    // }

    while (train != select_next_train()) {
        pthread_cond_wait(&track_available, &track_mutex);
        // ret = pthread_cond_wait(&track_available, &track_mutex);
        // if (ret != 0) {
        //     fprintf(stderr, "Error with waiting: %s\n", strerror(ret));
        //     exit(EXIT_FAILURE);
        // }
    }

    if (train->direction == last_direction) {
        consecutive_same_direction++;
    } else {
        consecutive_same_direction = 1;
        last_direction = train->direction;
    }

    train->state = CROSSING;
    printf("%s Train %2d is ON the main track going %s\n", current_time_str(), train->id, train->direction == EAST ? "East" : "West");

    usleep(train->cross_time * DECISECOND_CONVERSION);

    train->state = GONE;
    printf("%s Train %2d is OFF the main track after going %s\n", current_time_str(), train->id, train->direction == EAST ? "East" : "West");

    pthread_cond_broadcast(&track_available);
    // ret = pthread_cond_broadcast(&track_available);
    // if (ret != 0) {
    //     fprintf(stderr, "Error with signaling: %s\n", strerror(ret));
    //     exit(EXIT_FAILURE);
    // }
    pthread_mutex_unlock(&track_mutex);
    // ret = pthread_mutex_unlock(&track_mutex);
    // if (ret != 0) {
    //     fprintf(stderr, "Error with unlocking track mutex: %s\n", strerror(ret));
    //     exit(EXIT_FAILURE);
    // }

    return NULL;
}

int main(int argc, char*argv[]) {
    gettimeofday(&start_time, NULL);
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <input_file>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    FILE *file = fopen(argv[1], "r");
    if (!file) {
        perror("Error opening file\n");
        exit(EXIT_FAILURE);
    }
    int ret;
    char direction;
    int priority, load_time, cross_time;
    while (fscanf(file, "%c %d %d\n", &direction, &load_time, &cross_time) == 3) {
        trains[train_count].id = train_count;
        trains[train_count].direction = (direction == 'E' || direction == 'e') ? EAST : WEST;
        trains[train_count].priority = (direction == 'E' || direction == 'W') ? HIGH : LOW;
        trains[train_count].load_time = load_time;
        trains[train_count].cross_time = cross_time;
        trains[train_count].state = UNLOADED;
        pthread_create(&trains[train_count].thread, NULL, train_behavior, &trains[train_count]);
        // ret = pthread_create(&trains[train_count].thread, NULL, train_behavior, &trains[train_count]);
        // if (ret != 0) {
        //     fprintf(stderr, "Error creating threads: %s\n", strerror(ret));
        //     exit(EXIT_FAILURE);
        // }
        train_count++;
    }
    fclose(file);

    for (int i = 0; i < train_count; i++) {
        pthread_join(trains[i].thread, NULL);
        // ret = pthread_join(trains[i].thread, NULL);
        // if (ret != 0) {
        //     fprintf(stderr, "Error joining threads: %s\n", strerror(ret));
        //     exit(EXIT_FAILURE);
        // }
    }
    return 0;
}