#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <semaphore.h>
#include <fcntl.h>
#include <time.h>
#include "myRecordDef.h"
#include <sys/times.h>

#define SHMSIZE 1024

// define the structure for the analytics
typedef struct
{
    int reader_count;           // number of readers processed
    float avg_duration_readers; // average duration of readers
    int writer_count;           // number of writes processed
    float avg_duration_writers; // average duration of writers
    float max_delay;            // maximum delay recorded for starting the work of either a reader or a writer.
    int num_records;            // number of records accessed / modified
} analytics;

int main(int argc, char *argv[])
{
    // time values
    // delay calculates time elapsed when entering the critical section
    // duration calculates the total time a writer process takes
    double delay_1, delay_2, duration_1, duration_2, total_delay, total_duration;
    struct tms delay_b1, delay_b2, duration_b1, duration_b2;
    double ticspersec;
    ticspersec = (double)sysconf(_SC_CLK_TCK);
    delay_1 = (double)times(&delay_b1);
    duration_1 = (double)times(&duration_b1);

    // parse command line arguments
    if (argc != 11)
    {
        printf("Usage: %s -f filename -l recidx -d time -s shmid -n totalrecords\n", argv[0]);
        return 1;
    }

    // storing the command line arguments
    char *filename; // filename
    int delay_time; // sleep time
    int key2;       // shared memory segment key
    int opt;
    int rec;         // record idx to modify
    int data_length; // total records in the file

    while ((opt = getopt(argc, argv, "f:l:d:s:n:")) != -1)
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
        case 'n':
            data_length = atoi(optarg);
            break;
        default:
            printf("Usage: %s -f filename -l recidx -d time -s shmid -n totalrecords\n", argv[0]);
            return 1;
        }
    }

    // initialise semaphores
    sem_t *wrt; // for writing to the file one at a time to avoid data corruption
    wrt = sem_open("wrt", O_CREAT, 0666, 1);

    sem_t *analytics_sem = sem_open("analytic", O_CREAT, 0666, 1); // controlling the analytics struct

    sem_t *sem_array[data_length]; // array of semaphores . for each record in the file
    char sem_name[20];
    for (int i = 0; i < data_length; i++)
    {
        sprintf(sem_name, "/my_sem_%d", i);
        sem_array[i] = sem_open(sem_name, O_CREAT, 0644, 1);
        if (sem_array[i] == SEM_FAILED)
        {
            perror("sem_open");
            exit(1);
        }
    }

    // locate and attach shared memory segments
    int shmid1;
    key_t key = 1234;

    // Locate the shared memory segment
    shmid1 = shmget(key, sizeof(analytics), 0666);

    // check for faiure (no segment found with that key)
    if (shmid1 < 0)
    {
        perror("shmget failure");
        exit(1);
    }

    // Attach the segment to the data space
    analytics *a = shmat(shmid1, NULL, 0);

    // check for failure
    if (a == (analytics *)-1)
    {
        perror("shmat failure");
        exit(1);
    }

    int shmid2;

    // Locate the shared memory segment
    shmid2 = shmget(key2, sizeof(MyRecord), 0666);

    // check for faiure (no segment found with that key)
    if (shmid2 < 0)
    {
        perror("shmget failure");
        exit(1);
    }

    // Attach the segment to the data space
    MyRecord *data = shmat(shmid2, NULL, 0);

    // check for failure
    if (data == (MyRecord *)-1)
    {
        perror("shmat failure");
        exit(1);
    }

    // lock the record in use
    sem_wait(sem_array[rec]);
    // entered critical section, calculate delay time
    delay_2 = (double)times(&delay_b2);
    total_delay = ((delay_2 - delay_1) / ticspersec) * 1000;

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
        data[rec].Marks[index] = random_grade;
    }

    // modify the gpa based on new grades
    data[rec].GPA = 0;
    for (int i = 0; i < 8; i++)
    {
        data[rec].GPA += data[rec].Marks[i];
    }
    data[rec].GPA /= 8;

    sem_post(sem_array[rec]); // release the lock from the record

    // lock the file
    sem_wait(wrt);
    // open the file for writing
    FILE *fp = fopen(filename, "w");
    if (fp == NULL)
    {
        printf("Failed to open file\n");
        return 1;
    }

    fwrite(data, sizeof(MyRecord), data_length, fp); // write to the binary file

    fclose(fp); // Close file

    sem_post(wrt); // release the lock from the file

    sleep(delay_time); // sleep

    // calculate time as writer done modifying
    duration_2 = (double)times(&duration_b2);
    total_duration = ((duration_2 - duration_1) / ticspersec) * 1000;

    // modify the analytics struct
    sem_wait(analytics_sem);
    float current_avg;
    float new_avg;

    current_avg = a->avg_duration_writers;
    new_avg = ((current_avg * a->writer_count) + 1) / (a->writer_count + 1);
    a->writer_count += 1;
    a->avg_duration_writers = new_avg;

    if (total_delay > a->max_delay)
    {
        a->max_delay = total_delay;
    }
    a->num_records += 1;
    sem_post(analytics_sem);

    // opening the log file
    FILE *logfile = fopen("logfile.txt", "a");
    sem_t *log = sem_open("log", O_CREAT, 0644, 1);
    sem_wait(log);
    fprintf(logfile, "Writer Process with PID %d updated record %d\n", getpid(), rec);
    sem_post(log);

    // close the semaphhores
    sem_close(log);
    sem_close(wrt);

    for (int i = 0; i < data_length; i++)
    {
        if (sem_close(sem_array[i]) < 0)
        {
            perror("sem_close");
            exit(EXIT_FAILURE);
        }
    }
    sem_close(*sem_array);
    sem_close(analytics_sem);
}