#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <semaphore.h>
#include <fcntl.h>

#define SHMSIZE 1024

#define SEM_MUTEX "/mutex_sem"
#define SEM_WRITE "/write_sem"
#define SEM_READ "/read_sem"

typedef struct
{
    int studentID;
    char lastName[20];
    char firstName[20];
    float g1;
    float g2;
    float g3;
    float g4;
    float g5;
    float g6;
    float g7;
    float g8;
    float GPA;
} student;

// define the structure for the analytics
struct analytics
{
    int reader_count;         // number of readers processed
    int avg_duration_readers; // average duration of readers
    int writer_count;         // number of writes processed
    int avg_duration_writers; // average duration of writers
    int max_delay;            // maximum delay recorded for starting the work of either a reader or a writer.
    int num_records;          // number of records accessed / modified
};

int main(int argc, char *argv[])
{
    // parse command line arguments
    if (argc != 9)
    {
        printf("Usage: %s -f filename -l recid -d time -s shmid\n", argv[0]);
        return 1;
    }

    char *filename;
    int time;
    int key2;
    int opt;
    int rec;
    while ((opt = getopt(argc, argv, "f:l:d:s:")) != -1)
    {
        switch (opt)
        {
        case 'f':
            filename = optarg;
            break;
        case 'l':
            rec = atoi(optarg);
            break;
        case 'd':
            time = atoi(optarg);
            break;
        case 's':
            key2 = atoi(optarg);
            break;
        default:
            printf("Usage: %s -f filename -l recid -d time -s shmid\n", argv[0]);
            return 1;
        }
    }

    int shmid1;
    key_t key = 1234;

    // Locate the shared memory segment
    shmid1 = shmget(key, sizeof(struct analytics), 0666);

    // check for faiure (no segment found with that key)
    if (shmid1 < 0)
    {
        perror("shmget failure");
        exit(1);
    }

    // Attach the segment to the data space
    struct analytics *a = shmat(shmid1, NULL, 0);

    // check for failure
    if (a == (struct analytics *)-1)
    {
        perror("shmat failure");
        exit(1);
    }

    // key_t key2 = 9999;
    int shmid2;

    // Locate the shared memory segment
    shmid2 = shmget(key2, sizeof(student), 0666);

    // check for faiure (no segment found with that key)
    if (shmid2 < 0)
    {
        perror("shmget failure");
        exit(1);
    }

    // Attach the segment to the data space
    student *data = shmat(shmid2, NULL, 0);

    // check for failure
    if (data == (student *)-1)
    {
        perror("shmat failure");
        exit(1);
    }

    




}