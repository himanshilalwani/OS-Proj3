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
#include <time.h>
#include "myRecordDef.h"

#define MAX_RECORD_PER_READER 5    // this is maximum number of records a reader can read at a time
#define MAX_SLEEP_TIME 3           // maximum time a reader / writer can sleep
#define FILENAME "Dataset-500.bin" // name of the file to work with
#define NUM_FORKS 15               // number of forks to create to spawn reader / writer processes

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

int main()
{
    // Create shared memory segment 1 with ID 1234 for analytics
    int shmid1;
    key_t key1 = 1234;
    analytics *a;

    if ((shmid1 = shmget(key1, sizeof(analytics), IPC_CREAT | 0666)) == -1)
    {
        perror("Failed to create shared memory1");
        exit(EXIT_FAILURE);
    }

    // Attach shared memory segment
    if ((a = shmat(shmid1, NULL, 0)) == (void *)-1)
    {
        perror("Failed to attach shared memory1");
        exit(EXIT_FAILURE);
    }

    // shared memory segment for read count details
    int shmid3;
    key_t key3 = 5555;
    int *readcount;
    // Create shared memory segment 2 with ID 5555
    if ((shmid3 = shmget(key3, sizeof(readcount), IPC_CREAT | 0666)) == -1)
    {
        perror("Failed to create shared memory3");
        exit(EXIT_FAILURE);
    }

    // // Attach shared memory segment
    if ((readcount = (int *)shmat(shmid3, NULL, 0)) == (int *)-1)
    {
        perror("Failed to attach shared memory3");
        exit(EXIT_FAILURE);
    }
    // *readcount = 0;

    // Read student details from BIN file
    FILE *fp;
    int n = 0; // number of students in the file
    MyRecord *student;

    // open the file
    fp = fopen(FILENAME, "rb");
    if (fp == NULL)
    {
        perror("fopen: Can not open the file.\n");
        exit(1);
    }

    // count the number of students in the file
    fseek(fp, 0L, SEEK_END);
    size_t size = ftell(fp);
    n = size / sizeof(MyRecord);
    printf("Number of Records in File: %d\n", n);
    rewind(fp);

    // attach the records to shared memory
    int shmid2;
    size_t shm_size = 4500000; // this is max size needed for the largest file size
    key_t key2 = 3010;

    shmid2 = shmget(key2, shm_size, IPC_CREAT | 0666);
    if (shmid2 < 0)
    {
        perror("shmget22");
        exit(1);
    }

    student = (MyRecord *)shmat(shmid2, NULL, 0);
    if (student == (MyRecord *)-1)
    {
        perror("shmat");
        exit(1);
    }

    // read the file into the shared memory segment
    int i = 0;
    while (fread(&student[i], sizeof(MyRecord), 1, fp) == 1)
    {
        i++;
    }
    printf("Successfully loaded all records in the shared memory.\n");

    fclose(fp);

    // initialize struct values
    a->reader_count = 0;
    a->avg_duration_readers = 0.0;
    a->writer_count = 0;
    a->avg_duration_writers = 0.0;
    a->max_delay = 0.0;
    a->num_records = 0;

    FILE *logfile = fopen("logfile.txt", "w"); // creating a new log file to see what each reader / writer is doing

    // initialize semaphores
    sem_t *mutex; // mututal exclusion readcount
    mutex = sem_open("mutex", O_CREAT, 0666, 1);

    sem_t *analytics_sem = sem_open("analytic", O_CREAT, 0666, 1); // mututal exclusion analytics

    // array of semaphores to lock all records if reader is reading
    sem_t *sem_array[n];
    char sem_name[20];
    for (int i = 0; i < n; i++)
    {
        sprintf(sem_name, "/my_sem_%d", i);
        sem_array[i] = sem_open(sem_name, O_CREAT, 0644, 1);
    }

    // semaphore to control the standard output, ensuring one reader prints at a time
    sem_t *output_sem;
    output_sem = sem_open("/output_sem", O_CREAT, 0644, 1);

    // logging semaphore
    sem_t *log = sem_open("log", O_CREAT, 0644, 1);

    sem_t *wrt; // for writing to the file one at a time to avoid data corruption
    wrt = sem_open("wrt", O_CREAT, 0666, 1);

    // spawn the reader writer processes
    for (int num = 0; num < NUM_FORKS; num++)
    {
        int decider = rand() % 5; // if <= 2, reader process. else, writer process.

        // Fork process
        pid_t pid = fork();

        if (pid < 0)
        {
            perror("Failed to fork");
            exit(EXIT_FAILURE);
        }

        if (pid == 0)
        {
            // pick reader if decider = 0, 1, 2
            if (decider <= 2)
            {
                srand(getpid());
                int x = rand() % MAX_RECORD_PER_READER + 1; // x is the random num of records the reader can read
                int *list = malloc(x * sizeof(int));
                int num_to_read = 0;

                // update the list to include all the line numbers to read
                while (num_to_read < x)
                {
                    int line_num = rand() % (n - 1);
                    int exists = 0;
                    // Check if the number already exists in the list
                    for (int k = 0; k < num_to_read; k++)
                    {
                        if (list[k] == line_num)
                        {
                            exists = 1;
                            break;
                        }
                    }
                    // If the number is not in the list, add it
                    if (!exists)
                    {
                        list[num_to_read] = line_num;
                        num_to_read++;
                    }
                }

                // Create a string to combine all values in list separated by comma
                char list_string[100]; // assuming the maximum length of the resulting string is 100 characters
                int index = 0;

                for (int i = 0; i < num_to_read; i++)
                {
                    index += sprintf(list_string + index, "%d,", list[i]);
                }
                // Remove the trailing comma
                if (num_to_read > 0)
                {
                    list_string[index - 1] = '\0';
                }

                int random_time = (rand() % MAX_SLEEP_TIME) + 1; // random sleep time
                char random_time_str[10];
                sprintf(random_time_str, "%d", random_time); // converting it to string to pass it in exec

                char key2_str[10];             // shared memory key
                sprintf(key2_str, "%d", key2); // converting it to string to pass it in exec

                char total_recs[20];
                sprintf(total_recs, "%d", n); // converting it to string to pass it in exec

                // invoke reader
                if (execlp("./reader", "./reader", "-f", FILENAME, "-l", list_string, "-d", random_time_str, "-s", key2_str, "-n", total_recs, NULL) < 0)
                {
                    perror("Exec Error");
                    exit(EXIT_FAILURE);
                }

                free(list);
            }
            else
            {
                srand(getpid());
                int random_time = (rand() % MAX_SLEEP_TIME) + 1; // random sleep time
                char random_time_str[10];
                sprintf(random_time_str, "%d", random_time); // converting it to string to pass it in exec

                char key2_str[10];             // shared memory key for MyRecord *student
                sprintf(key2_str, "%d", key2); // converting it to string to pass it in exec

                int random_record = rand() % n; // generate a random number between 0 and n (num records in the file) to mofidy the record at that index
                char random_record_str[10];     // // converting it to string to pass it in exec
                sprintf(random_record_str, "%d", random_record);

                char total_recs[20];
                sprintf(total_recs, "%d", n); // // converting total recs to string to pass it in exec

                if (execlp("./writer", "./writer", "-f", FILENAME, "-l", random_record_str, "-d", random_time_str, "-s", key2_str, "-n", total_recs, NULL) < 0)
                {
                    perror("Exec Error");
                    exit(EXIT_FAILURE);
                }
            }
        }
    }

    // waiting for all child processes to finish
    for (int c = 0; c < NUM_FORKS; c++)
    {
        int status;
        pid_t child_pid = wait(&status);
        if (child_pid == -1)
        {
            perror("Failed to wait");
            exit(EXIT_FAILURE);
        }
    }

    // printing the analytics
    printf("Number of readers processed: %d\n", a->reader_count);
    printf("Number of writers processed: %d\n", a->writer_count);
    printf("Avg duration of readers: %f\n", a->avg_duration_readers);
    printf("Avg duration of writer: %f\n", a->avg_duration_writers);
    printf("Number of records accessed / modified: %d\n", a->num_records);
    printf("Max Delay Encountered: %f\n", a->max_delay);

    // close the semaphores
    sem_close(log);
    for (int i = 0; i < n; i++)
    {
        if (sem_close(sem_array[i]) < 0)
        {
            perror("sem_close");
            exit(EXIT_FAILURE);
        }
        sprintf(sem_name, "/my_sem_%d", i);
        sem_unlink(sem_name);
    }
    sem_close(*sem_array);
    sem_close(analytics_sem);
    sem_close(mutex);
    sem_close(output_sem);
    sem_close(wrt);

    // unlike the semaphores
    sem_unlink("log");
    sem_unlink("analytic");
    sem_unlink("mutex");
    sem_unlink("/output_sem");
    sem_unlink("wrt");

    // detaching and removing shared segments
    if (shmdt(a) == -1)
    {
        perror("Failed to detach shared memory");
        exit(EXIT_FAILURE);
    }
    if (shmctl(shmid1, IPC_RMID, NULL) == -1)
    {
        perror("Failed to remove shared memory 1");
        exit(EXIT_FAILURE);
    }

    if (shmdt(student) == -1)
    {
        perror("Failed to detach shared memory");
        exit(EXIT_FAILURE);
    }
    if (shmctl(shmid2, IPC_RMID, NULL) == -1)
    {
        perror("Failed to remove shared memory 2");
        exit(EXIT_FAILURE);
    }

    if (shmdt(readcount) == -1)
    {
        perror("Failed to detach shared memory");
        exit(EXIT_FAILURE);
    }
    if (shmctl(shmid3, IPC_RMID, NULL) == -1)
    {
        perror("Failed to remove shared memory 3");
        exit(EXIT_FAILURE);
    }
}
