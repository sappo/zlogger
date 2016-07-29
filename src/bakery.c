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
    int argn;
    for (argn = 1; argn < argc; argn++) {
        if (streq (argv [argn], "--help")
        ||  streq (argv [argn], "-h")) {
            puts ("bakery [options] ...");
            puts ("  --verbose / -v         verbose test output");
            puts ("  --help / -h            this information");
            return 0;
        }
        else
        if (streq (argv [argn], "--verbose")
        ||  streq (argv [argn], "-v"))
            verbose = true;
        else {
            printf ("Unknown option: %s\n", argv [argn]);
            return 1;
        }
    }
    //  Insert main code here
    if (verbose)
        zsys_info ("bakery - Bakery with zlogger support");

    zactor_t *zlog = zactor_new (zlog_actor, NULL);

    zstr_send (zlog, "START");
    //  Give time to interconnect and elect
    zclock_sleep (750);

    // TODO: Bakery stuff!
    zstr_sendm (zlog, "SEND RANDOM");
    zstr_send (zlog, "STIRRED");

    zclock_sleep (5000);

    zactor_destroy (&zlog);
    //  Give time to disconnect
    zclock_sleep (250);

    return 0;
}
