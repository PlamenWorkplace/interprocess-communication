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
#include <signal.h>


#include "settings.h"  
#include "messages.h"

#define GROUP_NUMBER 94

void open_mqs(mqd_t *client2dealer_queue, mqd_t *dealer2worker1_queue, mqd_t *dealer2worker2_queue, mqd_t *worker2dealer_queue,
              char client2dealer_name[], char dealer2worker1_name[], char dealer2worker2_name[], char worker2dealer_name[]);
void create_processes(pid_t *pid_client, pid_t *pid_workers, char client2dealer_name[], char dealer2worker1_name[], char dealer2worker2_name[], char worker2dealer_name[]);
void close_mqs(mqd_t client2dealer_queue, mqd_t dealer2worker1_queue, mqd_t dealer2worker2_queue, mqd_t worker2dealer_queue,
              char client2dealer_name[], char dealer2worker1_name[], char dealer2worker2_name[], char worker2dealer_name[]);
void validate_mq(mqd_t mq, char queue_name[]);
bool is_child_running(pid_t child_pid);
bool receive_queue(mqd_t queue, MQ_MESSAGE *msg);
void transfer_to_worker(mqd_t dealer2worker1_queue, mqd_t dealer2worker2_queue, MQ_MESSAGE m);
bool process_worker_response(mqd_t queue);
void process_responses_until_workers_exit(mqd_t worker2dealer_queue, pid_t *pid_workers, int num_workers);

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
  pid_t pid_workers[1 + N_SERV1 + N_SERV2];
  
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
  create_processes(&pid_client, pid_workers, client2dealer_name, dealer2worker1_name, dealer2worker2_name, worker2dealer_name);
  
  MQ_MESSAGE m;

  while (is_child_running(pid_client))
  {
    bool has_client_msg = receive_queue(client2dealer_queue, &m);
    if (has_client_msg) 
    {
      transfer_to_worker(dealer2worker1_queue, dealer2worker2_queue, m);
    }

    process_worker_response(worker2dealer_queue);
  }

  // Drain any remaining requests from client2dealer_queue
  bool has_client_msg = receive_queue(client2dealer_queue, &m);
  while (has_client_msg)
  {
    transfer_to_worker(dealer2worker1_queue, dealer2worker2_queue, m);
    process_worker_response(worker2dealer_queue);
    has_client_msg = receive_queue(client2dealer_queue, &m);
  }

  process_responses_until_workers_exit(worker2dealer_queue, pid_workers, N_SERV1 + N_SERV2);
  fprintf(stderr, "All workers terminated and cleaned up.\n");

  // Drain any remaining responses from worker2dealer_queue
  bool has_worker_msg = process_worker_response(worker2dealer_queue);
  while (has_worker_msg)
  {
    has_worker_msg = process_worker_response(worker2dealer_queue);
  }
  
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
  *dealer2worker1_queue = mq_open(dealer2worker1_name, O_WRONLY | O_CREAT | O_EXCL, 0600, &attr);
  validate_mq(*dealer2worker1_queue, dealer2worker1_name);
  *dealer2worker2_queue = mq_open(dealer2worker2_name, O_WRONLY | O_CREAT | O_EXCL, 0600, &attr);
  validate_mq(*dealer2worker2_queue, dealer2worker2_name);
  *worker2dealer_queue = mq_open(worker2dealer_name, O_RDONLY | O_CREAT | O_EXCL | O_NONBLOCK, 0600, &attr);
  validate_mq(*worker2dealer_queue, worker2dealer_name);
}

void create_processes(pid_t *pid_client, pid_t *pid_workers, char client2dealer_name[], char dealer2worker1_name[], char dealer2worker2_name[], char worker2dealer_name[]) 
{
  pid_t pid;
  int pid_index = 0;

  // Client
  *pid_client = fork();
  if (*pid_client < 0) 
  {
    perror("fork() failed");
    exit(1);
  } 
  else if (*pid_client == 0) 
  {
    execlp("./client", "./client", client2dealer_name, NULL);
    perror("execlp() failed");
    exit(1);
  }

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
      execlp("./worker_s1", "./worker_s1", dealer2worker1_name, worker2dealer_name, NULL);
      perror("execlp() failed");
      exit(1);
    }

    pid_workers[pid_index++] = pid;
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
        execlp("./worker_s2", "./worker_s2", dealer2worker2_name, worker2dealer_name, NULL);
        perror("execlp() failed");
        exit(1);
    }

    pid_workers[pid_index++] = pid;
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
}

bool is_child_running(pid_t child_pid) 
{
  int status;
  pid_t result = waitpid(child_pid, &status, WNOHANG);
  
  if (result == child_pid) // Terminated
  { 
    if (WIFEXITED(status)) 
      fprintf(stderr, "Child process %d exited with status %d.\n", child_pid, WEXITSTATUS(status));
    else if (WIFSIGNALED(status)) 
      fprintf(stderr, "Child process %d was terminated by signal %d.\n", child_pid, WTERMSIG(status));
    
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
  struct mq_attr attr;
  if (mq_getattr(queue, &attr) == -1) 
  {
      perror("mq_getattr failed");
      exit(1);
  }

  // Check if the queue is in non-blocking mode
  bool is_nonblocking = (attr.mq_flags & O_NONBLOCK) != 0;

  // If the queue is empty, skip
  if (attr.mq_curmsgs == 0) 
  {
      return false;
  }

  // There is a message
  int result = mq_receive(queue, (char*)msg, sizeof(*msg), NULL);

  if (result == -1) 
  {
    if (errno == EAGAIN && is_nonblocking) 
    {
      return false;  // Queue is empty, no messages available
    }
    perror("Receiving failed");
    exit(1);
  }

  return true;
}

void transfer_to_worker(mqd_t dealer2worker1_queue, mqd_t dealer2worker2_queue, MQ_MESSAGE m)
{
  int result = -1;

  if (m.service == 1)
  {
    result = mq_send(dealer2worker1_queue, (char*)&m, sizeof(MQ_MESSAGE), 0);
  }
  else if (m.service == 2)
  {
    result = mq_send(dealer2worker2_queue, (char*)&m, sizeof(MQ_MESSAGE), 0);
  }
  else
  {
    fprintf(stderr, "Invalid message service: %d\n", m.service);
    exit(1);
  }

  if (result == -1) 
  {
    // Worker queues are blocking, no need to check EAGAIN
    perror("mq_send failed");
    exit(1);
  }
  fprintf(stderr, "router (PID %d): sent job=%d to service %d\n", getpid(), m.job, m.service);
}

bool process_worker_response(mqd_t queue) 
{
  MQ_MESSAGE m;
  bool has_received = receive_queue(queue, &m);
  
  if (has_received) 
  {
    fprintf(stdout, "data: %d, job %d, service: %d\n", m.data, m.job, m.service);
    return true;
  }

  return false;
}

void process_responses_until_workers_exit(mqd_t worker2dealer_queue, pid_t *pid_workers, int num_workers) {
  int active_workers = num_workers; // Track active workers

  // Process responses while workers are still alive
  while (active_workers > 0) {
    process_worker_response(worker2dealer_queue);

    // Check if any workers have terminated
    for (int i = 0; i < num_workers; i++) {
      if (pid_workers[i] > 0) { // If worker is still alive
        fprintf(stdout, "Worker pid %d is still alive\n", pid_workers[i]);
        int status;
        pid_t result = waitpid(pid_workers[i], &status, WNOHANG);
        if (result > 0) { // Worker has exited
          fprintf(stderr, "Worker %d exited with status %d.\n", pid_workers[i], status);
          pid_workers[i] = -1; // Mark as handled
          active_workers--;
        }
      }
    }
  }
}