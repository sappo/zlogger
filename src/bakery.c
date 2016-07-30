/*  =========================================================================
    bakery - Bakery with zlogger support

    Copyright (c) the Contributors as noted in the AUTHORS file.
    This file is part of zlogger.

    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.
    =========================================================================
*/

/*
@header
    bakery - Bakery with zlogger support
@discuss
@end
*/

#include "zlog_classes.h"

int main (int argc, char *argv [])
{
    bool verbose = false;
    bool dump_ts = false;
    int argn;
    unsigned long waittime = 10;
    for (argn = 1; argn < argc; argn++) {
        if (streq (argv [argn], "--help")
        ||  streq (argv [argn], "-h")) {
            puts ("bakery [options] ...");
            puts ("  --dump / -d            dump time space subgraph");
            puts ("  --wait / -w            wait s until terminating");
            puts ("  --verbose / -v         verbose test output");
            puts ("  --help / -h            this information");
            return 0;
        }
        else
        if (streq (argv [argn], "--verbose")
        ||  streq (argv [argn], "-v"))
            verbose = true;
        else
        if (streq (argv [argn], "--dump")
        ||  streq (argv [argn], "-d"))
            dump_ts = true;
        else
        if (streq (argv [argn], "--wait")
        ||  streq (argv [argn], "-w"))
            waittime = strtoul (argv [++argn], NULL, 10);
        else {
            printf ("Unknown option: %s\n", argv [argn]);
            return 1;
        }
    }

    //  Insert main code here
    zactor_t *zlog = zactor_new (zlog_actor, NULL);

    if (verbose) {
        zsys_info ("bakery - Bakery with zlogger support");
        zstr_send (zlog, "VERBOSE");
    }
    if (dump_ts)
        zstr_send (zlog, "DUMP TS");

    zstr_send (zlog, "START");
    //  Give time to interconnect and elect
    zclock_sleep (750);

    int64_t time = zclock_mono ();
    waittime *= 1000;

    // Bakery stuff!
    zstr_sendm (zlog, "SEND RANDOM");
    zstr_send (zlog, "STIRRED");
    while (zclock_mono () - time < waittime) {
        zmsg_t *msg = zmsg_recv_nowait (zlog);
        if (msg) {
            char *content = zmsg_popstr (msg);
            char *owner = zmsg_popstr (msg);
            if (strncmp (content, "STIRRED", 6) == 0) {
                zstr_sendm (zlog, "SEND RANDOM");
                zstr_sendm (zlog, "BAKED");
                zstr_send (zlog, owner);

            }
            else
            if (strncmp (content, "BAKED", 5) == 0) {
                zstr_sendm (zlog, "SEND RANDOM");
                zstr_sendm (zlog, "EATEN");
                zstr_send (zlog, owner);
            }

            zstr_free (&content);
            zstr_free (&owner);
            zmsg_destroy (&msg);
        }
        /*zclock_sleep (100);*/
    }


    zactor_destroy (&zlog);
    //  Give time to disconnect
    zclock_sleep (250);

    return 0;
}
