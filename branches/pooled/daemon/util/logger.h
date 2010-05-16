#ifndef LOGGER_H
#define LOGGER_H

#include <stdlib.h>
#include <stdio.h>

void log_success (FILE *log, const char *msg, ...);

void log_failure (FILE *log, const char *msg, ...);

void log_recv (FILE *log, const char *msg, ...);

void log_send (FILE *log, const char *msg, ...);

#endif//LOGGER_H
