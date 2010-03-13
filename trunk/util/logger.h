#ifndef LOGGER_H
#define LOGGER_H

#include <stdlib.h>
#include <stdio.h>

void log_success (FILE *log, char *msg, ...);

void log_failure (FILE *log, char *msg, ...);

#endif//LOGGER_H
