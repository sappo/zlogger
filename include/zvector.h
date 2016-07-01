/*  =========================================================================
    zvector - Implements a dynamic vector clock

    Copyright (c) the Contributors as noted in the AUTHORS file.
    This file is part of zlogger.

    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.
    =========================================================================
*/

#ifndef ZVECTOR_H_INCLUDED
#define ZVECTOR_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

//  @interface
//  Create a new zvector
ZLOG_EXPORT zvector_t *
    zvector_new (const char* pid);

//  Destroy the zvector
ZLOG_EXPORT void
    zvector_destroy (zvector_t **self_p);

//  Eventing own clock & packing vectorclock with given msg
ZLOG_EXPORT zmsg_t *
    zvector_send_prepare (zvector_t *self, zmsg_t *msg);

//  Recv the zvector & updates own vectorclock
ZLOG_EXPORT void
    zvector_recv (zvector_t *self, zmsg_t *msg);

//  Log informational message - low priority. Prepends the current VC.
ZLOG_EXPORT void
    zvector_info (zvector_t *self, char *format, ...);

//  Prints the zvector for debug purposes
ZLOG_EXPORT void
    zvector_print (zvector_t *self);

//  Self test of this class
ZLOG_EXPORT void
    zvector_test (bool verbose);

//  @end

#ifdef __cplusplus
}
#endif

#endif
