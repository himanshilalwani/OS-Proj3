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
        printf("Usage: %s -f filename -l recid[,recid] -d time -s shmid\n", argv[0]);
        return 1;
    }

    char *filename;
    char *recid_str;
    int time;
    int shmid2;
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
            shmid2 = atoi(optarg);
            break;
        default:
            printf("Usage: %s -f filename -l recid[,recid] -d time -s shmid\n", argv[0]);
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

    // parse record IDs to lookup
    int recid_list[100];
    int recid_count = 0;
    char *token = strtok(recid_str, ",");
    while (token != NULL)
    {
        recid_list[recid_count++] = atoi(token);

        token = strtok(NULL, ",");
    }
    printf("sleeping for %d seconds\n", time);
    sleep(time);
    // read the lines in recid_list
    for (int i = 0; i < recid_count; i++)
    {
        printf("Student ID: %d\n", data[recid_list[i]].studentID);
        printf("First Name: %s\n", data[recid_list[i]].firstName);
        printf("Last Name: %s\n", data[recid_list[i]].lastName);
        printf("Grade 1: %f\n", data[recid_list[i]].g1);
        printf("Grade 2: %f\n", data[recid_list[i]].g2);
        printf("Grade 3: %f\n", data[recid_list[i]].g3);
        printf("Grade 4: %f\n", data[recid_list[i]].g4);
        printf("Grade 5: %f\n", data[recid_list[i]].g5);
        printf("Grade 6: %f\n", data[recid_list[i]].g6);
        printf("Grade 7: %f\n", data[recid_list[i]].g7);
        printf("Grade 8: %f\n", data[recid_list[i]].g8);
        printf("GPA: %f\n", data[recid_list[i]].GPA);
        printf("=================================\n");
    }
}