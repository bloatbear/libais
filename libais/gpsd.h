/* gpsd.h -- fundamental types and structures for the gpsd library
 *
 * This file is Copyright (c) 2010 by the GPSD project
 * BSD terms apply: see the file COPYING in the distribution root for details.
 */

#ifndef _GPSD_H_
#define _GPSD_H_

/* Feature configuration switches end here
 *
 * This file is Copyright (c) 2010 by the GPSD project
 * BSD terms apply: see the file COPYING in the distribution root for details.
 */

#include <termios.h>
#include <stdint.h>
#include <stdarg.h>
#include "gps.h"

/*
 * Tell GCC that we want thread-safe behavior with _REENTRANT;
 * in particular, errno must be thread-local.
 * Tell POSIX-conforming implementations with _POSIX_THREAD_SAFE_FUNCTIONS.
 * See http://www.unix.org/whitepapers/reentrant.html
 */

/*
 * For NMEA-conforming receivers this is supposed to be 82, but
 * some receivers (TN-200, GSW 2.3.2) emit oversized sentences.
 * The current hog champion is the Trimble BX-960 receiver, which
 * emits a 91-character GGA message.
 */
#define NMEA_MAX	91		/* max length of NMEA sentence */
#define NMEA_BIG_BUF	(2*NMEA_MAX+1)	/* longer than longest NMEA sentence */

/*
 * The packet buffers need to be as long than the longest packet we
 * expect to see in any protocol, because we have to be able to hold
 * an entire packet for checksumming...
 * First we thought it had to be big enough for a SiRF Measured Tracker
 * Data packet (188 bytes). Then it had to be big enough for a UBX SVINFO
 * packet (206 bytes). Now it turns out that a couple of ITALK messages are
 * over 512 bytes. I know we like verbose output, but this is ridiculous.
 */
#define MAX_PACKET_LENGTH	516	/* 7 + 506 + 3 */

struct gpsd_errout_t {
    int debug;				/* lexer debug level */
    void (*report)(const char *);	/* reporting hook for lexer errors */
    char *label;
};

/* state for resolving interleaved Type 24 packets */
struct ais_type24a_t {
    unsigned int mmsi;
    char shipname[AIS_SHIPNAME_MAXLEN+1];
};
#define MAX_TYPE24_INTERLEAVE	8	/* max number of queued type 24s */
struct ais_type24_queue_t {
    struct ais_type24a_t ships[MAX_TYPE24_INTERLEAVE];
    int index;
};

/* state for resolving AIVDM decodes */
struct aivdm_context_t {
    /* hold context for decoding AIDVM packet sequences */
    int decoded_frags;		/* for tracking AIDVM parts in a multipart sequence */
    unsigned char bits[2048];
    size_t bitlen; /* how many valid bits */
    struct ais_type24_queue_t type24_queue;
};

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

# if __GNUC__ >= 3 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 7)
__attribute__((__format__(__printf__, 3, 4))) void gpsd_report(const struct gpsd_errout_t *, const int, const char *, ...);
# else /* not a new enough GCC, use the unprotected prototype */
void gpsd_report(const struct gpsd_errout_t *, const int, const char *, ...);
#endif

#define NITEMS(x) (int)(sizeof(x)/sizeof(x[0]))

#endif /* _GPSD_H_ */
// Local variables:
// mode: c
// end:
