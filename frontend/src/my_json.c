#include <string.h>
#include <stdio.h>
#include <json.h>
#include "my_json.h"

/**
 * Initialize a sample with all fields
 */
void sample_ctor(EMDCsample* sample,
                 long long ts,
                 int dc_id,
                 int rarr,
                 int status,
                 const char* device_id,
                 const char* event_type,
                 double value,
                 const char* unit,
                 const char* source)
{
        if (sample == NULL) return;

        memset(sample, 0, sizeof(*sample));

        sample->ts = ts;
        sample->dc_id = dc_id;
        sample->rarr = rarr;
        sample->status = status;
        sample->value = value;

        if (device_id)
        {
                strncpy(sample->device_id, device_id, sizeof(sample->device_id) - 1);
                sample->device_id[sizeof(sample->device_id) - 1] = '\0';
        }

        if (event_type)
        {
                strncpy(sample->event_type, event_type, sizeof(sample->event_type) - 1);
                sample->event_type[sizeof(sample->event_type) - 1] = '\0';
        }

        if (unit)
        {
                strncpy(sample->unit, unit, sizeof(sample->unit) - 1);
                sample->unit[sizeof(sample->unit) - 1] = '\0';
        }

        if (source)
        {
                strncpy(sample->source, source, sizeof(sample->source) - 1);
                sample->source[sizeof(sample->source) - 1] = '\0';
        }
}

/**
 * Deserialize a JSON string into a sample
 */
void sample_from_json(EMDCsample* sample, const char* json)
{
    if (sample == NULL || json == NULL) return;

    struct json_object *job = json_tokener_parse(json);
    if (!job) return;

    struct json_object* j_ts        = json_object_object_get(job, TS);
    struct json_object* j_dc_id     = json_object_object_get(job, DC_ID);
    struct json_object* j_rarr      = json_object_object_get(job, RARR);
    struct json_object* j_status    = json_object_object_get(job, STATUS);
    struct json_object* j_device_id = json_object_object_get(job, DEVICE_ID);
    struct json_object* j_event_type= json_object_object_get(job, EVENT_TYPE);
    struct json_object* j_value     = json_object_object_get(job, VALUE);
    struct json_object* j_unit      = json_object_object_get(job, UNIT);
    struct json_object* j_source    = json_object_object_get(job, SOURCE);

    sample->ts     = j_ts        ? json_object_get_int64(j_ts) : 0;
    sample->dc_id  = j_dc_id     ? json_object_get_int(j_dc_id) : 0;
    sample->rarr   = j_rarr      ? json_object_get_int(j_rarr) : 0;
    sample->status = j_status    ? json_object_get_int(j_status) : 0;

    if (j_device_id)
        snprintf(sample->device_id, sizeof(sample->device_id), "%s", json_object_get_string(j_device_id));
    else
        sample->device_id[0] = '\0';

    if (j_event_type)
        snprintf(sample->event_type, sizeof(sample->event_type), "%s", json_object_get_string(j_event_type));
    else
        sample->event_type[0] = '\0';

    sample->value = j_value ? json_object_get_double(j_value) : 0.0;

    if (j_unit)
        snprintf(sample->unit, sizeof(sample->unit), "%s", json_object_get_string(j_unit));
    else
        sample->unit[0] = '\0';

    if (j_source)
        snprintf(sample->source, sizeof(sample->source), "%s", json_object_get_string(j_source));
    else
        sample->source[0] = '\0';

    json_object_put(job);
}

/**
 * Serialize an EMDCsample to a JSON string
 * NOTE: assumes json buffer is at least 1024 bytes
 */
void sample_to_json(EMDCsample* sample, char* json)
{
        if (sample == NULL || json == NULL) return;

        struct json_object* job = json_object_new_object();

        json_object_object_add(job, TS, json_object_new_int64(sample->ts));
        json_object_object_add(job, DC_ID, json_object_new_int(sample->dc_id));
        json_object_object_add(job, RARR, json_object_new_int(sample->rarr));
        json_object_object_add(job, STATUS, json_object_new_int(sample->status));

        json_object_object_add(job, DEVICE_ID, json_object_new_string(sample->device_id));
        json_object_object_add(job, EVENT_TYPE, json_object_new_string(sample->event_type));
        json_object_object_add(job, VALUE, json_object_new_double(sample->value));
        json_object_object_add(job, UNIT, json_object_new_string(sample->unit));
        json_object_object_add(job, SOURCE, json_object_new_string(sample->source));

        const char* str = json_object_to_json_string(job);
        snprintf(json, 1024, "%s", str);

        json_object_put(job);
}
