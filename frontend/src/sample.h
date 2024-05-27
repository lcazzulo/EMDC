#ifndef __SAMPLE_H_
#define __SAMPLE_H_

#define TS	("ts")
#define DC_ID	("dc_id")
#define RARR	("rarr")
#define STATUS  ("status")

#define STATUS_TO_DELIVER	(1)
#define STATUS_DELIVERED	(2)
#define STATUS_NOT_DELIVERED	(3)

typedef struct _EMDCsample
{
        long long ts;
        int dc_id;
        int rarr;
        int status;
} EMDCsample;

#endif
