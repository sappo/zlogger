/*  =========================================================================
    zvector - Implements a dynamic vector clock

    Copyright (c) the Contributors as noted in the AUTHORS file.
    This file is part of zlogger.

    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.
    =========================================================================
*/

/*
@header
    zvector - Implements a dynamic vector clock
@discuss
@end
*/

#include "zlog_classes.h"

//  Structure of our class

struct _zvector_t {
    char* own_pid;
    zhashx_t *clock;
};

//  --------------------------------------------------------------------------
//  Local helper

static void
s_destroy_clock_value (void **clock_value_p)
{
    assert (clock_value_p);
    if (*clock_value_p) {
        unsigned long *clock_value = (unsigned long *) *clock_value_p;
        free (clock_value);
    }
}

static void
s_destroy_clock_key (void **key_p)
{
    assert (key_p);
    char **clock_key_p = (char **) key_p;
    zstr_free (clock_key_p);
}

//  --------------------------------------------------------------------------
//  Create a new zvector

zvector_t *
zvector_new (const char* pid)
{
    zvector_t *self = (zvector_t *) zmalloc (sizeof (zvector_t));

    assert (self);
    //  Initialize class properties here
    self->own_pid = strdup (pid);
    self->clock = zhashx_new ();
    zhashx_set_destructor (self->clock, s_destroy_clock_value);
    zhashx_set_key_destructor (self->clock, s_destroy_clock_key);
    unsigned long *clock_val = (unsigned long *) zmalloc (sizeof (unsigned long));
    *clock_val = 0;
    zhashx_insert (self->clock, strdup(pid), clock_val);
    return self;
}


//  --------------------------------------------------------------------------
//  Destroy the zvector


void
zvector_destroy (zvector_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        zvector_t *self = *self_p;
        //  Free class properties here
        zstr_free (&self->own_pid);
        zhashx_destroy (&self->clock);
        //  Free object itself
        free (self);
        *self_p = NULL;
    }
}


//  --------------------------------------------------------------------------
//  Event the zvector

void
zvector_event (zvector_t *self)
{
    assert (self);
    unsigned long *own_clock_value = (unsigned long*) zhashx_lookup (self->clock, self->own_pid);
    (*own_clock_value)++;
}


//  --------------------------------------------------------------------------
//  Send the zvector


zmsg_t *
zvector_send_prepare (zvector_t *self, zmsg_t *msg)
{
    zvector_event (self);
    zframe_t *packed_clock = zhashx_pack (self->clock);
    zmsg_prepend (msg, &packed_clock);
    return msg;
}


//  --------------------------------------------------------------------------
//  Recv the zvector


void
zvector_recv (zvector_t *self, zhashx_t *sender_clock)
{
    assert (self);
    unsigned long *own_clock_value = (unsigned long*) zhashx_lookup (self->clock, self->own_pid);
    (*own_clock_value)++;

    zlistx_t *sender_clock_procs = zhashx_keys (sender_clock);
    zlistx_set_destructor (sender_clock_procs, s_destroy_clock_key);
    const char* pid = (const char*) zlistx_first (sender_clock_procs);
    while (pid) {
        if (zhashx_lookup (self->clock, pid)) {
            unsigned long *own_pid_clock_value = (unsigned long*) zhashx_lookup (self->clock, pid);
            unsigned long *sender_pid_clock_value = (unsigned long*) zhashx_lookup (sender_clock, pid);

            if (*sender_pid_clock_value > *own_pid_clock_value)
                 *own_pid_clock_value = *sender_pid_clock_value;
        }
        else{
            unsigned long *sender_pid_clock_value = (unsigned long*) zhashx_lookup (sender_clock, pid);
            unsigned long *own_pid_clock_value = (unsigned long *) zmalloc (sizeof (unsigned long));
            *own_pid_clock_value = *sender_pid_clock_value;
            zhashx_insert (self->clock, strdup (pid), own_pid_clock_value);
        }

        pid = (const char*) zlistx_next (sender_clock_procs);
    }

    zlistx_destroy (&sender_clock_procs);
}

//  --------------------------------------------------------------------------
//  Self test of this class

void
zvector_test (bool verbose)
{
    printf (" * zvector: ");

    //  @selftest
    //  Simple create/destroy test
    zvector_t *self = zvector_new ("1231");
    assert (self);

    zhashx_t *sender_clock1 = zhashx_new ();
    zhashx_set_destructor (sender_clock1, s_destroy_clock_value);
    zhashx_set_key_destructor (sender_clock1, s_destroy_clock_key);


    unsigned long *value1 = (unsigned long *) zmalloc (sizeof (unsigned long));
    *value1 = 5;
    zhashx_insert (sender_clock1, strdup("1231"), value1);
    unsigned long *value2 = (unsigned long *) zmalloc (sizeof (unsigned long));
    *value2 = 10;
    zhashx_insert (sender_clock1, strdup("1232"), value2);

    zvector_recv (self, sender_clock1);

    zhashx_destroy (&sender_clock1);
    zvector_destroy (&self);

    //  @end
    printf ("OK\n");
}
