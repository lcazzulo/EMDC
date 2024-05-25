#include <stdlib.h>
#include <string.h>
#include <zlog.h>
#include "../http.h"

zlog_category_t *c = NULL;

void init ()
{
	int rc = zlog_init("../etc/log.conf");
	if (rc)
	{
		printf ("zlog_init() failed. Exiting\n");
		exit (-1);
	}
	
	c = zlog_get_category("test_http");
	if (!c)
	{
		printf("zlog_get_category() failed. Exiting\n");
		zlog_fini();
	}
}

int main(int argc, char* argv[])
{
	char* ret_data; 
	init ();
	EMDC_http_init ();
	while (1)
	{
		EMDC_http_post ("http://luca-aws.no-ip.org:8080/2test.jsp", "questo e' il valore della variabile", &ret_data);
		if (ret_data == NULL)
		{
			zlog_info (c, "no data");
		}
		zlog_info (c, "bytes_read [%d]", EMDC_http_get_bytes_read());
		char* str = (char*) malloc (EMDC_http_get_bytes_read() + 1);
		memset (str, '\x0', EMDC_http_get_bytes_read() + 1);
		memcpy (str, ret_data, EMDC_http_get_bytes_read());
		zlog_info (c, str);
		free (str);
		free (ret_data);
		sleep (1);
	}
	EMDC_http_release();		
}
