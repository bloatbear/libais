//
//  libais.h
//  libais
//
//  Created by Chris Bünger on 26/10/14.
//  Copyright (c) 2014 Chris Bünger. All rights reserved.
//

#ifndef __libais__libais__
#define __libais__libais__

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <stdbool.h>

#include "bits.h"
#include "gpsd.h"
#include "gps_json.h"

//#define JSON_BOOL(x)	((x)?"true":"false")
#define NITEMS(x) (int)(sizeof(x)/sizeof(x[0]))

//#define JSON_ATTR_MAX	31	/* max chars in JSON attribute name */
//#define JSON_VAL_MAX	512	/* max chars in JSON value part */


/* logging levels */
#define LOG_ERROR 	-1	/* errors, display always */
#define LOG_SHOUT	0	/* not an error but we should always see it */
#define LOG_WARN	1	/* not errors but may indicate a problem */
#define LOG_CLIENT	2	/* log JSON reports to clients */
#define LOG_INF 	3	/* key informative messages */
#define LOG_PROG	4	/* progress messages */
#define LOG_IO  	5	/* IO to and from devices */
#define LOG_DATA	6	/* log data management messages */
#define LOG_SPIN	7	/* logging for catching spin bugs */
#define LOG_RAW 	8	/* raw low-level I/O */

extern /*@ observer @*/ const char *gpsd_hexdump(/*@out@*/char *, size_t,
                                                 /*@null@*/char *, size_t);

extern bool aivdm_decode(const char *buf, size_t buflen,
                         struct gps_device_t *session,
                         struct ais_t *ais,
                         int debug);

extern bool ais_binary_decode(const struct gpsd_errout_t *errout,
                              struct ais_t *ais,
                              const unsigned char *, size_t,
                              /*@null@*/struct ais_type24_queue_t *);

void gpsd_report(const struct gpsd_errout_t *, const int, const char *, ...);

#endif /* defined(__libais__libais__) */
