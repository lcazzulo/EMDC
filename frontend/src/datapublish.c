#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <limits.h>
#include <sys/time.h>
#include <errno.h>
#include <zlog.h>
#include <iniparser.h>
#include "defines.h"
#include "queue.h"


typedef struct _EMDC_datasender_globals
{
        char EMDC_HOME[PATH_MAX];
        char init_file_path[PATH_MAX];
        char log_conf_file_path[PATH_MAX];
	mqd_t queue_in;
        mqd_t queue_out;
	char broker_address[512];
	char user[65];
	char password[64];

} EMDC_datasender_globals;

static int go = 1;
static int signum = 0;
static EMDC_datasender_globals globals;

int init ();
int main_loop ();
int fini ();
int send_select_request ();
void send_message_remote (const char* msg);
short more_to_fetch (const char* msg);
short has_samples (const char* msg);

void signal_callback_handler(int sgnm)
{
        go = 0;
        signum = sgnm;
}


zlog_category_t *c = NULL;

int main (int argc, char *atgv[])
{
	init ();
        main_loop ();
        fini ();
	return 0;
}

int init ()
{
        int rc;
        char* emdc_home;
	dictionary* ini = NULL;
	char *s;
	int ii;

        memset ((void*) &globals, '\x0', sizeof (EMDC_datasender_globals));

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

        c = zlog_get_category("datasnd");
        if (!c)
        {
                printf("zlog_get_category() for \"datasnd\" failed. Exiting\n");
                zlog_fini();
        }

	snprintf (globals.init_file_path, sizeof (globals.init_file_path) - 1, "%s/etc/emdc.conf", globals.EMDC_HOME);
        zlog_info(c, "init file path [%s]", globals.init_file_path);
        ini = iniparser_load(globals.init_file_path);
        if (ini == NULL)
        {
                zlog_fatal (c, "cannot parse ini file [%s]", globals.init_file_path);
                exit(-1);
        }

	s = iniparser_getstring(ini, "DATAPUBLISHER:BROKER_ADDRESS", NULL);
        if (s == NULL)
        {
                zlog_fatal (c, "cannot find entry DATAPUBLISHER:BROKER_ADDRESS in ini file [%s]", globals.init_file_path);
                exit(-1);
        }
	zlog_info (c, "DATAPUBLISHER:BROKER_ADDRESS = %s", s);
        strncpy (globals.data_rec_address, s, sizeof (globals.data_rec_address) - 1);

	s = iniparser_getstring(ini, "DATAPUBLISHER:USER", NULL);
	if (s == NULL)
        {
                zlog_fatal (c, "cannot find entry DATAPUBLISHER:USER in ini file [%s]", globals.init_file_path);
                exit(-1);
        }
        strncpy (globals.user, s, sizeof (globals.user) - 1);
        zlog_info (c, "DATASENDER:USER = %s", globals.user);

	s = iniparser_getstring(ini, "DATAPUBLISHER:PASSWORD", NULL);
        if (s == NULL)
        {
                zlog_fatal (c, "cannot find entry DATAPUBLISHER:PASSWORD in ini file [%s]", globals.init_file_path);
                exit(-1); 
        }
        strncpy (globals.password, s, sizeof (globals.password) - 1);
        zlog_info (c, "DATAPUBLISHER:PASSWORD = %s", globals.password);


        /* open the sending message queue */
        globals.queue_out = EMDC_queue_init (EMDC_QUEUE_IN_NAME, O_WRONLY, 1, -1, -1);
	/* open the receiving message queue */
	globals.queue_in = EMDC_queue_init (EMDC_QUEUE_OUT_NAME, O_RDONLY, 0, -1, -1);
        /* connect to broker */
        signal(SIGINT, signal_callback_handler);
        signal(SIGTERM, signal_callback_handler);
        zlog_info(c, "datasnd started");
        return 0;
}

int main_loop ()
{
	int ret;
	int msg_length_in = EMDC_get_queue_msg_length (globals.queue_in);
	char* buffer_in = (char*) malloc (msg_length_in);

	while (go)
        {
                // preleva i messaggi dalla coda globals.qin
		// invia messaggio a broker
		// se messaggio consegnato accoda alla coda qlobals.qout messaggio con stato DELIVERED
                // altrimenti con stato UNDELIVERED
        }

        zlog_debug (c, "exiting main loop");
}

int fini ()
{
        if (signum != 0)
        {
                zlog_info(c, "got signal [%d], %s", signum, strsignal(signum));
        }


	EMDC_queue_release (globals.queue_in);
	EMDC_queue_release (globals.queue_out);
        zlog_info (c, "datapublish exits");
        zlog_fini ();
        return 0;
}

