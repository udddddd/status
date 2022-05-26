#pragma once

typedef struct fmt_opt {
	char p;
	enum { FmtTypeInteger, FmtTypeString, FmtTypeHuman } type;
	const void *ptr;
} fmt_opt;

int snfmt(char *output, size_t len, const char *fmt, const fmt_opt *opts, size_t nopts);
