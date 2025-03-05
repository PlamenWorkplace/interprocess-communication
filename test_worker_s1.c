#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/wait.h>
#include <mqueue.h>
#include <string.h>

#include "messages.h"  
                        
#define QUEUE_S1 "/test_s1_q"
#define QUEUE_RSP "/test_rsp_q"

static mqd_t create_queue(const char *name, long maxmsg, long msgsize, int oflag)
{
    mq_unlink(name);

    struct mq_attr attr;
    memset(&attr, 0, sizeof(attr));
    attr.mq_flags = 0;          
    attr.mq_maxmsg = maxmsg;
    attr.mq_msgsize = msgsize;
    attr.mq_curmsgs = 0;

    mqd_t q = mq_open(name, oflag | O_CREAT | O_EXCL, 0600, &attr);
    if (q == (mqd_t)-1)
    {
        perror("mq_open failed");
        exit(EXIT_FAILURE);
    }
    return q;
}

int main(void)
{
    mqd_t mq_s1 = create_queue(QUEUE_S1, 10, sizeof(MQ_MESSAGE), O_WRONLY);
    mqd_t mq_rsp = create_queue(QUEUE_RSP, 10, sizeof(MQ_MESSAGE), O_RDONLY);

    pid_t pid = fork();
    if (pid < 0)
    {
        perror("fork failed");
        exit(EXIT_FAILURE);
    }
    else if (pid == 0)
    {
        mq_close(mq_s1);
        mq_close(mq_rsp);

        execlp("./worker_s1", "./worker_s1", QUEUE_S1, QUEUE_RSP, NULL);

        perror("execlp worker_s1 failed");
        exit(EXIT_FAILURE);
    }
    else
    {

        MQ_MESSAGE msg_req;
        msg_req.job = 100; 
        msg_req.data = 5; 
        printf("PARENT: sending job=%d, data=%d\n", msg_req.job, msg_req.data);

        if (mq_send(mq_s1, (char *)&msg_req, sizeof(msg_req), 0) == -1)
        {
            perror("mq_send to worker_s1");
        }

        MQ_MESSAGE msg_rsp;
        ssize_t bytes = mq_receive(mq_rsp, (char *)&msg_rsp, sizeof(msg_rsp), NULL);
        if (bytes == -1)
        {
            perror("mq_receive from worker_s1");
        }
        else
        {
            printf("PARENT: received job=%d, result=%d\n", msg_rsp.job, msg_rsp.data);
        }

        mq_close(mq_s1);
        mq_close(mq_rsp);
        mq_unlink(QUEUE_S1);
        mq_unlink(QUEUE_RSP);

        waitpid(pid, NULL, 0);

        printf("PARENT: done testing worker_s1\n");
    }
    return 0;
}
