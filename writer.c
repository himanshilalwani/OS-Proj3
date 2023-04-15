#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <semaphore.h>
#include <fcntl.h>
#include <time.h>

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
    int delay_time;
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
            delay_time = atoi(optarg);
            break;
        case 's':
            key2 = atoi(optarg);
            break;
        default:
            printf("Usage: %s -f filename -l recid -d time -s shmid\n", argv[0]);
            return 1;
        }
    }

    sem_t *wrt;
    wrt = sem_open("wrt", O_CREAT, 0666, 1);

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

    sem_wait(wrt);

    // initialize the grades array with 8 grades
    float grades[8] = {3.0, 2.5, 1.5, 4.0, 2.0, 3.5, 0.5, 1.0};
    srand(time(NULL));
    // randomly select one or more grades to modify
    int num_grades_to_modify = rand() % 8 + 1; // modify at least one course
    // initialize the random number generator
    srand(time(NULL));
    for (int i = 0; i < num_grades_to_modify; i++)
    {
        // pick a random value from the grades array
        int index = rand() % 8;
        float random_grade = grades[index];
        switch (i)
        {
        case 0:
            data[rec].g1 = random_grade;
            break;
        case 1:
            data[rec].g2 = random_grade;
            break;
        case 2:
            data[rec].g3 = random_grade;
            break;
        case 3:
            data[rec].g4 = random_grade;
            break;
        case 4:
            data[rec].g5 = random_grade;
            break;
        case 5:
            data[rec].g6 = random_grade;
            break;
        case 6:
            data[rec].g7 = random_grade;
            break;
        case 7:
            data[rec].g8 = random_grade;
            break;
        }
    }

    // modify the gpa based on new grades
    data[rec].GPA = (data[rec].g1 + data[rec].g2 + data[rec].g3 + data[rec].g4 + data[rec].g5 + data[rec].g6 + data[rec].g7 + data[rec].g8) / 8.0;

    // open the students.csv file for writing
    FILE *fp = fopen("students.csv", "w");
    if (fp == NULL)
    {
        printf("Failed to open file\n");
        return 1;
    }

    int num_students = 0;
    while (data[num_students].studentID != 0)
    {
        num_students++;
    }

    // loop through the array of student structs and write each struct to the file
    for (int i = 0; i < num_students; i++)
    {
        fprintf(fp, "%d,", data[i].studentID);
        fprintf(fp, "%s,", data[i].lastName);
        fprintf(fp, "%s,", data[i].firstName);
        fprintf(fp, "%.1f,", data[i].g1);
        fprintf(fp, "%.1f,", data[i].g2);
        fprintf(fp, "%.1f,", data[i].g3);
        fprintf(fp, "%.1f,", data[i].g4);
        fprintf(fp, "%.1f,", data[i].g5);
        fprintf(fp, "%.1f,", data[i].g6);
        fprintf(fp, "%.1f,", data[i].g7);
        fprintf(fp, "%.1f,", data[i].g8);
        fprintf(fp, "%.1f\n", data[i].GPA);
    }

    // Close file
    fclose(fp);

    sleep(delay_time);

    sem_post(wrt);

    sem_close(wrt);
    sem_unlink("wrt");
}