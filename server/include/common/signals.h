#ifndef _SIGNALS_H
#define _SIGNALS_H

#include <stdbool.h>

int  signals_install(int sig);          
bool signals_should_terminate(void); 

#endif