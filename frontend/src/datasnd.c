#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <limits.h>
#include <sys/time.h>
#include <errno.h>
#include <zlog.h>
#include <iniparser.h>
#include "defines.h"
#include "list.h"
#include "queue.h"
#include "xml.h"
#include "http.h"

typedef struct _EMDC_datasender_globals
{
        char EMDC_HOME[PATH_MAX];
        char init_file_path[PATH_MAX];
        char log_conf_file_path[PATH_MAX];
	mqd_t queue_in;
        mqd_t queue_out;
	char data_rec_address[512];
	int sleep_time;
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

	s = iniparser_getstring(ini, "DATASENDER:DATA_REC_ADDRESS", NULL);
        if (s == NULL)
        {
                zlog_fatal (c, "cannot find entry DATASENDER:DATA_REC_ADDRESS in ini file [%s]", globals.init_file_path);
                exit(-1);
        }
	zlog_info (c, "DATASENDER:DATA_REC_ADDRESS = %s", s);
        snprintf (globals.data_rec_address, sizeof (globals.data_rec_address) - 1, "%s/EMDC/DataReceiver", s);
        zlog_info (c, "Receiver servlet url = %s", globals.data_rec_address);

	ii = iniparser_getint (ini, "DATASENDER:SLEEP_TIME", -1);
	if (ii == -1)
	{	
		globals.sleep_time = 10;
		zlog_warn (c, "cannot find entry DATASENDER:SLEEP_TIME in ini file [%s], assuming value of %d",
		globals.init_file_path, globals.sleep_time);
	}
	else
	{
		globals.sleep_time = ii;
		zlog_info (c, "DATASENDER:SLEEP_TIME = %d", globals.sleep_time);	
	}

	s = iniparser_getstring(ini, "DATASENDER:USER", NULL);
	if (s == NULL)
        {
                zlog_fatal (c, "cannot find entry DATASENDER:USER in ini file [%s]", globals.init_file_path);
                exit(-1);
        }
        strncpy (globals.user, s, sizeof (globals.user) - 1);
        zlog_info (c, "DATASENDER:USER = %s", globals.user);

	s = iniparser_getstring(ini, "DATASENDER:PASSWORD", NULL);
        if (s == NULL)
        {
                zlog_fatal (c, "cannot find entry DATASENDER:PASSWORD in ini file [%s]", globals.init_file_path);
                exit(-1); 
        }
        strncpy (globals.password, s, sizeof (globals.password) - 1);
        zlog_info (c, "DATASENDER:PASSWORD = %s", globals.password);


        /* open the sending message queue */
        globals.queue_out = EMDC_queue_init (EMDC_QUEUE_IN_NAME, O_WRONLY, 1, -1, -1);
	/* open the receiving message queue */
	globals.queue_in = EMDC_queue_init (EMDC_QUEUE_OUT_NAME, O_RDONLY, 0, -1, -1);
	EMDC_xml_init ();
	EMDC_http_init ();
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
                memset (buffer_in, '\x0', msg_length_in);
		short on_queue = 1; 
		while (on_queue > 0 && EMDC_queue_rcv (globals.queue_in, buffer_in, msg_length_in) >= 0)
                {
				short more; 
				short samples;
				on_queue = EMDC_get_msg_on_queue (globals.queue_in);
				zlog_debug (c, "currently on queue [%d] message(s)", on_queue); 
				zlog_info (c, "received msg from dbmgr: %s", buffer_in);
				more = more_to_fetch (buffer_in);
				samples = has_samples (buffer_in);
				if (samples)
				{
					send_message_remote(buffer_in);
				}
				else
				{
					zlog_info (c, "nothing to send");
				}
				if (!more && on_queue == 0)
				{
					usleep (globals.sleep_time * 1000);
				}
				else
				{
					zlog_info (c, "more to send ...");
				}
                }
		if (go)
		{
			zlog_debug (c, "running ...");
			send_select_request ();
		}
        }
	free (buffer_in);
        zlog_debug (c, "exiting main loop");
}

int fini ()
{
        if (signum != 0)
        {
                zlog_info(c, "got signal [%d], %s", signum, strsignal(signum));
        }
	EMDC_http_release();
	EMDC_xml_release ();
	EMDC_queue_release (globals.queue_in);
	EMDC_queue_release (globals.queue_out);
        zlog_info (c, "datasnd exits");
        zlog_fini ();
        return 0;
}

int send_select_request ()
{
	EMDCmsg* m = init_message (EMDCrequest);
        EMDCsamples *ss = init_samples (EMDCselect, -1);
        add_samples (m, ss);

        char *xmlRet = EMDC_xml_build (m);
        zlog_info (c, "sending msg to dbmgr: %s", xmlRet);
	int ret = EMDC_queue_send (globals.queue_out, xmlRet);
        if (ret != 0)
        {
                if (errno == EAGAIN)
                {
                        zlog_fatal (c, "queue is full. Exiting");
                }
                else
                {
                        zlog_fatal (c, "error %d [%s] in mq_send()", errno, strerror(errno));
                }
                exit (-1);
        }
	free (xmlRet);
	free_message (m);
	free (m);
}

void send_message_remote (const char* msg)
{
	char* ret_data;
	EMDC_http_post (globals.data_rec_address, globals.user, globals.password, msg, &ret_data);
	if (ret_data == NULL)
        {
        	zlog_warn (c, "no data");
		return;
       	} 
        zlog_info (c, "bytes_read [%d]", EMDC_http_get_bytes_read());
        char* str = (char*) malloc (EMDC_http_get_bytes_read() + 1);
        memset (str, '\x0', EMDC_http_get_bytes_read() + 1);
        memcpy (str, ret_data, EMDC_http_get_bytes_read());
	zlog_info (c, "msg received from server %s, sending to dbmgr", str);
	int ret = EMDC_queue_send (globals.queue_out, str);
        if (ret != 0)
        {
                if (errno == EAGAIN)
                {
                        zlog_fatal (c, "queue is full. Exiting");
                }
                else
                {
                        zlog_fatal (c, "error %d [%s] in mq_send()", errno, strerror(errno));
                }
                exit (-1);
        }
        free (str);
        free (ret_data);
}

/* controlla brutalmente se nel messaggio e' valorizzato l'attributo "more"
   del nodo Samples. In caso ci siano ancora record da elabolare non effettua la sleep 
 */
short more_to_fetch (const char* msg)
{
	if (strstr (msg, "more=\"yes\""))
	{
		return 1;
	}
	return 0;
}

/* controlla brutalmente se nel messaggio rirornato da dbmgr e' presente almeno
   un nodo <Sample>. Nel caso il messaggio non contenga alcun nodo <Sample> non
   effettual l'invio dei dati
 */
short has_samples (const char* msg)
{
	if (strstr (msg, "<Sample "))
	{
		return 1;
	}
	return 0;
}
