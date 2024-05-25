#include "xml.h"
#include <string.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xmlschemas.h>
#include <zlog.h>

#define DC_ID_ATTR		"dc_id"
#define RARR_ATTR		"rarr"
#define MORE_ATTR		"more"
#define ACTION_ATTR		"action"

extern zlog_category_t *c;

static const char xsd[] = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
"<xs:schema xmlns:xs=\"http://www.w3.org/2001/XMLSchema\">"
  "<xs:element name=\"Message\">"
    "<xs:complexType>"
      "<xs:sequence>"
        "<xs:element name=\"Samples\" maxOccurs=\"unbounded\">"
          "<xs:complexType>"
            "<xs:sequence>"
              "<xs:element name=\"Sample\" minOccurs=\"0\" maxOccurs=\"unbounded\">"
	       "<xs:complexType>"
                  "<xs:simpleContent>"
                    "<xs:extension base=\"xs:long\">"
                      "<xs:attribute name=\"dc_id\" use=\"required\">"
                        "<xs:simpleType>"
                          "<xs:restriction base=\"xs:int\">"
                            "<xs:minInclusive value=\"0\"/>"
                            "<xs:maxInclusive value=\"255\"/>"
                          "</xs:restriction>"
                        "</xs:simpleType>"
                      "</xs:attribute>"
                      "<xs:attribute name=\"rarr\" use=\"required\">"
                        "<xs:simpleType>"
                          "<xs:restriction base=\"xs:int\">"
                            "<xs:minInclusive value=\"0\"/>"
                            "<xs:maxInclusive value=\"1\"/>"
                          "</xs:restriction>"
                        "</xs:simpleType>"
                      "</xs:attribute>"
                    "</xs:extension>"
                  "</xs:simpleContent>"
                "</xs:complexType>"
	      "</xs:element>"
            "</xs:sequence>"
            "<xs:attribute name=\"action\" use=\"required\">"
              "<xs:simpleType>"
                "<xs:restriction base=\"xs:string\">"
                  "<xs:enumeration value=\"INSERT\"/>"
                  "<xs:enumeration value=\"DELETE\"/>"
                  "<xs:enumeration value=\"SELECT\"/>"
                "</xs:restriction>"
              "</xs:simpleType>"
            "</xs:attribute>"
            "<xs:attribute name=\"more\">"
              "<xs:simpleType>"
                "<xs:restriction base=\"xs:string\">"
                  "<xs:enumeration value=\"yes\"/>"
		  "<xs:enumeration value=\"no\"/>"
                "</xs:restriction>"
              "</xs:simpleType>"
            "</xs:attribute>"
          "</xs:complexType>"
        "</xs:element>"
      "</xs:sequence>"
      "<xs:attribute name=\"type\" use=\"required\">"
        "<xs:simpleType>"
            "<xs:restriction base=\"xs:string\">"
            "<xs:enumeration value=\"request\"/>"
            "<xs:enumeration value=\"response\"/>"
          "</xs:restriction>"
        "</xs:simpleType>"
      "</xs:attribute>"
    "</xs:complexType>"
  "</xs:element>"
"</xs:schema>";

static xmlDocPtr xsd_doc = NULL;
static xmlSchemaParserCtxtPtr parser_ctxt = NULL;
static xmlSchemaPtr schema = NULL;
static xmlSchemaValidCtxtPtr valid_ctxt = NULL;

int EMDC_xml_init ()
{
	xsd_doc = xmlParseMemory (xsd, strlen(xsd));
	if (xsd_doc != NULL)
	{
		zlog_info (c, "parse xsd OK");
	}
	else
	{
		zlog_error (c, "parse xsd KO");
		return -1;
	}

	parser_ctxt = xmlSchemaNewDocParserCtxt(xsd_doc);
	if (parser_ctxt != NULL)
	{
                zlog_info (c, "parse context OK");
        }
        else
        {
                zlog_error (c, "parse context KO");
		xmlFreeDoc (xsd_doc);
                return -1;
        }

	schema = xmlSchemaParse(parser_ctxt);
	if (schema != NULL)
        {
                zlog_info (c, "schema OK");
        }
        else
        {
                zlog_error (c, "schema KO");
		xmlSchemaFreeParserCtxt(parser_ctxt);		
                xmlFreeDoc (xsd_doc);
                return -1;
        }
	
	valid_ctxt = xmlSchemaNewValidCtxt(schema);
	if (valid_ctxt != NULL)
	{
		zlog_info (c, "valid ctx OK");
	}	
	else
	{
		zlog_error (c, "valid ctx KO");
		xmlSchemaFree(schema);
		xmlSchemaFreeParserCtxt(parser_ctxt);
                xmlFreeDoc (xsd_doc);
	}

	return 0;
}

int EMDC_xml_release ()
{
	if (valid_ctxt) xmlSchemaFreeValidCtxt (valid_ctxt);
	if (schema) xmlSchemaFree(schema);
        if (parser_ctxt) xmlSchemaFreeParserCtxt(parser_ctxt);
        if (xsd_doc) xmlFreeDoc (xsd_doc);
}

EMDCmsg* EMDC_xml_parse (const char* xml)
{
	char* msg_type;
	char* action;
	EMDCmsg* ret = NULL;
	EMDCsamples* samples = NULL;
	xmlNodePtr cur_node = NULL;
	xmlNodePtr sample_node = NULL;
	int samples_node_count = 0;
	int sample_node_count;
	long long val;
	int dc_id, rarr;
	xmlDocPtr xml_doc = xmlParseMemory (xml, (int)strlen (xml));
	if (xml_doc == NULL)
	{
		zlog_error (c, "EMDC_xml_parse(): error parsing doc");
		return ret;
	}
	if (xmlSchemaValidateDoc(valid_ctxt, xml_doc) == 0)
	{
		zlog_debug (c, "EMDC_xml_parse(): document is valid");
	}	
	else
	{
		zlog_error (c, "EMDC_xml_parse(): document is not valid");
		xmlFreeDoc (xml_doc);
		return ret;
	}
	/* points to root element (Message)*/
	cur_node = xmlDocGetRootElement(xml_doc);
	/* get message type (request/response) */
	msg_type = (char*) xmlGetProp (cur_node, cur_node->properties->name);
	if (!strcmp(msg_type, "request"))
	{
		ret = init_message (EMDCrequest);
		zlog_debug (c, "message is request");\
	}
	else
	{
		ret = init_message (EMDCresponse);
		zlog_debug (c, "message is response");
	}
	xmlFree (msg_type);	
	/* points to first "Samples" children */
	cur_node = cur_node->xmlChildrenNode;
	while (cur_node != NULL)
	{
		if (!strcmp(cur_node->name, "Samples"))
		{
			sample_node_count = 0;
			short _more = -1;
			/* get action (INSERT/DELETE/SELECT) */
			action = (char*) xmlGetProp (cur_node, ACTION_ATTR);
			zlog_debug (c, "[%d] Samples child [%s]", samples_node_count, action);
			if (!strcmp (action, "INSERT"))
			{
				samples = init_samples (EMDCinsert, _more);
				zlog_debug (c, "Samples action is insert");
			}
			else if (!strcmp (action, "DELETE"))
			{
				samples = init_samples (EMDCdelete, _more);
				zlog_debug (c, "Samples action is delete");
			}
			else
			{	
				char* m = (char*) xmlGetProp (cur_node, MORE_ATTR);
				if (m != NULL && !strcmp (m, "yes"))
				{
					_more = 1;
				}
				else
				{
					_more = 0;
				}
				samples = init_samples (EMDCselect, _more);
				zlog_debug (c, "Samples action is select");
				xmlFree (m);
			}
			xmlFree (action);
			add_samples (ret, samples);
			/* points to first "Sample" children (if existing) */
			sample_node = cur_node->xmlChildrenNode;
			while (sample_node != NULL)
			{
				if (!strcmp (sample_node->name, "Sample"))
				{
					xmlChar *v = xmlNodeListGetString(xml_doc, sample_node->children, 1);
					val = atoll (v);	
					xmlFree (v);
					v = xmlGetProp (sample_node, DC_ID_ATTR);
					dc_id = atoi (v);
					xmlFree (v); 
					v = xmlGetProp (sample_node, RARR_ATTR);
					rarr = atoi (v);
					xmlFree (v);
					zlog_debug (c, "   [%d] Sample child, value [%lld] dc_id [%d] rarr [%d]", 
					sample_node_count, val, dc_id, rarr);
					add_sample (samples, val, dc_id, rarr);
					sample_node_count++;
				}
				/* next "Sample" children */
				sample_node = sample_node->next;
			}		
			samples_node_count++;
		}
		/* next "Samples" children */
		cur_node = cur_node->next;
	}
	xmlFreeDoc (xml_doc);
	return ret;
}

char* EMDC_xml_build (const EMDCmsg* msg)
{
	EMDCsamples* curr_samples;
	EMDCsample* curr_sample;
	xmlNodePtr node, node1 = NULL;
	char val[64];
	xmlChar * doc_txt_ptr;
	int doc_txt_len;
	xmlAttrPtr attr;

	if (msg != NULL && msg->head != NULL)
	{
		xmlDocPtr doc = xmlNewDoc(BAD_CAST "1.0");		
		if (doc == NULL)
		{
			zlog_error (c, "xmlNewDoc() returned NULL");
			return NULL;
		}
		xmlNodePtr root_node = xmlNewNode(NULL, "Message");
		if (root_node == NULL)
		{
			zlog_error (c, "xmlNewNode() returned NULL");
			xmlFreeDoc(doc);
			return NULL;
		}
		attr = xmlNewProp(root_node, BAD_CAST "type", BAD_CAST (msg->type == EMDCrequest ? "request" : "response"));
    		if (attr == NULL)
		{
			zlog_error (c, "xmlNewProp() returned NULL");
                        xmlFreeDoc(doc);
                        return NULL;
		}
		xmlDocSetRootElement(doc, root_node);
		curr_samples = msg->head;
		while (curr_samples != NULL)
		{
			node = xmlNewChild(root_node, NULL, BAD_CAST "Samples" , NULL);
			if (node == NULL)
			{
				zlog_error (c, "xmlNewChild() returned NULL");
                        	xmlFreeDoc(doc);
                        	return NULL;	
			}
			if (curr_samples->a == EMDCselect)
			{
				attr = xmlNewProp(node, BAD_CAST "action", BAD_CAST "SELECT");
				if (curr_samples->more == 1)
				{
					xmlNewProp(node, BAD_CAST MORE_ATTR, BAD_CAST "yes");
				}
				else if (curr_samples->more == 0)
				{
					xmlNewProp(node, BAD_CAST MORE_ATTR, BAD_CAST "no"); 
				} 
			}
			else if (curr_samples->a == EMDCinsert)
			{
				attr = xmlNewProp(node, BAD_CAST "action", BAD_CAST "INSERT");
			}
			else
			{
				attr = xmlNewProp(node, BAD_CAST "action", BAD_CAST "DELETE");
			}
			if (attr == NULL)
                	{
                        	zlog_error (c, "xmlNewProp() returned NULL");
                        	xmlFreeDoc(doc);
                        	return NULL;
                	}
			curr_sample = curr_samples->head;
			while (curr_sample != NULL)
			{
				sprintf (val, "%lld", curr_sample->val);
				if ((node1 = xmlNewChild (node, NULL, BAD_CAST "Sample", BAD_CAST val)) == NULL)
				{
					zlog_error (c, "xmlNewChild() returned NULL");
					xmlFreeDoc(doc);
                                	return NULL;
				} 
				sprintf (val, "%d", curr_sample->dc_id);
				attr = xmlNewProp(node1, BAD_CAST DC_ID_ATTR, val);
				if (attr == NULL)
                        	{
                                	zlog_error (c, "xmlNewProp() returned NULL");
                                	xmlFreeDoc(doc);
                                	return NULL;
                       	 	}
				sprintf (val, "%d", curr_sample->rarr);
                                attr = xmlNewProp(node1, BAD_CAST RARR_ATTR, val);
                                if (attr == NULL)
                                {
                                        zlog_error (c, "xmlNewProp() returned NULL");
                                        xmlFreeDoc(doc);
                                        return NULL;
                                }
				curr_sample = curr_sample->next;	
			}
			curr_samples = curr_samples->next;
		}
		xmlDocDumpMemoryEnc (doc, &doc_txt_ptr, &doc_txt_len, "UTF-8");
		xmlFreeDoc (doc);
		return ((char*) doc_txt_ptr);
	}	
	else 
	{
		if (msg == NULL)
		{
			zlog_warn (c, "passed instance of EMDCmsg is null");
		}
		else
		{
			zlog_warn (c, "passed instance of EMDCmsg has no \"Samples\" child");
		}
		return NULL;
	}
}

