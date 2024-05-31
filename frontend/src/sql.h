#ifndef _EMDC_SQL_H_
#define _EMDC_SQL_H_


int EMDC_sql_init (const char* sql_db, short in_memory);
int EMDC_sql_release ();
int EMDC_sql_begin_tnx ();
int EMDC_sql_commit_tnx ();
int EMDC_sql_rollback_tnx ();
int EMDC_sql_insert (long long val, int dc_id, int rarr, int status);
int EMDC_sql_update (int status, long long val, int dc_id, int rarr);
//EMDCmsg* EMDC_sql_select (int num);

#endif
