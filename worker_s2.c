/* 
 * Operating Systems  (2INCO)  Practical Assignment
 * Interprocess Communication
 *
 * STUDENT_NAME_1 (STUDENT_NR_1)
 * STUDENT_NAME_2 (STUDENT_NR_2)
 *
 * Grading:
 * Your work will be evaluated based on the following criteria:
 * - Satisfaction of all the specifications
 * - Correctness of the program
 * - Coding style
 * - Report quality
 * - Deadlock analysis
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>      // for perror()
#include <unistd.h>     // for getpid()
#include <mqueue.h>     // for mq-stuff
#include <time.h>       // for time()

#include "messages.h"
#include "service2.h"

static void rsleep (int t);

char* name = "NO_NAME_DEFINED";
mqd_t dealer2worker;
mqd_t worker2dealer;


int main (int argc, char * argv[])
{
    // TODO:
    // (see message_queue_test() in interprocess_basic.c)
    //  * open the two message queues (whose names are provided in the
    //    arguments)
    //  * repeatedly:
    //      - read from the S2 message queue the new job to do
    //      - wait a random amount of time (e.g. rsleep(10000);)
    //      - do the job 
    //      - write the results to the Rsp message queue
    //    until there are no more tasks to do
    //  * close the message queues
    if (argc<3){
        fprintf(stderr,"usage: %s <S2_QUEUE_NAME> <RSP_QUEUE_NAME>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Open the first queue - S2, as read only
    mqd_t mq_fd_s2 = mq_open(argv[1], O_RDONLY);
    if (mq_fd_s2 == (mqd_t)-1){
        perror("mq_open S2 fail");
        exit(EXIT_FAILURE);
    }

    // Open the second queue - RSP, as write only
    mqd_t mq_fd_rsp = mq_open(argv[2], O_WRONLY);
    if (mq_fd_rsp == (mqd_t)-1){
        perror("mq_open RSP fail");
        mq_close(mq_fd_s2);
        exit(EXIT_FAILURE);
    }

    printf("worker_s2 (PID %d): started, listening on '%s', responding on '%s'\n",
    getpid(), argv[1], argv[2]);

    // Handle requests
    while (true){
        MQ_MESSAGE req; //change this when we have a message queue
        ssize_t bytes_read = mq_receive(mq_fd_s2, (char*)&req, sizeof(req), NULL);

        //use this later to handle interrupted messages
        if (bytes_read == -1){
            
            if (errno == EINTR){
                continue;
            }
            perror("mq_receive S1 fail");
            break;
        }

        printf("worker_sq (PID %d): received job=%d, data=%d\n",
        getpid(), req.job, req.data);

        rsleep(10000);    

        //result
        int result = service(req.data);

        //reply
        MQ_MESSAGE rsp;
        rsp.job = req.job;
        rsp.data = result;

        printf("worker_s2 (PID %d): sending job=%d, result=%d\n",
        getpid(), rsp.job, rsp.data);

        if (mq_send(mq_fd_rsp, (char *)&rsp, sizeof(rsp), 0) == -1){
            perror("mq_send RSP fail");
            break;
        }
    }

    //close queues
    mq_close(mq_fd_s2);
    mq_close(mq_fd_rsp);
    return(0);
    return(0);
}

/*
 * rsleep(int t)
 *
 * The calling thread will be suspended for a random amount of time
 * between 0 and t microseconds
 * At the first call, the random generator is seeded with the current time
 */
static void rsleep (int t)
{
    static bool first_call = true;
    
    if (first_call == true)
    {
        srandom (time (NULL) % getpid ());
        first_call = false;
    }
    usleep (random() % t);
}
