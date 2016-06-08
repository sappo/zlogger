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

/*
@header
    zecho - Implements the echo algorithms
@discuss
@end
*/

#include "zlog_classes.h"

//  Structure of our class

struct _zecho_t {
    int filler;     //  Declare class properties here
};


//  --------------------------------------------------------------------------
//  Create a new zecho

zecho_t *
zecho_new (void)
{
    zecho_t *self = (zecho_t *) zmalloc (sizeof (zecho_t));
    assert (self);
    //  Initialize class properties here
    return self;
}


//  --------------------------------------------------------------------------
//  Destroy the zecho

void
zecho_destroy (zecho_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        zecho_t *self = *self_p;
        //  Free class properties here
        //  Free object itself
        free (self);
        *self_p = NULL;
    }
}

//  --------------------------------------------------------------------------
//  Self test of this class

void
zecho_test (bool verbose)
{
    printf (" * zecho: ");

    //  @selftest
    //  Simple create/destroy test
    zecho_t *self = zecho_new ();
    assert (self);
    zecho_destroy (&self);
    //  @end
    printf ("OK\n");
}
