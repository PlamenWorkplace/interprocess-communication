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

void open_mqs(mqd_t *client2dealer_queue, mqd_t *dealer2worker1_queue, mqd_t *dealer2worker2_queue, mqd_t *worker2dealer_queue,
              char client2dealer_name[], char dealer2worker1_name[], char dealer2worker2_name[], char worker2dealer_name[]);
void create_processes(pid_t *pid_client, char client2dealer_name[], char dealer2worker1_name[], char dealer2worker2_name[], char worker2dealer_name[]);
void close_mqs(mqd_t client2dealer_queue, mqd_t dealer2worker1_queue, mqd_t dealer2worker2_queue, mqd_t worker2dealer_queue,
              char client2dealer_name[], char dealer2worker1_name[], char dealer2worker2_name[], char worker2dealer_name[]);
void validate_mq(mqd_t mq, char queue_name[]);
bool is_child_running(pid_t child_pid);
void receiveFromMq(void);

int main(int argc, char * argv[])
{
  if (argc != 1)
  {
    fprintf(stderr, "%s: invalid arguments\n", argv[0]);
  }

  char client2dealer_name[30];
  char dealer2worker1_name[30];
  char dealer2worker2_name[30];
  char worker2dealer_name[30];

  mqd_t client2dealer_queue;
  mqd_t dealer2worker1_queue;
  mqd_t dealer2worker2_queue;
  mqd_t worker2dealer_queue;

  pid_t pid_client;
  
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

  open_mqs(&client2dealer_queue, &dealer2worker1_queue, &dealer2worker2_queue, &worker2dealer_queue, 
            client2dealer_name, dealer2worker1_name, dealer2worker2_name, worker2dealer_name);
  create_processes(&pid_client, client2dealer_name, dealer2worker1_name, dealer2worker2_name, worker2dealer_name);
  
  MQ_MESSAGE m;

  while (is_child_running(pid_client))
  {
    receiveFromMq();
    sleep(1);  // Avoid busy-waiting
  }

  

  while (wait(NULL) > 0);  // Wait for all children
  fprintf (stderr, "All child processes have finished.\n");
  close_mqs(client2dealer_queue, dealer2worker1_queue, dealer2worker2_queue, worker2dealer_queue, 
              client2dealer_name, dealer2worker1_name, dealer2worker2_name, worker2dealer_name);
  return (0);
}

void open_mqs(mqd_t *client2dealer_queue, mqd_t *dealer2worker1_queue, mqd_t *dealer2worker2_queue, mqd_t *worker2dealer_queue,
              char client2dealer_name[], char dealer2worker1_name[], char dealer2worker2_name[], char worker2dealer_name[])
{
  // Fill in queue names
  sprintf (client2dealer_name, "/Req_queue_%d", GROUP_NUMBER);
  sprintf (dealer2worker1_name, "/S1_queue_%d", GROUP_NUMBER);
  sprintf (dealer2worker2_name, "/S2_queue_%d", GROUP_NUMBER);
  sprintf (worker2dealer_name, "/Rsp_queue_%d", GROUP_NUMBER);

  // Open queues
  struct mq_attr attr = { .mq_maxmsg = 10, .mq_msgsize = sizeof(MQ_MESSAGE) };

  *client2dealer_queue = mq_open(client2dealer_name, O_RDONLY | O_CREAT | O_EXCL | O_NONBLOCK, 0600, &attr);
  validate_mq(*client2dealer_queue, client2dealer_name);
  *dealer2worker1_queue = mq_open(dealer2worker1_name, O_WRONLY | O_CREAT | O_EXCL, 0600, &attr);
  validate_mq(*dealer2worker1_queue, dealer2worker1_name);
  *dealer2worker2_queue = mq_open(dealer2worker2_name, O_WRONLY | O_CREAT | O_EXCL, 0600, &attr);
  validate_mq(*dealer2worker2_queue, dealer2worker2_name);
  *worker2dealer_queue = mq_open(worker2dealer_name, O_RDONLY | O_CREAT | O_EXCL | O_NONBLOCK, 0600, &attr);
  validate_mq(*worker2dealer_queue, worker2dealer_name);
}

void create_processes(pid_t *pid_client, char client2dealer_name[], char dealer2worker1_name[], char dealer2worker2_name[], char worker2dealer_name[]) {
  // Client
  *pid_client = fork();
  if (*pid_client < 0) {
      perror("fork() failed");
      exit(1);
  } else if (*pid_client == 0) {
      fprintf (stderr, "Child created. Starting process ./client\n");
      execlp("./client", "./client", client2dealer_name, NULL);
      perror("execlp() failed");
      exit(1);
  }

  pid_t pid;
  // Worker 1
  for (int i = 0; i < N_SERV1; i++) {
    pid = fork();
    if (pid < 0) {
        perror("fork() failed");
        exit(1);
    } else if (pid == 0) {
        fprintf (stderr, "Child created. Starting process ./worker_s1 for the %d time\n", i+1);
        execlp("./worker_s1", "./worker_s1", dealer2worker1_name, worker2dealer_name, NULL);
        perror("execlp() failed");
        exit(1);
    }
  }

  // Worker 2
  for (int i = 0; i < N_SERV2; i++) {
    pid = fork();
    if (pid < 0) {
        perror("fork() failed");
        exit(1);
    } else if (pid == 0) {
        fprintf (stderr, "Child created. Starting process ./worker_s2 for the %d time\n", i+1);
        execlp("./worker_s2", "./worker_s2", dealer2worker2_name, worker2dealer_name, NULL);
        perror("execlp() failed");
        exit(1);
    }
  }
}

void close_mqs(mqd_t client2dealer_queue, mqd_t dealer2worker1_queue, mqd_t dealer2worker2_queue, mqd_t worker2dealer_queue,
                char client2dealer_name[], char dealer2worker1_name[], char dealer2worker2_name[], char worker2dealer_name[])
{
  mqd_t queues[] = {client2dealer_queue, dealer2worker1_queue, dealer2worker2_queue, worker2dealer_queue};
  char *names[] = {client2dealer_name, dealer2worker1_name, dealer2worker2_name, worker2dealer_name};

  for (int i = 0; i < 4; i++) {
      mq_close(queues[i]);
      mq_unlink(names[i]);
  }
}

void validate_mq(mqd_t mq, char queue_name[])
{
  if (mq == (mqd_t)-1) 
  {
    fprintf(stderr, "validate_queue_init(%s) failed: ", queue_name);
    perror("");
    exit(1);
  }

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

bool is_child_running(pid_t child_pid) {
  int status;
  pid_t result = waitpid(child_pid, &status, WNOHANG);
  
  if (result == child_pid) // Terminated
  { 
    if (WIFEXITED(status)) 
        printf("Child process %d exited with status %d.\n", child_pid, WEXITSTATUS(status));
    else if (WIFSIGNALED(status)) 
        printf("Child process %d was terminated by signal %d.\n", child_pid, WTERMSIG(status));
    
    return false;
  } 
  else if (result == 0) // Still running
  {
    return true;
  }
  else
  {
      perror("waitpid() failed");
      exit (1);
  }
}

void receiveFromMq(void)
{

}