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
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>    
#include <unistd.h>    // for execlp
#include <mqueue.h>    // for mq


#include "settings.h"  
#include "messages.h"

#define GROUP_NUMBER 94

void create_message_queues(void);
void validate_queue_init(mqd_t mq, char queue_name[]);
void getattr(mqd_t mq, char queue_name[]);

char client2dealer_name[30];
char dealer2worker1_name[30];
char dealer2worker2_name[30];
char worker2dealer_name[30];

int main(int argc, char * argv[])
{
  if (argc != 1)
  {
    fprintf(stderr, "%s: invalid arguments\n", argv[0]);
  }
  
  // TODO:
    //  * create the message queues (see message_queue_test() in
    //    interprocess_basic.c)
    //  * create the child processes (see process_test() and
    //    message_queue_test())
    //  * read requests from the Req queue and transfer them to the workers
    //    with the Sx queues
    //  * read answers from workers in the Rep queue and print them
    //  * wait until the client has been stopped (see process_test())
    //  * clean up the message queues (see message_queue_test())

    // Important notice: make sure that the names of the message queues
    // contain your goup number (to ensure uniqueness during testing)
  
  create_message_queues();
  return (0);
}

void create_message_queues(void)
{
  // Fill in queue names
  sprintf (client2dealer_name, "/Req_queue_%d", GROUP_NUMBER);
  sprintf (dealer2worker1_name, "/S1_queue_%d", GROUP_NUMBER);
  sprintf (dealer2worker2_name, "/S2_queue_%d", GROUP_NUMBER);
  sprintf (worker2dealer_name, "/Rsp_queue_%d", GROUP_NUMBER);

  // Open queues
  struct mq_attr attr;
  attr.mq_maxmsg = 10;
  attr.mq_msgsize = sizeof(MQ_MESSAGE);

  mqd_t client2dealer_queue = mq_open(client2dealer_name, O_RDONLY | O_CREAT | O_EXCL, 0600, &attr);
  validate_queue_init(client2dealer_queue, client2dealer_name);
  mqd_t dealer2worker1_queue = mq_open(dealer2worker1_name, O_WRONLY | O_CREAT | O_EXCL, 0600, &attr);
  validate_queue_init(dealer2worker1_queue, dealer2worker1_name);
  mqd_t dealer2worker2_queue = mq_open(dealer2worker2_name, O_WRONLY | O_CREAT | O_EXCL, 0600, &attr);
  validate_queue_init(dealer2worker2_queue, dealer2worker2_name);
  mqd_t worker2dealer_queue = mq_open(worker2dealer_name, O_RDONLY | O_CREAT | O_EXCL, 0600, &attr);
  validate_queue_init(worker2dealer_queue, worker2dealer_name);
  
  // Close queues
  mq_close (client2dealer_queue);
  mq_close (dealer2worker1_queue);
  mq_close (dealer2worker2_queue);
  mq_close (worker2dealer_queue);

  mq_unlink (client2dealer_name);
  mq_unlink (dealer2worker1_name);
  mq_unlink (dealer2worker2_name);
  mq_unlink (worker2dealer_name);
}

void validate_queue_init(mqd_t mq, char queue_name[])
{
  if (mq == (mqd_t)-1) {
    fprintf(stderr, "validate_queue_init(%s) failed: ", queue_name);
    perror("");
    exit(1);
  }
  getattr(mq, queue_name);
}

void getattr (mqd_t mq, char queue_name[])
{
  struct mq_attr attr;
  int rtnval;
  rtnval = mq_getattr(mq, &attr);
  if (rtnval == -1)
  {
    fprintf(stderr, "mq_getattr(%s) failed: ", queue_name);
    perror("");
    exit (1);
  }
  fprintf (stderr, "%d: mqdes=%d max=%ld size=%ld nrof=%ld\n", 
    getpid(), mq, attr.mq_maxmsg, attr.mq_msgsize, attr.mq_curmsgs);
}