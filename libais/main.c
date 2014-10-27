//
//  main.c
//  libais
//
//  Created by Chris Bünger on 26/10/14.
//  Copyright (c) 2014 Chris Bünger. All rights reserved.
//

#include <stdio.h>
#include "libais.h"
#include "gps.h"

int main(int argc, const char * argv[]) {
    // insert code here...
    printf("Hello, World!\n");
    
    struct ais_t ais;
    const char msg[] = "!AIVDM,1,1,,B,177KI=011nrFFK1p0wTKII2>06;`,0*04";
    size_t len = sizeof(msg);

    char buf[JSON_VAL_MAX * 2 + 1];
    size_t buflen = sizeof(buf);
    
    aivdm_decode(msg, sizeof(msg), &ais, 0);
    printf("type: %d, repeat: %d, mmsi: %d\n", ais.type, ais.repeat, ais.mmsi);
    json_aivdm_dump(&ais, NULL, true, buf, buflen);
    printf("JSON: %s", buf);

    return 0;
}
