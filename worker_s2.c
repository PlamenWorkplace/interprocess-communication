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
#include <fcntl.h>
#include <sys/types.h>
#include <sys/time.h>

#include "messages.h"
#include "service2.h"

#define TIMEOUT_SEC 1

static void rsleep (int t);
void validate_mq(mqd_t mq, char queue_name[]);
void send_to_router(MQ_MESSAGE *msg);
bool receive_with_timeout(mqd_t queue, MQ_MESSAGE *msg);

mqd_t mq_fd_s2, mq_fd_rsp;
char *queue_name_s2, *queue_name_rsp;

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

    // Store queue names
    queue_name_s2 = argv[1];
    queue_name_rsp = argv[2];

    // Open the first queue - S2, as read only
    mq_fd_s2 = mq_open(queue_name_s2, O_RDONLY);
    validate_mq(mq_fd_s2, queue_name_s2);

    // Open the second queue - RSP, as write only
    mq_fd_rsp = mq_open(queue_name_rsp, O_WRONLY);
    validate_mq(mq_fd_rsp, queue_name_rsp);

    fprintf(stderr, "worker_s2 (PID %d): started, listening on '%s', responding on '%s'\n", getpid(), queue_name_s2, queue_name_rsp);

    // Handle requests
    while (true)
    {
      MQ_MESSAGE msg;
      bool has_received = receive_with_timeout(mq_fd_s2, &msg);

      if (!has_received)
      {
        mq_close(mq_fd_s2);
        mq_close(mq_fd_rsp);
        fprintf(stderr, "(PID %d): Resources cleaned, terminating...\n", getpid());
        exit(0);
      }

      send_to_router(&msg);
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

void send_to_router(MQ_MESSAGE *msg)
{
  fprintf(stderr, "worker_s2 (PID %d): received job=%d, data=%d\n", getpid(), msg->job, msg->data);

  rsleep(10000);    

  int result = service(msg->data);
  msg->data = result;

  fprintf(stderr, "worker_s2 (PID %d): sending job=%d, result=%d\n", getpid(), msg->job, msg->data);

  if (mq_send(mq_fd_rsp, (char *)msg, sizeof(MQ_MESSAGE), 0) == -1)
  {
    if (errno == EAGAIN) 
    {
      fprintf(stderr, "worker_s2 (PID %d): Response queue is full, job %d is lost \n", getpid(), msg->job);
    } 
    else 
    {
      perror("mq_send RSP fail");
      exit(1);
    }
  }
}

bool receive_with_timeout(mqd_t queue, MQ_MESSAGE *msg) 
{
  struct timeval timeout;
  fd_set fds;
  int ret;

  timeout.tv_sec = TIMEOUT_SEC;
  timeout.tv_usec = 0;

  // Prepare for select call
  FD_ZERO(&fds);
  FD_SET(queue, &fds);

  // Use select to wait for the message queue to be ready for reading
  ret = select(FD_SETSIZE, &fds, NULL, NULL, &timeout);

  if (ret == -1) 
  {
    perror("select failed");
    exit(1);
  }
  if (ret == 0) 
  {
    fprintf(stderr, "worker_s2 (PID %d): Timeout reached. No message received.\n", getpid());
    return false;
  }
  if (FD_ISSET(queue, &fds)) {
    // Try to receive the message from the queue
    ssize_t bytes_read = mq_receive(queue, (char *)msg, sizeof(MQ_MESSAGE), NULL);
    if (bytes_read == -1) {
        perror("mq_receive failed");
        exit(1);
    }

    return true;  // Successfully received message
  }

  return false;  // This should never be reached in this logic
}