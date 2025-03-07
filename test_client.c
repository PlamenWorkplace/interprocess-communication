/*
 * test_client.c
 *
 * Minimal test harness for the client application.
 *
 * This creates a "fake" router side that just reads messages from
 * the Req queue. The client app is run as a child process, sending
 * messages to the Req queue. We then observe the messages here
 * to confirm the client is working as intended.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include <mqueue.h>
#include <string.h>

#include "messages.h" 

#define TEST_REQ_NAME "/test_req_queue"

mqd_t create_queue(const char *name, long maxmsg, long msgsize)
{
    mq_unlink(name);

    struct mq_attr attr;
    memset(&attr, 0, sizeof(attr));
    attr.mq_flags = 0;
    attr.mq_maxmsg = maxmsg;
    attr.mq_msgsize = msgsize;
    attr.mq_curmsgs = 0;

    mqd_t q = mq_open(name, O_RDONLY | O_CREAT | O_EXCL, 0600, &attr);
    if (q == (mqd_t)-1) {
        perror("mq_open");
        exit(EXIT_FAILURE);
    }
    return q;
}

int main(void)
{
    mqd_t mq_req = create_queue(TEST_REQ_NAME, 10, sizeof(MQ_MESSAGE));

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    }
    else if (pid == 0) {
        execlp("./client", "./client", TEST_REQ_NAME, NULL);
        perror("execlp ./client");
        exit(EXIT_FAILURE);
    }
    else {
        MQ_MESSAGE msg;
        ssize_t bytes;
        unsigned int prio;

        printf("PARENT: waiting for messages in '%s'...\n", TEST_REQ_NAME);

        while (1) {
            bytes = mq_receive(mq_req, (char *)&msg, sizeof(msg), &prio);
            if (bytes == -1) {
                if (errno == EINTR)
                    continue;
                perror("mq_receive failed");
                break;
            }

            printf("PARENT: received job=%d, data=%d, service=%d (prio=%u)\n",
                   msg.job, msg.data, msg.service, prio);
        }

        waitpid(pid, NULL, 0);

        mq_close(mq_req);
        mq_unlink(TEST_REQ_NAME);

        printf("PARENT: done testing client.\n");
    }

    return 0;
}
