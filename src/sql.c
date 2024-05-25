#include <stdlib.h>
#include <string.h>
#include <zlog.h>
#include <sqlite3.h>
#include "sql.h"

extern zlog_category_t *c;

/* sqlite db handle */
static sqlite3 *pDb = NULL;

/* statement insert */
static const char szInsert[] = "insert into samples values ( ?, ?, ? )";

/* handle statement insert */
static sqlite3_stmt *pInsert = NULL;

/* statement select */
static const char szSelect[] = "select sample_ts, dc_id, rarr_flag from samples";

/* handle statement select */
static sqlite3_stmt *pSelect = NULL;

/* statement delete */
static const char szDelete[] = "delete from samples where sample_ts = ? and dc_id = ? and rarr_flag = ?";

/* handle statement delete */
static sqlite3_stmt *pDelete = NULL;

/* statement begin transaction */
static const char szBegin[] = "begin transaction";

/* handle statement begin transaction */
static sqlite3_stmt *pBegin = NULL;

/* statement commit */
static const char szCommit[] = "commit";

/* handle statement commit */
static sqlite3_stmt *pCommit = NULL;

/* statement rollback */
static const char szRollback[] = "rollback";

/* handle statement rollback */
static sqlite3_stmt *pRollback = NULL;

/* statement create table (for in memory db) */
static const char szCreateTable[] = "create table samples ( "
	"sample_ts integer not null, "
	"dc_id integer not null, "
	"rarr_flag integer not null, "
	"primary key (sample_ts, dc_id, rarr_flag))";

int prepare_statements ();


int EMDC_sql_init (const char* sql_db, short in_memory)
{
	int ret;
	if (in_memory == 1)
	{
		char* err;
		zlog_info (c, "opening in memory db");
		ret = sqlite3_open (":memory:", &pDb);
		if (pDb == NULL || ret != SQLITE_OK)
                {
                        zlog_error (c, "error [%d] opening db [%s]", ret, sql_db);
                        return -1;
                }
                zlog_info (c, "open in memory db ok");
		ret = sqlite3_exec (pDb, szCreateTable, NULL, NULL, &err);
		if (ret != SQLITE_OK || err != NULL)
		{
			zlog_error (c, "error [%d - %s] creating table in memory database", ret, err);
			sqlite3_free (err);
			return -1;
		}
		zlog_info (c, "table samples created OK");
	}
	else
	{
		ret = sqlite3_open (sql_db, &pDb);
		if (pDb == NULL || ret != SQLITE_OK)
		{
			zlog_error (c, "error [%d] opening db [%s]", ret, sql_db);
			return -1;
		}
		zlog_info (c, "open db [%s] ok", sql_db);
	}
	if (prepare_statements ())
	{
		sqlite3_close (pDb);
		return -1;
	}	
	return 0;
}

int EMDC_sql_release ()
{
	sqlite3_finalize (pInsert);
	sqlite3_finalize (pSelect);
	sqlite3_finalize (pDelete);
	sqlite3_finalize (pBegin);
	sqlite3_finalize (pCommit);
	sqlite3_finalize (pRollback);
	sqlite3_close (pDb);
	return 0;
}

int EMDC_sql_begin_tnx ()
{
	int ret, rc;
	ret = 0;	
	rc = sqlite3_step(pBegin);
	if ( rc !=  SQLITE_DONE )
	{
		zlog_error (c, "error [%d] - %s in sqlite3_step()", rc, sqlite3_errmsg (pDb));
		ret = -1;
	}
	rc = sqlite3_reset(pBegin);
	if ( rc !=  SQLITE_OK )
	{
		zlog_error (c, "error [%d] - %s in sqlite3_reset()", rc, sqlite3_errmsg (pDb));
		ret = -1;
	}
	if (!ret)
	{
		zlog_debug (c, "\"%s\" execute ok", szBegin);
	}
	return ret;
}

int EMDC_sql_commit_tnx ()
{
	int ret, rc;
	ret = 0;

        rc = sqlite3_step(pCommit);
        if (rc !=  SQLITE_DONE)
        {
                zlog_error (c, "error [%d] - %s in sqlite3_step()", rc, sqlite3_errmsg (pDb));
                ret = -1;
        }
        rc = sqlite3_reset(pCommit);
        if (rc !=  SQLITE_OK)
        {
                zlog_error (c, "error [%d] - %s in sqlite3_reset()", ret, sqlite3_errmsg (pDb));
                ret = -1;
        }
	if (!ret)
	{
        	zlog_debug (c, "\"%s\" execute ok", szCommit);
	}
	return ret;
}

int EMDC_sql_rollback_tnx ()
{
	int ret, rc;
	ret = 0;

        rc = sqlite3_step(pRollback);
        if (rc !=  SQLITE_DONE)
        {
                zlog_error (c, "error [%d] - %s in sqlite3_step()", rc, sqlite3_errmsg (pDb));
                ret = -1;
        }
        rc = sqlite3_reset(pRollback);
        if (rc !=  SQLITE_OK)
        {
                zlog_error (c, "error [%d] - %s in sqlite3_reset()", rc, sqlite3_errmsg (pDb));
                ret = -1;
        }
	if (!ret)
	{
        	zlog_debug (c, "\"%s\" execute ok", szRollback);
	}
	return ret;
}

int EMDC_sql_insert (long long val, int dc_id, int rarr)
{
	int ret, rc;
	ret = 0;

	rc = sqlite3_bind_int64 ( pInsert, 1, val );
	if ( rc != SQLITE_OK ) 
	{
		zlog_error (c, "error [%d] - %s in sqlite3_bind_int64()", rc, sqlite3_errmsg (pDb));
		ret = -1;
	}
	rc = sqlite3_bind_int ( pInsert, 2, dc_id );
	if ( rc != SQLITE_OK )
        {
                zlog_error (c, "error [%d] - %s in sqlite3_bind_int()", rc, sqlite3_errmsg (pDb));
                ret = -1;
        }
	rc = sqlite3_bind_int ( pInsert, 3, rarr );
	if ( rc != SQLITE_OK )
        {
                zlog_error (c, "error [%d] - %s in sqlite3_bind_int()", rc, sqlite3_errmsg (pDb));
                ret = -1;
        }
	rc = sqlite3_step(pInsert);
	if ( rc !=  SQLITE_DONE )
	{
		zlog_error (c, "error [%d] - %s in sqlite3_step()", rc, sqlite3_errmsg (pDb));
		ret = -1;
	}
	rc = sqlite3_clear_bindings(pInsert);
	if ( rc !=  SQLITE_OK )
	{
		zlog_error (c, "error [%d] - %s in sqlite3_clear_bindings()", rc, sqlite3_errmsg (pDb));
		ret = -1;
	}
	rc = sqlite3_reset(pInsert);
	if ( rc !=  SQLITE_OK )
	{
		zlog_error (c, "error [%d] - %s in sqlite3_reset()", rc, sqlite3_errmsg (pDb));
		ret = -1;
	}
	if (!ret)
	{
		zlog_debug (c, "\"%s\" execute ok", szInsert);
	}
	return ret;
}

int EMDC_sql_delete (long long val, int dc_id, int rarr)
{
	int ret, rc;
	ret = 0;

        rc = sqlite3_bind_int64 ( pDelete, 1, val );
        if ( rc != SQLITE_OK )
        {
                zlog_error (c, "error [%d] - %s in sqlite3_bind_int64()", rc, sqlite3_errmsg (pDb));
                ret = -1;
        }
	rc = sqlite3_bind_int ( pDelete, 2, dc_id );
        if ( rc != SQLITE_OK )
        {
                zlog_error (c, "error [%d] - %s in sqlite3_bind_int64()", rc, sqlite3_errmsg (pDb));
                ret = -1;
        }
	rc = sqlite3_bind_int ( pDelete, 3, rarr );
        if ( rc != SQLITE_OK )
        {
                zlog_error (c, "error [%d] - %s in sqlite3_bind_int64()", rc, sqlite3_errmsg (pDb));
                ret = -1;
        }
        rc = sqlite3_step(pDelete);
        if ( rc !=  SQLITE_DONE )
        {
                zlog_error (c, "error [%d] - %s in sqlite3_step()", rc, sqlite3_errmsg (pDb));
                ret = -1;
        }
        rc = sqlite3_clear_bindings(pDelete);
        if ( rc !=  SQLITE_OK )
        {
                zlog_error (c, "error [%d] - %s in sqlite3_clear_bindings()", rc, sqlite3_errmsg (pDb));
                ret = -1;
        }
        rc = sqlite3_reset(pDelete);
        if ( rc !=  SQLITE_OK )
        {
                zlog_error (c, "error [%d] - %s in sqlite3_reset()", rc, sqlite3_errmsg (pDb));
                ret = -1;
        }
	if (!ret)
	{
		zlog_debug (c, "\"%s\" execute ok", szDelete);
	}
        return ret;	
}

EMDCmsg* EMDC_sql_select (int num)
{
	int ret;
	int count = 0;
	EMDCmsg* m = init_message (EMDCresponse);
	EMDCsamples *ss = init_samples (EMDCselect, -1);
	while (1)
	{
		ret = sqlite3_step(pSelect);
		if(ret == SQLITE_ROW && (num == -1 || count < num))
		{
			zlog_debug (c, "fetched row");
			add_sample(ss, 
				   sqlite3_column_int64(pSelect, 0),
				   sqlite3_column_int(pSelect, 1),
				   sqlite3_column_int(pSelect, 2));
			count++;
		}
		else
		{
			/*
			break when
			ret != SQLITE_ROW
			or
			num != -1 and count == num
			*/
			if (ret == SQLITE_DONE)
			{
				zlog_debug (c, "end of fetch");
			}
			else
			{	
				if (ret == SQLITE_ROW)
				{
					zlog_info (c, "more rows to fetch ...");
					ss->more = 1;
				}
				else
				{
					zlog_error (c, "end of fetch with sqlite3_step() return value [%d]", ret);
				}
			}
			break;
		}
	}
	add_samples (m, ss);
	ret = sqlite3_reset(pSelect);
        if ( ret !=  SQLITE_OK )
        {
                zlog_error (c, "error [%d] - %s in sqlite3_reset()", ret, sqlite3_errmsg (pDb));
        }
	return m;
}

int prepare_statements ()
{
	int ret;
	
	/*************************************************************************************************/
	/* INSERT ****************************************************************************************/
	ret =  sqlite3_prepare_v2(
				pDb,            		/* Database handle */
				szInsert,       		/* SQL statement, UTF-8 encoded */
				strlen(szInsert),       	/* Maximum length of zSql in bytes. */
				&pInsert,  			/* OUT: Statement handle */
				NULL 				/* OUT: Pointer to unused portion of zSql */
	);
	if (pInsert == NULL || ret != SQLITE_OK)
	{
		zlog_error (c, "error [%d] in sqlite3_prepare_v2() for statement \"%s\"", ret, szInsert);
		return -1;
	}
	zlog_info (c, "sqlite3_prepare_v2() ok for statement \"%s\"", szInsert);
	/*************************************************************************************************/

	/*************************************************************************************************/
	/* SELECT ****************************************************************************************/
	ret =  sqlite3_prepare_v2(
                                pDb,                            /* Database handle */
                                szSelect,                       /* SQL statement, UTF-8 encoded */
                                strlen(szSelect),               /* Maximum length of zSql in bytes. */
                                &pSelect,                       /* OUT: Statement handle */
                                NULL                            /* OUT: Pointer to unused portion of zSql */ 
        );
        if (pSelect == NULL || ret != SQLITE_OK) 
        {
                zlog_error (c, "error [%d] in sqlite3_prepare_v2() for statement \"%s\"", ret, szSelect);
		sqlite3_finalize ( pInsert );
                return -1;
        }
        zlog_info (c, "sqlite3_prepare_v2() ok for statement \"%s\"", szSelect);
	/*************************************************************************************************/

	/*************************************************************************************************/	
	/* DELETE ****************************************************************************************/
	ret =  sqlite3_prepare_v2(
                                pDb,                            /* Database handle */
                                szDelete,                       /* SQL statement, UTF-8 encoded */
                                strlen(szDelete),               /* Maximum length of zSql in bytes. */
				&pDelete,                       /* OUT: Statement handle */
                                NULL                            /* OUT: Pointer to unused portion of zSql */
        );
        if (pDelete == NULL || ret != SQLITE_OK)
        {
                zlog_error (c, "error [%d] in sqlite3_prepare_v2() for statement \"%s\"", ret, szDelete);
		sqlite3_finalize ( pInsert );
        	sqlite3_finalize ( pSelect );
                return -1;
        }
        zlog_info (c, "sqlite3_prepare_v2() ok for statement \"%s\"", szDelete);
	/*************************************************************************************************/

	/*************************************************************************************************/	
	/* BEGIN *****************************************************************************************/
	ret =  sqlite3_prepare_v2(
				pDb,            		/* Database handle */
				szBegin,	       		/* SQL statement, UTF-8 encoded */
				strlen(szBegin),	    	/* Maximum length of zSql in bytes. */
				&pBegin,  			/* OUT: Statement handle */
				NULL 				/* OUT: Pointer to unused portion of zSql */
	);
	
	if (pBegin == NULL || ret != SQLITE_OK)
	{
		zlog_error (c, "error [%d] in sqlite3_prepare_v2() for statement \"%s\"", ret, szBegin);
		sqlite3_finalize ( pInsert );
        	sqlite3_finalize ( pSelect );
        	sqlite3_finalize ( pDelete );
                return -1;
        }
	zlog_info (c, "sqlite3_prepare_v2() ok for statement \"%s\"", szBegin);
	/*************************************************************************************************/

	/*************************************************************************************************/
	/* COMMIT ****************************************************************************************/
	ret =  sqlite3_prepare_v2(
				pDb,            		/* Database handle */
				szCommit,       		/* SQL statement, UTF-8 encoded */
				strlen(szCommit),       	/* Maximum length of zSql in bytes. */
				&pCommit,  			/* OUT: Statement handle */
				NULL 				/* OUT: Pointer to unused portion of zSql */
	);
	
	if (pCommit == NULL || ret != SQLITE_OK)
	{
                zlog_error (c, "error [%d] in sqlite3_prepare_v2() for statement \"%s\"", ret, szCommit);
		sqlite3_finalize ( pInsert );
        	sqlite3_finalize ( pSelect );
        	sqlite3_finalize ( pDelete );
        	sqlite3_finalize ( pBegin );
                return -1;
        }
        zlog_info (c, "sqlite3_prepare_v2() ok for statement \"%s\"", szCommit);
	/*************************************************************************************************/

	/*************************************************************************************************/
	/* ROLLBACK **************************************************************************************/
        ret =  sqlite3_prepare_v2(
                                pDb,                            /* Database handle */
                                szRollback,                     /* SQL statement, UTF-8 encoded */
                                strlen(szRollback),             /* Maximum length of zSql in bytes. */
                                &pRollback,                     /* OUT: Statement handle */
                                NULL                            /* OUT: Pointer to unused portion of zSql */ 
        );

        if (pRollback == NULL || ret != SQLITE_OK)
        {
                zlog_error (c, "error [%d] in sqlite3_prepare_v2() for statement \"%s\"", ret, szRollback);
		sqlite3_finalize ( pInsert );
        	sqlite3_finalize ( pSelect );
        	sqlite3_finalize ( pDelete );
        	sqlite3_finalize ( pBegin );
        	sqlite3_finalize ( pCommit );
                return -1;
        }
        zlog_info (c, "sqlite3_prepare_v2() ok for statement \"%s\"", szRollback);
        /*************************************************************************************************/	
	return 0;
}


