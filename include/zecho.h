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

// Handle INFORM or COLLECT  messages
typedef zmsg_t * (zecho_handle_fn) (
    void *self, zmsg_t *msg);

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

//  Self test of this class
ZLOG_EXPORT void
    zecho_test (bool verbose);

//  @end

#ifdef __cplusplus
}
#endif

#endif
