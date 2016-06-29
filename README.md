
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

**<a href="#toc3-587">Hints to Contributors</a>**

**<a href="#toc3-598">This Document</a>**

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
    
    //  Self test of this class
    ZLOG_EXPORT void
        zvector_test (bool verbose);
    
```

This is the class self test code:

```c
    //  Simple create/destroy test
    zvector_t *test1_self = zvector_new ("1231");
    assert (test1_self);
    zhashx_t *test1_sender_clock = zhashx_new ();
    zhashx_set_destructor (test1_sender_clock, s_destroy_clock_value);
    
    zhashx_destroy (&test1_sender_clock);
    zvector_destroy (&test1_self);
    
    
    //  Simple event test
    zvector_t *test2_self = zvector_new ("1231");
    assert (test2_self);
    zhashx_t *test2_sender_clock1 = zhashx_new ();
    zhashx_set_destructor (test2_sender_clock1, s_destroy_clock_value);
    
    unsigned long *test2_value1 = (unsigned long *) zmalloc (sizeof (unsigned long));
    *test2_value1 = 5;
    zhashx_insert (test2_sender_clock1, "1232", test2_value1);
    
    unsigned long *test2_self_own_clock_value = (unsigned long *) zhashx_lookup (test2_self->clock, "1231");
    assert (*test2_self_own_clock_value == 0);
    unsigned long *test2_self_sender_clock_value = (unsigned long *) zhashx_lookup (test2_sender_clock1, "1232");
    assert (*test2_self_sender_clock_value == 5);
    
    zvector_event (test2_self);
    test2_self_own_clock_value = (unsigned long *) zhashx_lookup (test2_self->clock, "1231");
    assert (*test2_self_own_clock_value == 1);
    test2_self_sender_clock_value = (unsigned long *) zhashx_lookup (test2_sender_clock1, "1232");
    assert (*test2_self_sender_clock_value == 5);
    
    zhashx_destroy (&test2_sender_clock1);
    zvector_destroy (&test2_self);
    
    
    //  Simple recv test
    zvector_t *test3_self_clock = zvector_new ("1231");
    assert (test3_self_clock);
    zhashx_t *test3_sender_clock1 = zhashx_new ();
    zhashx_set_destructor (test3_sender_clock1, s_destroy_clock_value);
    zhashx_t *test3_sender_clock2 = zhashx_new ();
    zhashx_set_destructor (test3_sender_clock2, s_destroy_clock_value);
    
    // insert some key-value pairs in test3_sender_clock1
    unsigned long *test3_inserted_value1 = (unsigned long *) zmalloc (sizeof (unsigned long));
    *test3_inserted_value1 = 5;
    zhashx_insert (test3_sender_clock1, "1231", test3_inserted_value1);
    unsigned long *test3_inserted_value2 = (unsigned long *) zmalloc (sizeof (unsigned long));
    *test3_inserted_value2 = 10;
    zhashx_insert (test3_sender_clock1, "1232", test3_inserted_value2);
    
    // insert some key-value pairs in test3_sender_clock2
    unsigned long *test3_inserted_value3 = (unsigned long *) zmalloc (sizeof (unsigned long));
    *test3_inserted_value3 = 20;
    zhashx_insert (test3_sender_clock2, "1231", test3_inserted_value3);
    unsigned long *test3_inserted_value4 = (unsigned long *) zmalloc (sizeof (unsigned long));
    *test3_inserted_value4 = 30;
    zhashx_insert (test3_sender_clock2, "1233", test3_inserted_value4);
    
    // receive sender clock 1 and add key-value pairs to own clock
    zmsg_t *test3_msg1 = zmsg_new ();
    zframe_t *test3_packed_clock1 = zhashx_pack_own (test3_sender_clock1, s_serialize_clock_value);
    zmsg_prepend (test3_msg1, &test3_packed_clock1);
    zvector_recv (test3_self_clock, test3_msg1);
    zmsg_destroy (&test3_msg1);
    unsigned long *test3_found_value1 = (unsigned long *) zhashx_lookup (test3_self_clock->clock, "1231");
    assert (*test3_found_value1 == 5);
    unsigned long *test3_found_value2 = (unsigned long *) zhashx_lookup (test3_self_clock->clock, "1232");
    assert (*test3_found_value2 == 10);
    
    // receive sender clock 2 and add key-value pairs to own clock
    zmsg_t *test3_msg2 = zmsg_new ();
    zframe_t *test3_packed_clock2 = zhashx_pack_own (test3_sender_clock2, s_serialize_clock_value);
    zmsg_prepend (test3_msg2, &test3_packed_clock2);
    zvector_recv (test3_self_clock, test3_msg2);
    zmsg_destroy (&test3_msg2);
    unsigned long *test3_found_value3 = (unsigned long *) zhashx_lookup (test3_self_clock->clock, "1231");
    assert (*test3_found_value3 == 20);
    test3_found_value2 = (unsigned long *) zhashx_lookup (test3_self_clock->clock, "1232");
    assert (*test3_found_value2 == 10);
    unsigned long *test3_found_value4 = (unsigned long *) zhashx_lookup (test3_self_clock->clock, "1233");
    assert (*test3_found_value4 == 30);
    
    zhashx_destroy (&test3_sender_clock1);
    zhashx_destroy (&test3_sender_clock2);
    zvector_destroy (&test3_self_clock);
    
    
    // Simple send_prepare test
    zvector_t *test4_self = zvector_new ("1231");
    assert (test4_self);
    
    zvector_event (test4_self);
    
    zmsg_t *test4_zmsg = zmsg_new ();
    zmsg_pushstr (test4_zmsg, "test");
    zvector_send_prepare (test4_self, test4_zmsg);
    
    zframe_t *test4_popped_frame = zmsg_pop (test4_zmsg);
    zhashx_t *test4_unpacked_clock = zhashx_unpack_own (test4_popped_frame, s_deserialize_clock_value);
    unsigned long *test4_found_value1 = (unsigned long *) zhashx_lookup (test4_unpacked_clock, "1231");
    assert (*test4_found_value1 == 2);
    char *test4_unpacked_string = zmsg_popstr (test4_zmsg);
    assert (streq (test4_unpacked_string, "test"));
    
    zframe_destroy (&test4_popped_frame);
    zstr_free (&test4_unpacked_string);
    zhashx_destroy (&test4_unpacked_clock);
    zmsg_destroy (&test4_zmsg);
    zvector_destroy (&test4_self);
    
    
    // Simple test for converting a zvector to stringrepresentation
    // and from stringrepresentation to a zvector
    zvector_t *test5_self = zvector_new ("1000");
    assert (test5_self);
    
    // inserting some clocks & values
    zvector_event (test5_self);
    unsigned long *test5_inserted_value1 = (unsigned long *) zmalloc (sizeof (unsigned long));
    *test5_inserted_value1 = 7;
    zhashx_insert (test5_self->clock, "1222", test5_inserted_value1);
    unsigned long *test5_inserted_value2 = (unsigned long *) zmalloc (sizeof (unsigned long));
    *test5_inserted_value2 = 11;
    zhashx_insert (test5_self->clock, "1444", test5_inserted_value2);
    
    char *test5_string = zvector_to_string (test5_self);
    assert (streq (test5_string, "VC:3;1000,1;1222,7;1444,11;"));
    
    zvector_t *test5_generated = zvector_from_string (test5_string);
    unsigned long *test5_found_value1 = (unsigned long *) zhashx_lookup (test5_generated->clock, "1000");
    assert (*test5_found_value1 == 1);
    test5_found_value1 = (unsigned long *) zhashx_lookup (test5_generated->clock, "1222");
    assert (*test5_found_value1 == 7);
    test5_found_value1 = (unsigned long *) zhashx_lookup (test5_generated->clock, "1444");
    assert (*test5_found_value1 == 11);
    
    zstr_free (&test5_string);
    zvector_destroy (&test5_self);
    zvector_destroy (&test5_generated);
    
```


<A name="toc3-587" title="Hints to Contributors" />
### Hints to Contributors

Zlogger is a nice, neat library, and you may not immediately appreciate why. Read the CLASS style guide please, and write your code to make it indistinguishable from the rest of the code in the library. That is the only real criteria for good style: it's invisible.

Don't include system headers in source files. The right place for these is CZMQ.

Do read your code after you write it and ask, "Can I make this simpler?" We do use a nice minimalist and yet readable style. Learn it, adopt it, use it.

Before opening a pull request read our [contribution guidelines](https://github.com/zeromq/zlogger/blob/master/CONTRIBUTING.md). Thanks!

<A name="toc3-598" title="This Document" />
### This Document

_This documentation was generated from zlogger/README.txt using [Gitdown](https://github.com/zeromq/gitdown)_
