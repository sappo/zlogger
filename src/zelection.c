/*  =========================================================================
    zelection - class description

    Copyright (c) the Contributors as noted in the AUTHORS file.       
    This file is part of zlogger.                                      
                                                                       
    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.           
    =========================================================================
*/

/*
@header
    zelection - 
@discuss
@end
*/

#include "zlog_classes.h"

//  Structure of our class

struct _zelection_t {
    int filler;     //  Declare class properties here
};


//  --------------------------------------------------------------------------
//  Create a new zelection

zelection_t *
zelection_new (void)
{
    zelection_t *self = (zelection_t *) zmalloc (sizeof (zelection_t));
    assert (self);
    //  Initialize class properties here
    return self;
}


//  --------------------------------------------------------------------------
//  Destroy the zelection

void
zelection_destroy (zelection_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        zelection_t *self = *self_p;
        //  Free class properties here
        //  Free object itself
        free (self);
        *self_p = NULL;
    }
}

//  --------------------------------------------------------------------------
//  Self test of this class

void
zelection_test (bool verbose)
{
    printf (" * zelection: ");

    //  @selftest
    //  Simple create/destroy test
    zelection_t *self = zelection_new ();
    assert (self);
    zelection_destroy (&self);
    //  @end
    printf ("OK\n");
}
