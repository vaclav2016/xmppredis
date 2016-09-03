/*
(с) 2016 Copyright by vaclav2016, https://github.com/vaclav2016/xmppredis/

Boost Software License - Version 1.0 - August 17th, 2003

Permission is hereby granted, free of charge, to any person or organization
obtaining a copy of the software and accompanying documentation covered by
this license (the "Software") to use, reproduce, display, distribute,
execute, and transmit the Software, and to prepare derivative works of the
Software, and to permit third-parties to whom the Software is furnished to
do so, all subject to the following:

The copyright notices in the Software and this entire statement, including
the above license grant, this restriction and the following disclaimer,
must be included in all copies of the Software, in whole or in part, and
all derivative works of the Software, unless such copies or derivative
works are solely in the form of machine-executable object code generated by
a source language processor.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.
*/

#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <math.h>
#include "curl-client.h"

const char *UserAgent = "libcurl-agent/1.0";

struct WebData {
	char *str;
	size_t size;
	size_t ofs;
};

void formatSize(char *c, double count) {
	int prefix = 0;

	while(count>1024 && prefix<4) {
		count = count / 1024;
		prefix++;
	}
	c[0] = 0;
	sprintf(c, "%.2f", count);
	switch(prefix) {
		case 0:
			break;
		case 1:
			strcat(c, "K");
			break;
		case 2:
			strcat(c, "M");
			break;
		case 3:
			strcat(c, "G");
			break;
		case 4:
			strcat(c, "T");
			break;
	}
}

int progress_func(void* ptr, double totalToDownload, double nowDownloaded, double totalToUpload, double nowUploaded) {
	char cur[16], tot[16];
	int totaldotz = 30, dotz, ii;
	double fractiondownloaded;

	if (totalToDownload <= 0.0) {
		return 0;
	}

	fractiondownloaded = nowDownloaded / totalToDownload;
	dotz = round(fractiondownloaded * totaldotz);

	printf("\033[0K \033[36m[", fractiondownloaded*100);
	for ( ii=0; ii < dotz;ii++) {
		printf("X");
	}
	for ( ; ii < totaldotz;ii++) {
		printf(" ");
	}
	formatSize(cur, nowDownloaded);
	formatSize(tot, totalToDownload);
	printf("] \033[33m%.1f%%\033[37m, \033[33m%s\033[37m / \033[33m%s\033[37m\r", fractiondownloaded * 100, cur, tot);
	fflush(stdout);
	return 0;
}

uint downloadPageWriter(char *in, uint size, uint nmemb, struct WebData *p) {
	size_t rsize = size*nmemb;

	if(p->str==NULL) {
		p->str = malloc(rsize + 1);
	} else {
		p->str = (char *) realloc(p->str, p->size + rsize + 1);
	}
	memcpy(p->str + p->size, in, rsize);
	p->size += rsize;
	p->str[p->size] = 0;

	return rsize;
}

uint uploadPageReader(char *out, uint size, uint nmemb, struct WebData *p) {
	size_t rsize = size*nmemb;
	rsize = (rsize > p->size - p->ofs) ? (p->size - p->ofs) : rsize;
	rsize = p->str == NULL ? 0 : rsize;
	if(rsize > 0) {
		memcpy(out, p->str+p->ofs, rsize);
		p->size += rsize;
	}
	return rsize;
}

char *do_call(char *url, char *uploadData, CURLoption option, long parameter);

char *url_get(char *url) {
	return do_call(url, NULL, CURLOPT_HTTPGET, 1);
}

char *url_post(char *url, char *data) {
	return do_call(url, data, CURLOPT_POST, 1);
}

char *url_put(char *url, char *data) {
	return do_call(url, data, CURLOPT_PUT, 1);
}

char *do_call(char *url, char *uploadData, CURLoption option, long parameter) {
	CURL *curl;
	CURLcode res;
	char *mem;
	char *result = NULL;
	char resOut[16];
	struct WebData inData, outData;

	curl_global_init(CURL_GLOBAL_DEFAULT);
	curl = curl_easy_init();
	if(curl) {
		curl_easy_setopt(curl, CURLOPT_URL, url);
		curl_easy_setopt(curl, CURLOPT_HEADER, 0L);
		curl_easy_setopt(curl, CURLOPT_VERBOSE, 0L);
		curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
#ifdef SKIP_PEER_VERIFICATION
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
#endif
#ifdef SKIP_HOSTNAME_VERIFICATION
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
#endif
		curl_easy_setopt(curl, CURLOPT_USERAGENT, UserAgent);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, downloadPageWriter);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &inData);

		if(uploadData != NULL) {
			outData.size = uploadData == NULL ? 0 : strlen(uploadData);
			outData.str = uploadData;
			outData.ofs = 0;
			curl_easy_setopt(curl, CURLOPT_READFUNCTION, uploadPageReader);
			curl_easy_setopt(curl, CURLOPT_READDATA, &outData);
		}

		curl_easy_setopt(curl, CURLOPT_TIMEOUT, 45);
		curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
		curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 3);

		curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, progress_func);

		inData.size = 0;
		inData.str = NULL;
		inData.ofs = 0;

		printf("\033[33m[DL]\033[32m %s\033[37m\n", url);
		res = curl_easy_perform(curl);
		printf("\r\033[0K\r");

		if(res != CURLE_OK) {
			fprintf(stderr, "\033[0K\033[31m[ERROR] %s\033[37m\n", curl_easy_strerror(res));
			curl_easy_cleanup(curl);
			if(inData.str!=NULL) {
				free(inData.str);
			}
		} else {
			formatSize(resOut, strlen(inData.str));
			printf("\033[33m[OK] Downloaded %s\033[0K\n", resOut);

			result = inData.str;
		}
		curl_global_cleanup();
	}
	return result;
}