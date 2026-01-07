#ifndef __JSON_H_
#define __JSON_H_

#include "sample.h"

/**
 * Constructor for EMDCsample.
 * Initialize all fields, including canonical event_type and source.
 */
void sample_ctor (
	EMDCsample* sample,
	long long val,
	int dc_id,
	int rarr,
	int status,
        const char* device_id,		// new canonical field
	const char* event_type,		// new canonical field
	double value,			// new canonical field
	const char* unit,		// new canonical field
	const char* source		// new canonical field
);

/**
 * Populate an EMDCsample from a JSON string.
 * Expects keys: ts, dc_id, rarr, status, event_type, source.
 */
void sample_from_json (EMDCsample* sample, const char* json);

/**
 * Serialize an EMDCsample to a JSON string.
 * Includes all canonical fields.
 */
void sample_to_json (EMDCsample* sample, char* json);

#endif
