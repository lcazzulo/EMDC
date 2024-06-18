#include <string.h>
#include <json.h>
#include "my_json.h"

void sample_ctor (EMDCsample* sample, long long ts, int dc_id, int rarr, int status)
{
        if (sample == NULL) return;
        sample->ts = ts;
        sample->dc_id = dc_id;
        sample->rarr = rarr;
        sample->status = status;
}

void sample_from_json (EMDCsample* sample, const char* json)
{
	struct json_object *job = json_tokener_parse(json);
	if (job != NULL && sample != NULL)
	{
		struct json_object* _ts = json_object_object_get(job, TS);
		struct json_object* _dc_id = json_object_object_get(job, DC_ID);
		struct json_object* _rarr = json_object_object_get(job, RARR);
		struct json_object* _status = json_object_object_get(job, STATUS);
		if (_ts != NULL && _dc_id != NULL && _rarr != NULL && _status != NULL)
		{
			int64_t ts =  json_object_get_int64 ( _ts );
			int dc_id = json_object_get_int ( _dc_id );
			int rarr = json_object_get_int ( _rarr );
			int status = json_object_get_int ( _status );
			sample->ts = ts;
			sample->dc_id = dc_id;
			sample->rarr = rarr;
			sample->status = status;
                        //json_object_put( _ts);
                        //json_object_put( _dc_id);
                        //json_object_put( _rarr);
                        //json_object_put( _status);
		}
                json_object_put(job);

	}
}

void sample_to_json (EMDCsample* sample, char* json)
{
	if (sample != NULL)
	{
		struct json_object* job = json_object_new_object();
		struct json_object* _ts = json_object_new_int64 (sample->ts);
		struct json_object* _dc_id = json_object_new_int (sample->dc_id);
		struct json_object* _rarr = json_object_new_int (sample->rarr);
           	struct json_object* _status = json_object_new_int (sample->status);
		json_object_object_add (job, TS, _ts);
		json_object_object_add (job, DC_ID, _dc_id);
		json_object_object_add (job, RARR, _rarr);
		json_object_object_add (job, STATUS, _status);
		const char * str = json_object_to_json_string (job);
		strcpy (json, str);
                //free((void*) str);
                //free((void*) _status);
                //free((void*) _rarr);
                //free((void*) _dc_id);
                //free((void*) _ts);
                json_object_put(job);
	}
}
