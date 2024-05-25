#ifndef _EMDC_QUEUE_H_
#define _EMDC_QUEUE_H_

#include <mqueue.h>

mqd_t EMDC_queue_init (const char* queue_name, int flags, int dont_block, int maxmsg, int msgsize);
void EMDC_queue_release (mqd_t q);
int EMDC_get_queue_msg_length (mqd_t q);
int EMDC_get_msg_on_queue (mqd_t q);
int EMDC_queue_send (mqd_t q, const char* buff);
int EMDC_queue_rcv (mqd_t q, char* buff, size_t len);

#endif
