/*  =========================================================================
    zlog_classes - private header file

    Copyright (c) the Contributors as noted in the AUTHORS file.
    This file is part of zlogger.

    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.
################################################################################
#  THIS FILE IS 100% GENERATED BY ZPROJECT; DO NOT EDIT EXCEPT EXPERIMENTALLY  #
#  Read the zproject/README.md for information about making permanent changes. #
################################################################################
    =========================================================================
*/

#ifndef ZLOG_CLASSES_H_INCLUDED
#define ZLOG_CLASSES_H_INCLUDED

//  Platform definitions, must come first
#include "platform.h"

//  External API
#include "../include/zlogger.h"

//  Extra headers

//  Opaque class structures to allow forward references

//  Internal API


//  *** To avoid double-definitions, only define if building without draft ***
#ifndef ZLOG_BUILD_DRAFT_API

//  Self test for private classes
ZLOG_PRIVATE void
    zlog_private_selftest (bool verbose);

#endif // ZLOG_BUILD_DRAFT_API

#endif
