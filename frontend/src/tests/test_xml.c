#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <zlog.h>
#include "../list.h"
#include "../xml.h"

zlog_category_t *c = NULL;

u_int32_t get_file_size(const char *file_name) 
{
    	struct stat buf;
   	if ( stat(file_name, &buf) != 0 ) return(0);
    	return (unsigned int)buf.st_size;
}

void init ()
{
	int rc = zlog_init("../etc/log.conf");
	if (rc)
	{
		printf ("zlog_init() failed. Exiting\n");
		exit (-1);
	}
	
	c = zlog_get_category("test_xml");
	if (!c)
	{
		printf("zlog_get_category() failed. Exiting\n");
		zlog_fini();
	}
}

int main(int argc, char* argv[])
{
	unsigned int i = 0;		
	EMDCmsg *m = NULL;
	init ();

	zlog_debug (c, "Starting ...");

	
	const char *xmlPath = "tests/EMDC.xml";
	int xmlLength = get_file_size(xmlPath);
    	char *xmlSource = (char *)malloc(sizeof(char) * xmlLength + 1);

    	FILE *p = fopen(xmlPath, "r");
    	int ch;
    	while ((ch = fgetc(p)) != EOF) 
	{
       		xmlSource[i++] = ch;
    	}
	xmlSource[xmlLength] = 0;

   	zlog_debug(c, "XML Source: %s", xmlSource);
    	fclose(p);
	
	EMDC_xml_init ();
	m = EMDC_xml_parse (xmlSource);
	if (m != NULL)
	{
		char *xmlRet = EMDC_xml_build (m);
		zlog_info (c, xmlRet);
		m = EMDC_xml_parse (xmlRet);
		free (xmlRet);
		xmlRet = EMDC_xml_build (m);
                zlog_info (c, xmlRet);
		free (xmlRet);
	}
	
	EMDC_xml_release ();
	free (xmlSource);
	return 0;
}
