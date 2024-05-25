#include "queue.h"

#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>           /* For FILE_MODE constants */
#include "zlog.h"

#define FILE_MODE               (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)
#define MIN_MSG_LENGTH		140

extern zlog_category_t* c;

mqd_t EMDC_queue_init (const char* queue_name, int flags, int blocking_mode, int maxmsg, int msgsize)
{
	int ret;	
	struct mq_attr attr;
	mqd_t mqd;
        /*
	memset (&attr, '\x0', sizeof(struct mq_attr));
        attr.mq_maxmsg = QUEUE_LENGTH;
        attr.mq_msgsize = MSG_LENGTH;
	*/
	zlog_info (c, "opening queue %s", queue_name);
	int f = flags | O_CREAT;
	if (blocking_mode)
	{
		zlog_info (c, "queue will be opened in non-blocking mode");
		f = f | O_NONBLOCK;
	}  

	/* if the queue does not still exists, it is created */	
	if (maxmsg != -1 && msgsize != -1)
	{
		memset (&attr, '\x0', sizeof(struct mq_attr));
        	attr.mq_maxmsg = maxmsg;
        	attr.mq_msgsize = msgsize;
		mqd = mq_open (queue_name, f, FILE_MODE, &attr);
	}
	else
	{
		mqd = mq_open (queue_name, f, FILE_MODE, NULL);
	}
        if (mqd == (mqd_t) -1)
	{
		zlog_fatal (c, "error %d [%s] opening queue. Exiting", errno, strerror(errno));
                exit(-1);
        }
	
	/* check if queue has the minimum message size */
	ret = mq_getattr (mqd, &attr);
        if (ret == -1)
        {
                zlog_fatal (c, "error %d [%s] in mq_getattr(). Exiting", errno, strerror(errno));
                exit(-1);
        }

        zlog_info (c, "max #msgs = %ld, max #bytes/msg = %ld, #currently on queue = %ld",
                attr.mq_maxmsg, attr.mq_msgsize, attr.mq_curmsgs);

	if (attr.mq_msgsize < MIN_MSG_LENGTH)
	{
		zlog_fatal (c, "queue message length smaller than minimum allowed, at least must be %d. Exiting", MIN_MSG_LENGTH);
                exit(-1);
	}
	zlog_info (c, "queue %s opened OK, descriptor [%d]", queue_name, mqd);
	return mqd;

}

void EMDC_queue_release (mqd_t q)
{
	int ret = mq_close (q);
        if (ret == -1)
        {
                zlog_error (c, "error %d [%s] closing queue [%d]", errno, strerror(errno), q);
                exit(-1);
        }
        zlog_info (c, "queue [%d] closed OK", q);
}

int EMDC_get_queue_msg_length (mqd_t q)
{
	struct mq_attr attr;
	int ret = mq_getattr (q, &attr);
        if (ret == -1)
        {
                zlog_error (c, "error %d [%s] in mq_getattr()", errno, strerror(errno));
                return ret;
        }
	return attr.mq_msgsize;

}

int EMDC_get_msg_on_queue (mqd_t q)
{
        struct mq_attr attr;
        int ret = mq_getattr (q, &attr);
        if (ret == -1)
        {
                zlog_error (c, "error %d [%s] in mq_getattr()", errno, strerror(errno));
                return ret;
        }
        return attr.mq_curmsgs;
}


int EMDC_queue_send (mqd_t q, const char* buff)
{
	int ret = mq_send (q, buff, strlen(buff), 0);
	if (ret == -1)
        {
                zlog_error (c, "error %d [%s] in mq_send()", errno, strerror(errno));
        }
        return ret;
}

int EMDC_queue_rcv (mqd_t q, char* buff, size_t len)
{
	struct timespec ts;
	ts.tv_sec = time(NULL) + 10; 
	ts.tv_nsec = 0;

	ssize_t ret = mq_timedreceive(q, buff, len, 0, &ts);
	if (ret == -1)
	{
		if (errno == ETIMEDOUT)
		{
			zlog_debug (c, "timeout expired waiting for message");
		}
		else if (errno == EINTR)
		{
			zlog_debug (c, "system call was interrupted by a signal handler"); 
		}
		else
		{
			zlog_error (c, "error %d [%s] in mq_timedreceive()", errno, strerror(errno));
		}
	}
	return ret;
}

