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

//  Self test of this class
ZLOG_EXPORT void
    zvector_test (bool verbose);

//  @end

#ifdef __cplusplus
}
#endif

#endif
