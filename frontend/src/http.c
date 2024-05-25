#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <zlog.h>
#include "http.h"

extern zlog_category_t *c;
static int bytes_read = 0;

size_t write_to_buffer(void* ptr, size_t size, size_t count, void* buff)
{
	zlog_debug (c, "callback called size [%d] count[%d]", size, count);
	size_t data_size = size * count;
	int old_bytes_read = bytes_read;
	bytes_read += data_size;
	char** data_from_server = (char**) buff;
	*data_from_server = realloc (*data_from_server, bytes_read);
	memcpy (*data_from_server + old_bytes_read, ptr, data_size);	
	return (size * count); 	
}

int EMDC_http_init ()
{
	/* global initialization */
	curl_global_init(CURL_GLOBAL_DEFAULT);
	return 0;
}

int EMDC_http_post (const char* url, const char* user, const char* password, const char* xml, char** ret_data)
{
	zlog_info(c, "sending msg: %s to remote server %s ...", xml, url);
	CURLcode ret;
	/* initiailization */
	CURL * curl = curl_easy_init ();
	if (curl == NULL)
        {
                zlog_fatal (c, "error initializing curl library. Exiting");
                exit (-1);
        }
	zlog_debug (c, "curl_easy_init () ok");

	/* set user agent */
	ret = curl_easy_setopt(curl, CURLOPT_USERAGENT, curl_version());
        if (ret != CURLE_OK)
        {
                zlog_error (c, "error %d - %s in curl_easy_setopt(CURLOPT_USERAGENT)", ret, curl_easy_strerror(ret));
                curl_easy_cleanup(curl);
                return -1;
        }
        zlog_debug (c, "curl_easy_setopt(CURLOPT_USERAGENT) with user agent [%s] ok", curl_version());
	
	/* set url */
	ret = curl_easy_setopt(curl, CURLOPT_URL, url);
        if (ret != CURLE_OK)
        {
                zlog_error (c, "error %d - %s in curl_easy_setopt(CURLOPT_URL)", ret, curl_easy_strerror(ret));
      		curl_easy_cleanup(curl);
                return -1;
        }
        zlog_debug (c, "curl_easy_setopt(CURLOPT_URL) with url [%s] ok", url);

	/* set operation timeout (to 30 seconds) */
	ret = curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30);
        if (ret != CURLE_OK)
        {
                zlog_error (c, "error %d - %s in curl_easy_setopt(CURLOPT_URL)", ret, curl_easy_strerror(ret));
                curl_easy_cleanup(curl);
                return -1;
        }
        zlog_debug (c, "curl_easy_setopt(CURLOPT_TIMEOUT) ok");

	*ret_data = NULL;
	char * buffer = malloc (strlen(user) + 6 + strlen(password) + 10 + strlen(xml) + 5);
	sprintf (buffer, "user=%s&password=%s&xml=%s", user, password, xml);
	ret = curl_easy_setopt(curl, CURLOPT_POSTFIELDS, buffer);
        if (ret != CURLE_OK)
        {
                zlog_error (c, "error %d - %s in curl_easy_setopt(CURLOPT_POSTFIELDS)", ret, curl_easy_strerror(ret));
                free (buffer);
		curl_easy_cleanup(curl);
                return -1;
        }
	zlog_debug (c, "curl_easy_setopt(CURLOPT_POSTFIELDS) ok");
	
	/* set callback function */
	ret = curl_easy_setopt (curl, CURLOPT_WRITEFUNCTION, write_to_buffer);
	if (ret != CURLE_OK)
        {
                zlog_error (c, "error %d - %s in curl_easy_setopt(CURLOPT_WRITEFUNCTION)", ret, curl_easy_strerror(ret));
		free (buffer);
		curl_easy_cleanup(curl);
                return -1;
        }
	zlog_debug (c, "curl_easy_setopt(CURLOPT_WRITEFUNCTION) ok");
	
	/* set receive data buffer */
	ret = curl_easy_setopt (curl, CURLOPT_WRITEDATA, ret_data);
        if (ret != CURLE_OK)
        {
                zlog_error (c, "error %d - %s in curl_easy_setopt(CURLOPT_WRITEDATA)", ret, curl_easy_strerror(ret));
                free (buffer);
		curl_easy_cleanup(curl);
                return -1;
        }
        zlog_debug (c, "curl_easy_setopt(CURLOPT_WRITEDATA) ok");

	/* perform http operation */
	bytes_read = 0;
	ret = curl_easy_perform (curl);
	if (ret != CURLE_OK)
        {
                zlog_error (c, "error %d - %s in curl_easy_perform()", ret, curl_easy_strerror(ret));
                free (buffer);
		curl_easy_cleanup(curl);
                return -1;
        }
	zlog_debug (c, "curl_easy_perform() ok");	

	/* clean up */
	free (buffer);
	curl_easy_cleanup(curl);
	zlog_info(c, "message sent sucessfully");

}

void EMDC_http_release()
{
	/* global clean up */
	curl_global_cleanup();	
}

int EMDC_http_get_bytes_read ()
{
	return bytes_read;
}

