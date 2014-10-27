/****************************************************************************

NAME
   gpsd_json.c - move data between in-core and JSON structures

DESCRIPTION
   These are functions (used only by the daemon) to dump the contents
of various core data structures in JSON.

PERMISSIONS
  Written by Eric S. Raymond, 2009
  This file is Copyright (c) 2010 by the GPSD project
  BSD terms apply: see the file COPYING in the distribution root for details.

***************************************************************************/

#include <stdio.h>
#include <math.h>
#include <assert.h>
#include <string.h>
#include <ctype.h>

#include "gpsd.h"
#include "bits.h"

#include "gps_json.h"

#define JSON_BOOL(x)	((x)?"true":"false")

char *json_stringify( /*@out@*/ char *to,
		     size_t len,
		     /*@in@*/ const char *from)
/* escape double quotes and control characters inside a JSON string */
{
    /*@-temptrans@*/
    const char *sp;
    char *tp;

    tp = to;
    /*
     * The limit is len-6 here because we need to be leave room for
     * each character to generate an up to 6-character Java-style
     * escape
     */
    for (sp = from; *sp != '\0' && ((tp - to) < ((int)len - 6)); sp++) {
	if (!isascii((unsigned char) *sp) || iscntrl((unsigned char) *sp)) {
	    *tp++ = '\\';
	    switch (*sp) {
	    case '\b':
		*tp++ = 'b';
		break;
	    case '\f':
		*tp++ = 'f';
		break;
	    case '\n':
		*tp++ = 'n';
		break;
	    case '\r':
		*tp++ = 'r';
		break;
	    case '\t':
		*tp++ = 't';
		break;
	    default:
		/* ugh, we'd prefer a C-style escape here, but this is JSON */
		/* http://www.ietf.org/rfc/rfc4627.txt
		 * section 2.5, escape is \uXXXX */
		/* don't forget the NUL in the output count! */
		(void)snprintf(tp, 6, "u%04x", 0x00ff & (unsigned int)*sp);
		tp += strlen(tp);
	    }
	} else {
	    if (*sp == '"' || *sp == '\\')
		*tp++ = '\\';
	    *tp++ = *sp;
	}
    }
    *tp = '\0';

    return to;
    /*@+temptrans@*/
}

void json_aivdm_dump(const struct ais_t *ais,
                     /*@null@*/const char *device, bool scaled,
                     /*@out@*/char *buf, size_t buflen)
{
    char buf1[JSON_VAL_MAX * 2 + 1];
    char buf2[JSON_VAL_MAX * 2 + 1];
    char buf3[JSON_VAL_MAX * 2 + 1];
    char scratchbuf[MAX_PACKET_LENGTH*2+1];
    int i;
    
    static char *nav_legends[] = {
        "Under way using engine",
        "At anchor",
        "Not under command",
        "Restricted manoeuverability",
        "Constrained by her draught",
        "Moored",
        "Aground",
        "Engaged in fishing",
        "Under way sailing",
        "Reserved for HSC",
        "Reserved for WIG",
        "Reserved",
        "Reserved",
        "Reserved",
        "Reserved",
        "Not defined",
    };
    
    static char *epfd_legends[] = {
        "Undefined",
        "GPS",
        "GLONASS",
        "Combined GPS/GLONASS",
        "Loran-C",
        "Chayka",
        "Integrated navigation system",
        "Surveyed",
        "Galileo",
    };
    
#define EPFD_DISPLAY(n) (((n) < (unsigned int)NITEMS(epfd_legends)) ? epfd_legends[n] : "INVALID EPFD")
    
    static char *ship_type_legends[100] = {
        "Not available",
        "Reserved for future use",
        "Reserved for future use",
        "Reserved for future use",
        "Reserved for future use",
        "Reserved for future use",
        "Reserved for future use",
        "Reserved for future use",
        "Reserved for future use",
        "Reserved for future use",
        "Reserved for future use",
        "Reserved for future use",
        "Reserved for future use",
        "Reserved for future use",
        "Reserved for future use",
        "Reserved for future use",
        "Reserved for future use",
        "Reserved for future use",
        "Reserved for future use",
        "Reserved for future use",
        "Wing in ground (WIG) - all ships of this type",
        "Wing in ground (WIG) - Hazardous category A",
        "Wing in ground (WIG) - Hazardous category B",
        "Wing in ground (WIG) - Hazardous category C",
        "Wing in ground (WIG) - Hazardous category D",
        "Wing in ground (WIG) - Reserved for future use",
        "Wing in ground (WIG) - Reserved for future use",
        "Wing in ground (WIG) - Reserved for future use",
        "Wing in ground (WIG) - Reserved for future use",
        "Wing in ground (WIG) - Reserved for future use",
        "Fishing",
        "Towing",
        "Towing: length exceeds 200m or breadth exceeds 25m",
        "Dredging or underwater ops",
        "Diving ops",
        "Military ops",
        "Sailing",
        "Pleasure Craft",
        "Reserved",
        "Reserved",
        "High speed craft (HSC) - all ships of this type",
        "High speed craft (HSC) - Hazardous category A",
        "High speed craft (HSC) - Hazardous category B",
        "High speed craft (HSC) - Hazardous category C",
        "High speed craft (HSC) - Hazardous category D",
        "High speed craft (HSC) - Reserved for future use",
        "High speed craft (HSC) - Reserved for future use",
        "High speed craft (HSC) - Reserved for future use",
        "High speed craft (HSC) - Reserved for future use",
        "High speed craft (HSC) - No additional information",
        "Pilot Vessel",
        "Search and Rescue vessel",
        "Tug",
        "Port Tender",
        "Anti-pollution equipment",
        "Law Enforcement",
        "Spare - Local Vessel",
        "Spare - Local Vessel",
        "Medical Transport",
        "Ship according to RR Resolution No. 18",
        "Passenger - all ships of this type",
        "Passenger - Hazardous category A",
        "Passenger - Hazardous category B",
        "Passenger - Hazardous category C",
        "Passenger - Hazardous category D",
        "Passenger - Reserved for future use",
        "Passenger - Reserved for future use",
        "Passenger - Reserved for future use",
        "Passenger - Reserved for future use",
        "Passenger - No additional information",
        "Cargo - all ships of this type",
        "Cargo - Hazardous category A",
        "Cargo - Hazardous category B",
        "Cargo - Hazardous category C",
        "Cargo - Hazardous category D",
        "Cargo - Reserved for future use",
        "Cargo - Reserved for future use",
        "Cargo - Reserved for future use",
        "Cargo - Reserved for future use",
        "Cargo - No additional information",
        "Tanker - all ships of this type",
        "Tanker - Hazardous category A",
        "Tanker - Hazardous category B",
        "Tanker - Hazardous category C",
        "Tanker - Hazardous category D",
        "Tanker - Reserved for future use",
        "Tanker - Reserved for future use",
        "Tanker - Reserved for future use",
        "Tanker - Reserved for future use",
        "Tanker - No additional information",
        "Other Type - all ships of this type",
        "Other Type - Hazardous category A",
        "Other Type - Hazardous category B",
        "Other Type - Hazardous category C",
        "Other Type - Hazardous category D",
        "Other Type - Reserved for future use",
        "Other Type - Reserved for future use",
        "Other Type - Reserved for future use",
        "Other Type - Reserved for future use",
        "Other Type - no additional information",
    };
    
#define SHIPTYPE_DISPLAY(n) (((n) < (unsigned int)NITEMS(ship_type_legends)) ? ship_type_legends[n] : "INVALID SHIP TYPE")
    
    static const char *station_type_legends[] = {
        "All types of mobiles",
        "Reserved for future use",
        "All types of Class B mobile stations",
        "SAR airborne mobile station",
        "Aid to Navigation station",
        "Class B shipborne mobile station",
        "Regional use and inland waterways",
        "Regional use and inland waterways",
        "Regional use and inland waterways",
        "Regional use and inland waterways",
        "Reserved for future use",
        "Reserved for future use",
        "Reserved for future use",
        "Reserved for future use",
        "Reserved for future use",
        "Reserved for future use",
    };
    
#define STATIONTYPE_DISPLAY(n) (((n) < (unsigned int)NITEMS(station_type_legends)) ? station_type_legends[n] : "INVALID STATION TYPE")
    
    static const char *navaid_type_legends[] = {
        "Unspecified",
        "Reference point",
        "RACON",
        "Fixed offshore structure",
        "Spare, Reserved for future use.",
        "Light, without sectors",
        "Light, with sectors",
        "Leading Light Front",
        "Leading Light Rear",
        "Beacon, Cardinal N",
        "Beacon, Cardinal E",
        "Beacon, Cardinal S",
        "Beacon, Cardinal W",
        "Beacon, Port hand",
        "Beacon, Starboard hand",
        "Beacon, Preferred Channel port hand",
        "Beacon, Preferred Channel starboard hand",
        "Beacon, Isolated danger",
        "Beacon, Safe water",
        "Beacon, Special mark",
        "Cardinal Mark N",
        "Cardinal Mark E",
        "Cardinal Mark S",
        "Cardinal Mark W",
        "Port hand Mark",
        "Starboard hand Mark",
        "Preferred Channel Port hand",
        "Preferred Channel Starboard hand",
        "Isolated danger",
        "Safe Water",
        "Special Mark",
        "Light Vessel / LANBY / Rigs",
    };
    
#define NAVAIDTYPE_DISPLAY(n) (((n) < (unsigned int)NITEMS(navaid_type_legends[0])) ? navaid_type_legends[n] : "INVALID NAVAID TYPE")
    
    // cppcheck-suppress variableScope
    static const char *signal_legends[] = {
        "N/A",
        "Serious emergency – stop or divert according to instructions.",
        "Vessels shall not proceed.",
        "Vessels may proceed. One way traffic.",
        "Vessels may proceed. Two way traffic.",
        "Vessels shall proceed on specific orders only.",
        "Vessels in main channel shall not proceed."
        "Vessels in main channel shall proceed on specific orders only.",
        "Vessels in main channel shall proceed on specific orders only.",
        "I = \"in-bound\" only acceptable.",
        "O = \"out-bound\" only acceptable.",
        "F = both \"in- and out-bound\" acceptable.",
        "XI = Code will shift to \"I\" in due time.",
        "XO = Code will shift to \"O\" in due time.",
        "X = Vessels shall proceed only on direction.",
    };
    
#define SIGNAL_DISPLAY(n) (((n) < (unsigned int)NITEMS(signal_legends[0])) ? signal_legends[n] : "INVALID SIGNAL TYPE")
    
    static const char *route_type[32] = {
        "Undefined (default)",
        "Mandatory",
        "Recommended",
        "Alternative",
        "Recommended route through ice",
        "Ship route plan",
        "Reserved for future use.",
        "Reserved for future use.",
        "Reserved for future use.",
        "Reserved for future use.",
        "Reserved for future use.",
        "Reserved for future use.",
        "Reserved for future use.",
        "Reserved for future use.",
        "Reserved for future use.",
        "Reserved for future use.",
        "Reserved for future use.",
        "Reserved for future use.",
        "Reserved for future use.",
        "Reserved for future use.",
        "Reserved for future use.",
        "Reserved for future use.",
        "Reserved for future use.",
        "Reserved for future use.",
        "Reserved for future use.",
        "Reserved for future use.",
        "Reserved for future use.",
        "Reserved for future use.",
        "Reserved for future use.",
        "Reserved for future use.",
        "Reserved for future use.",
        "Cancel route identified by message linkage",
    };
    
    // cppcheck-suppress variableScope
    static const char *idtypes[] = {
        "mmsi",
        "imo",
        "callsign",
        "other",
    };
    
    // cppcheck-suppress variableScope
    static const char *racon_status[] = {
        "No RACON installed",
        "RACON not monitored",
        "RACON operational",
        "RACON ERROR"
    };
    
    // cppcheck-suppress variableScope
    static const char *light_status[] = {
        "No light or no monitoring",
        "Light ON",
        "Light OFF",
        "Light ERROR"
    };
    
    // cppcheck-suppress variableScope
    static const char *rta_status[] = {
        "Operational",
        "Limited operation",
        "Out of order",
        "N/A",
    };
    
    // cppcheck-suppress variableScope
    const char *position_types[8] = {
        "Not available",
        "Port-side to",
        "Starboard-side to",
        "Mediterranean (end-on) mooring",
        "Mooring buoy",
        "Anchorage",
        "Reserved for future use",
        "Reserved for future use",
    };
    
    (void)snprintf(buf, buflen, "{\"class\":\"AIS\",");
    if (device != NULL && device[0] != '\0')
        (void)snprintf(buf + strlen(buf), buflen - strlen(buf),
                       "\"device\":\"%s\",", device);
    (void)snprintf(buf + strlen(buf), buflen - strlen(buf),
                   "\"type\":%u,\"repeat\":%u,\"mmsi\":%u,\"scaled\":%s,",
                   ais->type, ais->repeat, ais->mmsi, JSON_BOOL(scaled));
    /*@ -formatcode -mustfreefresh @*/
    switch (ais->type) {
        case 1:			/* Position Report */
        case 2:
        case 3:
            if (scaled) {
                char turnlegend[20];
                char speedlegend[20];
                
                /*
                 * Express turn as nan if not available,
                 * "fastleft"/"fastright" for fast turns.
                 */
                if (ais->type1.turn == -128)
                    (void)strlcpy(turnlegend, "\"nan\"", sizeof(turnlegend));
                else if (ais->type1.turn == -127)
                    (void)strlcpy(turnlegend, "\"fastleft\"", sizeof(turnlegend));
                else if (ais->type1.turn == 127)
                    (void)strlcpy(turnlegend, "\"fastright\"",
                                  sizeof(turnlegend));
                else {
                    double rot1 = ais->type1.turn / 4.733;
                    (void)snprintf(turnlegend, sizeof(turnlegend),
                                   "%.0f", rot1 * rot1);
                }
                
                /*
                 * Express speed as nan if not available,
                 * "fast" for fast movers.
                 */
                if (ais->type1.speed == AIS_SPEED_NOT_AVAILABLE)
                    (void)strlcpy(speedlegend, "\"nan\"", sizeof(speedlegend));
                else if (ais->type1.speed == AIS_SPEED_FAST_MOVER)
                    (void)strlcpy(speedlegend, "\"fast\"", sizeof(speedlegend));
                else
                    (void)snprintf(speedlegend, sizeof(speedlegend),
                                   "%.1f", ais->type1.speed / 10.0);
                
                (void)snprintf(buf + strlen(buf), buflen - strlen(buf),
                               "\"status\":\"%u\",\"status_text\":\"%s\","
                               "\"turn\":%s,\"speed\":%s,"
                               "\"accuracy\":%s,\"lon\":%.4f,\"lat\":%.4f,"
                               "\"course\":%.1f,\"heading\":%u,\"second\":%u,"
                               "\"maneuver\":%u,\"raim\":%s,\"radio\":%u}\r\n",
                               ais->type1.status,
                               nav_legends[ais->type1.status],
                               turnlegend,
                               speedlegend,
                               JSON_BOOL(ais->type1.accuracy),
                               ais->type1.lon / AIS_LATLON_DIV,
                               ais->type1.lat / AIS_LATLON_DIV,
                               ais->type1.course / 10.0,
                               ais->type1.heading,
                               ais->type1.second,
                               ais->type1.maneuver,
                               JSON_BOOL(ais->type1.raim), ais->type1.radio);
            } else {
                (void)snprintf(buf + strlen(buf), buflen - strlen(buf),
                               "\"status\":%u,\"status_text\":\"%s\","
                               "\"turn\":%d,\"speed\":%u,"
                               "\"accuracy\":%s,\"lon\":%d,\"lat\":%d,"
                               "\"course\":%u,\"heading\":%u,\"second\":%u,"
                               "\"maneuver\":%u,\"raim\":%s,\"radio\":%u}\r\n",
                               ais->type1.status,
                               nav_legends[ais->type1.status],
                               ais->type1.turn,
                               ais->type1.speed,
                               JSON_BOOL(ais->type1.accuracy),
                               ais->type1.lon,
                               ais->type1.lat,
                               ais->type1.course,
                               ais->type1.heading,
                               ais->type1.second,
                               ais->type1.maneuver,
                               JSON_BOOL(ais->type1.raim), ais->type1.radio);
            }
            break;
        case 4:			/* Base Station Report */
        case 11:			/* UTC/Date Response */
            /* some fields have beem merged to an ISO8601 date */
            if (scaled) {
                // The use of %u instead of %04u for the year is to allow
                // out-of-band year values.
                (void)snprintf(buf + strlen(buf), buflen - strlen(buf),
                               "\"timestamp\":\"%04u-%02u-%02uT%02u:%02u:%02uZ\","
                               "\"accuracy\":%s,\"lon\":%.4f,\"lat\":%.4f,"
                               "\"epfd\":%u,\"epfd_text\":\"%s\","
                               "\"raim\":%s,\"radio\":%u}\r\n",
                               ais->type4.year,
                               ais->type4.month,
                               ais->type4.day,
                               ais->type4.hour,
                               ais->type4.minute,
                               ais->type4.second,
                               JSON_BOOL(ais->type4.accuracy),
                               ais->type4.lon / AIS_LATLON_DIV,
                               ais->type4.lat / AIS_LATLON_DIV,
                               ais->type4.epfd,
                               EPFD_DISPLAY(ais->type4.epfd),
                               JSON_BOOL(ais->type4.raim), ais->type4.radio);
            } else {
                (void)snprintf(buf + strlen(buf), buflen - strlen(buf),
                               "\"timestamp\":\"%04u-%02u-%02uT%02u:%02u:%02uZ\","
                               "\"accuracy\":%s,\"lon\":%d,\"lat\":%d,"
                               "\"epfd\":%u,\"epfd_text\":\"%s\","
                               "\"raim\":%s,\"radio\":%u}\r\n",
                               ais->type4.year,
                               ais->type4.month,
                               ais->type4.day,
                               ais->type4.hour,
                               ais->type4.minute,
                               ais->type4.second,
                               JSON_BOOL(ais->type4.accuracy),
                               ais->type4.lon,
                               ais->type4.lat,
                               ais->type4.epfd,
                               EPFD_DISPLAY(ais->type4.epfd),
                               JSON_BOOL(ais->type4.raim), ais->type4.radio);
            }
            break;
        case 5:			/* Ship static and voyage related data */
            /* some fields have beem merged to an ISO8601 partial date */
            if (scaled) {
                /* *INDENT-OFF* */
                (void)snprintf(buf + strlen(buf), buflen - strlen(buf),
                               "\"imo\":%u,\"ais_version\":%u,\"callsign\":\"%s\","
                               "\"shipname\":\"%s\","
                               "\"shiptype\":%u,\"shiptype_text\":\"%s\","
                               "\"to_bow\":%u,\"to_stern\":%u,\"to_port\":%u,"
                               "\"to_starboard\":%u,"
                               "\"epfd\":%u,\"epfd_text\":\"%s\","
                               "\"eta\":\"%02u-%02uT%02u:%02uZ\","
                               "\"draught\":%.1f,\"destination\":\"%s\","
                               "\"dte\":%u}\r\n",
                               ais->type5.imo,
                               ais->type5.ais_version,
                               json_stringify(buf1, sizeof(buf1),
                                              ais->type5.callsign),
                               json_stringify(buf2, sizeof(buf2),
                                              ais->type5.shipname),
                               ais->type5.shiptype,
                               SHIPTYPE_DISPLAY(ais->type5.shiptype),
                               ais->type5.to_bow, ais->type5.to_stern,
                               ais->type5.to_port, ais->type5.to_starboard,
                               ais->type5.epfd,
                               EPFD_DISPLAY(ais->type5.epfd),
                               ais->type5.month,
                               ais->type5.day,
                               ais->type5.hour, ais->type5.minute,
                               ais->type5.draught / 10.0,
                               json_stringify(buf3, sizeof(buf3),
                                              ais->type5.destination),
                               ais->type5.dte);
                /* *INDENT-ON* */
            } else {
                (void)snprintf(buf + strlen(buf), buflen - strlen(buf),
                               "\"imo\":%u,\"ais_version\":%u,\"callsign\":\"%s\","
                               "\"shipname\":\"%s\","
                               "\"shiptype\":%u,\"shiptype_text\":\"%s\","
                               "\"to_bow\":%u,\"to_stern\":%u,\"to_port\":%u,"
                               "\"to_starboard\":%u,"
                               "\"epfd\":%u,\"epfd_text\":\"%s\","
                               "\"eta\":\"%02u-%02uT%02u:%02uZ\","
                               "\"draught\":%u,\"destination\":\"%s\","
                               "\"dte\":%u}\r\n",
                               ais->type5.imo,
                               ais->type5.ais_version,
                               json_stringify(buf1, sizeof(buf1),
                                              ais->type5.callsign),
                               json_stringify(buf2, sizeof(buf2),
                                              ais->type5.shipname),
                               ais->type5.shiptype,
                               SHIPTYPE_DISPLAY(ais->type5.shiptype),
                               ais->type5.to_bow,
                               ais->type5.to_stern,
                               ais->type5.to_port,
                               ais->type5.to_starboard,
                               ais->type5.epfd,
                               EPFD_DISPLAY(ais->type5.epfd),
                               ais->type5.month,
                               ais->type5.day,
                               ais->type5.hour,
                               ais->type5.minute,
                               ais->type5.draught,
                               json_stringify(buf3, sizeof(buf3),
                                              ais->type5.destination),
                               ais->type5.dte);
            }
            break;
        case 6:			/* Binary Message */
            (void)snprintf(buf + strlen(buf), buflen - strlen(buf),
                           "\"seqno\":%u,\"dest_mmsi\":%u,"
                           "\"retransmit\":%s,\"dac\":%u,\"fid\":%u,",
                           ais->type6.seqno,
                           ais->type6.dest_mmsi,
                           JSON_BOOL(ais->type6.retransmit),
                           ais->type6.dac,
                           ais->type6.fid);
            if (!ais->type6.structured) {
                (void)snprintf(buf + strlen(buf), buflen - strlen(buf),
                               "\"data\":\"%zd:%s\"}\r\n",
                               ais->type6.bitcount,
                               json_stringify(buf1, sizeof(buf1),
                                              gpsd_hexdump(scratchbuf, sizeof(scratchbuf),
                                                           (char *)ais->type6.bitdata,
                                                           BITS_TO_BYTES(ais->type6.bitcount))));
                break;
            }
            if (ais->type6.dac == 200) {
                switch (ais->type6.fid) {
                    case 21:
                        (void)snprintf(buf + strlen(buf), buflen - strlen(buf),
                                       "\"country\":\"%s\",\"locode\":\"%s\",\"section\":\"%s\",\"terminal\":\"%s\",\"hectometre\":\"%s\",\"eta\":\"%u-%uT%u:%u\",\"tugs\":%u,\"airdraught\":%u}",
                                       ais->type6.dac200fid21.country,
                                       ais->type6.dac200fid21.locode,
                                       ais->type6.dac200fid21.section,
                                       ais->type6.dac200fid21.terminal,
                                       ais->type6.dac200fid21.hectometre,
                                       ais->type6.dac200fid21.month,
                                       ais->type6.dac200fid21.day,
                                       ais->type6.dac200fid21.hour,
                                       ais->type6.dac200fid21.minute,
                                       ais->type6.dac200fid21.tugs,
                                       ais->type6.dac200fid21.airdraught);
                        break;
                    case 22:
                        (void)snprintf(buf + strlen(buf), buflen - strlen(buf),
                                       "\"country\":\"%s\",\"locode\":\"%s\","
                                       "\"section\":\"%s\","
                                       "\"terminal\":\"%s\",\"hectometre\":\"%s\","
                                       "\"eta\":\"%u-%uT%u:%u\","
                                       "\"status\":%u,\"status_text\":\"%s\"}",
                                       ais->type6.dac200fid22.country,
                                       ais->type6.dac200fid22.locode,
                                       ais->type6.dac200fid22.section,
                                       ais->type6.dac200fid22.terminal,
                                       ais->type6.dac200fid22.hectometre,
                                       ais->type6.dac200fid22.month,
                                       ais->type6.dac200fid22.day,
                                       ais->type6.dac200fid22.hour,
                                       ais->type6.dac200fid22.minute,
                                       ais->type6.dac200fid22.status,
                                       rta_status[ais->type6.dac200fid22.status]);
                        break;
                    case 55:
                        (void)snprintf(buf + strlen(buf), buflen - strlen(buf),
                                       "\"crew\":%u,\"passengers\":%u,\"personnel\":%u}",
                                       
                                       ais->type6.dac200fid55.crew,
                                       ais->type6.dac200fid55.passengers,
                                       ais->type6.dac200fid55.personnel);
                        break;
                }
            }
            else if (ais->type6.dac == 235 || ais->type6.dac == 250) {
                switch (ais->type6.fid) {
                    case 10:	/* GLA - AtoN monitoring data */
                        (void)snprintf(buf + strlen(buf), buflen - strlen(buf),
                                       "\"off_pos\":%s,\"alarm\":%s,"
                                       "\"stat_ext\":%u,",
                                       JSON_BOOL(ais->type6.dac235fid10.off_pos),
                                       JSON_BOOL(ais->type6.dac235fid10.alarm),
                                       ais->type6.dac235fid10.stat_ext);
                        if (scaled && ais->type6.dac235fid10.ana_int != 0)
                            (void)snprintf(buf + strlen(buf), buflen - strlen(buf),
                                           "\"ana_int\":%.2f,",
                                           ais->type6.dac235fid10.ana_int*0.05);
                        else
                            (void)snprintf(buf + strlen(buf), buflen - strlen(buf),
                                           "\"ana_int\":%u,",
                                           ais->type6.dac235fid10.ana_int);
                        if (scaled && ais->type6.dac235fid10.ana_ext1 != 0)
                            (void)snprintf(buf + strlen(buf), buflen - strlen(buf),
                                           "\"ana_ext1\":%.2f,",
                                           ais->type6.dac235fid10.ana_ext1*0.05);
                        else
                            (void)snprintf(buf + strlen(buf), buflen - strlen(buf),
                                           "\"ana_ext1\":%u,",
                                           ais->type6.dac235fid10.ana_ext1);
                        if (scaled && ais->type6.dac235fid10.ana_ext2 != 0)
                            (void)snprintf(buf + strlen(buf), buflen - strlen(buf),
                                           "\"ana_ext2\":%.2f,",
                                           ais->type6.dac235fid10.ana_ext2*0.05);
                        else
                            (void)snprintf(buf + strlen(buf), buflen - strlen(buf),
                                           "\"ana_ext2\":%u,",
                                           ais->type6.dac235fid10.ana_ext2);
                        (void)snprintf(buf + strlen(buf), buflen - strlen(buf),
                                       "\"racon\":%u,"
                                       "\"racon_text\":\"%s\","
                                       "\"light\":%u,"
                                       "\"light_text\":\"%s\"",
                                       ais->type6.dac235fid10.racon,
                                       racon_status[ais->type6.dac235fid10.racon],
                                       ais->type6.dac235fid10.light,
                                       light_status[ais->type6.dac235fid10.light]);
                        if (buf[strlen(buf) - 1] == ',')
                            buf[strlen(buf)-1] = '\0';
                        (void)strlcat(buf, "}\r\n", buflen);
                        break;
                }
            }
            else if (ais->type6.dac == 1) {
                char buf4[JSON_VAL_MAX * 2 + 1];
                switch (ais->type6.fid) {
                    case 12:	/* IMO236 -Dangerous cargo indication */
                        /* some fields have beem merged to an ISO8601 partial date */
                        (void)snprintf(buf + strlen(buf), buflen - strlen(buf),
                                       "\"lastport\":\"%s\",\"departure\":\"%02u-%02uT%02u:%02uZ\","
                                       "\"nextport\":\"%s\",\"eta\":\"%02u-%02uT%02u:%02uZ\","
                                       "\"dangerous\":\"%s\",\"imdcat\":\"%s\","
                                       "\"unid\":%u,\"amount\":%u,\"unit\":%u}\r\n",
                                       json_stringify(buf1, sizeof(buf1),
                                                      ais->type6.dac1fid12.lastport),
                                       ais->type6.dac1fid12.lmonth,
                                       ais->type6.dac1fid12.lday,
                                       ais->type6.dac1fid12.lhour,
                                       ais->type6.dac1fid12.lminute,
                                       json_stringify(buf2, sizeof(buf2),
                                                      ais->type6.dac1fid12.nextport),
                                       ais->type6.dac1fid12.nmonth,
                                       ais->type6.dac1fid12.nday,
                                       ais->type6.dac1fid12.nhour,
                                       ais->type6.dac1fid12.nminute,
                                       json_stringify(buf3, sizeof(buf3),
                                                      ais->type6.dac1fid12.dangerous),
                                       json_stringify(buf4, sizeof(buf4),
                                                      ais->type6.dac1fid12.imdcat),
                                       ais->type6.dac1fid12.unid,
                                       ais->type6.dac1fid12.amount,
                                       ais->type6.dac1fid12.unit);
                        break;
                    case 15:	/* IMO236 - Extended Ship Static and Voyage Related Data */
                        (void)snprintf(buf + strlen(buf), buflen - strlen(buf),
                                       "\"airdraught\":%u}\r\n",
                                       ais->type6.dac1fid15.airdraught);
                        break;
                    case 16:	/* IMO236 - Number of persons on board */
                        (void)snprintf(buf + strlen(buf), buflen - strlen(buf),
                                       "\"persons\":%u}\t\n", ais->type6.dac1fid16.persons);
                        break;
                    case 18:	/* IMO289 - Clearance time to enter port */
                        (void)snprintf(buf + strlen(buf), buflen - strlen(buf),
                                       "\"linkage\":%u,\"arrival\":\"%02u-%02uT%02u:%02uZ\",\"portname\":\"%s\",\"destination\":\"%s\",",
                                       ais->type6.dac1fid18.linkage,
                                       ais->type6.dac1fid18.month,
                                       ais->type6.dac1fid18.day,
                                       ais->type6.dac1fid18.hour,
                                       ais->type6.dac1fid18.minute,
                                       json_stringify(buf1, sizeof(buf1),
                                                      ais->type6.dac1fid18.portname),
                                       json_stringify(buf2, sizeof(buf2),
                                                      ais->type6.dac1fid18.destination));
                        if (scaled)
                            (void)snprintf(buf + strlen(buf), buflen - strlen(buf),
                                           "\"lon\":%.3f,\"lat\":%.3f}\r\n",
                                           ais->type6.dac1fid18.lon/AIS_LATLON3_DIV,
                                           ais->type6.dac1fid18.lat/AIS_LATLON3_DIV);
                        else
                            (void)snprintf(buf + strlen(buf), buflen - strlen(buf),
                                           "\"lon\":%d,\"lat\":%d}\r\n",
                                           ais->type6.dac1fid18.lon,
                                           ais->type6.dac1fid18.lat);
                        break;
                    case 20:        /* IMO289 - Berthing Data */
                        (void)snprintf(buf + strlen(buf), buflen - strlen(buf),
                                       "\"linkage\":%u,\"berth_length\":%u,"
                                       "\"position\":%u,\"position_text\":\"%s\","
                                       "\"arrival\":\"%u-%uT%u:%u\","
                                       "\"availability\":%u,"
                                       "\"agent\":%u,\"fuel\":%u,\"chandler\":%u,"
                                       "\"stevedore\":%u,\"electrical\":%u,"
                                       "\"water\":%u,\"customs\":%u,\"cartage\":%u,"
                                       "\"crane\":%u,\"lift\":%u,\"medical\":%u,"
                                       "\"navrepair\":%u,\"provisions\":%u,"
                                       "\"shiprepair\":%u,\"surveyor\":%u,"
                                       "\"steam\":%u,\"tugs\":%u,\"solidwaste\":%u,"
                                       "\"liquidwaste\":%u,\"hazardouswaste\":%u,"
                                       "\"ballast\":%u,\"additional\":%u,"
                                       "\"regional1\":%u,\"regional2\":%u,"
                                       "\"future1\":%u,\"future2\":%u,"
                                       "\"berth_name\":\"%s\",",
                                       ais->type6.dac1fid20.linkage,
                                       ais->type6.dac1fid20.berth_length,
                                       ais->type6.dac1fid20.position,
                                       position_types[ais->type6.dac1fid20.position],
                                       ais->type6.dac1fid20.month,
                                       ais->type6.dac1fid20.day,
                                       ais->type6.dac1fid20.hour,
                                       ais->type6.dac1fid20.minute,
                                       ais->type6.dac1fid20.availability,
                                       ais->type6.dac1fid20.agent,
                                       ais->type6.dac1fid20.fuel,
                                       ais->type6.dac1fid20.chandler,
                                       ais->type6.dac1fid20.stevedore,
                                       ais->type6.dac1fid20.electrical,
                                       ais->type6.dac1fid20.water,
                                       ais->type6.dac1fid20.customs,
                                       ais->type6.dac1fid20.cartage,
                                       ais->type6.dac1fid20.crane,
                                       ais->type6.dac1fid20.lift,
                                       ais->type6.dac1fid20.medical,
                                       ais->type6.dac1fid20.navrepair,
                                       ais->type6.dac1fid20.provisions,
                                       ais->type6.dac1fid20.shiprepair,
                                       ais->type6.dac1fid20.surveyor,
                                       ais->type6.dac1fid20.steam,
                                       ais->type6.dac1fid20.tugs,
                                       ais->type6.dac1fid20.solidwaste,
                                       ais->type6.dac1fid20.liquidwaste,
                                       ais->type6.dac1fid20.hazardouswaste,
                                       ais->type6.dac1fid20.ballast,
                                       ais->type6.dac1fid20.additional,
                                       ais->type6.dac1fid20.regional1,
                                       ais->type6.dac1fid20.regional2,
                                       ais->type6.dac1fid20.future1,
                                       ais->type6.dac1fid20.future2,
                                       json_stringify(buf1, sizeof(buf1),
                                                      ais->type6.dac1fid20.berth_name));
                        if (scaled)
                            (void)snprintf(buf + strlen(buf), buflen - strlen(buf),
                                           "\"berth_lon\":%.3f,"
                                           "\"berth_lat\":%.3f,"
                                           "\"berth_depth\":%.1f}\r\n",
                                           ais->type6.dac1fid20.berth_lon / AIS_LATLON3_DIV,
                                           ais->type6.dac1fid20.berth_lat / AIS_LATLON3_DIV,
                                           ais->type6.dac1fid20.berth_depth * 0.1);
                        else
                            (void)snprintf(buf + strlen(buf), buflen - strlen(buf),
                                           "\"berth_lon\":%d,"
                                           "\"berth_lat\":%d,"
                                           "\"berth_depth\":%u}\r\n",
                                           ais->type6.dac1fid20.berth_lon,
                                           ais->type6.dac1fid20.berth_lat,
                                           ais->type6.dac1fid20.berth_depth);
                        break;
                    case 23:    /* IMO289 - Area notice - addressed */
                        break;
                    case 25:	/* IMO289 - Dangerous cargo indication */
                        (void)snprintf(buf + strlen(buf), buflen - strlen(buf),
                                       "\"unit\":%u,\"amount\":%u,\"cargos\":[",
                                       ais->type6.dac1fid25.unit,
                                       ais->type6.dac1fid25.amount);
                        for (i = 0; i < (int)ais->type6.dac1fid25.ncargos; i++)
                            (void)snprintf(buf + strlen(buf), buflen - strlen(buf),
                                           "{\"code\":%u,\"subtype\":%u},",
                                           
                                           ais->type6.dac1fid25.cargos[i].code,
                                           ais->type6.dac1fid25.cargos[i].subtype);
                        if (buf[strlen(buf) - 1] == ',')
                            buf[strlen(buf) - 1] = '\0';
                        (void)strlcat(buf, "]}\r\n", buflen);
                        break;
                    case 28:	/* IMO289 - Route info - addressed */
                        (void)snprintf(buf + strlen(buf), buflen - strlen(buf),
                                       "\"linkage\":%u,\"sender\":%u,"
                                       "\"rtype\":%u,"
                                       "\"rtype_text\":\"%s\","
                                       "\"start\":\"%02u-%02uT%02u:%02uZ\","
                                       "\"duration\":%u,\"waypoints\":[",
                                       ais->type6.dac1fid28.linkage,
                                       ais->type6.dac1fid28.sender,
                                       ais->type6.dac1fid28.rtype,
                                       route_type[ais->type6.dac1fid28.rtype],
                                       ais->type6.dac1fid28.month,
                                       ais->type6.dac1fid28.day,
                                       ais->type6.dac1fid28.hour,
                                       ais->type6.dac1fid28.minute,
                                       ais->type6.dac1fid28.duration);
                        for (i = 0; i < ais->type6.dac1fid28.waycount; i++) {
                            if (scaled)
                                (void)snprintf(buf + strlen(buf), buflen - strlen(buf),
                                               "{\"lon\":%.4f,\"lat\":%.4f},",
                                               ais->type6.dac1fid28.waypoints[i].lon / AIS_LATLON4_DIV,
                                               ais->type6.dac1fid28.waypoints[i].lat / AIS_LATLON4_DIV);
                            else
                                (void)snprintf(buf + strlen(buf), buflen - strlen(buf),
                                               "{\"lon\":%d,\"lat\":%d},",
                                               ais->type6.dac1fid28.waypoints[i].lon,
                                               ais->type6.dac1fid28.waypoints[i].lat);
                        }
                        if (buf[strlen(buf) - 1] == ',')
                            buf[strlen(buf)-1] = '\0';
                        (void)strlcat(buf, "]}\r\n", buflen);
                        break;
                    case 30:	/* IMO289 - Text description - addressed */
                        (void)snprintf(buf + strlen(buf), buflen - strlen(buf),
                                       "\"linkage\":%u,\"text\":\"%s\"}\r\n",
                                       ais->type6.dac1fid30.linkage,
                                       json_stringify(buf1, sizeof(buf1),
                                                      ais->type6.dac1fid30.text));
                        break;
                    case 14:	/* IMO236 - Tidal Window */
                    case 32:	/* IMO289 - Tidal Window */
                        (void)snprintf(buf + strlen(buf), buflen - strlen(buf),
                                       "\"month\":%u,\"day\":%u,\"tidals\":[",
                                       ais->type6.dac1fid32.month,
                                       ais->type6.dac1fid32.day);
                        for (i = 0; i < ais->type6.dac1fid32.ntidals; i++) {
                            const struct tidal_t *tp =  &ais->type6.dac1fid32.tidals[i];
                            if (scaled)
                                (void)snprintf(buf + strlen(buf), buflen - strlen(buf),
                                               "{\"lon\":%.3f,\"lat\":%.3f,",
                                               tp->lon / AIS_LATLON3_DIV,
                                               tp->lat / AIS_LATLON3_DIV);
                            else
                                (void)snprintf(buf + strlen(buf), buflen - strlen(buf),
                                               "{\"lon\":%d,\"lat\":%d,",
                                               tp->lon,
                                               tp->lat);
                            (void)snprintf(buf + strlen(buf), buflen - strlen(buf),
                                           "\"from_hour\":%u,\"from_min\":%u,\"to_hour\":%u,\"to_min\":%u,\"cdir\":%u,",
                                           tp->from_hour,
                                           tp->from_min,
                                           tp->to_hour,
                                           tp->to_min,
                                           tp->cdir);
                            if (scaled)
                                (void)snprintf(buf + strlen(buf), buflen - strlen(buf),
                                               "\"cspeed\":%.1f},",
                                               tp->cspeed / 10.0);
                            else
                                (void)snprintf(buf + strlen(buf), buflen - strlen(buf),
                                               "\"cspeed\":%u},",
                                               tp->cspeed);
                        }
                        if (buf[strlen(buf) - 1] == ',')
                            buf[strlen(buf)-1] = '\0';
                        (void)strlcat(buf, "]}\r\n", buflen);
                        break;
                }
            }
            break;
        case 7:			/* Binary Acknowledge */
        case 13:			/* Safety Related Acknowledge */
            (void)snprintf(buf + strlen(buf), buflen - strlen(buf),
                           "\"mmsi1\":%u,\"mmsi2\":%u,\"mmsi3\":%u,\"mmsi4\":%u}\r\n",
                           ais->type7.mmsi1,
                           ais->type7.mmsi2, ais->type7.mmsi3, ais->type7.mmsi4);
            break;
        case 8:			/* Binary Broadcast Message */
            (void)snprintf(buf + strlen(buf), buflen - strlen(buf),
                           "\"dac\":%u,\"fid\":%u,",ais->type8.dac, ais->type8.fid);
            if (!ais->type8.structured) {
                (void)snprintf(buf + strlen(buf), buflen - strlen(buf),
                               "\"data\":\"%zd:%s\"}\r\n",
                               ais->type8.bitcount,
                               json_stringify(buf1, sizeof(buf1),
                                              gpsd_hexdump(scratchbuf, sizeof(scratchbuf),
                                                           (char *)ais->type8.bitdata,
                                                           BITS_TO_BYTES(ais->type8.bitcount))));
                break;
            }
            if (ais->type8.dac == 1) {
                const char *trends[] = {
                    "steady",
                    "increasing",
                    "decreasing",
                    "N/A",
                };
                // WMO 306, Code table 4.201
                const char *preciptypes[] = {
                    "reserved",
                    "rain",
                    "thunderstorm",
                    "freezing rain",
                    "mixed/ice",
                    "snow",
                    "reserved",
                    "N/A",
                };
                const char *ice[] = {
                    "no",
                    "yes",
                    "reserved",
                    "N/A",
                };
                switch (ais->type8.fid) {
                    case 11:        /* IMO236 - Meteorological/Hydrological data */
                        /* some fields have been merged to an ISO8601 partial date */
                        /* layout is almost identical to FID=31 from IMO289 */
                        if (scaled)
                            (void)snprintf(buf + strlen(buf), buflen - strlen(buf),
                                           "\"lat\":%.3f,\"lon\":%.3f,",
                                           ais->type8.dac1fid11.lat / AIS_LATLON3_DIV,
                                           ais->type8.dac1fid11.lon / AIS_LATLON3_DIV);
                        else
                            (void)snprintf(buf + strlen(buf), buflen - strlen(buf),
                                           "\"lat\":%d,\"lon\":%d,",
                                           ais->type8.dac1fid11.lat,
                                           ais->type8.dac1fid11.lon);
                        (void)snprintf(buf + strlen(buf), buflen - strlen(buf),
                                       "\"timestamp\":\"%02uT%02u:%02uZ\","
                                       "\"wspeed\":%u,\"wgust\":%u,\"wdir\":%u,"
                                       "\"wgustdir\":%u,\"humidity\":%u,",
                                       ais->type8.dac1fid11.day,
                                       ais->type8.dac1fid11.hour,
                                       ais->type8.dac1fid11.minute,
                                       ais->type8.dac1fid11.wspeed,
                                       ais->type8.dac1fid11.wgust,
                                       ais->type8.dac1fid11.wdir,
                                       ais->type8.dac1fid11.wgustdir,
                                       ais->type8.dac1fid11.humidity);
                        if (scaled)
                            (void)snprintf(buf + strlen(buf), buflen - strlen(buf),
                                           "\"airtemp\":%.1f,\"dewpoint\":%.1f,"
                                           "\"pressure\":%u,\"pressuretend\":\"%s\",",
                                           (ais->type8.dac1fid11.airtemp - DAC1FID11_AIRTEMP_OFFSET) / DAC1FID11_AIRTEMP_DIV,
                                           (ais->type8.dac1fid11.dewpoint - DAC1FID11_DEWPOINT_OFFSET) / DAC1FID11_DEWPOINT_DIV,
                                           ais->type8.dac1fid11.pressure - DAC1FID11_PRESSURE_OFFSET,
                                           trends[ais->type8.dac1fid11.pressuretend]);
                        else
                            (void)snprintf(buf + strlen(buf), buflen - strlen(buf),
                                           "\"airtemp\":%u,\"dewpoint\":%u,"
                                           "\"pressure\":%u,\"pressuretend\":%u,",
                                           ais->type8.dac1fid11.airtemp,
                                           ais->type8.dac1fid11.dewpoint,
                                           ais->type8.dac1fid11.pressure,
                                           ais->type8.dac1fid11.pressuretend);
                        
                        if (scaled)
                            (void)snprintf(buf + strlen(buf), buflen - strlen(buf),
                                           "\"visibility\":%.1f,",
                                           ais->type8.dac1fid11.visibility / DAC1FID11_VISIBILITY_DIV);
                        else
                            (void)snprintf(buf + strlen(buf), buflen - strlen(buf),
                                           "\"visibility\":%u,",
                                           ais->type8.dac1fid11.visibility);
                        if (!scaled)
                            (void)snprintf(buf + strlen(buf), buflen - strlen(buf),
                                           "\"waterlevel\":%d,",
                                           ais->type8.dac1fid11.waterlevel);
                        else
                            (void)snprintf(buf + strlen(buf), buflen - strlen(buf),
                                           "\"waterlevel\":%.1f,",
                                           (ais->type8.dac1fid11.waterlevel - DAC1FID11_WATERLEVEL_OFFSET) / DAC1FID11_WATERLEVEL_DIV);
                        
                        if (scaled) {
                            (void)snprintf(buf + strlen(buf), buflen - strlen(buf),
                                           "\"leveltrend\":\"%s\","
                                           "\"cspeed\":%.1f,\"cdir\":%u,"
                                           "\"cspeed2\":%.1f,\"cdir2\":%u,\"cdepth2\":%u,"
                                           "\"cspeed3\":%.1f,\"cdir3\":%u,\"cdepth3\":%u,"
                                           "\"waveheight\":%.1f,\"waveperiod\":%u,\"wavedir\":%u,"
                                           "\"swellheight\":%.1f,\"swellperiod\":%u,\"swelldir\":%u,"
                                           "\"seastate\":%u,\"watertemp\":%.1f,"
                                           "\"preciptype\":%u,\"preciptype_text\":\"%s\","
                                           "\"salinity\":%.1f,\"ice\":%u,\"ice_text\":\"%s\"",
                                           trends[ais->type8.dac1fid11.leveltrend],
                                           ais->type8.dac1fid11.cspeed / DAC1FID11_CSPEED_DIV,
                                           ais->type8.dac1fid11.cdir,
                                           ais->type8.dac1fid11.cspeed2 / DAC1FID11_CSPEED_DIV,
                                           ais->type8.dac1fid11.cdir2,
                                           ais->type8.dac1fid11.cdepth2,
                                           ais->type8.dac1fid11.cspeed3 / DAC1FID11_CSPEED_DIV,
                                           ais->type8.dac1fid11.cdir3,
                                           ais->type8.dac1fid11.cdepth3,
                                           ais->type8.dac1fid11.waveheight / DAC1FID11_WAVEHEIGHT_DIV,
                                           ais->type8.dac1fid11.waveperiod,
                                           ais->type8.dac1fid11.wavedir,
                                           ais->type8.dac1fid11.swellheight / DAC1FID11_WAVEHEIGHT_DIV,
                                           ais->type8.dac1fid11.swellperiod,
                                           ais->type8.dac1fid11.swelldir,
                                           ais->type8.dac1fid11.seastate,
                                           (ais->type8.dac1fid11.watertemp - DAC1FID11_WATERTEMP_OFFSET) / DAC1FID11_WATERTEMP_DIV,
                                           ais->type8.dac1fid11.preciptype,
                                           preciptypes[ais->type8.dac1fid11.preciptype],
                                           ais->type8.dac1fid11.salinity / DAC1FID11_SALINITY_DIV,
                                           ais->type8.dac1fid11.ice,
                                           ice[ais->type8.dac1fid11.ice]);
                        } else
                            (void)snprintf(buf + strlen(buf), buflen - strlen(buf),
                                           "\"leveltrend\":%u,"
                                           "\"cspeed\":%u,\"cdir\":%u,"
                                           "\"cspeed2\":%u,\"cdir2\":%u,\"cdepth2\":%u,"
                                           "\"cspeed3\":%u,\"cdir3\":%u,\"cdepth3\":%u,"
                                           "\"waveheight\":%u,\"waveperiod\":%u,\"wavedir\":%u,"
                                           "\"swellheight\":%u,\"swellperiod\":%u,\"swelldir\":%u,"
                                           "\"seastate\":%u,\"watertemp\":%u,"
                                           "\"preciptype\":%u,\"preciptype_text\":\"%s\","
                                           "\"salinity\":%u,\"ice\":%u,\"ice_text\":\"%s\"",
                                           ais->type8.dac1fid11.leveltrend,
                                           ais->type8.dac1fid11.cspeed,
                                           ais->type8.dac1fid11.cdir,
                                           ais->type8.dac1fid11.cspeed2,
                                           ais->type8.dac1fid11.cdir2,
                                           ais->type8.dac1fid11.cdepth2,
                                           ais->type8.dac1fid11.cspeed3,
                                           ais->type8.dac1fid11.cdir3,
                                           ais->type8.dac1fid11.cdepth3,
                                           ais->type8.dac1fid11.waveheight,
                                           ais->type8.dac1fid11.waveperiod,
                                           ais->type8.dac1fid11.wavedir,
                                           ais->type8.dac1fid11.swellheight,
                                           ais->type8.dac1fid11.swellperiod,
                                           ais->type8.dac1fid11.swelldir,
                                           ais->type8.dac1fid11.seastate,
                                           ais->type8.dac1fid11.watertemp,
                                           ais->type8.dac1fid11.preciptype,
                                           preciptypes[ais->type8.dac1fid11.preciptype],
                                           ais->type8.dac1fid11.salinity,
                                           ais->type8.dac1fid11.ice,
                                           ice[ais->type8.dac1fid11.ice]);
                        (void)strlcat(buf, "}\r\n", buflen);
                        break;
                    case 13:        /* IMO236 - Fairway closed */
                        (void)snprintf(buf + strlen(buf), buflen - strlen(buf),
                                       "\"reason\":\"%s\",\"closefrom\":\"%s\","
                                       "\"closeto\":\"%s\",\"radius\":%u,"
                                       "\"extunit\":%u,"
                                       "\"from\":\"%02u-%02uT%02u:%02u\","
                                       "\"to\":\"%02u-%02uT%02u:%02u\"}\r\n",
                                       json_stringify(buf1, sizeof(buf1),
                                                      ais->type8.dac1fid13.reason),
                                       json_stringify(buf2, sizeof(buf2),
                                                      ais->type8.dac1fid13.closefrom),
                                       json_stringify(buf3, sizeof(buf3),
                                                      ais->type8.dac1fid13.closeto),
                                       ais->type8.dac1fid13.radius,
                                       ais->type8.dac1fid13.extunit,
                                       ais->type8.dac1fid13.fmonth,
                                       ais->type8.dac1fid13.fday,
                                       ais->type8.dac1fid13.fhour,
                                       ais->type8.dac1fid13.fminute,
                                       ais->type8.dac1fid13.tmonth,
                                       ais->type8.dac1fid13.tday,
                                       ais->type8.dac1fid13.thour,
                                       ais->type8.dac1fid13.tminute);
                        break;
                    case 15:        /* IMO236 - Extended ship and voyage */
                        (void)snprintf(buf + strlen(buf), buflen - strlen(buf),
                                       "\"airdraught\":%u}\r\n",
                                       ais->type8.dac1fid15.airdraught);
                        break;
                    case 16:	/* IMO289 - Number of persons on board */
                        (void)snprintf(buf + strlen(buf), buflen - strlen(buf),
                                       "\"persons\":%u}\t\n", ais->type6.dac1fid16.persons);
                        break;
                    case 17:        /* IMO289 - VTS-generated/synthetic targets */
                        (void)strlcat(buf, "\"targets\":[", buflen);
                        for (i = 0; i < ais->type8.dac1fid17.ntargets; i++) {
                            (void)snprintf(buf + strlen(buf), buflen - strlen(buf),
                                           "{\"idtype\":%u,\"idtype_text\":\"%s\",",
                                           ais->type8.dac1fid17.targets[i].idtype,
                                           idtypes[ais->type8.dac1fid17.targets[i].idtype]);
                            switch (ais->type8.dac1fid17.targets[i].idtype) {
                                case DAC1FID17_IDTYPE_MMSI:
                                    (void)snprintf(buf + strlen(buf), buflen - strlen(buf),
                                                   "\"%s\":\"%u\",",
                                                   idtypes[ais->type8.dac1fid17.targets[i].idtype],
                                                   ais->type8.dac1fid17.targets[i].id.mmsi);
                                    break;
                                case DAC1FID17_IDTYPE_IMO:
                                    (void)snprintf(buf + strlen(buf), buflen - strlen(buf),
                                                   "\"%s\":\"%u\",",
                                                   idtypes[ais->type8.dac1fid17.targets[i].idtype],
                                                   ais->type8.dac1fid17.targets[i].id.imo);
                                    break;
                                case DAC1FID17_IDTYPE_CALLSIGN:
                                    (void)snprintf(buf + strlen(buf), buflen - strlen(buf),
                                                   "\"%s\":\"%s\",",
                                                   idtypes[ais->type8.dac1fid17.targets[i].idtype],
                                                   json_stringify(buf1, sizeof(buf1),
                                                                  ais->type8.dac1fid17.targets[i].id.callsign));
                                    break;
                                default:
                                    (void)snprintf(buf + strlen(buf), buflen - strlen(buf),
                                                   "\"%s\":\"%s\",",
                                                   idtypes[ais->type8.dac1fid17.targets[i].idtype],
                                                   json_stringify(buf1, sizeof(buf1),
                                                                  ais->type8.dac1fid17.targets[i].id.other));
                            }
                            if (scaled)
                                (void)snprintf(buf + strlen(buf), buflen - strlen(buf),
                                               "\"lat\":%.3f,\"lon\":%.3f,",
                                               ais->type8.dac1fid17.targets[i].lat / AIS_LATLON3_DIV,
                                               ais->type8.dac1fid17.targets[i].lon / AIS_LATLON3_DIV);
                            else
                                (void)snprintf(buf + strlen(buf), buflen - strlen(buf),
                                               "\"lat\":%d,\"lon\":%d,",
                                               ais->type8.dac1fid17.targets[i].lat,
                                               ais->type8.dac1fid17.targets[i].lon);
                            (void)snprintf(buf + strlen(buf), buflen - strlen(buf),
                                           "\"course\":%u,\"second\":%u,\"speed\":%u},",
                                           ais->type8.dac1fid17.targets[i].course,
                                           ais->type8.dac1fid17.targets[i].second,
                                           ais->type8.dac1fid17.targets[i].speed);
                        }
                        if (buf[strlen(buf) - 1] == ',')
                            buf[strlen(buf) - 1] = '\0';
                        (void)strlcat(buf, "]}\r\n", buflen);
                        break;
                    case 19:        /* IMO289 - Marine Traffic Signal */
                        (void)snprintf(buf + strlen(buf), buflen - strlen(buf),
                                       "\"linkage\":%u,\"station\":\"%s\","
                                       "\"lon\":%.3f,\"lat\":%.3f,\"status\":%u,"
                                       "\"signal\":%u,\"signal_text\":\"%s\","
                                       "\"hour\":%u,\"minute\":%u,"
                                       "\"nextsignal\":%u"
                                       "\"nextsignal_text\":\"%s\""
                                       "}\r\n",
                                       ais->type8.dac1fid19.linkage,
                                       json_stringify(buf1, sizeof(buf1),
                                                      ais->type8.dac1fid19.station),
                                       ais->type8.dac1fid19.lon / AIS_LATLON3_DIV,
                                       ais->type8.dac1fid19.lat / AIS_LATLON3_DIV,
                                       ais->type8.dac1fid19.status,
                                       ais->type8.dac1fid19.signal,
                                       SIGNAL_DISPLAY(ais->type8.dac1fid19.signal),
                                       ais->type8.dac1fid19.hour,
                                       ais->type8.dac1fid19.minute,
                                       ais->type8.dac1fid19.nextsignal,
                                       SIGNAL_DISPLAY(ais->type8.dac1fid19.nextsignal));
                        break;
                    case 21:        /* IMO289 - Weather obs. report from ship */
                        break;
                    case 22:        /* IMO289 - Area notice - broadcast */
                        break;
                    case 24:        /* IMO289 - Extended ship static & voyage-related data */
                        break;
                    case 25:        /* IMO289 - Dangerous Cargo Indication */
                        break;
                    case 27:        /* IMO289 - Route information - broadcast */
                        (void)snprintf(buf + strlen(buf), buflen - strlen(buf),
                                       "\"linkage\":%u,\"sender\":%u,"
                                       "\"rtype\":%u,"
                                       "\"rtype_text\":\"%s\","
                                       "\"start\":\"%02u-%02uT%02u:%02uZ\","
                                       "\"duration\":%u,\"waypoints\":[",
                                       ais->type8.dac1fid27.linkage,
                                       ais->type8.dac1fid27.sender,
                                       ais->type8.dac1fid27.rtype,
                                       route_type[ais->type8.dac1fid27.rtype],
                                       ais->type8.dac1fid27.month,
                                       ais->type8.dac1fid27.day,
                                       ais->type8.dac1fid27.hour,
                                       ais->type8.dac1fid27.minute,
                                       ais->type8.dac1fid27.duration);
                        for (i = 0; i < ais->type8.dac1fid27.waycount; i++) {
                            if (scaled)
                                (void)snprintf(buf + strlen(buf), buflen - strlen(buf),
                                               "{\"lon\":%.4f,\"lat\":%.4f},",
                                               ais->type8.dac1fid27.waypoints[i].lon / AIS_LATLON4_DIV,
                                               ais->type8.dac1fid27.waypoints[i].lat / AIS_LATLON4_DIV);
                            else
                                (void)snprintf(buf + strlen(buf), buflen - strlen(buf),
                                               "{\"lon\":%d,\"lat\":%d},",
                                               ais->type8.dac1fid27.waypoints[i].lon,
                                               ais->type8.dac1fid27.waypoints[i].lat);
                        }
                        if (buf[strlen(buf) - 1] == ',')
                            buf[strlen(buf) - 1] = '\0';
                        (void)strlcat(buf, "]}\r\n", buflen);
                        break;
                    case 29:        /* IMO289 - Text Description - broadcast */
                        (void)snprintf(buf + strlen(buf), buflen - strlen(buf),
                                       "\"linkage\":%u,\"text\":\"%s\"}\r\n",
                                       ais->type8.dac1fid29.linkage,
                                       json_stringify(buf1, sizeof(buf1),
                                                      ais->type8.dac1fid29.text));
                        break;
                    case 31:        /* IMO289 - Meteorological/Hydrological data */
                        /* some fields have been merged to an ISO8601 partial date */
                        /* layout is almost identical to FID=11 from IMO236 */
                        if (scaled)
                            (void)snprintf(buf + strlen(buf), buflen - strlen(buf),
                                           "\"lat\":%.3f,\"lon\":%.3f,",
                                           ais->type8.dac1fid31.lat / AIS_LATLON3_DIV,
                                           ais->type8.dac1fid31.lon / AIS_LATLON3_DIV);
                        else
                            (void)snprintf(buf + strlen(buf), buflen - strlen(buf),
                                           "\"lat\":%d,\"lon\":%d,",
                                           ais->type8.dac1fid31.lat,
                                           ais->type8.dac1fid31.lon);
                        (void)snprintf(buf + strlen(buf), buflen - strlen(buf),
                                       "\"accuracy\":%s,",
                                       JSON_BOOL(ais->type8.dac1fid31.accuracy));
                        (void)snprintf(buf + strlen(buf), buflen - strlen(buf),
                                       "\"timestamp\":\"%02uT%02u:%02uZ\","
                                       "\"wspeed\":%u,\"wgust\":%u,\"wdir\":%u,"
                                       "\"wgustdir\":%u,\"humidity\":%u,",
                                       ais->type8.dac1fid31.day,
                                       ais->type8.dac1fid31.hour,
                                       ais->type8.dac1fid31.minute,
                                       ais->type8.dac1fid31.wspeed,
                                       ais->type8.dac1fid31.wgust,
                                       ais->type8.dac1fid31.wdir,
                                       ais->type8.dac1fid31.wgustdir,
                                       ais->type8.dac1fid31.humidity);
                        if (scaled)
                            (void)snprintf(buf + strlen(buf), buflen - strlen(buf),
                                           "\"airtemp\":%.1f,\"dewpoint\":%.1f,"
                                           "\"pressure\":%u,\"pressuretend\":\"%s\","
                                           "\"visgreater\":%s,",
                                           ais->type8.dac1fid31.airtemp / DAC1FID31_AIRTEMP_DIV,
                                           ais->type8.dac1fid31.dewpoint / DAC1FID31_DEWPOINT_DIV,
                                           ais->type8.dac1fid31.pressure - DAC1FID31_PRESSURE_OFFSET,
                                           trends[ais->type8.dac1fid31.pressuretend],
                                           JSON_BOOL(ais->type8.dac1fid31.visgreater));
                        else
                            (void)snprintf(buf + strlen(buf), buflen - strlen(buf),
                                           "\"airtemp\":%d,\"dewpoint\":%d,"
                                           "\"pressure\":%u,\"pressuretend\":%u,"
                                           "\"visgreater\":%s,",
                                           ais->type8.dac1fid31.airtemp,
                                           ais->type8.dac1fid31.dewpoint,
                                           ais->type8.dac1fid31.pressure,
                                           ais->type8.dac1fid31.pressuretend,
                                           JSON_BOOL(ais->type8.dac1fid31.visgreater));
                        
                        if (scaled)
                            (void)snprintf(buf + strlen(buf), buflen - strlen(buf),
                                           "\"visibility\":%.1f,",
                                           ais->type8.dac1fid31.visibility / DAC1FID31_VISIBILITY_DIV);
                        else
                            (void)snprintf(buf + strlen(buf), buflen - strlen(buf),
                                           "\"visibility\":%u,",
                                           ais->type8.dac1fid31.visibility);
                        if (!scaled)
                            (void)snprintf(buf + strlen(buf), buflen - strlen(buf),
                                           "\"waterlevel\":%d,",
                                           ais->type8.dac1fid31.waterlevel);
                        else
                            (void)snprintf(buf + strlen(buf), buflen - strlen(buf),
                                           "\"waterlevel\":%.1f,",
                                           (ais->type8.dac1fid31.waterlevel - DAC1FID31_WATERLEVEL_OFFSET) / DAC1FID31_WATERLEVEL_DIV);
                        
                        if (scaled) {
                            (void)snprintf(buf + strlen(buf), buflen - strlen(buf),
                                           "\"leveltrend\":\"%s\","
                                           "\"cspeed\":%.1f,\"cdir\":%u,"
                                           "\"cspeed2\":%.1f,\"cdir2\":%u,\"cdepth2\":%u,"
                                           "\"cspeed3\":%.1f,\"cdir3\":%u,\"cdepth3\":%u,"
                                           "\"waveheight\":%.1f,\"waveperiod\":%u,\"wavedir\":%u,"
                                           "\"swellheight\":%.1f,\"swellperiod\":%u,\"swelldir\":%u,"
                                           "\"seastate\":%u,\"watertemp\":%.1f,"
                                           "\"preciptype\":\"%s\",\"salinity\":%.1f,\"ice\":\"%s\"",
                                           trends[ais->type8.dac1fid31.leveltrend],
                                           ais->type8.dac1fid31.cspeed / DAC1FID31_CSPEED_DIV,
                                           ais->type8.dac1fid31.cdir,
                                           ais->type8.dac1fid31.cspeed2 / DAC1FID31_CSPEED_DIV,
                                           ais->type8.dac1fid31.cdir2,
                                           ais->type8.dac1fid31.cdepth2,
                                           ais->type8.dac1fid31.cspeed3 / DAC1FID31_CSPEED_DIV,
                                           ais->type8.dac1fid31.cdir3,
                                           ais->type8.dac1fid31.cdepth3,
                                           ais->type8.dac1fid31.waveheight / DAC1FID31_HEIGHT_DIV,
                                           ais->type8.dac1fid31.waveperiod,
                                           ais->type8.dac1fid31.wavedir,
                                           ais->type8.dac1fid31.swellheight / DAC1FID31_HEIGHT_DIV,
                                           ais->type8.dac1fid31.swellperiod,
                                           ais->type8.dac1fid31.swelldir,
                                           ais->type8.dac1fid31.seastate,
                                           ais->type8.dac1fid31.watertemp / DAC1FID31_WATERTEMP_DIV,
                                           preciptypes[ais->type8.dac1fid31.preciptype],
                                           ais->type8.dac1fid31.salinity / DAC1FID31_SALINITY_DIV,
                                           ice[ais->type8.dac1fid31.ice]);
                        } else
                            (void)snprintf(buf + strlen(buf), buflen - strlen(buf),
                                           "\"leveltrend\":%u,"
                                           "\"cspeed\":%u,\"cdir\":%u,"
                                           "\"cspeed2\":%u,\"cdir2\":%u,\"cdepth2\":%u,"
                                           "\"cspeed3\":%u,\"cdir3\":%u,\"cdepth3\":%u,"
                                           "\"waveheight\":%u,\"waveperiod\":%u,\"wavedir\":%u,"
                                           "\"swellheight\":%u,\"swellperiod\":%u,\"swelldir\":%u,"
                                           "\"seastate\":%u,\"watertemp\":%d,"
                                           "\"preciptype\":%u,\"salinity\":%u,\"ice\":%u",
                                           ais->type8.dac1fid31.leveltrend,
                                           ais->type8.dac1fid31.cspeed,
                                           ais->type8.dac1fid31.cdir,
                                           ais->type8.dac1fid31.cspeed2,
                                           ais->type8.dac1fid31.cdir2,
                                           ais->type8.dac1fid31.cdepth2,
                                           ais->type8.dac1fid31.cspeed3,
                                           ais->type8.dac1fid31.cdir3,
                                           ais->type8.dac1fid31.cdepth3,
                                           ais->type8.dac1fid31.waveheight,
                                           ais->type8.dac1fid31.waveperiod,
                                           ais->type8.dac1fid31.wavedir,
                                           ais->type8.dac1fid31.swellheight,
                                           ais->type8.dac1fid31.swellperiod,
                                           ais->type8.dac1fid31.swelldir,
                                           ais->type8.dac1fid31.seastate,
                                           ais->type8.dac1fid31.watertemp,
                                           ais->type8.dac1fid31.preciptype,
                                           ais->type8.dac1fid31.salinity,
                                           ais->type8.dac1fid31.ice);
                        (void)strlcat(buf, "}\r\n", buflen);
                        break;
                }
            }
            else if (ais->type8.dac == 200) {
                struct {
                    const unsigned int code;
                    const unsigned int ais;
                    const char *legend;
                } *cp, shiptypes[] = {
                    /*
                     * The Inland AIS standard is not clear which numbers are
                     * supposed to be in the type slot.  The ranges are disjoint,
                     * so we'll match on both.
                     */
                    {8000, 99, "Vessel, type unknown"},
                    {8010, 79, "Motor freighter"},
                    {8020, 89, "Motor tanker"},
                    {8021, 80, "Motor tanker, liquid cargo, type N"},
                    {8022, 80, "Motor tanker, liquid cargo, type C"},
                    {8023, 89, "Motor tanker, dry cargo as if liquid (e.g. cement)"},
                    {8030, 79, "Container vessel"},
                    {8040, 80, "Gas tanker"},
                    {8050, 79, "Motor freighter, tug"},
                    {8060, 89, "Motor tanker, tug"},
                    {8070, 79, "Motor freighter with one or more ships alongside"},
                    {8080, 89, "Motor freighter with tanker"},
                    {8090, 79, "Motor freighter pushing one or more freighters"},
                    {8100, 89, "Motor freighter pushing at least one tank-ship"},
                    {8110, 79, "Tug, freighter"},
                    {8120, 89, "Tug, tanker"},
                    {8130, 31, "Tug freighter, coupled"},
                    {8140, 31, "Tug, freighter/tanker, coupled"},
                    {8150, 99, "Freightbarge"},
                    {8160, 99, "Tankbarge"},
                    {8161, 90, "Tankbarge, liquid cargo, type N"},
                    {8162, 90, "Tankbarge, liquid cargo, type C"},
                    {8163, 99, "Tankbarge, dry cargo as if liquid (e.g. cement)"},
                    {8170, 99, "Freightbarge with containers"},
                    {8180, 90, "Tankbarge, gas"},
                    {8210, 79, "Pushtow, one cargo barge"},
                    {8220, 79, "Pushtow, two cargo barges"},
                    {8230, 79, "Pushtow, three cargo barges"},
                    {8240, 79, "Pushtow, four cargo barges"},
                    {8250, 79, "Pushtow, five cargo barges"},
                    {8260, 79, "Pushtow, six cargo barges"},
                    {8270, 79, "Pushtow, seven cargo barges"},
                    {8280, 79, "Pushtow, eigth cargo barges"},
                    {8290, 79, "Pushtow, nine or more barges"},
                    {8310, 80, "Pushtow, one tank/gas barge"},
                    {8320, 80, "Pushtow, two barges at least one tanker or gas barge"},
                    {8330, 80, "Pushtow, three barges at least one tanker or gas barge"},
                    {8340, 80, "Pushtow, four barges at least one tanker or gas barge"},
                    {8350, 80, "Pushtow, five barges at least one tanker or gas barge"},
                    {8360, 80, "Pushtow, six barges at least one tanker or gas barge"},
                    {8370, 80, "Pushtow, seven barges at least one tanker or gas barg"},
                    {0, 0, "Illegal ship type value."},
                };
                const char *hazard_types[] = {
                    "0 blue cones/lights",
                    "1 blue cone/light",
                    "2 blue cones/lights",
                    "3 blue cones/lights",
                    "4 B-Flag",
                    "Unknown",
                };
#define HTYPE_DISPLAY(n) (((n) < (unsigned int)NITEMS(hazard_types)) ? hazard_types[n] : "INVALID HAZARD TYPE")
                const char *lstatus_types[] = {
                    "N/A (default)",
                    "Unloaded",
                    "Loaded",
                };
#define LSTATUS_DISPLAY(n) (((n) < (unsigned int)NITEMS(lstatus_types)) ? lstatus_types[n] : "INVALID LOAD STATUS")
                const char *emma_types[] = {
                    "Not Available",
                    "Wind",
                    "Rain",
                    "Snow and ice",
                    "Thunderstorm",
                    "Fog",
                    "Low temperature",
                    "High temperature",
                    "Flood",
                    "Forest Fire",
                };
#define EMMA_TYPE_DISPLAY(n) (((n) < (unsigned int)NITEMS(emma_types)) ? emma_types[n] : "INVALID EMMA TYPE")
                const char *emma_classes[] = {
                    "Slight",
                    "Medium",
                    "Strong",
                };
#define EMMA_CLASS_DISPLAY(n) (((n) < (unsigned int)NITEMS(emma_classes)) ? emma_classes[n] : "INVALID EMMA TYPE")
                const char *emma_winds[] = {
                    "N/A",
                    "North",
                    "North East",
                    "East",
                    "South East",
                    "South",
                    "South West",
                    "West",
                    "North West",
                };
#define EMMA_WIND_DISPLAY(n) (((n) < (unsigned int)NITEMS(emma_winds)) ? emma_winds[n] : "INVALID EMMA WIND DIRECTION")
                const char *direction_vocabulary[] = {
                    "Unknown",
                    "Upstream",
                    "Downstream",
                    "To left bank",
                    "To right bank",
                };
#define DIRECTION_DISPLAY(n) (((n) < (unsigned int)NITEMS(direction_vocabulary)) ? direction_vocabulary[n] : "INVALID DIRECTION")
                const char *status_vocabulary[] = {
                    "Unknown",
                    "No light",
                    "White",
                    "Yellow",
                    "Green",
                    "Red",
                    "White flashing",
                    "Yellow flashing.",
                };
#define STATUS_DISPLAY(n) (((n) < (unsigned int)NITEMS(status_vocabulary)) ? status_vocabulary[n] : "INVALID STATUS")
                
                switch (ais->type8.fid) {
                    case 10:        /* Inland ship static and voyage-related data */
                        for (cp = shiptypes; cp < shiptypes + NITEMS(shiptypes); cp++)
                            if (cp->code == ais->type8.dac200fid10.shiptype
                                || cp->ais == ais->type8.dac200fid10.shiptype
                                || cp->code == 0)
                                break;
                        (void)snprintf(buf + strlen(buf), buflen - strlen(buf),
                                       "\"vin\":\"%s\",\"length\":%u,\"beam\":%u,"
                                       "\"shiptype\":%u,\"shiptype_text\":\"%s\","
                                       "\"hazard\":%u,\"hazard_text\":\"%s\","
                                       "\"draught\":%u,"
                                       "\"loaded\":%u,\"loaded_text\":\"%s\","
                                       "\"speed_q\":%s,"
                                       "\"course_q\":%s,"
                                       "\"heading_q\":%s}\r\n",
                                       ais->type8.dac200fid10.vin,
                                       ais->type8.dac200fid10.length,
                                       ais->type8.dac200fid10.beam,
                                       ais->type8.dac200fid10.shiptype,
                                       cp->legend,
                                       ais->type8.dac200fid10.hazard,
                                       HTYPE_DISPLAY(ais->type8.dac200fid10.hazard),
                                       ais->type8.dac200fid10.draught,
                                       ais->type8.dac200fid10.loaded,
                                       LSTATUS_DISPLAY(ais->type8.dac200fid10.loaded),
                                       JSON_BOOL(ais->type8.dac200fid10.speed_q),
                                       JSON_BOOL(ais->type8.dac200fid10.course_q),
                                       JSON_BOOL(ais->type8.dac200fid10.heading_q));
                        break;
                    case 23:	/* EMMA warning */
                        if (!ais->type8.structured)
                            break;
                        (void)snprintf(buf + strlen(buf), buflen - strlen(buf),
                                       "\"start\":\"%4u-%02u-%02uT%02u:%02u\","
                                       "\"end\":\"%4u-%02u-%02uT%02u:%02u\",",
                                       ais->type8.dac200fid23.start_year + 2000,
                                       ais->type8.dac200fid23.start_month,
                                       ais->type8.dac200fid23.start_hour,
                                       ais->type8.dac200fid23.start_minute,
                                       ais->type8.dac200fid23.start_day,
                                       ais->type8.dac200fid23.end_year + 2000,
                                       ais->type8.dac200fid23.end_month,
                                       ais->type8.dac200fid23.end_day,
                                       ais->type8.dac200fid23.end_hour,
                                       ais->type8.dac200fid23.end_minute);
                        if (scaled)
                            (void)snprintf(buf + strlen(buf), buflen - strlen(buf),
                                           "\"start_lon\":%.4f,\"start_lat\":%.4f,\"end_lon\":%.4f,\"end_lat\":%.4f,",
                                           ais->type8.dac200fid23.start_lon / AIS_LATLON_DIV,
                                           ais->type8.dac200fid23.start_lat / AIS_LATLON_DIV,
                                           ais->type8.dac200fid23.end_lon / AIS_LATLON_DIV,
                                           ais->type8.dac200fid23.end_lat / AIS_LATLON_DIV);
                        else
                            (void)snprintf(buf + strlen(buf), buflen - strlen(buf),
                                           "\"start_lon\":%d,\"start_lat\":%d,\"end_lon\":%d,\"end_lat\":%d,",
                                           ais->type8.dac200fid23.start_lon,
                                           ais->type8.dac200fid23.start_lat,
                                           ais->type8.dac200fid23.end_lon,
                                           ais->type8.dac200fid23.end_lat);
                        (void)snprintf(buf + strlen(buf), buflen - strlen(buf),
                                       "\"type\":%u,\"type_text\":\"%s\",\"min\":%d,\"max\":%d,\"class\":%u,\"class_text\":\"%s\",\"wind\":%u,\"wind_text\":\"%s\"}\r\n",
                                       
                                       ais->type8.dac200fid23.type,
                                       EMMA_TYPE_DISPLAY(ais->type8.dac200fid23.type),
                                       ais->type8.dac200fid23.min,
                                       ais->type8.dac200fid23.max,
                                       ais->type8.dac200fid23.intensity,
                                       EMMA_CLASS_DISPLAY(ais->type8.dac200fid23.intensity),
                                       ais->type8.dac200fid23.wind,
                                       EMMA_WIND_DISPLAY(ais->type8.dac200fid23.wind));
                        break;
                    case 24:	/* Inland AIS Water Levels */
                        (void)snprintf(buf + strlen(buf), buflen - strlen(buf),
                                       "\"country\":\"%s\",\"gauges\":[",
                                       ais->type8.dac200fid24.country);
                        for (i = 0; i < ais->type8.dac200fid24.ngauges; i++) {
                            (void)snprintf(buf + strlen(buf), buflen - strlen(buf),
                                           "{\"id\":%u,\"level\":%d}",
                                           ais->type8.dac200fid24.gauges[i].id,
                                           ais->type8.dac200fid24.gauges[i].level);
                        }
                        if (buf[strlen(buf)-1] == ',')
                            buf[strlen(buf)-1] = '\0';
                        (void)strlcat(buf, "]}\r\n", buflen - strlen(buf));
                        break;
                    case 40:	/* Inland AIS Signal Strength */
                        if (scaled)
                            (void)snprintf(buf + strlen(buf), buflen - strlen(buf),
                                           "\"lon\":%.4f,\"lat\":%.4f,",
                                           ais->type8.dac200fid40.lon / AIS_LATLON_DIV,
                                           ais->type8.dac200fid40.lat / AIS_LATLON_DIV);
                        else
                            (void)snprintf(buf + strlen(buf), buflen - strlen(buf),
                                           "\"lon\":%d,\"lat\":%d,",
                                           ais->type8.dac200fid40.lon,
                                           ais->type8.dac200fid40.lat);
                        (void)snprintf(buf + strlen(buf), buflen - strlen(buf),
                                       "\"form\":%u,\"facing\":%u,\"direction\":%u,\"direction_text\":\"%s\",\"status\":%u,\"status_text\":\"%s\"}\r\n",
                                       ais->type8.dac200fid40.form,
                                       ais->type8.dac200fid40.facing,
                                       ais->type8.dac200fid40.direction,
                                       DIRECTION_DISPLAY(ais->type8.dac200fid40.direction),
                                       ais->type8.dac200fid40.status,
                                       STATUS_DISPLAY(ais->type8.dac200fid40.status));
                        break;
                }
            }
            break;
        case 9:			/* Standard SAR Aircraft Position Report */
            if (scaled) {
                char altlegend[20];
                char speedlegend[20];
                
                /*
                 * Express altitude as nan if not available,
                 * "high" for above the reporting ceiling.
                 */
                if (ais->type9.alt == AIS_ALT_NOT_AVAILABLE)
                    (void)strlcpy(altlegend, "\"nan\"", sizeof(altlegend));
                else if (ais->type9.alt == AIS_ALT_HIGH)
                    (void)strlcpy(altlegend, "\"high\"", sizeof(altlegend));
                else
                    (void)snprintf(altlegend, sizeof(altlegend),
                                   "%u", ais->type9.alt);
                
                /*
                 * Express speed as nan if not available,
                 * "high" for above the reporting ceiling.
                 */
                if (ais->type9.speed == AIS_SAR_SPEED_NOT_AVAILABLE)
                    (void)strlcpy(speedlegend, "\"nan\"", sizeof(speedlegend));
                else if (ais->type9.speed == AIS_SAR_FAST_MOVER)
                    (void)strlcpy(speedlegend, "\"fast\"", sizeof(speedlegend));
                else
                    (void)snprintf(speedlegend, sizeof(speedlegend),
                                   "%u", ais->type1.speed);
                
                (void)snprintf(buf + strlen(buf), buflen - strlen(buf),
                               "\"alt\":%s,\"speed\":%s,\"accuracy\":%s,"
                               "\"lon\":%.4f,\"lat\":%.4f,\"course\":%.1f,"
                               "\"second\":%u,\"regional\":%u,\"dte\":%u,"
                               "\"raim\":%s,\"radio\":%u}\r\n",
                               altlegend,
                               speedlegend,
                               JSON_BOOL(ais->type9.accuracy),
                               ais->type9.lon / AIS_LATLON_DIV,
                               ais->type9.lat / AIS_LATLON_DIV,
                               ais->type9.course / 10.0,
                               ais->type9.second,
                               ais->type9.regional,
                               ais->type9.dte,
                               JSON_BOOL(ais->type9.raim), ais->type9.radio);
            } else {
                (void)snprintf(buf + strlen(buf), buflen - strlen(buf),
                               "\"alt\":%u,\"speed\":%u,\"accuracy\":%s,"
                               "\"lon\":%d,\"lat\":%d,\"course\":%u,"
                               "\"second\":%u,\"regional\":%u,\"dte\":%u,"
                               "\"raim\":%s,\"radio\":%u}\r\n",
                               ais->type9.alt,
                               ais->type9.speed,
                               JSON_BOOL(ais->type9.accuracy),
                               ais->type9.lon,
                               ais->type9.lat,
                               ais->type9.course,
                               ais->type9.second,
                               ais->type9.regional,
                               ais->type9.dte,
                               JSON_BOOL(ais->type9.raim), ais->type9.radio);
            }
            break;
        case 10:			/* UTC/Date Inquiry */
            (void)snprintf(buf + strlen(buf), buflen - strlen(buf),
                           "\"dest_mmsi\":%u}\r\n", ais->type10.dest_mmsi);
            break;
        case 12:			/* Safety Related Message */
            (void)snprintf(buf + strlen(buf), buflen - strlen(buf),
                           "\"seqno\":%u,\"dest_mmsi\":%u,\"retransmit\":%s,\"text\":\"%s\"}\r\n",
                           ais->type12.seqno,
                           ais->type12.dest_mmsi,
                           JSON_BOOL(ais->type12.retransmit),
                           json_stringify(buf1, sizeof(buf1), ais->type12.text));
            break;
        case 14:			/* Safety Related Broadcast Message */
            (void)snprintf(buf + strlen(buf), buflen - strlen(buf),
                           "\"text\":\"%s\"}\r\n",
                           json_stringify(buf1, sizeof(buf1), ais->type14.text));
            break;
        case 15:			/* Interrogation */
            (void)snprintf(buf + strlen(buf), buflen - strlen(buf),
                           "\"mmsi1\":%u,\"type1_1\":%u,\"offset1_1\":%u,"
                           "\"type1_2\":%u,\"offset1_2\":%u,\"mmsi2\":%u,"
                           "\"type2_1\":%u,\"offset2_1\":%u}\r\n",
                           ais->type15.mmsi1,
                           ais->type15.type1_1,
                           ais->type15.offset1_1,
                           ais->type15.type1_2,
                           ais->type15.offset1_2,
                           ais->type15.mmsi2,
                           ais->type15.type2_1, ais->type15.offset2_1);
            break;
        case 16:
            (void)snprintf(buf + strlen(buf), buflen - strlen(buf),
                           "\"mmsi1\":%u,\"offset1\":%u,\"increment1\":%u,"
                           "\"mmsi2\":%u,\"offset2\":%u,\"increment2\":%u}\r\n",
                           ais->type16.mmsi1,
                           ais->type16.offset1,
                           ais->type16.increment1,
                           ais->type16.mmsi2,
                           ais->type16.offset2, ais->type16.increment2);
            break;
        case 17:
            if (scaled) {
                (void)snprintf(buf + strlen(buf), buflen - strlen(buf),
                               "\"lon\":%.1f,\"lat\":%.1f,\"data\":\"%zd:%s\"}\r\n",
                               ais->type17.lon / AIS_GNSS_LATLON_DIV,
                               ais->type17.lat / AIS_GNSS_LATLON_DIV,
                               ais->type17.bitcount,
                               gpsd_hexdump(scratchbuf, sizeof(scratchbuf),
                                            (char *)ais->type17.bitdata,
                                            BITS_TO_BYTES(ais->type17.bitcount)));
            } else {
                (void)snprintf(buf + strlen(buf), buflen - strlen(buf),
                               "\"lon\":%d,\"lat\":%d,\"data\":\"%zd:%s\"}\r\n",
                               ais->type17.lon,
                               ais->type17.lat,
                               ais->type17.bitcount,
                               gpsd_hexdump(scratchbuf, sizeof(scratchbuf),
                                            (char *)ais->type17.bitdata,
                                            BITS_TO_BYTES(ais->type17.bitcount)));
            }
            break;
        case 18:
            if (scaled) {
                (void)snprintf(buf + strlen(buf), buflen - strlen(buf),
                               "\"reserved\":%u,\"speed\":%.1f,\"accuracy\":%s,"
                               "\"lon\":%.4f,\"lat\":%.4f,\"course\":%.1f,"
                               "\"heading\":%u,\"second\":%u,\"regional\":%u,"
                               "\"cs\":%s,\"display\":%s,\"dsc\":%s,\"band\":%s,"
                               "\"msg22\":%s,\"raim\":%s,\"radio\":%u}\r\n",
                               ais->type18.reserved,
                               ais->type18.speed / 10.0,
                               JSON_BOOL(ais->type18.accuracy),
                               ais->type18.lon / AIS_LATLON_DIV,
                               ais->type18.lat / AIS_LATLON_DIV,
                               ais->type18.course / 10.0,
                               ais->type18.heading,
                               ais->type18.second,
                               ais->type18.regional,
                               JSON_BOOL(ais->type18.cs),
                               JSON_BOOL(ais->type18.display),
                               JSON_BOOL(ais->type18.dsc),
                               JSON_BOOL(ais->type18.band),
                               JSON_BOOL(ais->type18.msg22),
                               JSON_BOOL(ais->type18.raim), ais->type18.radio);
            } else {
                (void)snprintf(buf + strlen(buf), buflen - strlen(buf),
                               "\"reserved\":%u,\"speed\":%u,\"accuracy\":%s,"
                               "\"lon\":%d,\"lat\":%d,\"course\":%u,"
                               "\"heading\":%u,\"second\":%u,\"regional\":%u,"
                               "\"cs\":%s,\"display\":%s,\"dsc\":%s,\"band\":%s,"
                               "\"msg22\":%s,\"raim\":%s,\"radio\":%u}\r\n",
                               ais->type18.reserved,
                               ais->type18.speed,
                               JSON_BOOL(ais->type18.accuracy),
                               ais->type18.lon,
                               ais->type18.lat,
                               ais->type18.course,
                               ais->type18.heading,
                               ais->type18.second,
                               ais->type18.regional,
                               JSON_BOOL(ais->type18.cs),
                               JSON_BOOL(ais->type18.display),
                               JSON_BOOL(ais->type18.dsc),
                               JSON_BOOL(ais->type18.band),
                               JSON_BOOL(ais->type18.msg22),
                               JSON_BOOL(ais->type18.raim), ais->type18.radio);
            }
            break;
        case 19:
            if (scaled) {
                (void)snprintf(buf + strlen(buf), buflen - strlen(buf),
                               "\"reserved\":%u,\"speed\":%.1f,\"accuracy\":%s,"
                               "\"lon\":%.4f,\"lat\":%.4f,\"course\":%.1f,"
                               "\"heading\":%u,\"second\":%u,\"regional\":%u,"
                               "\"shipname\":\"%s\","
                               "\"shiptype\":%u,\"shiptype_text\":\"%s\","
                               "\"to_bow\":%u,\"to_stern\":%u,\"to_port\":%u,"
                               "\"to_starboard\":%u,"
                               "\"epfd\":%u,\"epfd_text\":\"%s\","
                               "\"raim\":%s,\"dte\":%u,\"assigned\":%s}\r\n",
                               ais->type19.reserved,
                               ais->type19.speed / 10.0,
                               JSON_BOOL(ais->type19.accuracy),
                               ais->type19.lon / AIS_LATLON_DIV,
                               ais->type19.lat / AIS_LATLON_DIV,
                               ais->type19.course / 10.0,
                               ais->type19.heading,
                               ais->type19.second,
                               ais->type19.regional,
                               json_stringify(buf1, sizeof(buf1),
                                              ais->type19.shipname),
                               ais->type19.shiptype,
                               SHIPTYPE_DISPLAY(ais->type19.shiptype),
                               ais->type19.to_bow,
                               ais->type19.to_stern,
                               ais->type19.to_port,
                               ais->type19.to_starboard,
                               ais->type19.epfd,
                               EPFD_DISPLAY(ais->type19.epfd),
                               JSON_BOOL(ais->type19.raim),
                               ais->type19.dte,
                               JSON_BOOL(ais->type19.assigned));
            } else {
                (void)snprintf(buf + strlen(buf), buflen - strlen(buf),
                               "\"reserved\":%u,\"speed\":%u,\"accuracy\":%s,"
                               "\"lon\":%d,\"lat\":%d,\"course\":%u,"
                               "\"heading\":%u,\"second\":%u,\"regional\":%u,"
                               "\"shipname\":\"%s\","
                               "\"shiptype\":%u,\"shiptype_text\":\"%s\","
                               "\"to_bow\":%u,\"to_stern\":%u,\"to_port\":%u,"
                               "\"to_starboard\":%u,"
                               "\"epfd\":%u,\"epfd_text\":\"%s\","
                               "\"raim\":%s,\"dte\":%u,\"assigned\":%s}\r\n",
                               ais->type19.reserved,
                               ais->type19.speed,
                               JSON_BOOL(ais->type19.accuracy),
                               ais->type19.lon,
                               ais->type19.lat,
                               ais->type19.course,
                               ais->type19.heading,
                               ais->type19.second,
                               ais->type19.regional,
                               json_stringify(buf1, sizeof(buf1),
                                              ais->type19.shipname),
                               ais->type19.shiptype,
                               SHIPTYPE_DISPLAY(ais->type19.shiptype),
                               ais->type19.to_bow,
                               ais->type19.to_stern,
                               ais->type19.to_port,
                               ais->type19.to_starboard,
                               ais->type19.epfd,
                               EPFD_DISPLAY(ais->type19.epfd),
                               JSON_BOOL(ais->type19.raim),
                               ais->type19.dte,
                               JSON_BOOL(ais->type19.assigned));
            }
            break;
        case 20:			/* Data Link Management Message */
            (void)snprintf(buf + strlen(buf), buflen - strlen(buf),
                           "\"offset1\":%u,\"number1\":%u,"
                           "\"timeout1\":%u,\"increment1\":%u,"
                           "\"offset2\":%u,\"number2\":%u,"
                           "\"timeout2\":%u,\"increment2\":%u,"
                           "\"offset3\":%u,\"number3\":%u,"
                           "\"timeout3\":%u,\"increment3\":%u,"
                           "\"offset4\":%u,\"number4\":%u,"
                           "\"timeout4\":%u,\"increment4\":%u}\r\n",
                           ais->type20.offset1,
                           ais->type20.number1,
                           ais->type20.timeout1,
                           ais->type20.increment1,
                           ais->type20.offset2,
                           ais->type20.number2,
                           ais->type20.timeout2,
                           ais->type20.increment2,
                           ais->type20.offset3,
                           ais->type20.number3,
                           ais->type20.timeout3,
                           ais->type20.increment3,
                           ais->type20.offset4,
                           ais->type20.number4,
                           ais->type20.timeout4, ais->type20.increment4);
            break;
        case 21:			/* Aid to Navigation */
            if (scaled) {
                (void)snprintf(buf + strlen(buf), buflen - strlen(buf),
                               "\"aid_type\":%u,\"aid_type_text\":\"%s\","
                               "\"name\":\"%s\",\"lon\":%.4f,"
                               "\"lat\":%.4f,\"accuracy\":%s,\"to_bow\":%u,"
                               "\"to_stern\":%u,\"to_port\":%u,\"to_starboard\":%u,"
                               "\"epfd\":%u,\"epfd_text\":\"%s\","
                               "\"second\":%u,\"regional\":%u,"
                               "\"off_position\":%s,\"raim\":%s,"
                               "\"virtual_aid\":%s}\r\n",
                               ais->type21.aid_type,
                               NAVAIDTYPE_DISPLAY(ais->type21.aid_type),
                               json_stringify(buf1, sizeof(buf1),
                                              ais->type21.name),
                               ais->type21.lon / AIS_LATLON_DIV,
                               ais->type21.lat / AIS_LATLON_DIV,
                               JSON_BOOL(ais->type21.accuracy),
                               ais->type21.to_bow, ais->type21.to_stern,
                               ais->type21.to_port, ais->type21.to_starboard,
                               ais->type21.epfd,
                               EPFD_DISPLAY(ais->type21.epfd),
                               ais->type21.second,
                               ais->type21.regional,
                               JSON_BOOL(ais->type21.off_position),
                               JSON_BOOL(ais->type21.raim),
                               JSON_BOOL(ais->type21.virtual_aid));
            } else {
                (void)snprintf(buf + strlen(buf), buflen - strlen(buf),
                               "\"aid_type\":%u,\"aid_type_text\":\"%s\","
                               "\"name\":\"%s\",\"accuracy\":%s,"
                               "\"lon\":%d,\"lat\":%d,\"to_bow\":%u,"
                               "\"to_stern\":%u,\"to_port\":%u,\"to_starboard\":%u,"
                               "\"epfd\":%u,\"epfd_text\":\"%s\","
                               "\"second\":%u,\"regional\":%u,"
                               "\"off_position\":%s,\"raim\":%s,"
                               "\"virtual_aid\":%s}\r\n",
                               ais->type21.aid_type,
                               NAVAIDTYPE_DISPLAY(ais->type21.aid_type),
                               json_stringify(buf1, sizeof(buf1),
                                              ais->type21.name),
                               JSON_BOOL(ais->type21.accuracy),
                               ais->type21.lon,
                               ais->type21.lat,
                               ais->type21.to_bow,
                               ais->type21.to_stern,
                               ais->type21.to_port,
                               ais->type21.to_starboard,
                               ais->type21.epfd,
                               EPFD_DISPLAY(ais->type21.epfd),
                               ais->type21.second,
                               ais->type21.regional,
                               JSON_BOOL(ais->type21.off_position),
                               JSON_BOOL(ais->type21.raim),
                               JSON_BOOL(ais->type21.virtual_aid));
            }
            break;
        case 22:			/* Channel Management */
            (void)snprintf(buf + strlen(buf), buflen - strlen(buf),
                           "\"channel_a\":%u,\"channel_b\":%u,"
                           "\"txrx\":%u,\"power\":%s,",
                           ais->type22.channel_a,
                           ais->type22.channel_b,
                           ais->type22.txrx, JSON_BOOL(ais->type22.power));
            if (ais->type22.addressed) {
                (void)snprintf(buf + strlen(buf), buflen - strlen(buf),
                               "\"dest1\":%u,\"dest2\":%u,",
                               ais->type22.mmsi.dest1, ais->type22.mmsi.dest2);
            } else if (scaled) {
                (void)snprintf(buf + strlen(buf), buflen - strlen(buf),
                               "\"ne_lon\":\"%f\",\"ne_lat\":\"%f\","
                               "\"sw_lon\":\"%f\",\"sw_lat\":\"%f\",",
                               ais->type22.area.ne_lon / AIS_CHANNEL_LATLON_DIV,
                               ais->type22.area.ne_lat / AIS_CHANNEL_LATLON_DIV,
                               ais->type22.area.sw_lon / AIS_CHANNEL_LATLON_DIV,
                               ais->type22.area.sw_lat /
                               AIS_CHANNEL_LATLON_DIV);
            } else {
                (void)snprintf(buf + strlen(buf), buflen - strlen(buf),
                               "\"ne_lon\":%d,\"ne_lat\":%d,"
                               "\"sw_lon\":%d,\"sw_lat\":%d,",
                               ais->type22.area.ne_lon,
                               ais->type22.area.ne_lat,
                               ais->type22.area.sw_lon, ais->type22.area.sw_lat);
            }
            (void)snprintf(buf + strlen(buf), buflen - strlen(buf),
                           "\"addressed\":%s,\"band_a\":%s,"
                           "\"band_b\":%s,\"zonesize\":%u}\r\n",
                           JSON_BOOL(ais->type22.addressed),
                           JSON_BOOL(ais->type22.band_a),
                           JSON_BOOL(ais->type22.band_b), ais->type22.zonesize);
            break;
        case 23:			/* Group Assignment Command */
            if (scaled) {
                (void)snprintf(buf + strlen(buf), buflen - strlen(buf),
                               "\"ne_lon\":\"%f\",\"ne_lat\":\"%f\","
                               "\"sw_lon\":\"%f\",\"sw_lat\":\"%f\","
                               "\"stationtype\":%u,\"stationtype_text\":\"%s\","
                               "\"shiptype\":%u,\"shiptype_text\":\"%s\","
                               "\"interval\":%u,\"quiet\":%u}\r\n",
                               ais->type23.ne_lon / AIS_CHANNEL_LATLON_DIV,
                               ais->type23.ne_lat / AIS_CHANNEL_LATLON_DIV,
                               ais->type23.sw_lon / AIS_CHANNEL_LATLON_DIV,
                               ais->type23.sw_lat / AIS_CHANNEL_LATLON_DIV,
                               ais->type23.stationtype,
                               STATIONTYPE_DISPLAY(ais->type23.stationtype),
                               ais->type23.shiptype,
                               SHIPTYPE_DISPLAY(ais->type23.shiptype),
                               ais->type23.interval, ais->type23.quiet);
            } else {
                (void)snprintf(buf + strlen(buf), buflen - strlen(buf),
                               "\"ne_lon\":%d,\"ne_lat\":%d,"
                               "\"sw_lon\":%d,\"sw_lat\":%d,"
                               "\"stationtype\":%u,\"stationtype_text\":\"%s\","
                               "\"shiptype\":%u,\"shiptype_text\":\"%s\","
                               "\"interval\":%u,\"quiet\":%u}\r\n",
                               ais->type23.ne_lon,
                               ais->type23.ne_lat,
                               ais->type23.sw_lon,
                               ais->type23.sw_lat,
                               ais->type23.stationtype,
                               STATIONTYPE_DISPLAY(ais->type23.stationtype),
                               ais->type23.shiptype,
                               SHIPTYPE_DISPLAY(ais->type23.shiptype),
                               ais->type23.interval, ais->type23.quiet);
            }
            break;
        case 24:			/* Class B CS Static Data Report */
            if (ais->type24.part != both) {
                static char *partnames[] = {"AB", "A", "B"};
                (void)snprintf(buf + strlen(buf), buflen - strlen(buf),
                               "\"part\":\"%s\",",
                               json_stringify(buf1, sizeof(buf1),
                                              partnames[ais->type24.part]));
            }
            if (ais->type24.part != part_b)
                (void)snprintf(buf + strlen(buf), buflen - strlen(buf),
                               "\"shipname\":\"%s\",",
                               json_stringify(buf1, sizeof(buf1),
                                              ais->type24.shipname));
            if (ais->type24.part != part_a) {
                (void)snprintf(buf + strlen(buf), buflen - strlen(buf),
                               "\"shiptype\":%u,\"shiptype_text\":\"%s\","
                               "\"vendorid\":\"%s\",\"model\":%u,\"serial\":%u,"
                               "\"callsign\":\"%s\",",
                               ais->type24.shiptype,
                               SHIPTYPE_DISPLAY(ais->type24.shiptype),
                               json_stringify(buf1, sizeof(buf1),
                                              ais->type24.vendorid),
                               ais->type24.model,
                               ais->type24.serial,
                               json_stringify(buf2, sizeof(buf2),
                                              ais->type24.callsign));
                if (AIS_AUXILIARY_MMSI(ais->mmsi)) {
                    (void)snprintf(buf + strlen(buf), buflen - strlen(buf),
                                   "\"mothership_mmsi\":%u}\r\n",
                                   ais->type24.mothership_mmsi);
                } else {
                    (void)snprintf(buf + strlen(buf), buflen - strlen(buf),
                                   "\"to_bow\":%u,\"to_stern\":%u,"
                                   "\"to_port\":%u,\"to_starboard\":%u",
                                   ais->type24.dim.to_bow,
                                   ais->type24.dim.to_stern,
                                   ais->type24.dim.to_port,
                                   ais->type24.dim.to_starboard);
                }
            }
            if (buf[strlen(buf)-1] == ',')
                buf[strlen(buf)-1] = '\0';
            strncat(buf, "}\r\n", buflen);
            break;
        case 25:			/* Binary Message, Single Slot */
            (void)snprintf(buf + strlen(buf), buflen - strlen(buf),
                           "\"addressed\":%s,\"structured\":%s,\"dest_mmsi\":%u,"
                           "\"app_id\":%u,\"data\":\"%zd:%s\"}\r\n",
                           JSON_BOOL(ais->type25.addressed),
                           JSON_BOOL(ais->type25.structured),
                           ais->type25.dest_mmsi,
                           ais->type25.app_id,
                           ais->type25.bitcount,
                           gpsd_hexdump(scratchbuf, sizeof(scratchbuf),
                                        (char *)ais->type25.bitdata,
                                        BITS_TO_BYTES(ais->type25.bitcount)));
            break;
        case 26:			/* Binary Message, Multiple Slot */
            (void)snprintf(buf + strlen(buf), buflen - strlen(buf),
                           "\"addressed\":%s,\"structured\":%s,\"dest_mmsi\":%u,"
                           "\"app_id\":%u,\"data\":\"%zd:%s\",\"radio\":%u}\r\n",
                           JSON_BOOL(ais->type26.addressed),
                           JSON_BOOL(ais->type26.structured),
                           ais->type26.dest_mmsi,
                           ais->type26.app_id,
                           ais->type26.bitcount,
                           gpsd_hexdump(scratchbuf, sizeof(scratchbuf),
                                        (char *)ais->type26.bitdata,
                                        BITS_TO_BYTES(ais->type26.bitcount)),
                           ais->type26.radio);
            break;
        case 27:			/* Long Range AIS Broadcast message */
            if (scaled)
                (void)snprintf(buf + strlen(buf), buflen - strlen(buf),
                               "\"status\":\"%s\","
                               "\"accuracy\":%s,\"lon\":%.1f,\"lat\":%.1f,"
                               "\"speed\":%u,\"course\":%u,\"raim\":%s,\"gnss\":%s}\r\n",
                               nav_legends[ais->type27.status],
                               JSON_BOOL(ais->type27.accuracy),
                               ais->type27.lon / AIS_LONGRANGE_LATLON_DIV,
                               ais->type27.lat / AIS_LONGRANGE_LATLON_DIV,
                               ais->type27.speed,
                               ais->type27.course,
                               JSON_BOOL(ais->type27.raim),
                               JSON_BOOL(ais->type27.gnss));
            else
                (void)snprintf(buf + strlen(buf), buflen - strlen(buf),
                               "\"status\":%u,"
                               "\"accuracy\":%s,\"lon\":%d,\"lat\":%d,"
                               "\"speed\":%u,\"course\":%u,\"raim\":%s,\"gnss\":%s}\r\n",
                               ais->type27.status,
                               JSON_BOOL(ais->type27.accuracy),
                               ais->type27.lon,
                               ais->type27.lat,
                               ais->type27.speed,
                               ais->type27.course,
                               JSON_BOOL(ais->type27.raim),
                               JSON_BOOL(ais->type27.gnss));
            break;
        default:
            if (buf[strlen(buf) - 1] == ',')
                buf[strlen(buf) - 1] = '\0';
            (void)strlcat(buf, "}\r\n", buflen);
            break;
    }
    /*@ +formatcode +mustfreefresh @*/
}

/* gpsd_json.c ends here */
