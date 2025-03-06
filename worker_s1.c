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
#include <signal.h>

#include "messages.h"
#include "service1.h"

static void rsleep (int t);
void close_mqs();
void validate_mq(mqd_t mq, char queue_name[]);
void handle_termination(int signum);

mqd_t mq_fd_s1, mq_fd_rsp;
char *queue_name_s1, *queue_name_rsp;

int main (int argc, char * argv[])
{
    // TODO:
    // (see message_queue_test() in interprocess_basic.c)
    //  * open the two message queues (whose names are provided in the
    //    arguments)
    //  * repeatedly:
    //      - read from the S1 message queue the new job to do
    //      - wait a random amount of time (e.g. rsleep(10000);)
    //      - do the job 
    //      - write the results to the Rsp message queue
    //    until there are no more tasks to do
    //  * close the message queues
    if (argc<3){
        fprintf(stderr,"usage: %s <S1_QUEUE_NAME> <RSP_QUEUE_NAME>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Store queue names
    queue_name_s1 = argv[1];
    queue_name_rsp = argv[2];

    // Open the first queue - S1, as read only
    mq_fd_s1 = mq_open(queue_name_s1, O_RDONLY);
    validate_mq(mq_fd_s1, queue_name_s1);

    // Open the second queue - RSP, as write only
    mq_fd_rsp = mq_open(queue_name_rsp, O_WRONLY);
    validate_mq(mq_fd_rsp, queue_name_rsp);

    fprintf(stderr, "worker_s1 (PID %d): started, listening on '%s', responding on '%s'\n", getpid(), queue_name_s1, queue_name_rsp);

    // Register the signal handler
    signal(SIGTERM, handle_termination);

    // Handle requests
    while (true){
        MQ_MESSAGE req; //change this when we have a message queue
        ssize_t bytes_read = mq_receive(mq_fd_s1, (char*)&req, sizeof(req), NULL);

        //use this later to handle interrupted messages
        if (bytes_read == -1){
            
            if (errno == EINTR){
                continue;
            }
            perror("mq_receive S1 fail");
            break;
        }

        fprintf(stderr, "worker_sq (PID %d): received job=%d, data=%d\n",
        getpid(), req.job, req.data);

        rsleep(10000);    

        //result
        int result = service(req.data);

        //reply
        MQ_MESSAGE rsp;
        rsp.job = req.job;
        rsp.data = result;

        fprintf(stderr, "worker_s1 (PID %d): sending job=%d, result=%d\n",
        getpid(), rsp.job, rsp.data);

        if (mq_send(mq_fd_rsp, (char *)&rsp, sizeof(rsp), 0) == -1){
            perror("mq_send RSP fail");
            break;
        }
    }

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

void close_mqs()
{
  if (mq_fd_s1 != (mqd_t) -1) {
    mq_close(mq_fd_s1);
    mq_unlink(queue_name_s1);
  }
  if (mq_fd_rsp != (mqd_t) -1) {
    mq_close(mq_fd_rsp);
    mq_unlink(queue_name_rsp);
  }
}

void validate_mq(mqd_t mq, char queue_name[])
{
  if (mq == (mqd_t)-1) 
  {
    fprintf(stderr, "PID %d, Queue %s: ", getpid(), queue_name);
    perror("");
    exit(1);
  }

  struct mq_attr attr;
  int rtnval;
  rtnval = mq_getattr(mq, &attr);
  if (rtnval == -1)
  {
    fprintf(stderr, "PID (%d), mq_getattr(%s) failed", getpid(), queue_name);
    perror("");
    exit (1);
  }
}

void handle_termination(int signum) {
  close_mqs();
  fprintf(stderr, "(PID %d): Resources cleaned, terminating...", getpid());
  exit(0);
}