#include "csyslog.h"
#include <execinfo.h>
#include <ctime>

using namespace core;

void csyslog::bt(int deep) const {
	void* array[deep];
	fprintf(stderr, "\n");
	int size = backtrace(array, deep);
	std::tm tm; syslog.ts(tm);
	if (size) {
		char** messages = backtrace_symbols(array, size);
		for (int n = 0; n < size; n++) {
			fprintf(stderr, "%02d/%02d %02d:%02d:%02d [ TRACE ] #%2d %s\n", tm.tm_mday, tm.tm_mon + 1, tm.tm_hour, tm.tm_min, tm.tm_sec, n, messages[n]);
		}
		free(messages);
	}
	else {
		fprintf(stderr, "%02d/%02d %02d:%02d:%02d [ TRACE ] No backtrace symbols\n", tm.tm_mday, tm.tm_mon + 1, tm.tm_hour, tm.tm_min, tm.tm_sec);
	}
	fflush(stderr);
}