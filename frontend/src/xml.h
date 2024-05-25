#ifndef _EMDC_XML_H_
#define _EMDC_XML_H_


#include "list.h"

int EMDC_xml_init ();
int EMDC_xml_release ();
EMDCmsg* EMDC_xml_parse (const char* xml);
char* EMDC_xml_build (const EMDCmsg* msg);

#endif

