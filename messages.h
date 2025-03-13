/* 
 * Operating Systems  (2INCO)  Practical Assignment
 * Interprocess Communication
 *
 * Simeon_Vazharov (1988077)
 * Plamen_Nikolov (1960059)
 * Valeri_Kitipov (1993313)
 *
 * Grading:
 * Your work will be evaluated based on the following criteria:
 * - Satisfaction of all the specifications
 * - Correctness of the program
 * - Coding style
 * - Report quality
 * - Deadlock analysis
 */

#ifndef MESSAGES_H
#define MESSAGES_H

// define the data structures for your messages here
typedef struct
{
    int job, data, service;
} MQ_MESSAGE;

#endif
