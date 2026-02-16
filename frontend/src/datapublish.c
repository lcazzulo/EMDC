#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <limits.h>
#include <time.h>
#include <unistd.h>
#include <sys/time.h>
#include <errno.h>
#include <zlog.h>
#include <iniparser.h>
#include <MQTTClient.h>
#include "defines.h"
#include "queue.h"
#include "my_json.h"
#include "my_amqp.h"

#define RETRY_CONNECTION_TIME (10)
#define MQTT_PORT             (1883)

typedef struct _EMDC_datapublish_globals
{
        char EMDC_HOME[PATH_MAX];
        char init_file_path[PATH_MAX];
        char log_conf_file_path[PATH_MAX];
	mqd_t queue_in;
        mqd_t queue_out;
	char broker_address[512];
        char mqtt_address[512];
	char user[65];
	char password[64];
        int enable_amqp;
        AMQP_Ctx *ctx;
        MQTTClient mqtt_client;
        int connected_to_mqtt;

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
        if (!globals.enable_amqp)
	{
        	return;
    	}
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

static void build_mqtt_uri(char *out, size_t out_sz,
                           const char *addr, int port)
{
    if (!addr || addr[0] == '\0') 
    {
        snprintf(out, out_sz, "tcp://localhost:%d", port);
        return;
    }

    /* If config already includes scheme, keep it as-is */
    if (strncmp(addr, "tcp://", 6) == 0 || strncmp(addr, "ssl://", 6) == 0) 
    {
        snprintf(out, out_sz, "%s", addr);
        return;
    }

    /* Otherwise build tcp://<addr>:<port> */
    snprintf(out, out_sz, "tcp://%s:%d", addr, port);
}


static int mqtt_connect_with_autoreconnect(void)
{
    int rc;
    char uri[600];

    build_mqtt_uri(uri, sizeof(uri), globals.mqtt_address, MQTT_PORT);
    zlog_info(c, "MQTT URI: %s", uri);

    rc = MQTTClient_create(&globals.mqtt_client,
                           uri,
                           "datapublish",
                           MQTTCLIENT_PERSISTENCE_NONE,
                           NULL);
    if (rc != MQTTCLIENT_SUCCESS)
    {
        zlog_error(c, "MQTTClient_create failed rc=%d", rc);
        globals.connected_to_mqtt = 0;
        return -1;
    }

    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    conn_opts.keepAliveInterval = 20;
    conn_opts.cleansession = 1;

    /* Enable built-in automatic reconnect */
    conn_opts.retryInterval = 5;  /* seconds */

    rc = MQTTClient_connect(globals.mqtt_client, &conn_opts);
    if (rc != MQTTCLIENT_SUCCESS) {
        zlog_error(c, "MQTTClient_connect failed rc=%d", rc);
        globals.connected_to_mqtt = 0;
        return -1;
    }

    globals.connected_to_mqtt = 1;
    zlog_info(c, "Connected to MQTT (auto-reconnect enabled)");
    return 0;
}

static void mqtt_init_until_connected(void)
{
    while (go)
    {
        if (mqtt_connect_with_autoreconnect() == 0)
        {
            return;
        }

        zlog_warn(c, "MQTT connect failed; retrying in %d seconds", RETRY_CONNECTION_TIME);
        sleep(RETRY_CONNECTION_TIME);
    }
}

static time_t last_mqtt_reconnect_try = 0;

static int mqtt_try_reconnect(void)
{
    zlog_debug(c, "Entering mqtt_try_reconnect() ...");

    time_t now = time(NULL);

    /* throttle reconnect attempts */
    if (last_mqtt_reconnect_try != 0 &&
        (now - last_mqtt_reconnect_try) < RETRY_CONNECTION_TIME)
    {
        zlog_debug(c, "mqtt_try_reconnect(): throttling");
        return -1;
    }
    last_mqtt_reconnect_try = now;

    /* If already connected, nothing to do */
    if (MQTTClient_isConnected(globals.mqtt_client))
    {
        zlog_debug(c, "mqtt_try_reconnect(): already connected");
        globals.connected_to_mqtt = 1;
        return 0;
    }

    zlog_warn(c, "MQTT disconnected, trying reconnect...");

    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    conn_opts.keepAliveInterval = 20;
    conn_opts.cleansession = 1;
    conn_opts.retryInterval = 5; /* ok to keep */

    int rc = MQTTClient_connect(globals.mqtt_client, &conn_opts);
    if (rc != MQTTCLIENT_SUCCESS)
    {
        zlog_error(c, "MQTT reconnect failed rc=%d", rc);
        globals.connected_to_mqtt = 0;
        return -1;
    }

    zlog_info(c, "MQTT reconnected");
    globals.connected_to_mqtt = 1;
    return 0;
}

static int mqtt_publish_json(const char *topic, const char *payload)
{
    if (!globals.connected_to_mqtt || !MQTTClient_isConnected(globals.mqtt_client)) 
    {
        globals.connected_to_mqtt = 0;
        mqtt_try_reconnect();  /* throttled */
        if (!globals.connected_to_mqtt)
        {
            return -1;
        }
    }


    MQTTClient_message msg = MQTTClient_message_initializer;
    msg.payload = (void*)payload;
    msg.payloadlen = (int)strlen(payload);
    msg.qos = 1;
    msg.retained = 0;

    MQTTClient_deliveryToken token;
    int rc = MQTTClient_publishMessage(globals.mqtt_client, topic, &msg, &token);
    if (rc != MQTTCLIENT_SUCCESS) {
        zlog_error(c, "MQTT publishMessage failed rc=%d (will auto-reconnect)", rc);
        globals.connected_to_mqtt = 0;
        return -1;
    }

    rc = MQTTClient_waitForCompletion(globals.mqtt_client, token, 3000);
    if (rc != MQTTCLIENT_SUCCESS) {
        zlog_error(c, "MQTT waitForCompletion failed rc=%d (will auto-reconnect)", rc);
        globals.connected_to_mqtt = 0;
        return -1;
    }

    return 0;
}


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

        globals.enable_amqp = iniparser_getboolean(ini, "DATAPUBLISH:ENABLE_AMQP", 1);
        zlog_info(c, "DATAPUBLISH:ENABLE_AMQP = %d", globals.enable_amqp);


        s = iniparser_getstring(ini, "DATAPUBLISH:MQTT_ADDRESS", NULL);
        if (s == NULL)
        {
                zlog_fatal (c, "cannot find entry DATAPUBLISH:MQTT_ADDRESS in ini file [%s]", globals.init_file_path);
                exit(-1);
        }
        zlog_info (c, "DATAPUBLISH:MQTT_ADDRESS = %s", s);
        strncpy (globals.mqtt_address, s, sizeof (globals.mqtt_address) - 1);


        /* open the sending message queue */
        globals.queue_out = EMDC_queue_init (EMDC_QUEUE_IN_NAME, O_WRONLY, 1, -1, -1);
	/* open the receiving message queue */
	globals.queue_in = EMDC_queue_init (EMDC_QUEUE_OUT_NAME, O_RDONLY, 0, -1, -1);
        
        if (globals.enable_amqp) 
	{
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
	}
	else 
        {
   		globals.ctx = NULL;
    		connected_to_broker = 0;
    		zlog_warn(c, "AMQP disabled by configuration");
	}

        signal(SIGINT, signal_callback_handler);
        signal(SIGTERM, signal_callback_handler);

        mqtt_init_until_connected();

        zlog_info(c, "datapublish started");
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
                    if (!globals.connected_to_mqtt) 
                    {
        		mqtt_try_reconnect(); /* throttled */
    		    }
		}
        }
        free (buffer_in);
        zlog_debug (c, "exiting main loop");
}


int publish_message(const char* str)
{
    int ret;
    char buffer[1024];
    const char *routing_key = NULL;
    const char *topic = NULL;

    EMDCsample* sample = (EMDCsample*)malloc(sizeof(EMDCsample));
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

    /* ---- Routing / topic decision ---- */

    if (strcmp(sample->event_type, "energy.active") == 0) {
        routing_key = "EMDC.EVENTS.ACTIVE";
        topic = "emdc/events/energy/active";
    }
    else if (strcmp(sample->event_type, "energy.reactive") == 0) {
        routing_key = "EMDC.EVENTS.REACTIVE";
        topic = "emdc/events/energy/reactive";
    }
    else {
        routing_key = "EMDC.EVENTS.OTHER";
        topic = "emdc/events/other";
    }

    /* Serialize enriched event once (used for MQTT and optionally AMQP) */
    sample_to_json(sample, buffer);

    /* ---- Publish to AMQP (keep current setup working) ---- */
    if (connected_to_broker == 1 && connected_to_broker == 1) {
        /* Keep AMQP publishing the original message (legacy behavior).
           If you want AMQP to receive the enriched JSON too, replace `str` with `buffer`. */
        ret = AMQP_Sendmessage(globals.ctx, "EMDC", routing_key, str);

        if (ret != 0) {
            zlog_error(c, "error publishing message to broker");
            connected_to_broker = 0;
            retry_connect();
        } else {
            zlog_info(c, "message published to AMQP routing key %s", routing_key);
        }
    }

    /* ---- Publish to MQTT (becomes the source of truth for STATUS_DELIVERED) ---- */
        ret = mqtt_publish_json(topic, buffer); /* publish enriched JSON */
        if (ret == 0) {
            globals.connected_to_mqtt = 1;
            zlog_info(c, "message published to MQTT topic %s", topic);

            /* Mark delivered ONLY after MQTT success */
            sample->status = STATUS_DELIVERED;
            sample_to_json(sample, buffer);
            EMDC_queue_send(globals.queue_out, buffer);
        } else {
            zlog_error(c, "error publishing message to MQTT topic %s", topic);
            globals.connected_to_mqtt = 0;
            /* Optional: enqueue UNDELIVERED status if your pipeline expects it */
            /* sample->status = STATUS_UNDELIVERED;
               sample_to_json(sample, buffer);
               EMDC_queue_send(globals.queue_out, buffer); */
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

        if (globals.connected_to_mqtt)
        {
    		MQTTClient_disconnect(globals.mqtt_client, 1000);
	}
        if (globals.enable_amqp && globals.ctx) 
        {
	    free(globals.ctx);
    	    globals.ctx = NULL;
	}
	MQTTClient_destroy(&globals.mqtt_client);
	EMDC_queue_release (globals.queue_in);
	EMDC_queue_release (globals.queue_out);
        zlog_info (c, "datapublish exits");
        zlog_fini ();
        return 0;
}

