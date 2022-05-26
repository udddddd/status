#pragma once

#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "fmt.h"

#define MODULE_BUF_SIZE 128
#define LEN(a) (sizeof(a) / sizeof((a)[0]))

#ifndef NDEBUG
#define LOG(fmt, ...) fprintf(stderr, MODULE_NAME ": " fmt "\n", ##__VA_ARGS__)
#else
#define LOG(...)
#endif

#define ERR(fmt, ...) fprintf(stderr, MODULE_NAME " error: " fmt "\n", \
                              ##__VA_ARGS__)

extern sig_atomic_t quit;
extern pthread_mutex_t lock;

#define signalrefresh() kill(getpid(), SIGUSR1)
#define msnfmt(output, fmt, fmtopts, len)                  \
	do {                                                   \
		pthread_mutex_lock(&lock);                         \
		snfmt(output, MODULE_BUF_SIZE, fmt, fmtopts, len); \
		pthread_mutex_unlock(&lock);                       \
		signalrefresh();                                   \
	} while(0);
