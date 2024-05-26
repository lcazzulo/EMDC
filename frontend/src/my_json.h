#ifndef __JSON_H_
#define __JSON_H_

#include "sample.h"

void sample_ctor (EMDCsample* sample, long long val, int dc_id, int rarr);
void sample_from_json (EMDCsample* sample, const char* json);
void sample_to_json (EMDCsample* sample, char* json);

#endif
