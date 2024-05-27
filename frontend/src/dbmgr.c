#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <errno.h>
#include <zlog.h>
#include <iniparser.h>

#include "defines.h"
//#include "list.h"
#include "sample.h"
#include "queue.h"
#include "sql.h"
#include "my_json.h"

zlog_category_t *c = NULL;

typedef struct _EMDC_dbmgr_globals
{
	char EMDC_HOME[PATH_MAX];
	char init_file_path[PATH_MAX];
	char log_conf_file_path[PATH_MAX];
	char db_file_path[PATH_MAX];
	mqd_t queue_in;
	mqd_t queue_out;
	int max_commit;
	int commit_interval;

} EMDC_dbmgr_globals;

static int go = 1;
static int signum = 0;
static EMDC_dbmgr_globals globals;
static int commit_count = 0;
static int do_commit_on_timer = 0;
static int max_samples;

int init ();
int init_timer ();
int main_loop ();
int fini ();
int process_msg (const char* msg);
int process_msg_insert (EMDCsample* sample);
int process_msg_update (EMDCsample* sample);
//int process_msg_select (EMDCsample* sample);

void timer_handler(int sig, siginfo_t *si, void *uc)
{
	do_commit_on_timer = 1;
}

void signal_callback_handler(int sgnm)
{
	go = 0;
	signum = sgnm;
}

int main (int argc, char *argv[])
{
	init ();
	main_loop ();
	fini ();
	return 0;
}

int init ()
{
	int rc;
	const char* s;
	char* emdc_home;
	dictionary* ini = NULL;

	memset ((void*) &globals, '\x0', sizeof (EMDC_dbmgr_globals));

	emdc_home = getenv ("EMDC_HOME");
	if (emdc_home == NULL || strlen(emdc_home) == 0)
	{
		printf ("Environment variable \"EMDC_HOME\" undefined. Exiting\n");
		exit (-1);
	}
	strncpy (globals.EMDC_HOME, emdc_home, sizeof (globals.EMDC_HOME) - 1);
	printf ("EMDC_HOME          [%s]\n", globals.EMDC_HOME);

	snprintf (globals.log_conf_file_path, sizeof (globals.log_conf_file_path) - 1, "%s/etc/log.conf", globals.EMDC_HOME)
	;
	printf ("log conf file path [%s]\n", globals.log_conf_file_path);

	rc = zlog_init(globals.log_conf_file_path);
        if (rc)
        {
                printf ("zlog_init() failed. Exiting\n");
                exit (-1);
        }

        c = zlog_get_category("dbmgr");
        if (!c)
        {
                printf("zlog_get_category() for \"dbmgr\" failed. Exiting\n");
                zlog_fini();
        }

	zlog_info (c, "dbmgr starting");
	snprintf (globals.init_file_path, sizeof (globals.init_file_path) - 1, "%s/etc/emdc.conf", globals.EMDC_HOME);
        zlog_info(c, "init file path [%s]", globals.init_file_path);
	ini = iniparser_load(globals.init_file_path);
        if (ini == NULL)
        {
                zlog_fatal (c, "cannot parse ini file [%s]", globals.init_file_path);
                exit(-1);
        }

	/* open the receiving message queue */
	globals.queue_in = EMDC_queue_init (EMDC_QUEUE_IN_NAME, O_RDONLY, 0, 5000, 8192);
	/* open the sending message queue */
	globals.queue_out = EMDC_queue_init (EMDC_QUEUE_OUT_NAME, O_WRONLY, 1, 1, 8192);
	/*
                calcolo aprossimativo per determinare il massimo numero di campioni
                che si possono inserire in una risposta a un comando si "SELECT":

                110 lunglezza di
                <?xml version="1.0" encoding="UTF-8"?>
                <Message type="response"><Samples action="SELECT"></Samples></Message>

                49 lunghezza di (aprox 50)
                <Sample dc_id="0" rarr="0">1366125139808</Sample>
        */
	max_samples = (EMDC_get_queue_msg_length (globals.queue_out) - 110) / 50;
	zlog_info (c, "max samples in message is %d", max_samples);
	if (max_samples <= 0)
	{
		zlog_fatal (c, "queue message length too small. Exiting");
		exit(-1);
	}
	short in_memory = 0;
	s = iniparser_getstring(ini, "DBMGR:IN_MEMORY", "NO");
	if ( !strcmp (s, "YES") || !strcmp (s, "yes") )
	{
		in_memory = 1;
		zlog_info (c, "using in memory database");
	}
	else
	{
		s = iniparser_getstring(ini, "DBMGR:DB_FILE_PATH", NULL);
		if (s == NULL)
		{
			zlog_fatal (c, "cannot find entry [DBMGR]DB_FILE_PATH in ini file [%s]", globals.init_file_path);
                	exit(-1);
		}
		strncpy (globals.db_file_path, s, sizeof (globals.db_file_path) - 1);
        	zlog_info (c, "[DBMGR]DB_FILE_PATH = %s", globals.db_file_path);
	}
	rc = EMDC_sql_init (globals.db_file_path, in_memory);
	if (rc)
	{
		zlog_fatal (c, "error in EMDC_sql_init(), exiting ...");
		exit (-1);
	}
	globals.max_commit =  iniparser_getint (ini, "DBMGR:COMMIT_COUNT", 100);
	zlog_info (c, "[DBMGR]COMMIT_COUNT = %d", globals.max_commit);
	globals.commit_interval = iniparser_getint (ini, "DBMGR:COMMIT_TIMER", 10);
	zlog_info (c, "[DBMGR]COMMIT_TIMER = %d", globals.commit_interval);
	signal(SIGINT, signal_callback_handler);
	signal(SIGTERM, signal_callback_handler);
	if (globals.max_commit > 1)
	{
		init_timer ();
	}
	iniparser_freedict(ini);
	zlog_info(c, "dbmgr started");
	return 0;
}

int init_timer ()
{
	/*
	time_t                  t;
        long                    l;
	*/
        timer_t                 timerid;
        struct itimerspec       value;
        struct sigevent         sev;
        struct sigaction        sa;

        /*
	t = time(NULL);
        l = t % 60;
        printf ("\n%d\n", l);
	*/
        value.it_value.tv_sec = globals.commit_interval;
        value.it_value.tv_nsec = 0;

        value.it_interval.tv_sec = globals.commit_interval;
        value.it_interval.tv_nsec = 0;

        sa.sa_flags = SA_SIGINFO;
        sa.sa_sigaction = timer_handler;
        sigemptyset(&sa.sa_mask);

        if (sigaction(SIGRTMIN, &sa, NULL) == -1)
        {
	       zlog_error (c, "error %d [%s] in sigaction()", errno, strerror(errno));
               return -1;
        }

        sev.sigev_notify = SIGEV_SIGNAL;
        sev.sigev_signo = SIGRTMIN;
        sev.sigev_value.sival_ptr = &timerid;

        if (timer_create(CLOCK_REALTIME, &sev, &timerid) == -1)
        {
               zlog_error (c, "error %d [%s] in timer_create()", errno, strerror(errno));
               return -1;
        }

        if (timer_settime (timerid, 0, &value, NULL) == -1)
        {
		zlog_error (c, "error %d [%s] in timer_settime()", errno, strerror(errno));
                return -1;
	}
	zlog_info (c, "init timer ok");
}

int main_loop ()
{
	int msg_length = EMDC_get_queue_msg_length (globals.queue_in);
	char* buffer_in = (char*) malloc (msg_length);
	while (go)
	{
		memset ((void*) buffer_in, '\x0', msg_length);
		int ret = EMDC_queue_rcv (globals.queue_in, buffer_in, msg_length);
		if (ret >= 0)
		{
			zlog_info (c, "received message, start processing ...");
			zlog_debug (c, "msg: %s", buffer_in);
			process_msg (buffer_in);
		}
		if (do_commit_on_timer == 1)
		{
			zlog_debug (c, "commit on timer - commit_count is %d", commit_count);
			if (commit_count > 0)
			{
				int ret = EMDC_sql_commit_tnx();
                        	if (!ret)
                        	{
                        		commit_count = 0;
                        	}
			}
			do_commit_on_timer = 0;
		}
	}
	free (buffer_in);
	zlog_debug (c, "exiting main loop");
}

int fini ()
{
	int ret;
	if (signum != 0)
	{
		zlog_info(c, "got signal [%d], %s", signum, strsignal(signum));
	}
	if (commit_count > 0)
	{
		EMDC_sql_commit_tnx();
	}
	ret = EMDC_sql_release ();
	if (ret)
	{
		zlog_error (c, "error in EMDC_sql_release()");
	}
	EMDC_queue_release (globals.queue_in);
	EMDC_queue_release (globals.queue_out);
	zlog_info (c, "dbmgr exits");
	zlog_fini ();
	return 0;
}

int process_msg (const char* str)
{
	EMDCsample* sample = (EMDCsample*) malloc(sizeof(EMDCsample));
        sample_from_json(sample, str);
	if (sample != NULL)
	{
		if (sample->status == STATUS_TO_DELIVER)
		{
			// save sample in local db
			zlog_info (c, "inserting new sample message ...");
			process_msg_insert (sample);
                        // send sample to EMDCpublisher
                        EMDC_queue_send (globals.queue_out, str);
		}
		else if (sample->status == STATUS_DELIVERED || sample->status == STATUS_NOT_DELIVERED)
		{
			// update status to DELIVERED or NOT_DELIVERED in local db
			zlog_info (c, "updating sample message with status %s ...", sample->status == STATUS_DELIVERED ? "DELIVERED" : "NOT_DELIVERED");
                        process_msg_update (sample);
		}

	}
	return 0;
}

int process_msg_insert (EMDCsample* sample)
{
	int ret;
	if (commit_count == 0)
	{
		EMDC_sql_begin_tnx();
	}
        EMDC_sql_insert(sample->ts, sample->dc_id, sample->rarr, sample->status);
	commit_count++;
	if (commit_count == globals.max_commit)
        {
                ret = EMDC_sql_commit_tnx();
                if (!ret)
                {
                        commit_count = 0;
                }
        }
	zlog_info (c, "done !");
	return 0;
}

int process_msg_update (EMDCsample* sample)
{
        return 0;
}

/*
int process_msg_select (EMDCsamples* ss)
{
	int ret;
	EMDCsample *s = ss->head;
	if (commit_count > 0)
	{
		ret = EMDC_sql_commit_tnx();
                if (!ret)
                {
                	commit_count = 0;
                }
	}
	if (s != NULL)
	{

		 // da implementare: restutuisce solo i campioni che sono passati in input
		 // se sono presenti nel data base

	}
	else
	{

		zlog_debug (c, "select all");
		EMDCmsg* m = EMDC_sql_select (max_samples);
		char *xmlRet = EMDC_xml_build (m);
		EMDC_queue_send (globals.queue_out, xmlRet);
        	zlog_info (c, "enqueuing message to datasender: %s", xmlRet);
        	free (xmlRet);
        	free_message (m);
        	free (m);
	}
	zlog_info (c, "done !");
	return 0;
}
*/
