#include <stdlib.h>
#include <stdio.h>
#include <zlog.h>
#include "../sample.h"
#include "../my_json.h"

zlog_category_t *c = NULL;
char buff[1024];
const char* json_string = "{\"ts\": 1716724056419, \"dc_id\": 1, \"rarr\": 0, \"status\": 1}";

void init ()
{
	int rc = zlog_init("../etc/log.conf");
	if (rc)
	{
		printf ("zlog_init() failed. Exiting\n");
		exit (-1);
	}

	c = zlog_get_category("test_json");
	if (!c)
	{
		printf("zlog_get_category() failed. Exiting\n");
		zlog_fini();
	}
}

int main(int argc, char* argv[])
{
	init ();

	zlog_debug (c, "Starting ...");

        while(1)
        {
            EMDCsample* sample = (EMDCsample*) malloc(sizeof(EMDCsample));
	    sample_from_json(sample, json_string);
	    printf ("ts     [%lld]\r\n", sample->ts);
	    printf ("dc_id  [%d]\r\n", sample->dc_id);
	    printf ("rarr   [%d]\r\n", sample->rarr);
	    printf ("status [%d]\r\n", sample->status);

	    sample_to_json (sample, buff);
	    printf ("json string: %s\r\n", buff);
	    free((void*)sample);
        }
	return 0;
}
