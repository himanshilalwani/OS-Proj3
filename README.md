# OS-Proj3
## Readers / Writers Problem

### Instructions to run the main file
- make
- ./main

### Invoking Reader.c
- gcc reader.c -o reader
- ./reader  -f filename -l recidx1,recidx2 -d time -s shmid -n totalrecords

### Invoking Writer.c
- gcc writer.c -o writer
- ./writer  -f filename -l recidx -d time -s shmid -n totalrecords

### Overview
- The program, written in C, addresses the reader / writer problem. 
- It uses fork() and exec() to call reader.c and writer.c from main.c.
    - We used this approach because it was easier to link the two files and address the problem directly through main.c this way. 
    - There is a decider variable which ensures that 3/5th of the time a reader process is spawned and the remaining times, a writer process.   
- The following shared memory structures have been created:
    - Readcount: This variable is used to keep track of active readers, suspend writer processes from running when readers are reading, and wake them up when all the readers are done reading.
    - Student: This is an array of Structs. Each struct stores one record in the given dataset.
    - Analytics: This is a struct that is used to track the analytics asked in the assignment prompt.
- The following semaphores have been created:
    - mutex: This semaphore is used to ensure that at a time only one reader can manipulate the Readcount variable.
    - analytics_sem: This semaphore is used to ensure that a time only one reader / writer can manipulate the analytics struct.
    - sem_array: This is an array of semaphores. Its size is equal to the length of the Student array, which is the total number of records read. It is used to create a lock on each of the records individually in the Students struct. All the semaphores in this array are locked when a reader is reading to ensure that no writer can access any record and freed when the readers are done reading. Similarly, when a writer is modifying a certain record, it locks that record so that no other writer can access it. This ensures that multiple writers can access different records concurrently.
    - output_sem: This semaphore is used to ensure that at a time only one reader can use the stdout to print the records. This way, the records are printed correctly and there's no overlap in the output of different readers.
    - log: This semaphore is used to ensure that at a time only one reader / writer can access the log file and append to it. 
    - wrt: This semaphore is used to ensure that at a time only one writer can modify the output file by writing the modified records. This way data corruption is avoided.
- We also have the following global variables which can be easily modified for testing purposes:
    - MAX_RECORD_PER_READER: Maximum number of records a reader can read at a time
    - MAX_SLEEP_TIME: Maximum duration a reader / writer can sleep
    - FILENAME: Name of the file to work with
    - NUM_FORKS: Number of forks to create to spawn reader / writer processes
