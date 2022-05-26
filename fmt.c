#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include "fmt.h"

static int fmt_compare(const void *a, const void *b) {
	return ((fmt_opt*)a)->p - ((fmt_opt*)b)->p;
}

static int snint(char *output, size_t len, int d, int base, int padding) {
	int baselen = len;
	char buf[sizeof(int) * 8 - 1];
	int n = 0;
	if(d < 0) {
		d = -d;
		*(output++) = '-';
		--len;
	}
	if(!d)
		buf[n++] = '0';
	while(d) {
		buf[n++] = "0123456789ABCDEDFGHIJKLMOPQRSTUVWXYZ"[d % base];
		d /= base;
	}
	while(n < padding)
		buf[n++] = ' ';
	while(--n >= 0 && len) {
		*(output++) = buf[n];
		--len;
	}
	return baselen - len;
}

int snfmt(char *output, size_t len, const char *fmt, const fmt_opt *opts, size_t nopts) {
	int baselen = len;
	--len;
	while(*fmt) {
		if(*fmt != '%') {
			*(output++) = *fmt;
			--len;
		} else {
			int padding = 0;
			++fmt;
			if(*fmt == '%') {
				*(output++) = *(fmt++);
				continue;
			}
			if(*fmt >= '0' && *fmt <= '9') {
				padding = strtol(fmt, (char**)&fmt, 10);
			}
			if(*fmt == '\0')
				return -1;
			fmt_opt key =  { .p = *fmt };
			fmt_opt *match;
			if(!(match = bsearch(&key, opts, nopts, sizeof(fmt_opt), fmt_compare))) {
				++fmt;
				continue;
			}
			if(match->p == *fmt) {
				switch(match->type) {
					case FmtTypeInteger: {
						int off = snint(output, len, *(int*)match->ptr, 10, padding);
						output += off;
						len -= off;
						break;
					}
					case FmtTypeString: {
						size_t l = strlen(match->ptr);
						if(l > len)
							l = len;
						strncpy(output, match->ptr, l);
						output += l;
						len -= l;
						break;
					}
					case FmtTypeHuman: {
						unsigned long i = *(unsigned long*)match->ptr;
						int tier = 0;
						while(i >= 1024) {
							i /= 1024;
							tier++;
						}
						int off = snint(output, len, i, 10, padding);
						output += off;
						len -= off;
						if(tier && len) {
							*(output++) = (char[]){ 'K', 'M', 'G', 'T' }[tier - 1];
							--len;
						}
					}
				}
			}
			if(!len)
				goto exit;
		}
		++fmt;
	}
exit:
	*output = '\0';
	return baselen - len - 1;
}
