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
    int g1;
    int g2;
    int g3;
    int g4;
    int g5;
    int g6;
    int g7;
    int g8;
    int GPA;
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
        printf("Usage: %s -f filename -l recid[,recid] -d time -s shmid\n", argv[0]);
        return 1;
    }

    char *filename;
    char *recid_str;
    int time;
    int shmid;
    int opt;
    while ((opt = getopt(argc, argv, "f:l:d:s:")) != -1)
    {
        switch (opt)
        {
        case 'f':
            filename = optarg;
            break;
        case 'l':
            recid_str = optarg;
            break;
        case 'd':
            time = atoi(optarg);
            break;
        case 's':
            shmid = atoi(optarg);
            break;
        default:
            printf("Usage: %s -f filename -l recid[,recid] -d time -s shmid\n", argv[0]);
            return 1;
        }
    }

    // parse record IDs to lookup
    int recid_list[100];
    int recid_count = 0;
    char *token = strtok(recid_str, ",");
    while (token != NULL)
    {
        recid_list[recid_count++] = atoi(token);

        token = strtok(NULL, ",");
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

    int shmid2;
    key_t key2 = 9999;

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