#include <stdlib.h>
#include <zlog.h>
#include "../sql.h"
#include "../xml.h"

zlog_category_t *c = NULL;

void init ()
{
	int rc = zlog_init("../etc/log.conf");
        if (rc)
        {
                printf ("zlog_init() failed. Exiting\n");
                exit (-1);
        }

        c = zlog_get_category("test_sql");
        if (!c)
        {
                printf("zlog_get_category() failed. Exiting\n");
                zlog_fini();
        }
}

void select_test ()
{
	EMDCmsg* m = NULL;
        char* xmlRet;
	m = EMDC_sql_select (-1);
        xmlRet = EMDC_xml_build (m);
        zlog_info (c, xmlRet);
        free (xmlRet);
        free_message (m);
        free (m);
}

void insert_test ()
{
	int i, ret;
	
	ret = EMDC_sql_begin_tnx();
        if (ret)
        {
                zlog_error (c, "begin tnx");
        }
        else
        {
                zlog_debug (c, "begin tnx ok");
        }

        for (i=0; i < 1000; i++)
        {
                ret = EMDC_sql_insert((long long)(i+100000), 0, 0);
                if (ret)
                {
                        zlog_error (c, "error insert");
                }
        }

        ret = EMDC_sql_commit_tnx();
        if (ret)
        {
                zlog_error (c, "commit tnx");
        }
        else
        {
                zlog_debug (c, "commit tnx ok");

        }
}

void delete_test ()
{
	int i, ret;

        ret = EMDC_sql_begin_tnx();
        if (ret)
        {
                zlog_error (c, "begin tnx");
        }
        else
        {
                zlog_debug (c, "begin tnx ok");
        }

        for (i=500; i < 1000; i++)
        {
                ret = EMDC_sql_delete((long long)(i+100000), 0, 0);
                if (ret)
                {
                        zlog_error (c, "error delete");
                }
        }

        ret = EMDC_sql_commit_tnx();
        if (ret)
        {
                zlog_error (c, "commit tnx");
        }
        else
        {
                zlog_debug (c, "commit tnx ok");

        }

}

int main(int argc, char *argv[])
{
	int ret;

	init ();
	zlog_debug (c, "Starting ...");
	ret = EMDC_sql_init ("/home/luca/devel/EMDC/var/samples.db");
	if (ret)
	{
		zlog_error (c, "error in EMDC_sql_init(), exiting ...");
		exit (-1);
	}
	/* insert_test (); */ 
	delete_test ();
	select_test (); 	
	ret = EMDC_sql_release ();
	if (ret)
	{	
		zlog_error (c, "error in EMDC_sql_release(), exiting ...");
                exit (-1);
	}
	return 0;
}

