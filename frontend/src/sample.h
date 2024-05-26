#ifndef __SAMPLE_H_
#define __SAMPLE_H_

#define TS	("ts")
#define DC_ID	("dc_id")
#define RARR	("rarr")

typedef struct _EMDCsample
{
        long long ts;
        int dc_id;
        int rarr;
} EMDCsample;

#endif
