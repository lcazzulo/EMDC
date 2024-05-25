#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <limits.h>
#include <sys/time.h>
#include <sys/types.h>
#include <errno.h>
#include <pwd.h>
#include <grp.h>
#include <getopt.h>
#include <zlog.h>
#include <iniparser.h>
#include <wiringPi.h>
#include "defines.h"
#include "list.h"
#include "queue.h"

#define MIN_INTERVAL_MILLI 1000
#define MAX_INTERVAL_MILLI 30000

typedef struct _EMDC_datacap_globals
{
        char EMDC_HOME[PATH_MAX];
        char init_file_path[PATH_MAX];
        char log_conf_file_path[PATH_MAX];
        mqd_t queue_out;
	int dc_id;
	int gpio_ra;
	int gpio_rr;
	char test_mode;
	int min_test_interval_milli;
	int max_test_interval_milli;

} EMDC_datacap_globals;

static int go = 1;
static int signum = 0;
static EMDC_datacap_globals globals;
static int ra_pin_status;
static int rr_pin_status;

int init (int argc, char *argv[]);
int main_loop ();
int sendmsg(int id, int rarr);
int fini ();
void process_cmd_line (int argc, char *argv[]);
void process_test_arg ();
void process_dc_id_arg ();
void sleep_randomically ();
void wiringPi_callback_ra (void);
void wiringPi_callback_rr (void);


void signal_callback_handler(int sgnm)
{
        go = 0;
        signum = sgnm;
}


zlog_category_t *c = NULL;

int main (int argc, char *argv[])
{
	memset ((void*) &globals, '\x0', sizeof (EMDC_datacap_globals));
	process_cmd_line (argc, argv);

	if (globals.test_mode == 0)
	{
		/* setup wiringPi Library, NEED to be executed as root */
		wiringPiSetup();
		/*******************************************************/

		/* get back to non-privileged user *********************/
                int ret;
                char* p_group = getenv ("DC_GROUP");
                if (p_group != NULL)
                {
                        printf ("DC_GROUP [%s]\n", p_group);
                        struct group * grp = getgrnam(p_group);
                        gid_t gid = grp->gr_gid;
                        ret = setgid(gid);
			if (ret != 0)
                        { 
                                printf ("error %d [%s] in setgid()\n", errno, strerror(errno));
                        }
                }
		else
		{
			printf ("please set \"DC_GROUP\" environment variable\r\n"); 
		}
		char* p_user = getenv ("DC_USER");
                if (p_user != NULL)
                {
                        printf ("DC_USER [%s]\n", p_user);
                        struct passwd * pwd = getpwnam(p_user);
                        uid_t uid = pwd->pw_uid;
                        ret = setuid(uid);
                        if (ret != 0)
                        {
                                printf ("error %d [%s] in setuid()\n", errno, strerror(errno));
                        }
                }
		else
                {
                        printf ("please set \"DC_USER\" environment variable\r\n");
                }

                /*******************************************************/
	}
	init (argc, argv);
        main_loop ();
        fini ();
	return 0;
}

void process_cmd_line (int argc, char *argv[])
{
	int ch;
	static struct option long_options[] =
        {
        	{"dc-id", required_argument, 0, 'i'},
		{"test", optional_argument, 0, 't'},
                {0, 0, 0, 0}
        };

	while (1)
	{
		int option_index = 0;
           	ch = getopt_long (argc, argv, "i:t::", long_options, &option_index);
		/* Detect the end of the options. */
           	if (ch == -1)
		{
             		break;
		}
           	switch (ch)
		{
		case 'i':
			process_dc_id_arg ();
			break;
		case 't':
			process_test_arg ();
			break;
		case '?':
			/* printf ("bad command line option setting 1\n"); */
			exit (-1);
		default:
			/* printf ("bad command line option setting 2\n"); */
			exit (-1);
		}
	}
}

void process_test_arg ()
{
	globals.test_mode = 1;
	globals.min_test_interval_milli = MIN_INTERVAL_MILLI;
        globals.max_test_interval_milli = MAX_INTERVAL_MILLI;

	if (optarg != NULL)
	{
		printf ("option -t with value [%s]\n", optarg);
		char* pos = strchr (optarg, '-');
		if (pos && pos != optarg)
		{
			int l = pos - optarg;
			int s = strlen(optarg);
			if (l + 1 < s)
			{
				char* str1 = (char*) malloc (1 + 1);
				strncpy (str1, optarg, l);
				str1[l] = '\x0';
				int val1 = atoi (str1);
				free (str1);
				char* str2 = (char*) malloc (s - l);
				strcpy (str2, optarg + l + 1);
				int val2 = atoi (str2);
				free (str2);
				if (val2 >= val1)
				{
					globals.min_test_interval_milli = val1;
        				globals.max_test_interval_milli = val2;	
				}
			}
		}
		else if (pos == NULL)
		{
			globals.min_test_interval_milli = globals.max_test_interval_milli = atoi (optarg);
		}
	}
	printf ("globals.min_test_interval_milli [%d]\n", globals.min_test_interval_milli);
	printf ("globals.max_test_interval_milli [%d]\n", globals.max_test_interval_milli);
}

void process_dc_id_arg ()
{
	printf ("option -i with value %s\n", optarg);
	int i = atoi (optarg);
	if (i < 0 || i > 255)
	{
		printf ("--dc-id must be between 0 and 255\n");
                exit (-1);
	}
        globals.dc_id = i;
}

int init (int argc, char *argv[])
{
        int rc;
        char* emdc_home;
	char log_category[10];
	dictionary* ini = NULL;
	int ii;
	char key[128];

	if (globals.test_mode == 1)
	{
		srand(time(NULL));
	}

        emdc_home = getenv ("EMDC_HOME");
        if (emdc_home == NULL || strlen(emdc_home) == 0)
        {
                printf ("Environment variable \"EMDC_HOME\" undefined. Exiting\n");
                exit (-1);
        }

        strncpy (globals.EMDC_HOME, emdc_home, sizeof (globals.EMDC_HOME) - 1);
        printf ("EMDC_HOME          [%s]\n", globals.EMDC_HOME);

        snprintf (globals.log_conf_file_path, sizeof (globals.log_conf_file_path) - 1, 
		"%s/etc/log.conf", globals.EMDC_HOME);
        printf ("log conf file path [%s]\n", globals.log_conf_file_path);

        rc = zlog_init(globals.log_conf_file_path);
        if (rc)
        {
                printf ("zlog_init() failed. Exiting\n");
                exit (-1);
        }

	sprintf (log_category, "datacap%d", globals.dc_id);
	printf ("log_category [%s]\n", log_category);
        c = zlog_get_category(log_category);
        if (!c)
        {
                printf("zlog_get_category() for \"%s\" failed. Exiting\n", log_category);
                zlog_fini();
		exit (-1);
        }

	zlog_info(c, "datacap starting");

	if (globals.test_mode == 1)
	{
		zlog_info(c, "running in test mode.");
		zlog_info(c, "min_test_interval_milli=[%d]", globals.min_test_interval_milli);
		zlog_info(c, "max_test_interval_milli=[%d]", globals.max_test_interval_milli);
	}

	zlog_info(c, "dc_id=[%d]", globals.dc_id);

	snprintf (globals.init_file_path, sizeof (globals.init_file_path) - 1, "%s/etc/emdc.conf", globals.EMDC_HOME);
        zlog_info(c, "init file path [%s]", globals.init_file_path);
	ini = iniparser_load(globals.init_file_path);
        if (ini == NULL)
        {
                zlog_fatal (c, "cannot parse ini file [%s]", globals.init_file_path);
                exit(-1);
        }

	if (globals.test_mode == 0)
        {
		sprintf (key, "DATACAP%d:GPIO_PIN_RA", globals.dc_id);
		ii = iniparser_getint (ini, key, -1);
		if (ii == -1)
		{
			zlog_error (c, "cannot find entry %s in %s. Exiting", key, globals.init_file_path);
			exit (-1);
		}
		globals.gpio_ra = ii;
		zlog_info (c, "%s  [%d]", key,  globals.gpio_ra);

		sprintf (key, "DATACAP%d:GPIO_PIN_RR", globals.dc_id);
        	ii = iniparser_getint (ini, key, -1);
        	if (ii == -1)
        	{
                	zlog_error (c, "cannot find entry %s in %s. Exiting", key, globals.init_file_path);
                	exit (-1);
        	}
        	globals.gpio_rr = ii;
		zlog_info (c, "%s  [%d]", key,  globals.gpio_rr);
	}

        /* open the receiving message queue */
        globals.queue_out = EMDC_queue_init (EMDC_QUEUE_IN_NAME, O_WRONLY, 1, -1, -1);
        signal(SIGINT, signal_callback_handler);
        signal(SIGTERM, signal_callback_handler);

	if (globals.test_mode == 0)
	{
		pinMode (globals.gpio_ra, INPUT);
		zlog_info (c, "pin [%d] mode set to INPUT", globals.gpio_ra);
		ra_pin_status = digitalRead (globals.gpio_ra);
		zlog_info (c, "detected status %s for pin [%d]", ra_pin_status == LOW ? "LOW" : "HIGH", globals.gpio_ra);
		pinMode (globals.gpio_rr, INPUT);
		zlog_info (c, "pin [%d] mode set to INPUT", globals.gpio_rr);
		rr_pin_status = digitalRead (globals.gpio_rr);
        	zlog_info (c, "detected status %s for pin [%d]", rr_pin_status == LOW ? "LOW" : "HIGH", globals.gpio_rr);

		/* setup wiringPi callback */
        	rc = wiringPiISR (globals.gpio_ra, INT_EDGE_BOTH, &wiringPi_callback_ra);
		zlog_info(c, "wiringPiISR() for pin [%d] returned [%d]", globals.gpio_ra, rc);
		rc = wiringPiISR (globals.gpio_rr, INT_EDGE_BOTH, &wiringPi_callback_rr);
        	zlog_info(c, "wiringPiISR() for pin [%d] returned [%d]", globals.gpio_rr, rc);
	}
        zlog_info(c, "datacap started");
        return 0;
}

int main_loop ()
{
	int msg_length = EMDC_get_queue_msg_length (globals.queue_out);
        char* buffer_out = (char*) malloc (msg_length);
        while (go)
        {
		if (globals.test_mode == 0)
		{
			zlog_info (c, "waiting for events ...");
			sleep (10);
		}
		else
		{
			sendmsg (globals.dc_id, 0);
			sleep_randomically ();
		}
        }
        free (buffer_out);
        zlog_debug (c, "exiting main loop");
}

int fini ()
{
        if (signum != 0)
        {
                zlog_info(c, "got signal [%d], %s", signum, strsignal(signum));
        }
        zlog_info (c, "datacap exits");
        zlog_fini ();
        return 0;
}

int sendmsg (int id, int rarr)
{
	int ret;
	long long time_in_milli;
	struct timeval tv;
	struct timezone tz;

	ret = gettimeofday (&tv, &tz);

	if (ret == -1)
	{
		zlog_fatal (c, "error %d [%s] in gettimeofday()", ret, strerror(ret));
		exit(-1);
	}

	time_t seconds = tv.tv_sec;
	suseconds_t microsec = tv.tv_usec;

	zlog_debug (c, "seconds       [%d]", seconds);
	zlog_debug (c, "microseconds  [%d]", microsec);

	time_in_milli = ((unsigned long long)seconds) * 1000LL + ((unsigned long long)microsec) / 1000LL;

	zlog_debug (c, "time in milli [%lld]", time_in_milli);

	/*
	=== OLD VERSION ===
	EMDCmsg* m = init_message (EMDCrequest);
	EMDCsamples *ss = init_samples (EMDCinsert, -1);
	add_sample(ss, time_in_milli, id, rarr);
	add_samples (m, ss);

	char *xmlRet = EMDC_xml_build (m);
	zlog_info (c, "enqueuing msg to dbmgr: %s", xmlRet);
	ret = EMDC_queue_send (globals.queue_out, xmlRet);
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
	*/

	/*
	=== NEW VERSION
	send a json message to globals.queue_out
	*/
	return 0;
}

void sleep_randomically ()
{
	int v;
	if (globals.min_test_interval_milli != globals.max_test_interval_milli)
	{
		v = rand() % (globals.max_test_interval_milli - globals.min_test_interval_milli) + globals.min_test_interval_milli;
	}
	else
	{
		v = globals.min_test_interval_milli;
	}
	usleep (v * 1000L);
}

void wiringPi_callback_ra (void)
{
	int ra_pin_status_now = digitalRead (globals.gpio_ra);

	zlog_debug(c, "status detected now in pin %d is %s",  globals.gpio_ra, ra_pin_status_now == LOW ? "LOW" : "HIGH");

	if (ra_pin_status == LOW && ra_pin_status_now == HIGH)
	{
		zlog_info(c, "detected change LOW->HIGH in pin %d", globals.gpio_ra);
	}
	else if (ra_pin_status == HIGH && ra_pin_status_now == LOW)
	{
		zlog_info(c, "detected change HIGH->LOW in pin %d", globals.gpio_ra);
		sendmsg (globals.dc_id, 0);
	}
	else
	{
		zlog_info(c, "callback with no status change for pin %d", globals.gpio_ra);
	}
	ra_pin_status = ra_pin_status_now;
}

void wiringPi_callback_rr (void)
{
	int rr_pin_status_now = digitalRead (globals.gpio_rr);

	zlog_debug(c, "status detected now in pin %d is %s",  globals.gpio_rr, rr_pin_status_now == LOW ? "LOW" : "HIGH");

	if (rr_pin_status == LOW && rr_pin_status_now == HIGH)
        {
                zlog_info(c, "detected change LOW->HIGH in pin %d", globals.gpio_rr);
        }
        else if (rr_pin_status == HIGH && rr_pin_status_now == LOW)
        {
                zlog_info(c, "detected change HIGH->LOW in pin %d", globals.gpio_rr);
		sendmsg (globals.dc_id, 1);
	}
	else
        {
                zlog_info(c, "callback with no status change for pin %d", globals.gpio_rr);
        }
        rr_pin_status = rr_pin_status_now;
}
