/**
 * Faculty of Information Technology, Brno University of Technology.
 * Operating Systems - Project 2, The child care problem
 * Author: Petr Marek
 * Login:  xmarek66
 * 
 * Brief description: 
 * ------------------
 * At a child care center, state regulations require that there is always
 * one adult present for every three children. 
 * 
 * As an adult enter, three children can follow him. If there is few enough children
 * to let and adult leave, he leaves. This implementation does not allow a child to enter, 
 * after another child leaved. Only the arrival of an adult can let a child in. 
 * ----------------------------------------------------------------------------------------------------
 *
 * Implementation details:
 * -----------------------
 * Even though, the deadlock is solved, 
 * without proper care a blocking of application could still in at least one case occur.
 * Specifically, it is the situation when all adults left and there will be no more generated.
 * In this case, when there are children waiting, there would be no way they could enter.
 * This problem is solved the following way:
 *      In particular case, children waiting to enter
 *      can break the main rule of center (one adult per three children) and enter.
 * ----------------------------------------------------------------------------------------------------
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/shm.h>

//                                          children to generate     children waiting to enter
int *children_in = NULL, *child_id = NULL, *children_to_gen = NULL, *children_waiting = NULL;
int  children_in_id = 0,  child_id_id = 0,  children_to_gen_id = 0,  children_waiting_id = 0;

//                                        adults to generate     adults leaving center
int *adults_in = NULL, *adult_id = NULL, *adults_to_gen = NULL, *adults_leaving = NULL;
int  adults_in_id = 0,  adult_id_id = 0,  adults_to_gen_id = 0,  adults_leaving_id = 0;

// "cntr" for counter     
// "int *to_finish" - number of processes waiting at the barrier to write finish and exit
int  *line_cntr = NULL, *to_finish = NULL;
int   line_cntr_id = 0,  to_finish_id = 0;

sem_t *mutex_child_id, *child_queue;
sem_t *mutex_adult_id, *adult_queue; 
sem_t *barrier;
sem_t *mutex_enter;

FILE *fw;                           

/* A   - number of adults to generate
 * C   - number of children to generate
 * AGT - time to sleep (wait) before adult is generated
 * CGT - time to sleep before child is generated
 * AWT - time that adults simulate activity in the center
 * CWT - time that children simulate acitivity in the center
 */
int load_parameters(int argc, char *argv[], int *A, int *C, int *AGT, int *CGT, int *AWT, int *CWT);

/* 
 * name   - A or C (adult or child)
 * number - number of processes to generate
 * family - Adults to generate + children to generate - numbers given by user as arguments A and C
 * Generator is the parent process of grandchildren (adults_in and children_in).
 * Grandchild_A (adults_in) and grandchild_C (children_in) are generated here.
 */
void run_generator(char * name, int number, int family, int AGT, int CGT, int AWT, int CWT);

int run_grandchild_A(); // Adults run here.
int run_grandchild_C(); // Children run in this function.

// id - identification number of process - both children and adults are counter from 1 separately
//    - this number is printed right behind C or A in the output
void adult_enter(int id);
void adult_leave(int id);

void child_enter(int id);
void child_leave(int id);

void finish(char * name, int id, int family); // processes must finish together

// auxiliary functions
int file_open();
int file_close();

int set_resources();
int clean_resources();
int resources_error();
int sem_error(); // error when semaphore allocation failed

void do_nothing(int max_time); // lets a process sleep for maximum of max_time
int min(int a, int b);


int main(int argc, char *argv[])
{   
    srandom(time(0));       // initialize generator of random numbers
    setbuf(stderr, NULL);   // do no buff stderr
    
    int A, C, AGT, CGT, AWT, CWT;
    int err = load_parameters(argc, argv, &A, &C, &AGT, &CGT, &AWT, &CWT);
    if (err != 0)
    {   
        fprintf(stderr, "Wrong usage of parameters.\n");
        return err;
    }

    if (set_resources() != 0)
        return 2;
    
    setbuf(fw, NULL);
    
    pid_t pid;

    int family = (A + C);
    (*adults_to_gen) = A;
    (*children_to_gen) = C;

    if ((pid = fork()) < 0)
    {
        perror("adult fork");
        clean_resources();
        return 2;
    }
    else if (pid == 0) // child
        run_generator("A", A, family, AGT, CGT, AWT, CWT);
          
    else if (pid > 0) // parent 
    {
        pid_t child;

        if ((child = fork()) < 0)
        {
            perror("child fork");
            clean_resources();
            return 2;
        }      
        else if (child == 0) // child of parent - generating children
            run_generator("C", C, family, AGT, CGT, AWT, CWT);

        // parent waits for child generator to finish
        if (waitpid(child, NULL, 0) != child) 
        {
            perror("child generator waitpid");
            clean_resources();
            return 2;
        }
    } 

    // parent process waits for adult generator to finish
    if (waitpid(pid, NULL, 0) != pid) 
    {
        perror("adult generator waitpid");
        clean_resources();
        return 2;
    }

    if(clean_resources() != 0)
        return 2;
    return 0;
}

/* 
 * Generator is the parent process of grandchildren (adults and children).
 * Grandchild_A (adults) and grandchild_C (children) are generated here.
 */
void run_generator(char * name, int number, int family, int AGT, int CGT, int AWT, int CWT)
{
    pid_t pid;
    int id = 0;
    int adult_gen = 0;

    for (int i = 0; i < number; i++)
    {   
        if (strcmp (name, "A") == 0)
        {
            adult_gen = 1;
            do_nothing(AGT);
        }
        else
            do_nothing(CGT);
            
        if ((pid = fork()) < 0) 
        {
            perror("generator fork");
            exit(2);
        } 
        else if (pid == 0) // grandchild 
        {  
            if (adult_gen == 1)
                id = run_grandchild_A(AWT);
            else
                id = run_grandchild_C(CWT);
   
            finish(name, id, family);     
        }
    }

    // wait for generated processes to end
    for (int i = 0; i < number; i++)
        wait(NULL);
    
    exit(0);
}

int run_grandchild_A(int max_time) // Adults run here.
{
    int id;
    int sleep = 1;
    int random_time = 0;

    if (max_time != 0)
        random_time = ((random() % max_time) * 1000);
    else
        sleep = 0;

    sem_wait(mutex_adult_id);
        id = ++(*adult_id);
    sem_post(mutex_adult_id);

    adult_enter(id);
    if(sleep)
        usleep(random_time); 
    adult_leave(id);

    return id;
}

int run_grandchild_C(int max_time) // Children run in this function.
{
    int id;
    int sleep = 1;
    int random_time = 0;

    if (max_time != 0)
        random_time = ((random() % max_time) * 1000);
    else
        sleep = 0;

    sem_wait(mutex_child_id);
        id = ++(*child_id);
    sem_post(mutex_child_id);

    child_enter(id);
    if (sleep)
        usleep(random_time); 
    child_leave(id);

    return id;
}

void adult_enter(int id)
{
    sem_wait(mutex_enter);
        
        (*adults_in)++;
        (*adults_to_gen)--;

        fprintf(fw, "%-8d: A %-5d: started\n", ++(*line_cntr), id);
        fprintf(fw, "%-8d: A %-5d: enter\n", ++(*line_cntr), id);

        /**
        * If there are adults leaving and children waiting to enter would prevent adults from leaving,
        * do not let children enter.
        */
        int n = min(3, *children_waiting);
        if ((*adults_leaving > 0) && ((*children_in + n) > (3 * (*adults_in - *adults_leaving))))
        {/*Do not let children enter*/}
        else
        {
            /**
             * If there are children waiting, check if the condition of center will be valid if we let maximum of three of them in.
             * In addition, let waiting child enter, only when there are no more children generating. 
             * This prevents unpredictable behavior. 
             * I.e. waiting children must wait for children starting and trying to enter. 
             * Then they can all try to leave.
             */
            if((*children_in + n) <= (3 * (*adults_in)) && (*children_to_gen == 0))
            {
                for (int i = 0; i < n; i++)
                    sem_post(child_queue);

                (*children_waiting)-=n;
                (*children_in)+=n;
            }
        }

    sem_post(mutex_enter);  
}

void adult_leave(int id)
{
    sem_wait(mutex_enter);    
        
        fprintf(fw, "%-8d: A %-5d: trying to leave\n", ++(*line_cntr), id);
        if (*children_in <= (3 * (*adults_in - 1)))             
            (*adults_in)--;
        else
        {
            (*adults_leaving)++;
            fprintf(fw, "%-8d: A %-5d: waiting: %-5d : %d\n", ++(*line_cntr), id, *adults_in, *children_in);

            sem_post(mutex_enter);
            sem_wait(adult_queue); // adults wait here 
            sem_wait(mutex_enter); //adults are free to leave here
        }

        fprintf(fw, "%-8d: A %-5d: leave\n", ++(*line_cntr), id);
        
        /** 
         * if there are no adults_in in center and no more adults_in will start,
         * then let all waiting children enter to prevent blocking.
         */
        if ((*adults_in == 0) && (*adults_to_gen == 0))
                for (int i = 0; i < *children_waiting; i++)
                    sem_post(child_queue);

    sem_post(mutex_enter);      
}

void child_enter(int id)
{    
    sem_wait(mutex_enter);
        
        fprintf(fw, "%-8d: C %-5d: started\n", ++(*line_cntr), id);
        (*children_to_gen)--;

         /**
          * if there are no more adults generating and no adults in center, 
          * to prevent blocking, let the child enter.
          */
        if ((*children_in < (3 * (*adults_in)))  || ((*adults_in == 0) && (*adults_to_gen == 0)))
            (*children_in)++;
        else
        {   
            (*children_waiting)++;

            fprintf(fw, "%-8d: C %-5d: waiting: %-5d : %d\n", ++(*line_cntr), id, *adults_in, *children_in);
            sem_post(mutex_enter);
            
            // children wait to enter here
            sem_wait(child_queue);          

            // children are free to enter here
            sem_wait(mutex_enter);
        }

        fprintf(fw, "%-8d: C %-5d: enter\n", ++(*line_cntr), id);
        sem_post(mutex_enter);
}

void child_leave(int id)
{
    sem_wait(mutex_enter);

        (*children_in)--;
        // write these two lines in succession (without an interrupt from outside)
            fprintf(fw, "%-8d: C %-5d: trying to leave\n", ++(*line_cntr), id);
            fprintf(fw, "%-8d: C %-5d: leave\n", ++(*line_cntr), id);

        /**
         * if there are adults leaving and there are few enough children to let an adult leave, 
         * check if there are no more children generating. If there aren't, give the adult permission to leave.
         */
        if ((*adults_leaving > 0) && (*children_in <= (3 * (*adults_in - 1))) && (*children_to_gen == 0))
        {
            (*adults_leaving)--;
            (*adults_in)--;

            sem_post(adult_queue);
        }

    sem_post(mutex_enter);
}

// Creates a barrier. Other processes must wait on the last one of them in order to finish together.
void finish(char * name, int id, int family) 
{
    (*to_finish)++;             

    if (*to_finish == family)
        sem_post(barrier);

    sem_wait(barrier);
    sem_post(barrier);
    
    fprintf(fw, "%-8d: %s %-5d: finished\n", ++(*line_cntr), name, id);
    exit(0);      
}

int load_parameters(int argc, char *argv[], int *A, int *C, int *AGT, int *CGT, int *AWT, int *CWT)
{
    if (argc != 7)
        return 1;

    for (int i = 1; i < argc; i++)
    {   
        for (unsigned int j = 0; j < strlen(argv[i]); j++)
            if (!isdigit(argv[i][j]))
                return 1;

        switch (i)
        {
            case 1 : *A = atoi(argv[i]);
                break;
            case 2 : *C = atoi(argv[i]);
                break;
            case 4 : *CGT = atoi(argv[i]); 
                break;
            case 3 : *AGT = atoi(argv[i]);
                break;
            case 5 : *AWT = atoi(argv[i]);
                break;
            case 6 : *CWT = atoi(argv[i]);
                break;
        }
    }

    if(!(*A > 0) || !(*C > 0) || !(*AGT >= 0 && *AGT < 5001) || !(*CGT >= 0 && *CGT < 5001) || !(*AWT >= 0 && *AWT < 5001) || !(*CWT >= 0 && *CWT < 5001))
        return 1;
    
    return 0;
}

int file_open()
{
    if((fw = fopen("proj2.out", "w")) == NULL) 
    {
      fprintf(stderr, "Can not open output file.\n");
      return 2;
    }
    else   
        return 0;
}

int file_close()
{
    if ((fclose(fw)) == EOF)
    {
        fprintf(stderr, "Can not close output file.\n");
        return 2;
    }
    else
        return 0;
}

int set_resources()
{
    int err = 0;

    if(file_open() != 0)
        err = 1;

    if ((children_in_id = shmget(IPC_PRIVATE, sizeof (int), IPC_CREAT | 0666)) == -1)       { err = 2; } 
    if ((children_in = (int *) shmat(children_in_id, NULL, 0)) == NULL)                     { err = 2; } 

    if ((child_id_id = shmget(IPC_PRIVATE, sizeof (int), IPC_CREAT | 0666)) == -1)          { err = 2; } 
    if ((child_id = (int *) shmat(child_id_id, NULL, 0)) == NULL)                           { err = 2; } 

    if ((children_to_gen_id = shmget(IPC_PRIVATE, sizeof (int), IPC_CREAT | 0666)) == -1)   { err = 2; } 
    if ((children_to_gen = (int *) shmat(children_to_gen_id, NULL, 0)) == NULL)             { err = 2; } 
    
    if ((children_waiting_id = shmget(IPC_PRIVATE, sizeof (int), IPC_CREAT | 0666)) == -1)  { err = 2; } 
    if ((children_waiting = (int *) shmat(children_waiting_id, NULL, 0)) == NULL)           { err = 2; } 


    if ((adults_in_id = shmget(IPC_PRIVATE, sizeof (int), IPC_CREAT | 0666)) == -1)         { err = 2; } 
    if ((adults_in = (int *) shmat(adults_in_id, NULL, 0)) == NULL)                         { err = 2; } 

    if ((adult_id_id = shmget(IPC_PRIVATE, sizeof (int), IPC_CREAT | 0666)) == -1)          { err = 2; } 
    if ((adult_id = (int *) shmat(adult_id_id, NULL, 0)) == NULL)                           { err = 2; } 

    if ((adults_to_gen_id = shmget(IPC_PRIVATE, sizeof (int), IPC_CREAT | 0666)) == -1)     { err = 2; } 
    if ((adults_to_gen = (int *) shmat(adults_to_gen_id, NULL, 0)) == NULL)                 { err = 2; }   

    if ((adults_leaving_id = shmget(IPC_PRIVATE, sizeof (int), IPC_CREAT | 0666)) == -1)    { err = 2; } 
    if ((adults_leaving = (int *) shmat(adults_leaving_id, NULL, 0)) == NULL)               { err = 2; } 


    if ((line_cntr_id = shmget(IPC_PRIVATE, sizeof (int), IPC_CREAT | 0666)) == -1)         { err = 2; } 
    if ((line_cntr = (int *) shmat(line_cntr_id, NULL, 0)) == NULL)                         { err = 2; } 
    
    if ((to_finish_id = shmget(IPC_PRIVATE, sizeof (int), IPC_CREAT | 0666)) == -1)         { err = 2; } 
    if ((to_finish = (int *) shmat(to_finish_id, NULL, 0)) == NULL)                         { err = 2; }

    if (err == 2)
        resources_error();

    *children_in = 0;
    *child_id = 0;
    *children_to_gen = 0;
    *children_waiting = 0;

    *adults_in = 0;
    *adult_id = 0;
    *adults_to_gen = 0;
    *adults_leaving = 0;

    *line_cntr = 0;
    *to_finish = 0;

   
    if ((mutex_enter = sem_open("/xmarek66_mutex", O_CREAT | O_EXCL, 0666, 1)) == SEM_FAILED)                   { err = 3; }
    if ((barrier = sem_open("/xmarek66_barrier", O_CREAT | O_EXCL, 0666, 0)) == SEM_FAILED)                     { err = 3; }
       
    if ((mutex_child_id = sem_open("/xmarek66_mutexChildId", O_CREAT | O_EXCL, 0666, 1)) == SEM_FAILED)         { err = 3; }
    if ((mutex_adult_id = sem_open("/xmarek66_mutexAdultId", O_CREAT | O_EXCL, 0666, 1)) == SEM_FAILED)         { err = 3; }

    if ((child_queue = sem_open("/xmarek66_childQueue", O_CREAT | O_EXCL, 0666, 0)) == SEM_FAILED)              { err = 3; }
    if ((adult_queue = sem_open("/xmarek66_adultQueue", O_CREAT | O_EXCL, 0666, 0)) == SEM_FAILED)              { err = 3; }

    if (err == 3)
        sem_error();

    if (err != 0)
        return 2;
    return 0;
}

int clean_resources()
{
    shmctl(children_in_id, IPC_RMID, NULL);
    shmctl(adults_in_id, IPC_RMID, NULL);

    shmctl(line_cntr_id, IPC_RMID, NULL);

    shmctl(child_id_id, IPC_RMID, NULL);
    shmctl(adult_id_id, IPC_RMID, NULL);

    shmctl(children_to_gen_id, IPC_RMID, NULL);
    shmctl(adults_to_gen_id, IPC_RMID, NULL);

    shmctl(to_finish_id, IPC_RMID, NULL);

    shmctl(adults_leaving_id, IPC_RMID, NULL);
    shmctl(children_waiting_id, IPC_RMID, NULL);


    sem_close(mutex_enter);
    sem_close(barrier);

    sem_close(mutex_child_id);
    sem_close(mutex_adult_id);

    sem_close(child_queue);
    sem_close(adult_queue);


    sem_unlink("/xmarek66_mutex");
    sem_unlink("/xmarek66_barrier");
    
    sem_unlink("/xmarek66_mutexChildId");
    sem_unlink("/xmarek66_mutexAdultId");

    sem_unlink("/xmarek66_childQueue");
    sem_unlink("/xmarek66_adultQueue");
    
    
    if(file_close() != 0)   
        return 2;
    return 0;
}

int resources_error()
{
    fprintf(stderr, "Shared memory allocation failed.\n");
    clean_resources();       
    return 2;
}

int sem_error() // error when semaphore allocation failed
{
    fprintf(stderr, "Semaphore allocation error.\n");
    clean_resources(); 
    return 2;
}

void do_nothing(int max_time) // let a process sleep for maximum of max_time
{
    if (max_time != 0)
        usleep((random() % max_time) * 1000);        
}

int min(int a, int b) 
{
    if (a > b)
        return b;
    return a;
}