#ifndef _EMCD_HTTP_H_
#define _EMCD_HTTP_H_

int EMDC_http_init ();
int EMDC_http_post (const char* url, const char* user, const char* password, const char* xml, char** ret_data);
int EMDC_http_get_bytes_read ();
void EMDC_http_release();

#endif
