#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "logger.h"

void logger(const char* tag, const char* message) {
	time_t t;
	time(&t);
	struct tm *now = localtime(&t);
	printf("%i-%i-%i %i:%i:%i [%s] %s\n", now->tm_year+1900, now->tm_mon+1, now->tm_mday, now->tm_hour, now->tm_min, now->tm_sec, tag, message);
}
