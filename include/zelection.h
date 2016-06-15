/*  =========================================================================
    zelection - class description

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
    zelection_new (void);

//  Destroy the zelection
ZLOG_EXPORT void
    zelection_destroy (zelection_t **self_p);

//  Self test of this class
ZLOG_EXPORT void
    zelection_test (bool verbose);

//  @end

#ifdef __cplusplus
}
#endif

#endif
