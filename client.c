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
#include "request.h"

static void rsleep (int t);


int main (int argc, char * argv[])
{
     // TODO:
    // (see message_queue_test() in interprocess_basic.c)
    //  * open the message queue (whose name is provided in the
    //    arguments)
    //  * repeatingly:
    //      - get the next job request 
    //      - send the request to the Req message queue
    //    until there are no more requests to send
    //  * close the message queue

    //check number of arguments
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <Req_queue_name>\n", argv[0]); //print used queue name
        exit(EXIT_FAILURE); 
    }

    // opens the message queue
    const char *reqQueueName = argv[1];
    mqd_t mq_req = mq_open(reqQueueName, O_WRONLY);
    if (mq_req == (mqd_t)-1) {
        perror("mq_open failed");
        exit(EXIT_FAILURE); // exit if failed
    }
    
    int jobID, data, service;
    int ret;
    
    // get the next job request
    while ((ret = getNextRequest(&jobID, &data, &service)) == NO_ERR) {
        MQ_MESSAGE msg;
        msg.job = jobID;
        msg.data = data;
        msg.service = service;

        fprintf(stderr, "client (PID %d): sending job=%d\n", getpid(), msg.job);
        
        // send the request to the Req message queue
        if (mq_send(mq_req, (char *)&msg, sizeof(msg), 0) == -1) {
            perror("mq_send failed");
            mq_close(mq_req);
            exit(EXIT_FAILURE);
        }
    }
    
    mq_close(mq_req);
    return 0;
}
