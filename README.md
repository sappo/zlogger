
<A name="toc1-3" title="Zlogger" />
# Zlogger

[![Build Status](https://travis-ci.org/zeromq/zlogger.svg?branch=master)](https://travis-ci.org/...)

<A name="toc2-8" title="Contents" />
## Contents


**<a href="#toc2-13">Overview</a>**

**<a href="#toc3-16">Scope and Goals</a>**

**<a href="#toc3-19">Ownership and License</a>**

**<a href="#toc2-30">Using Zlogger</a>**

**<a href="#toc3-33">Building on Linux</a>**

**<a href="#toc3-107">Building on Windows</a>**

**<a href="#toc3-143">Linking with an Application</a>**

**<a href="#toc3-150">API Summary</a>**
*  <a href="#toc4-155">zlog - zlog actor</a>
*  <a href="#toc4-239">zecho - Implements the echo algorithms</a>
*  <a href="#toc4-416">zvector - Implements a dynamic vector clock</a>

**<a href="#toc3-632">Hints to Contributors</a>**

**<a href="#toc3-643">This Document</a>**

<A name="toc2-13" title="Overview" />
## Overview

<A name="toc3-16" title="Scope and Goals" />
### Scope and Goals

<A name="toc3-19" title="Ownership and License" />
### Ownership and License

The contributors are listed in AUTHORS. This project uses the MPL v2 license, see LICENSE.

Zlogger uses the [C4.1 (Collective Code Construction Contract)](http://rfc.zeromq.org/spec:22) process for contributions.

Zlogger uses the [CLASS (C Language Style for Scalabilty)](http://rfc.zeromq.org/spec:21) guide for code style.

To report an issue, use the [Zlogger issue tracker](https://github.com/zeromq/zlogger/issues) at github.com.

<A name="toc2-30" title="Using Zlogger" />
## Using Zlogger

<A name="toc3-33" title="Building on Linux" />
### Building on Linux

To start with, you need at least these packages:

* {{git-all}} -- git is how we share code with other people.

* {{build-essential}}, {{libtool}}, {{pkg-config}} - the C compiler and related tools.

* {{autotools-dev}}, {{autoconf}}, {{automake}} - the GNU autoconf makefile generators.

* {{cmake}} - the CMake makefile generators (an alternative to autoconf).

Plus some others:

* {{uuid-dev}}, {{libpcre3-dev}} - utility libraries.

* {{valgrind}} - a useful tool for checking your code.

Which we install like this (using the Debian-style apt-get package manager):

```
    sudo apt-get update
    sudo apt-get install -y \
    git-all build-essential libtool \
    pkg-config autotools-dev autoconf automake cmake \
    uuid-dev libpcre3-dev valgrind

    # only execute this next line if interested in updating the man pages as
    # well (adds to build time):
    sudo apt-get install -y asciidoc
```
Here's how to build Zlogger from GitHub (building from packages is very similar, you don't clone a repo but unpack a tarball), including the libsodium (for security) and libzmq (ZeroMQ core) libraries:

```
    git clone --depth 1 -b stable https://github.com/jedisct1/libsodium.git
    cd libsodium
    ./autogen.sh && ./configure && make check
    sudo make install
    cd ..

    git clone git://github.com/zeromq/libzmq.git
    cd libzmq
    ./autogen.sh
    # do not specify "--with-libsodium" if you prefer to use internal tweetnacl
    # security implementation (recommended for development)
    ./configure --with-libsodium
    make check
    sudo make install
    sudo ldconfig
    cd ..

    git clone git://github.com/zeromq/czmq.git
    cd czmq
    ./autogen.sh && ./configure && make check
    sudo make install
    sudo ldconfig
    cd ..

    git clone git://github.com/zeromq/zyre.git
    cd zyre
    ./autogen.sh && ./configure && make check
    sudo make install
    sudo ldconfig
    cd ..

    git clone git://github.com/.../...git
    cd zlogger
    ./autogen.sh && ./configure && make check
    sudo make install
    sudo ldconfig
    cd ..
```

<A name="toc3-107" title="Building on Windows" />
### Building on Windows

To start with, you need MS Visual Studio (C/C++). The free community edition works well.

Then, install git, and make sure it works from a DevStudio command prompt:

```
git
```

Now let's build Zlogger from GitHub:

```
    git clone --depth 1 -b stable https://github.com/jedisct1/libsodium.git
    git clone git://github.com/zeromq/libzmq.git
    git clone git://github.com/zeromq/czmq.git
    git clone git://github.com/zeromq/zyre.git
    git clone git://github.com/zeromq/zlogger.git
    cd zlogger\builds\msvc
    configure.bat
    cd build
    buildall.bat
    cd ..\..\..\..
```

Test by running the `zlogger_selftest` command:
```
    dir/s/b zlogger_selftest.exe
    zlogger\builds\msvc\vs2013\DebugDEXE\zlogger_selftest.exe
    zlogger\builds\msvc\vs2013\ReleaseDEXE\zlogger_selftest.exe

    :: select your choice and run it
    zlogger\builds\msvc\vs2013\DebugDEXE\zlogger_selftest.exe
```

<A name="toc3-143" title="Linking with an Application" />
### Linking with an Application

Include `zlogger.h` in your application and link with libzlogger. Here is a typical gcc link command:

    gcc -lzlogger -lzyre -lczmq -lzmq myapp.c -o myapp

<A name="toc3-150" title="API Summary" />
### API Summary

This is the API provided by Zlogger 0.x, in alphabetical order.

<A name="toc4-155" title="zlog - zlog actor" />
#### zlog - zlog actor

zlog - zlog actor

Please add @discuss section in ../src/zlog.c.

This is the class interface:

```h
    //  Create new zlog actor instance.
    //  @TODO: Describe the purpose of this actor!
    //
    //      zactor_t *zlog = zactor_new (zlog, NULL);
    //
    //  Destroy zlog instance.
    //
    //      zactor_destroy (&zlog);
    //
    //  Enable verbose logging of commands and activity:
    //
    //      zstr_send (zlog, "VERBOSE");
    //
    //  Start zlog actor.
    //
    //      zstr_sendx (zlog, "START", NULL);
    //
    //  Stop zlog actor.
    //
    //      zstr_sendx (zlog, "STOP", NULL);
    //
    //  This is the zlog constructor as a zactor_fn;
    ZLOG_EXPORT void
        zlog_actor (zsock_t *pipe, void *args);
    
    //  Self test of this actor
    ZLOG_EXPORT void
        zlog_test (bool verbose);
```

This is the class self test code:

```c
    char *params1[3] = {"inproc://logger1", "logger1", "GOSSIP MASTER"};
    zactor_t *zlog = zactor_new (zlog_actor, params1);
    
    char *params2[3] = {"inproc://logger2", "logger2", "GOSSIP SLAVE"};
    zactor_t *zlog2 = zactor_new (zlog_actor, params2);
    
    char *params3[3] = {"inproc://logger3", "logger3", "GOSSIP SLAVE"};
    zactor_t *zlog3 = zactor_new (zlog_actor, params3);
    
    char *params4[3] = {"inproc://logger4", "logger4", "GOSSIP SLAVE"};
    zactor_t *zlog4 = zactor_new (zlog_actor, params4);
    
    if (verbose) {
        zstr_send (zlog, "VERBOSE");
        zstr_send (zlog2, "VERBOSE");
        zstr_send (zlog3, "VERBOSE");
        zstr_send (zlog4, "VERBOSE");
    }
    
    zstr_send (zlog, "START");
    zstr_send (zlog2, "START");
    zstr_send (zlog3, "START");
    zstr_send (zlog4, "START");
    
    //  Give time to interconnect and elect
    zclock_sleep (750);
    
    zstr_send (zlog, "STOP");
    zstr_send (zlog2, "STOP");
    zstr_send (zlog3, "STOP");
    zstr_send (zlog4, "STOP");
    
    //  Give time to disconnect
    zclock_sleep (250);
    
    zactor_destroy (&zlog);
    zactor_destroy (&zlog2);
    zactor_destroy (&zlog3);
    zactor_destroy (&zlog4);
```

<A name="toc4-239" title="zecho - Implements the echo algorithms" />
#### zecho - Implements the echo algorithms

zecho - Implements the echo algorithms

Please add @discuss section in ../src/zecho.c.

This is the class interface:

```h
    
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
    
    //  Enable/disable verbose logging.
    ZLOG_EXPORT void
        zecho_set_verbose (zecho_t *self, bool verbose);
    
    //  Self test of this class
    ZLOG_EXPORT void
        zecho_test (bool verbose);
    
```

This is the class self test code:

```c
    int rc;
    //  Init zyre nodes
    zyre_t *node1 = zyre_new ("node1");
    assert (node1);
    //  Set inproc endpoint for this node
    rc = zyre_set_endpoint (node1, "inproc://zyre-node1");
    assert (rc == 0);
    //  Set up gossip network for this node
    zyre_gossip_bind (node1, "inproc://gossip-hub");
    rc = zyre_start (node1);
    assert (rc == 0);
    
    zyre_t *node2 = zyre_new ("node2");
    assert (node2);
    //  Set inproc endpoint for this node
    rc = zyre_set_endpoint (node2, "inproc://zyre-node2");
    assert (rc == 0);
    //  Set up gossip network for this node
    zyre_gossip_connect (node2, "inproc://gossip-hub");
    rc = zyre_start (node2);
    assert (rc == 0);
    
    zyre_t *node3 = zyre_new ("node3");
    assert (node3);
    //  Set inproc endpoint for this node
    rc = zyre_set_endpoint (node3, "inproc://zyre-node3");
    assert (rc == 0);
    //  Set up gossip network for this node
    zyre_gossip_connect (node3, "inproc://gossip-hub");
    rc = zyre_start (node3);
    assert (rc == 0);
    
    //  Setup echo
    zecho_t *echo1 = zecho_new (node1);
    zecho_t *echo2 = zecho_new (node2);
    zecho_t *echo3 = zecho_new (node3);
    zecho_set_verbose (echo1, verbose);
    zecho_set_verbose (echo2, verbose);
    zecho_set_verbose (echo3, verbose);
    
    //  Join topology
    zyre_join (node1, "GLOBAL");
    zyre_join (node2, "GLOBAL");
    zyre_join (node2, "LOCAL");
    zyre_join (node3, "LOCAL");
    
    //  Give time for them to interconnect
    zclock_sleep (500);
    
    if (verbose) {
        zyre_dump (node1);
        zclock_sleep (50);
        zyre_dump (node2);
        zclock_sleep (50);
        zyre_dump (node3);
        zclock_sleep (50);
    }
    
    zecho_init (echo1);
    zclock_sleep (500);
    
    zyre_event_t *event = NULL;
    
    do {
        event = zyre_event_new (node2);
        if (!streq (zyre_event_type (event), "WHISPER"))
            zyre_event_destroy (&event);
        else
            break;
    } while (1);
    char *type = zmsg_popstr (zyre_event_msg (event));
    assert (streq (type, "ZE"));
    zstr_free (&type);
    zecho_recv (echo2, event);
    
    do {
        event = zyre_event_new (node3);
        if (!streq (zyre_event_type (event), "WHISPER"))
            zyre_event_destroy (&event);
        else
            break;
    } while (1);
    type = zmsg_popstr (zyre_event_msg (event));
    assert (streq (type, "ZE"));
    zstr_free (&type);
    zecho_recv (echo3, event);
    
    do {
        event = zyre_event_new (node2);
        if (!streq (zyre_event_type (event), "WHISPER"))
            zyre_event_destroy (&event);
        else
            break;
    } while (1);
    type = zmsg_popstr (zyre_event_msg (event));
    assert (streq (type, "ZE"));
    zstr_free (&type);
    zecho_recv (echo2, event);
    
    do {
        event = zyre_event_new (node1);
        if (!streq (zyre_event_type (event), "WHISPER"))
            zyre_event_destroy (&event);
        else
            break;
    } while (1);
    type = zmsg_popstr (zyre_event_msg (event));
    assert (streq (type, "ZE"));
    zstr_free (&type);
    zecho_recv (echo1, event);
    
    if (verbose) {
        // Print result
        zecho_print (echo1);
        zecho_print (echo2);
        zecho_print (echo3);
    }
    
    //  Cleanup
    zecho_destroy (&echo1);
    zecho_destroy (&echo2);
    zecho_destroy (&echo3);
    
    zyre_stop (node1);
    zyre_stop (node2);
    zyre_stop (node3);
    
    zyre_destroy (&node1);
    zyre_destroy (&node2);
    zyre_destroy (&node3);
    
```

<A name="toc4-416" title="zvector - Implements a dynamic vector clock" />
#### zvector - Implements a dynamic vector clock

zvector - Implements a dynamic vector clock

Please add @discuss section in ../src/zvector.c.

This is the class interface:

```h
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
    
```

This is the class self test code:

```c
    
    
    
    //  Simple create/destroy test of zvector_t
    zvector_t *test1_self = zvector_new ("1000");
    assert (test1_self);
    zvector_destroy (&test1_self);
    
    
    
    // Simple test for converting a zvector to stringrepresentation
    // and from stringrepresentation to a zvector
    zvector_t *test2_self = zvector_new ("1000");
    assert (test2_self);
    
    // inserting some clocks & values
    zvector_event (test2_self);
    unsigned long *test2_inserted_value1 = (unsigned long *) zmalloc (sizeof (unsigned long));
    *test2_inserted_value1 = 7;
    zhashx_insert (test2_self->clock, "1001", test2_inserted_value1);
    unsigned long *test2_inserted_value2 = (unsigned long *) zmalloc (sizeof (unsigned long));
    *test2_inserted_value2 = 11;
    zhashx_insert (test2_self->clock, "1002", test2_inserted_value2);
    
    char *test2_string = zvector_to_string (test2_self);
    assert (streq (test2_string, "VC:3;1001,7;1000,1;1002,11;"));
    
    zvector_t *test2_generated = zvector_from_string (test2_string);
    assert ( *(unsigned long *) zhashx_lookup (test2_generated->clock, "1000") == 1 );
    assert ( *(unsigned long *) zhashx_lookup (test2_generated->clock, "1001") == 7 );
    assert ( *(unsigned long *) zhashx_lookup (test2_generated->clock, "1002") == 11 );
    
    zstr_free (&test2_string);
    zvector_destroy (&test2_self);
    zvector_destroy (&test2_generated);
    
    
    
    //  Simple event test
    zvector_t *test3_self = zvector_new ("1000");
    assert (test3_self);
    
    unsigned long *test3_value1 = (unsigned long *) zmalloc (sizeof (unsigned long));
    *test3_value1 = 5;
    zhashx_insert (test3_self->clock, "1001", test3_value1);
    assert ( *(unsigned long *) zhashx_lookup (test3_self->clock, "1000") == 0 );
    
    zvector_event (test3_self);
    assert ( *(unsigned long *) zhashx_lookup (test3_self->clock, "1000") == 1 );
    assert ( *(unsigned long *) zhashx_lookup (test3_self->clock, "1001") == 5 );
    
    zvector_destroy (&test3_self);
    
    
    
    //  Simple recv test
    zvector_t *test4_self_clock = zvector_new ("1000");
    char *test4_sender_clock1_stringRep = zsys_sprintf ("%s", "VC:2;1000,5;1001,10;");
    char *test4_sender_clock2_stringRep = zsys_sprintf ("%s", "VC:2;1000,20;1002,30;");
    zvector_t *test4_sender_clock1 = zvector_from_string (test4_sender_clock1_stringRep);
    zvector_t *test4_sender_clock2 = zvector_from_string (test4_sender_clock2_stringRep);
    assert (test4_self_clock);
    assert (test4_sender_clock1);
    assert (test4_sender_clock2);
    
    // receive sender clock 1 and add key-value pairs to own clock
    zmsg_t *test4_msg1 = zmsg_new ();
    zframe_t *test4_packed_clock1 = zhashx_pack_own (test4_sender_clock1->clock, s_serialize_clock_value);
    zmsg_prepend (test4_msg1, &test4_packed_clock1);
    zvector_recv (test4_self_clock, test4_msg1);
    zmsg_destroy (&test4_msg1);
    assert ( *(unsigned long *) zhashx_lookup (test4_self_clock->clock, "1000") == 5 );
    assert ( *(unsigned long *) zhashx_lookup (test4_self_clock->clock, "1001") == 10 );
    
    // receive sender clock 2 and add key-value pairs to own clock
    test4_msg1 = zmsg_new ();
    zframe_t *test4_packed_clock2 = zhashx_pack_own (test4_sender_clock2->clock, s_serialize_clock_value);
    zmsg_prepend (test4_msg1, &test4_packed_clock2);
    zvector_recv (test4_self_clock, test4_msg1);
    zmsg_destroy (&test4_msg1);
    assert ( *(unsigned long *) zhashx_lookup (test4_self_clock->clock, "1000") == 20 );
    assert ( *(unsigned long *) zhashx_lookup (test4_self_clock->clock, "1001") == 10 );
    assert ( *(unsigned long *) zhashx_lookup (test4_self_clock->clock, "1002") == 30 );
    
    zstr_free (&test4_sender_clock1_stringRep);
    zstr_free (&test4_sender_clock2_stringRep);
    zvector_destroy (&test4_self_clock);
    zvector_destroy (&test4_sender_clock1);
    zvector_destroy (&test4_sender_clock2);
    
    
    
    // Simple send_prepare test
    zvector_t *test5_self = zvector_new ("1000");
    assert (test5_self);
    zvector_event (test5_self);
    zmsg_t *test5_zmsg = zmsg_new ();
    zmsg_pushstr (test5_zmsg, "test");
    
    zvector_send_prepare (test5_self, test5_zmsg);
    zframe_t *test5_popped_frame = zmsg_pop (test5_zmsg);
    zhashx_t *test5_unpacked_clock = zhashx_unpack_own (test5_popped_frame, s_deserialize_clock_value);
    char *test5_unpacked_string = zmsg_popstr (test5_zmsg);
    assert (streq (test5_unpacked_string, "test"));
    assert ( *(unsigned long *) zhashx_lookup (test5_unpacked_clock, "1000") == 2 );
    
    zframe_destroy (&test5_popped_frame);
    zstr_free (&test5_unpacked_string);
    zhashx_destroy (&test5_unpacked_clock);
    zmsg_destroy (&test5_zmsg);
    zvector_destroy (&test5_self);
    
    
    
    // Simple compare test
    char *test6_self_stringrep = zsys_sprintf ("%s", "VC:3;1000,10;2000,10;3000,10;");
    char *test6_before_stringrep1 = zsys_sprintf ("%s", "VC:2;1100,10;3000,9;");
    char *test6_before_stringrep2 = zsys_sprintf ("%s", "VC:3;1100,10;2100,10;3000,9;");
    char *test6_before_stringrep3 = zsys_sprintf ("%s", "VC:4;1100,10;2100,10;3000,9;4000,9");
    char *test6_parallel_stringrep1 = zsys_sprintf ("%s", "VC:2;1500,10;2500,10;");
    char *test6_parallel_stringrep2 = zsys_sprintf ("%s", "VC:3;1500,10;2500,10;3500,10;");
    char *test6_parallel_stringrep3 = zsys_sprintf ("%s", "VC:4;500,20;1500,10;2500,10;3500,10;");
    char *test6_after_stringrep1 = zsys_sprintf ("%s", "VC:2;1000,11;2200,10;");
    char *test6_after_stringrep2 = zsys_sprintf ("%s", "VC:3;1200,10;2200,10;3000,11;");
    char *test6_after_stringrep3 = zsys_sprintf ("%s", "VC:4;1200,10;2200,10;3000,11;2000,11;");
    zvector_t *test6_self = zvector_from_string (test6_self_stringrep);
    zvector_t *test6_before1 = zvector_from_string (test6_before_stringrep1);
    zvector_t *test6_before2 = zvector_from_string (test6_before_stringrep2);
    zvector_t *test6_before3 = zvector_from_string (test6_before_stringrep3);
    zvector_t *test6_parallel1 = zvector_from_string (test6_parallel_stringrep1);
    zvector_t *test6_parallel2 = zvector_from_string (test6_parallel_stringrep2);
    zvector_t *test6_parallel3 = zvector_from_string (test6_parallel_stringrep3);
    zvector_t *test6_after1 = zvector_from_string (test6_after_stringrep1);
    zvector_t *test6_after2 = zvector_from_string (test6_after_stringrep2);
    zvector_t *test6_after3 = zvector_from_string (test6_after_stringrep3);
    
    assert ( zvector_compare_to (test6_self, test6_before1) == -1);
    assert ( zvector_compare_to (test6_self, test6_before2) == -1);
    assert ( zvector_compare_to (test6_self, test6_before3) == -1);
    assert ( zvector_compare_to (test6_self, test6_parallel1) == 0);
    assert ( zvector_compare_to (test6_self, test6_parallel2) == 0);
    assert ( zvector_compare_to (test6_self, test6_parallel3) == 0);
    assert ( zvector_compare_to (test6_self, test6_after1) == 1);
    assert ( zvector_compare_to (test6_self, test6_after2) == 1);
    assert ( zvector_compare_to (test6_self, test6_after3) == 1);
    assert ( zvector_compare_to (test6_self, test6_self) == 2);
    
    zstr_free (&test6_self_stringrep);
    zstr_free (&test6_before_stringrep1);
    zstr_free (&test6_before_stringrep2);
    zstr_free (&test6_before_stringrep3);
    zstr_free (&test6_parallel_stringrep1);
    zstr_free (&test6_parallel_stringrep2);
    zstr_free (&test6_parallel_stringrep3);
    zstr_free (&test6_after_stringrep1);
    zstr_free (&test6_after_stringrep2);
    zstr_free (&test6_after_stringrep3);
    zvector_destroy (&test6_self);
    zvector_destroy (&test6_before1);
    zvector_destroy (&test6_before2);
    zvector_destroy (&test6_before3);
    zvector_destroy (&test6_parallel1);
    zvector_destroy (&test6_parallel2);
    zvector_destroy (&test6_parallel3);
    zvector_destroy (&test6_after1);
    zvector_destroy (&test6_after2);
    zvector_destroy (&test6_after3);
    
    
    
```


<A name="toc3-632" title="Hints to Contributors" />
### Hints to Contributors

Zlogger is a nice, neat library, and you may not immediately appreciate why. Read the CLASS style guide please, and write your code to make it indistinguishable from the rest of the code in the library. That is the only real criteria for good style: it's invisible.

Don't include system headers in source files. The right place for these is CZMQ.

Do read your code after you write it and ask, "Can I make this simpler?" We do use a nice minimalist and yet readable style. Learn it, adopt it, use it.

Before opening a pull request read our [contribution guidelines](https://github.com/zeromq/zlogger/blob/master/CONTRIBUTING.md). Thanks!

<A name="toc3-643" title="This Document" />
### This Document

_This documentation was generated from zlogger/README.txt using [Gitdown](https://github.com/zeromq/gitdown)_
