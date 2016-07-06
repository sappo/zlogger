/*  =========================================================================
    zelection - Holds an election with all connected peers

    Copyright (c) the Contributors as noted in the AUTHORS file.
    This file is part of zlogger.

    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.
    =========================================================================
*/

#ifndef ZELECTION_H_INCLUDED
#define ZELECTION_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

//  @interface
//  Create a new zelection
ZLOG_EXPORT zelection_t *
    zelection_new (zyre_t *node);

//  Destroy the zelection
ZLOG_EXPORT void
    zelection_destroy (zelection_t **self_p);

//  Initiate election
ZLOG_EXPORT void
    zelection_start (zelection_t *self);

//  Handle received election and leader messages. Return 1 if election is
//  still in progress, 0 if election is concluded and -1 is an error occurred.
ZLOG_EXPORT int
    zelection_recv (zelection_t *self, zyre_event_t *event);

//  Returns the leader if an election is finished, otherwise NULL.
ZLOG_EXPORT const char *
    zelection_leader (zelection_t *self);

//  Returns true if an election is finished and won.
ZLOG_EXPORT bool
    zelection_finished (zelection_t *self);

//  Set a vector clock handle. Election message will be prepended with the
//  vector if not NULL.
ZLOG_EXPORT void
    zelection_set_clock (zelection_t *self, zvector_t *clock);

//  Enable/disable verbose logging.
ZLOG_EXPORT void
    zelection_set_verbose (zelection_t *self, bool verbose);

//  Print election status to command line
ZLOG_EXPORT void
    zelection_print (zelection_t *self);

//  Self test of this class
ZLOG_EXPORT void
    zelection_test (bool verbose);

//  @end

#ifdef __cplusplus
}
#endif

#endif
