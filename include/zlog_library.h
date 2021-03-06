/*  =========================================================================
    zlogger - generated layer of public API

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

#ifndef ZLOG_LIBRARY_H_INCLUDED
#define ZLOG_LIBRARY_H_INCLUDED

//  Set up environment for the application

//  External dependencies
#include <zyre.h>

//  ZLOG version macros for compile-time API detection
#define ZLOG_VERSION_MAJOR 0
#define ZLOG_VERSION_MINOR 1
#define ZLOG_VERSION_PATCH 0

#define ZLOG_MAKE_VERSION(major, minor, patch) \
    ((major) * 10000 + (minor) * 100 + (patch))
#define ZLOG_VERSION \
    ZLOG_MAKE_VERSION(ZLOG_VERSION_MAJOR, ZLOG_VERSION_MINOR, ZLOG_VERSION_PATCH)

#if defined (__WINDOWS__)
#   if defined ZLOG_STATIC
#       define ZLOG_EXPORT
#   elif defined ZLOG_INTERNAL_BUILD
#       if defined DLL_EXPORT
#           define ZLOG_EXPORT __declspec(dllexport)
#       else
#           define ZLOG_EXPORT
#       endif
#   elif defined ZLOG_EXPORTS
#       define ZLOG_EXPORT __declspec(dllexport)
#   else
#       define ZLOG_EXPORT __declspec(dllimport)
#   endif
#   define ZLOG_PRIVATE
#elif defined (__CYGWIN__)
#   define ZLOG_EXPORT
#   define ZLOG_PRIVATE
#else
#   define ZLOG_EXPORT
#   if (defined __GNUC__ && __GNUC__ >= 4) || defined __INTEL_COMPILER
#       define ZLOG_PRIVATE __attribute__ ((visibility ("hidden")))
#   else
#       define ZLOG_PRIVATE
#   endif
#endif

//  Project has no stable classes, so we build the draft API
#undef  ZLOG_BUILD_DRAFT_API
#define ZLOG_BUILD_DRAFT_API

//  Opaque class structures to allow forward references
//  These classes are stable or legacy and built in all releases
//  Draft classes are by default not built in stable releases
#ifdef ZLOG_BUILD_DRAFT_API
typedef struct _zecho_t zecho_t;
#define ZECHO_T_DEFINED
typedef struct _zvector_t zvector_t;
#define ZVECTOR_T_DEFINED
typedef struct _zelection_t zelection_t;
#define ZELECTION_T_DEFINED
typedef struct _selection_t selection_t;
#define SELECTION_T_DEFINED
typedef struct _zlog_t zlog_t;
#define ZLOG_T_DEFINED
#endif // ZLOG_BUILD_DRAFT_API


//  Public classes, each with its own header file
#ifdef ZLOG_BUILD_DRAFT_API
#include "zecho.h"
#include "zvector.h"
#include "zelection.h"
#include "selection.h"
#include "zlog.h"
#endif // ZLOG_BUILD_DRAFT_API

#ifdef ZLOG_BUILD_DRAFT_API
//  Self test for private classes
ZLOG_EXPORT void
    zlog_private_selftest (bool verbose);
#endif // ZLOG_BUILD_DRAFT_API

#endif
/*
################################################################################
#  THIS FILE IS 100% GENERATED BY ZPROJECT; DO NOT EDIT EXCEPT EXPERIMENTALLY  #
#  Read the zproject/README.md for information about making permanent changes. #
################################################################################
*/
