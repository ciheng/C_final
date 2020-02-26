#define _GNU_SOURCE
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include "sbuffer.h"
#include "config.h"
#include <stdint.h>
#include <time.h>
#include "connmgr.h"
#include "datamgr.h"
#include "sensor_db.h"
#include "sbuffer.h"
#include "lib/dplist.h"
#include "lib/tcpsock.h"
#include <sys/stat.h>
#include <sys/types.h>
#include "errmacros.h"
#include <sqlite3.h>


void writeFIFO(char* msg);
