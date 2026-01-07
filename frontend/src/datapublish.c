#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <limits.h>
#include <time.h>
#include <sys/time.h>
#include <errno.h>
#include <zlog.h>
#include <iniparser.h>
#include "defines.h"
#include "queue.h"
#include "my_json.h"
#include "my_amqp.h"

#define RETRY_CONNECTION_TIME (10)

typedef struct _EMDC_datapublish_globals
{
        char EMDC_HOME[PATH_MAX];
        char init_file_path[PATH_MAX];
        char log_conf_file_path[PATH_MAX];
	mqd_t queue_in;
        mqd_t queue_out;
	char broker_address[512];
	char user[65];
	char password[64];
        AMQP_Ctx *ctx;

} EMDC_datapublish_globals;

static int go = 1;
static int signum = 0;
static EMDC_datapublish_globals globals;
static int connected_to_broker = 0;

int init ();
int main_loop ();
int fini ();
int publish_message (const char* msg);
int retry_connect ();
int init_timer ();

void signal_callback_handler(int sgnm)
{
        go = 0;
        signum = sgnm;
}

void timer_handler(int sig, siginfo_t *si, void *uc)
{
        int ret = AMQP_Init(globals.ctx, globals.broker_address, 5672, globals.user, globals.password);
        if (ret != 0)
        {
                retry_connect();
        }
        else
        {
                connected_to_broker = 1;
        }
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
	const char *s;
	int ii;

        memset ((void*) &globals, '\x0', sizeof (EMDC_datapublish_globals));

        emdc_home = getenv ("EMDC_HOME");
        if (emdc_home == NULL || strlen(emdc_home) == 0)
        {
                printf ("Environment variable \"EMDC_HOME\" undefined. Exiting\n");
                exit (-1);
        }
        strncpy (globals.EMDC_HOME, emdc_home, sizeof (globals.EMDC_HOME) - 1);
        printf ("EMDC_HOME          [%s]\n", globals.EMDC_HOME);

        snprintf (globals.log_conf_file_path, sizeof (globals.log_conf_file_path) - 1, "%s/etc/log.conf", globals.EMDC_HOME);
        printf ("log conf file path [%s]\n", globals.log_conf_file_path);

        rc = zlog_init(globals.log_conf_file_path);
        if (rc)
        {
                printf ("zlog_init() failed. Exiting\n");
                exit (-1);
        }

        c = zlog_get_category("datapublish");
        if (!c)
        {
                printf("zlog_get_category() for \"datapublish\" failed. Exiting\n");
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

	s = iniparser_getstring(ini, "DATAPUBLISH:BROKER_ADDRESS", NULL);
        if (s == NULL)
        {
                zlog_fatal (c, "cannot find entry DATAPUBLISH:BROKER_ADDRESS in ini file [%s]", globals.init_file_path);
                exit(-1);
        }
	zlog_info (c, "DATAPUBLISH:BROKER_ADDRESS = %s", s);
        strncpy (globals.broker_address, s, sizeof (globals.broker_address) - 1);

	s = iniparser_getstring(ini, "DATAPUBLISH:USER", NULL);
	if (s == NULL)
        {
                zlog_fatal (c, "cannot find entry DATAPUBLISH:USER in ini file [%s]", globals.init_file_path);
                exit(-1);
        }
        strncpy (globals.user, s, sizeof (globals.user) - 1);
        zlog_info (c, "DATAPUBLISH:USER = %s", globals.user);

	s = iniparser_getstring(ini, "DATAPUBLISH:PASSWORD", NULL);
        if (s == NULL)
        {
                zlog_fatal (c, "cannot find entry DATAPUBLISH:PASSWORD in ini file [%s]", globals.init_file_path);
                exit(-1);
        }
        strncpy (globals.password, s, sizeof (globals.password) - 1);
        zlog_info (c, "DATAPUBLISH:PASSWORD = %s", globals.password);


        /* open the sending message queue */
        globals.queue_out = EMDC_queue_init (EMDC_QUEUE_IN_NAME, O_WRONLY, 1, -1, -1);
	/* open the receiving message queue */
	globals.queue_in = EMDC_queue_init (EMDC_QUEUE_OUT_NAME, O_RDONLY, 0, -1, -1);
        /* connect to broker */
	globals.ctx = (AMQP_Ctx*)malloc(sizeof(AMQP_Ctx));
        int ret = AMQP_Init(globals.ctx, globals.broker_address, 5672, globals.user, globals.password);
        if (ret != 0)
        {
		zlog_error (c, "error connecting to broker");
		retry_connect();
        }
	else
	{
		connected_to_broker = 1;
	}

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
                memset ((void*) buffer_in, '\x0', msg_length_in);
                int ret = EMDC_queue_rcv (globals.queue_in, buffer_in, msg_length_in);
                if (ret >= 0)
                {
                    zlog_info (c, "received message, start processing ...");
                    zlog_debug (c, "msg: %s", buffer_in);
                    // invia messaggio a broker
                    // se messaggio consegnato accoda alla coda qlobals.qout messaggio con stato DELIVERED
                    // altrimenti con stato UNDELIVERED
		    publish_message (buffer_in);
                }
                else
		{
		    zlog_debug (c, "no message in queue");
		}
        }
        free (buffer_in);
        zlog_debug (c, "exiting main loop");
}


int publish_message (const char* str)
{
    int ret;
    char buffer[1024];
    const char *routing_key = NULL;

    EMDCsample* sample = (EMDCsample*) malloc(sizeof(EMDCsample));
    if (!sample) {
        zlog_error(c, "malloc failed for EMDCsample");
        return -1;
    }

    /* Parse incoming JSON */
    sample_from_json(sample, str);

    /* ---- Canonical field enrichment (legacy-compatible) ---- */

    /* device_id */
    if (sample->device_id[0] == '\0') {
        snprintf(sample->device_id, sizeof(sample->device_id),
                 "raspberry.emdc.%d", sample->dc_id);
    }

    /* event_type, value, unit */
    if (sample->event_type[0] == '\0') {
        if (sample->rarr == 0) {
            strncpy(sample->event_type, "energy.active",
                    sizeof(sample->event_type) - 1);
            sample->value = 0.001;
            strncpy(sample->unit, "kWh",
                    sizeof(sample->unit) - 1);
        } else {
            strncpy(sample->event_type, "energy.reactive",
                    sizeof(sample->event_type) - 1);
            sample->value = 0.001;
            strncpy(sample->unit, "kWh",
                    sizeof(sample->unit) - 1);
        }

        /* enforce null-termination */
        sample->event_type[sizeof(sample->event_type) - 1] = '\0';
        sample->unit[sizeof(sample->unit) - 1] = '\0';
    }

    /* source */
    if (sample->source[0] == '\0') {
        strncpy(sample->source, "datapublish",
                sizeof(sample->source) - 1);
        sample->source[sizeof(sample->source) - 1] = '\0';
    }

    /* ---- Routing decision ---- */

    if (strcmp(sample->event_type, "energy.active") == 0) {
        routing_key = "EMDC.EVENTS.ACTIVE";
    }
    else if (strcmp(sample->event_type, "energy.reactive") == 0) {
        routing_key = "EMDC.EVENTS.REACTIVE";
    }
    else {
        routing_key = "EMDC.EVENTS.OTHER";
    }

    /* ---- Publish ---- */

    if (connected_to_broker == 1) {

        ret = AMQP_Sendmessage(globals.ctx, "EMDC", routing_key, str);

        if (ret != 0) {
            zlog_error(c, "error publishing message to broker");
            connected_to_broker = 0;
            retry_connect();
        }
        else {
            zlog_info(c, "message published to %s", routing_key);

            /* mark delivered and re-serialize enriched event */
            sample->status = STATUS_DELIVERED;
            sample_to_json(sample, buffer);

            EMDC_queue_send(globals.queue_out, buffer);
        }
    }

    free(sample);
    return 0;
}


int retry_connect ()
{
        timer_t                 timerid;
        struct itimerspec       value;
        struct sigevent         sev;
        struct sigaction        sa;

        value.it_value.tv_sec = RETRY_CONNECTION_TIME;
        value.it_value.tv_nsec = 0;

        value.it_interval.tv_sec = 0;
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
        zlog_debug (c, "retrying connection to broker in %d seconds", RETRY_CONNECTION_TIME);
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

