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
//  Create a new zecho
ZLOG_EXPORT zecho_t *
    zecho_new (zyre_t *node);

//  Destroy the zecho
ZLOG_EXPORT void
    zecho_destroy (zecho_t **self_p);

//  Self test of this class
ZLOG_EXPORT void
    zecho_test (bool verbose);

//  @end

#ifdef __cplusplus
}
#endif

#endif
