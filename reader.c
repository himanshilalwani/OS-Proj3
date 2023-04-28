# Authors: Fatima Nadeem, Himanshi Lalwani
# OS Programming Assignment 3

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <semaphore.h>
#include <fcntl.h>
#include "myRecordDef.h"
#include <sys/times.h>

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
    // duration calculates the total time a reader process takes
    double delay_1, delay_2, duration_1, duration_2, total_delay, total_duration;
    struct tms delay_b1, delay_b2, duration_b1, duration_b2;
    double ticspersec;
    ticspersec = (double)sysconf(_SC_CLK_TCK);
    delay_1 = (double)times(&delay_b1);
    duration_1 = (double)times(&duration_b1);

    // parse command line arguments
    if (argc != 11)
    {
        printf("Usage: %s -f filename -l recidx1,recidx2 -d time -s shmid -n totalrecords\n", argv[0]);
        return 1;
    }

    // storing the command line arguments
    char *filename;  // filename
    char *recid_str; // record idxs (as comma separated strings)
    int time;        // sleep time
    int key2;        // shared memory key
    int opt;
    int data_length; // total records in the file
    while ((opt = getopt(argc, argv, "f:l:d:s:n:")) != -1)
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
            key2 = atoi(optarg);
            break;
        case 'n':
            data_length = atoi(optarg);
            break;
        default:
            printf("Usage: %s -f filename -l recidx1,recidx2 -d time -s shmid -n totalrecords\n", argv[0]);
            return 1;
        }
    }

    // copy comma separated record idxs for storing in the log file
    char *recid_str2 = malloc(strlen(recid_str) + 1); // Allocate memory for the copy
    if (recid_str2 == NULL)
    {
        // Handle error: unable to allocate memory
        return 1;
    }
    strcpy(recid_str2, recid_str);

    // initialize semaphores
    sem_t *mutex; // mututal exclusion readcount
    mutex = sem_open("mutex", O_CREAT, 0666, 1);

    sem_t *analytics_sem = sem_open("analytic", O_CREAT, 0666, 1); // mututal exclusion analytics

    // array of semaphores to lock all records if reader is reading
    sem_t *sem_array[data_length];
    char sem_name[20];
    for (int i = 0; i < data_length; i++)
    {
        sprintf(sem_name, "/my_sem_%d", i);
        sem_array[i] = sem_open(sem_name, O_CREAT, 0644, 1);
    }

    // semaphore to control the standard output, ensuring one reader prints at a time
    sem_t *output_sem;
    output_sem = sem_open("/output_sem", O_CREAT, 0644, 1);

    // controlling the log file
    sem_t *log = sem_open("log", O_CREAT, 0644, 1);

    // get the shared memory segment for readcount
    int shmid3;
    key_t key3 = 5555;

    shmid3 = shmget(key3, sizeof(int), 0666);

    // check for faiure (no segment found with that key)
    if (shmid3 < 0)
    {
        perror("shmget failure");
        exit(1);
    }

    // Attach the segment to the data space
    int *readcount = shmat(shmid3, NULL, 0);

    // get the shared memory segment for analytics
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

    // Locate the shared memory segment for records
    int shmid2;

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

    // if semaphore's value > 0, resource is AVAILABLE & decrement value by 1
    // if semaphore current value == 0 , call BLOCKS (process waits) till value > 0
    sem_wait(mutex);
    *readcount += 1;
    if (*readcount == 1)
    {
        for (int i = 0; i < data_length; i++)
        {
            sem_wait(sem_array[i]);
        }
    }
    sem_post(mutex);

    // record the delay as we have entered the critical section
    delay_2 = (double)times(&delay_b2);
    total_delay = ((delay_2 - delay_1) / ticspersec) * 1000;

    // parse record IDs to lookup by converting the comma separated string to a list of rec idxs
    int recid_list[100];
    int recid_count = 0;

    char *token = strtok(recid_str, ",");

    while (token != NULL)
    {
        recid_list[recid_count++] = atoi(token);
        token = strtok(NULL, ",");
    }

    // sleep for a bit
    sleep(time);

    // print the records on the standard outpit
    sem_wait(output_sem);
    // read the lines in recid_list
    for (int i = 0; i < recid_count; i++)
    {
        printf("ID: %ld\n", data[recid_list[i]].custid);
        printf("Last Name: %s\n", data[recid_list[i]].LastName);
        printf("First Name: %s\n", data[recid_list[i]].FirstName);
        printf("Grades: ");
        for (int j = 0; j < 8; j++)
        {
            printf("%4.2f ", data[recid_list[i]].Marks[j]);
        }
        printf("\n");
        printf("GPA: %4.2f\n", data[recid_list[i]].GPA);
        printf("=================================\n");
    }
    sem_post(output_sem);

    // reduce readcount and allow the writers to read if no readers
    sem_wait(mutex);
    *readcount -= 1;
    if (*readcount == 1)
    {

        for (int i = 0; i < data_length; i++)
        {
            sem_post(sem_array[i]);
        }
    }
    sem_post(mutex);

    // reading process finished, calculate the total duration
    duration_2 = (double)times(&duration_b2);
    total_duration = ((duration_2 - duration_1) / ticspersec) * 1000;

    // record the analytics
    sem_wait(analytics_sem);
    float current_avg;
    float new_avg;
    current_avg = a->avg_duration_readers;
    new_avg = ((current_avg * a->reader_count) + 1) / (a->reader_count + 1);

    a->reader_count += 1;
    a->avg_duration_readers = new_avg;

    if (total_delay > a->max_delay)
    {
        a->max_delay = total_delay;
    }
    a->num_records += recid_count;
    sem_post(analytics_sem);

    // creating the log file
    FILE *logfile = fopen("logfile.txt", "a");

    sem_wait(log);
    fprintf(logfile, "Reader Process with PID %d read records %s\n", getpid(), recid_str2);
    sem_post(log);

    // close the semaphores
    sem_close(log);
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
    sem_close(mutex);
    sem_close(output_sem);

    return 0;
}
