/*  =========================================================================
    zlog - zlog actor

    Copyright (c) the Contributors as noted in the AUTHORS file.
    This file is part of zlogger.

    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.
    =========================================================================
*/

#ifndef ZLOG_H_INCLUDED
#define ZLOG_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif


//  @interface
//  Create new zlog actor instance.
//  @TODO: Describe the purpose of this actor!
//
//      zactor_t *zlog = zactor_new (zlog, NULL);
//
//  Destroy zlog instance.
//
//      zactor_destroy (&zlog);
//
//  Enable verbose logging of commands and activity:
//
//      zstr_send (zlog, "VERBOSE");
//
//  Start zlog actor.
//
//      zstr_sendx (zlog, "START", NULL);
//
//  Stop zlog actor.
//
//      zstr_sendx (zlog, "STOP", NULL);
//
//  This is the zlog constructor as a zactor_fn;
ZLOG_EXPORT void
    zlog_actor (zsock_t *pipe, void *args);


//  --------------------------------------------------------------------------
//  Compares the zvector_t's of given logMsg a to logMsg b.
//  Returns -1 if a < b, 0 if a = b, 1 if a > b and 2 if a and b parallel
ZLOG_EXPORT int
    zlog_compare_logMsg (const char *logMsg_a, const char *logMsg_b);

//  Orders log with the given filepath
ZLOG_EXPORT void
    zlog_order_log (const char *path_src, const char *path_dst);

//  Self test of this actor
ZLOG_EXPORT void
    zlog_test (bool verbose);
//  @end

#ifdef __cplusplus
}
#endif

#endif
