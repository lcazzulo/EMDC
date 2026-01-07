#ifndef __SAMPLE_H_
#define __SAMPLE_H_

#define TS	("ts")
#define DC_ID	("dc_id")			// legacy field, can now be thought of as device_id
#define RARR	("rarr")
#define STATUS  ("status")

// Canonical fields
#define DEVICE_ID   ("device_id")		// new canonical field
#define EVENT_TYPE  ("event_type")     		// new canonical field
#define VALUE       ("value")			// new canonical field
#define UNIT        ("unit")			// new canonical field
#define SOURCE      ("source")         		// new canonical field

#define STATUS_TO_DELIVER	(1)
#define STATUS_DELIVERED	(2)
#define STATUS_NOT_DELIVERED	(3)

typedef struct _EMDCsample
{
        long long ts;
        int dc_id;			// keep for legacy compatibility
        int rarr;
        int status;

	// new canonical fields
	char device_id[64];     	// unique identifier of the device
	char event_type[64];        	// new canonical event type, e.g. energy.active, temperature.value
	double value;           	// numeric value (e.g. temperature in °C)
	char unit[16];          	// unit of measure (kWh, kW, °C, %)
    	char source[64];            	// host or application name producing the event
} EMDCsample;

#endif
