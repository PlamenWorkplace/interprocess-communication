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
bool receive_queue(mqd_t queue, MQ_MESSAGE *msg);
bool transfer_to_worker(mqd_t dealer2worker1_queue, mqd_t dealer2worker2_queue, MQ_MESSAGE m);
void add_job(int **jobList, int *jobListSize, int jobId);
void remove_job(int **jobList, int *jobListSize, int jobId);
void process_wroker_response(mqd_t queue, int **jobList, int *jobListSize);

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
  int *jobList = NULL;
  int jobListSize = 0;
  
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
    bool has_received = receive_queue(client2dealer_queue, &m);
    if (has_received) 
    {
      bool has_sent = transfer_to_worker(dealer2worker1_queue, dealer2worker2_queue, m);
      if (has_sent)
        add_job(&jobList, &jobListSize, m.job);
    }

    process_wroker_response(worker2dealer_queue, &jobList, &jobListSize);
  }

  while (jobListSize != 0)
  {
    process_wroker_response(worker2dealer_queue, &jobList, &jobListSize);
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
  struct mq_attr attr = { .mq_maxmsg = MQ_MAX_MESSAGES, .mq_msgsize = sizeof(MQ_MESSAGE) };

  *client2dealer_queue = mq_open(client2dealer_name, O_RDONLY | O_CREAT | O_EXCL, 0600, &attr);
  validate_mq(*client2dealer_queue, client2dealer_name);
  *dealer2worker1_queue = mq_open(dealer2worker1_name, O_WRONLY | O_CREAT | O_EXCL | O_NONBLOCK, 0600, &attr);
  validate_mq(*dealer2worker1_queue, dealer2worker1_name);
  *dealer2worker2_queue = mq_open(dealer2worker2_name, O_WRONLY | O_CREAT | O_EXCL | O_NONBLOCK, 0600, &attr);
  validate_mq(*dealer2worker2_queue, dealer2worker2_name);
  *worker2dealer_queue = mq_open(worker2dealer_name, O_RDONLY | O_CREAT | O_EXCL, 0600, &attr);
  validate_mq(*worker2dealer_queue, worker2dealer_name);
}

void create_processes(pid_t *pid_client, char client2dealer_name[], char dealer2worker1_name[], char dealer2worker2_name[], char worker2dealer_name[]) 
{
  // Client
  *pid_client = fork();
  if (*pid_client < 0) 
  {
    perror("fork() failed");
    exit(1);
  } 
  else if (*pid_client == 0) 
  {
    fprintf (stderr, "Child created. Starting process ./client\n");
    execlp("./client", "./client", client2dealer_name, NULL);
    perror("execlp() failed");
    exit(1);
  }

  pid_t pid;
  // Worker 1
  for (int i = 0; i < N_SERV1; i++) 
  {
    pid = fork();
    if (pid < 0) 
    {
      perror("fork() failed");
      exit(1);
    } 
    else if (pid == 0)
    {
      fprintf (stderr, "Child created. Starting process ./worker_s1 for the %d time\n", i+1);
      execlp("./worker_s1", "./worker_s1", dealer2worker1_name, worker2dealer_name, NULL);
      perror("execlp() failed");
      exit(1);
    }
  }

  // Worker 2
  for (int i = 0; i < N_SERV2; i++) 
  {
    pid = fork();
    if (pid < 0) 
    {
        perror("fork() failed");
        exit(1);
    } 
    else if (pid == 0) 
    {
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

  for (int i = 0; i < 4; i++) 
  {
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

bool is_child_running(pid_t child_pid) 
{
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
      exit(1);
  }
}

bool receive_queue(mqd_t queue, MQ_MESSAGE *msg)
{
  int result = mq_receive(queue, (char*)msg, sizeof(MQ_MESSAGE), NULL);

  if (result == -1) 
  {
    if (errno != EAGAIN)
    {
      perror("Receiving failed");
      exit(1);
    }

    return false;
  }
  return true;
}

bool transfer_to_worker(mqd_t dealer2worker1_queue, mqd_t dealer2worker2_queue, MQ_MESSAGE m)
{
  int result = -1;

  if (m.service == 1)
    result = mq_send(dealer2worker1_queue, (char*)&m, sizeof(MQ_MESSAGE), 0);
  else if (m.service == 2)
    result = mq_send(dealer2worker2_queue, (char*)&m, sizeof(MQ_MESSAGE), 0);
  else
  {
    fprintf(stderr, "Invalid message service: %d\n", m.service);
    exit(1);
  }

  if (result == -1) 
  {
    if (errno == EAGAIN) 
    {
      fprintf(stderr, "Queue is full. Message with service %d could not be sent.\n", m.service);
      return false;
    }
    else 
    {
      perror("mq_send failed");
      exit(1);
    }
  }
  
  return true;
}

void add_job(int **jobList, int *jobListSize, int jobId) 
{
  // Resize the list if necessary
  (*jobListSize)++;
  *jobList = realloc(*jobList, (*jobListSize) * sizeof(int));
  
  if (*jobList == NULL) 
  {
    perror("Failed to reallocate memory");
    exit(1); // Exit if realloc fails
  }

  // Add the new jobId to the list
  (*jobList)[(*jobListSize) - 1] = jobId;
}

void remove_job(int **jobList, int *jobListSize, int jobId) 
{
  // Find the index of the jobId to remove
  int index = -1;
  for (int i = 0; i < *jobListSize; i++) 
  {
    if ((*jobList)[i] == jobId) 
    {
      index = i;
      break;
    }
  }

  // If jobId is found, remove it
  if (index != -1) {
    // Shift elements to the left to fill the gap
    for (int i = index; i < *jobListSize - 1; i++) {
      (*jobList)[i] = (*jobList)[i + 1];
    }

    // Resize the list to reflect the removal
    (*jobListSize)--;
    *jobList = realloc(*jobList, (*jobListSize) * sizeof(int));

    if (*jobList == NULL && *jobListSize > 0) {
      perror("Failed to reallocate memory after removal");
      exit(1); // Exit if realloc fails
    }
  }
}

void process_wroker_response(mqd_t queue, int **jobList, int *jobListSize) 
{
  MQ_MESSAGE m;
  bool has_received = receive_queue(queue, &m);
  
  if (has_received) 
  {
    printf("Service: %d, Data: %d\n", m.service, m.data);
    remove_job(jobList, jobListSize, m.job);
  }
}