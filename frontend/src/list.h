#ifndef _EMDC_LIST_H_
#define _EMDC_LIST_H_

typedef enum EMDCaction
{
	EMDCselect,
	EMDCinsert,
	EMDCdelete
} EMDCaction;

typedef enum EMDCmsg_type
{
	EMDCrequest,
	EMDCresponse
} EMDCmsg_type;

typedef struct _EMDCsample
{
	long long val;
	int dc_id;
	int rarr;
	struct _EMDCsample* next;
} EMDCsample;

typedef struct _EMDCsamples
{
	EMDCaction a;
	short more;
	EMDCsample* head;
	EMDCsample* tail;
	struct _EMDCsamples* next;
} EMDCsamples;

typedef struct _EMDCmsg
{
	EMDCmsg_type type;
	EMDCsamples* head;
	EMDCsamples* tail;
} EMDCmsg;

EMDCsamples* init_samples(EMDCaction _a, short _more);
EMDCmsg* init_message(EMDCmsg_type t);
void free_samples (EMDCsamples* ss);
void add_sample(EMDCsamples* ss, long long val, int dc_id, int rarr);
void add_samples(EMDCmsg* m, EMDCsamples* s);
void free_message(EMDCmsg* m);

#endif
