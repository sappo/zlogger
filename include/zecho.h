/*  =========================================================================
    zecho - Implements the echo algorithms

    Copyright (c) the Contributors as noted in the AUTHORS file.
    This file is part of CZMQ, the high-level C binding for 0MQ:
    http://czmq.zeromq.org.

    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.
    =========================================================================
*/

#ifndef ZECHO_H_INCLUDED
#define ZECHO_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

//  @interface

//  Process INFORM or COLLECT messages
typedef void (zecho_process_fn) (
    zecho_t *self, zmsg_t *msg,  void *handler);
//  Create custom INFORM or COLLECT messages content
typedef zmsg_t * (zecho_create_fn) (
    zecho_t *self, void *handler);

//  Create a new zecho
ZLOG_EXPORT zecho_t *
    zecho_new (zyre_t *node);

//  Destroy the zecho
ZLOG_EXPORT void
    zecho_destroy (zecho_t **self_p);

//  Initiate the echo algorithm
ZLOG_EXPORT void
    zecho_init (zecho_t *self);

//  Handle a received echo token
ZLOG_EXPORT int
    zecho_recv (zecho_t *self, zyre_event_t *token);

//  Sets a handler which is passed to custom collect functions.
ZLOG_EXPORT void
    zecho_set_collect_handler (zecho_t *self, void *handler);

//  Set a user-defined function to process collect messages from peers; This
//  function is invoked during the second (incoming) wave.
ZLOG_EXPORT void
    zecho_set_collect_process (zecho_t *self, zecho_process_fn *process_fn);

//  Set a user-defined function to create a custom message part for the collect
//  message; This function is invoked during the second (incoming) wave. The
//  returned message's content is appended to the wave message.
ZLOG_EXPORT void
    zecho_set_collect_create (zecho_t *self, zecho_create_fn *collect_fn);

//  Sets a handler which is passed to custom inform functions.
ZLOG_EXPORT void
    zecho_set_inform_handler (zecho_t *self, void *handler);

//  Set a user-defined function to process inform messages from peers; This
//  function is invoked during the first (outgoing) wave.
ZLOG_EXPORT void
    zecho_set_inform_process (zecho_t *self, zecho_process_fn *process_fn);

//  Set a user-defined function to create a custom message part for the inform
//  message; This function is invoked during the first (outgoing) wave. The
//  returned message's content is appended to the wave message.
ZLOG_EXPORT void
    zecho_set_inform_create (zecho_t *self, zecho_create_fn *collect_fn);

//  Set a vector clock handle. Echo messages will be prepended with the
//  vector if not NULL.
ZLOG_EXPORT void
    zecho_set_clock (zecho_t *self, zvector_t *clock);

//  Enable/disable verbose logging.
ZLOG_EXPORT void
    zecho_set_verbose (zecho_t *self, bool verbose);

//  Self test of this class
ZLOG_EXPORT void
    zecho_test (bool verbose);

//  @end

#ifdef __cplusplus
}
#endif

#endif
