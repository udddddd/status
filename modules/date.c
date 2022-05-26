#include <unistd.h>
#include <time.h>

#define MODULE_NAME "date"

#include "../common.h"
#include "date.h"

void date(char *output, void *arg) {
	LOG("startup");
	TimeDateOptions *opts = arg;

	for(;;) {
		pthread_mutex_lock(&lock);
		time_t t = time(NULL);
		strftime(output, MODULE_BUF_SIZE, opts->format, localtime(&t));
		pthread_mutex_unlock(&lock);
		signalrefresh();
		if(sleep(60 - t % 60))
			break;
	}
	LOG("shutdown");
}
