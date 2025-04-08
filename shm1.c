/* Our first program is a consumer. After the headers the shared memory segment
 (the size of our shared memory structure) is created with a call to shmget,
 with the IPC_CREAT bit specified. */

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <sys/shm.h>
#include <sys/sem.h>

#include "shm_com.h"
#include "semun.h"

static int semaphore_down(int);
static int semaphore_up(int);
static int set_semvalue(int, int);
static void del_semvalue(int);

static int semid_cheio;
static int semid_vazio;
static int semid_mutex;

int main()
{
    int running = 1;
    void *shared_memory = (void *)0;
    struct shared_use_st *shared_stuff;
    int shmid;

    srand((unsigned int)getpid());

    shmid = shmget((key_t)1234, sizeof(struct shared_use_st), 0666 | IPC_CREAT);

    semid_cheio = semget((key_t)3030, 1, 0666 | IPC_CREAT);
    semid_vazio = semget((key_t)3060, 1, 0666 | IPC_CREAT);
    semid_mutex = semget((key_t)3090, 1, 0666 | IPC_CREAT);

    set_semvalue(semid_vazio, 10);

/* We now make the shared memory accessible to the program. */

    shared_memory = shmat(shmid, (void *)0, 0);

    printf("Memory attached at %X\n", (int)shared_memory);

/* The next portion of the program assigns the shared_memory segment to shared_stuff,
 which then prints out any text in written_by_you. The loop continues until end is found
 in written_by_you. The call to sleep forces the consumer to sit in its critical section,
 which makes the producer wait. */

    shared_stuff = (struct shared_use_st *)shared_memory;
    shared_stuff->written_by_you = 0;
    shared_stuff->pos_c = 0;

    while(running) {
        // substituir este if para usar semáforos
        if (semaphore_down(semid_cheio)) {
            semaphore_down(semid_mutex);
            int pos = shared_stuff->pos_c;
            printf("You wrote: %s", shared_stuff->some_text[pos]);
            sleep( rand() % 4 ); /* make the other process wait for us ! */
            shared_stuff->written_by_you = 0;
            semaphore_up(semid_vazio);
            if (strncmp(shared_stuff->some_text[pos], "end", 3) == 0) {
                running = 0;
            }
            shared_stuff->pos_c = (pos + 1) % 10;
            semaphore_up(semid_mutex);
        } else {
            sleep(1);
            break;
        }
    }

/* Lastly, the shared memory is detached and then deleted. */

    shmdt(shared_memory);
    shmctl(shmid, IPC_RMID, 0);
    exit(EXIT_SUCCESS);
}

static int set_semvalue(int sem_id, int value)
{
    union semun sem_union;

    sem_union.val = value;
    if (semctl(sem_id, 0, SETVAL, sem_union) == -1) return(0);
    return(1);
}

/* The del_semvalue function has almost the same form, except the call to semctl uses
 the command IPC_RMID to remove the semaphore's ID. */

static void del_semvalue(int sem_id)
{
    union semun sem_union;
    
    if (semctl(sem_id, 0, IPC_RMID, sem_union) == -1)
        fprintf(stderr, "Failed to delete semaphore\n");
}


static int semaphore_down(int sem_id)
{
    struct sembuf sem_b;

    sem_b.sem_num = 0;
    sem_b.sem_op = -1; /* P() */
    sem_b.sem_flg = SEM_UNDO;
    if (semop(sem_id, &sem_b, 1) == -1)
    {
        fprintf(stderr, "semaphore_p failed\n");
        return (0);
    }
    return (1);
}

static int semaphore_up(int sem_id)
{
    struct sembuf sem_b;

    sem_b.sem_num = 0;
    sem_b.sem_op = 1; /* V() */
    sem_b.sem_flg = SEM_UNDO;
    if (semop(sem_id, &sem_b, 1) == -1)
    {
        fprintf(stderr, "semaphore_v failed\n");
        return (0);
    }
    return (1);
}