
<A name="toc1-3" title="Zlogger" />
# Zlogger

<A name="toc2-6" title="Contents" />
## Contents


**<a href="#toc2-11">Overview</a>**

**<a href="#toc3-14">Scope and Goals</a>**

**<a href="#toc3-17">Ownership and License</a>**

**<a href="#toc2-29">Using Zlogger</a>**

**<a href="#toc3-32">Building on Linux</a>**

**<a href="#toc3-104">Building on Windows</a>**

**<a href="#toc3-139">Linking with an Application</a>**

**<a href="#toc3-146">Demo</a>**

**<a href="#toc3-151">Bakery</a>**

**<a href="#toc3-177">API Summary</a>**
*  <a href="#toc4-182">zlog - zlog actor</a>
*  <a href="#toc4-287">zecho - Implements the echo algorithms</a>
*  <a href="#toc4-512">zvector - Implements a dynamic vector clock</a>
*  <a href="#toc4-724">bakery - Bakery with zlogger support</a>

**<a href="#toc3-740">Hints to Contributors</a>**

**<a href="#toc3-752">This Document</a>**

<A name="toc2-11" title="Overview" />
## Overview

<A name="toc3-14" title="Scope and Goals" />
### Scope and Goals

<A name="toc3-17" title="Ownership and License" />
### Ownership and License

The contributors are listed in AUTHORS. This project uses the MPL v2 license, see LICENSE.

Zlogger uses the [C4.1 (Collective Code Construction Contract)](http://rfc.zeromq.org/spec:22) process for contributions.

Zlogger uses the [CLASS (C Language Style for Scalabilty)](http://rfc.zeromq.org/spec:21) guide for code style.

To report an issue, use the [Zlogger issue tracker](https://zenon.cs.hs-rm.de/causality-logger/zlogger/issues) at
gitlab.com.

<A name="toc2-29" title="Using Zlogger" />
## Using Zlogger

<A name="toc3-32" title="Building on Linux" />
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

Here's how to build Zlogger from GitHub/Gitlab (building from packages is very
similar, you don't clone a repo but unpack a tarball), including the libsodium
(for security) and libzmq (ZeroMQ core) libraries:

```
    git clone git://github.com/zeromq/libzmq.git
    cd libzmq
    ./autogen.sh
    # do not specify "--with-libsodium" if you prefer to use internal tweetnacl
    # security implementation (recommended for development)
    ./configure
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

    git clone https://zenon.cs.hs-rm.de/causality-logger/zlogger.git
    cd zlogger
    ./autogen.sh && ./configure && make check
    sudo make install
    sudo ldconfig
    cd ..

```

<A name="toc3-104" title="Building on Windows" />
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
    git clone https://zenon.cs.hs-rm.de/causality-logger/zlogger.git
    cd zlogger\builds\msvc configure.bat
    cd build buildall.bat
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

<A name="toc3-139" title="Linking with an Application" />
### Linking with an Application

Include `zlogger.h` in your application and link with libzlogger. Here is a typical gcc link command:

    gcc -lzlogger -lzyre -lczmq -lzmq myapp.c -o myapp

<A name="toc3-146" title="Demo" />
### Demo

The demo scripts are available in demo/ folder. Please refer to demo/README.md.

<A name="toc3-151" title="Bakery" />
### Bakery

At first you need to copy the rsyslog configuration file 1337-logger.conf into
the directory /etc/rsyslog.d/ and restart your rsyslog daemon

    cp 1337-logger.conf /etc/rsyslog.d/1337-logger.conf
    sudo service rsyslog restart

To run the bakery with 3 apprentices locally

    ./src/bakery & ./src/bakery & ./src/bakery & wait

Each bakery will have its own log file at /tmp/vc_*.log. The leader will write
the causal ordered log to ./ordered.log. If this file is empty, rsyslog did not
yet write any log entries or the collection algorithm has not yet been
triggered. To provide the bakery with more time to get all log files use the
option -w [n]s. To dump the Space-Time diagrams use -d parameter.

    ./src/bakery -d -w 15 & ./src/bakery -d -w 15 & ./src/bakery -d -w 15 & wait

The global syslog log is written to /tmp/global.log

To generate the final space time diagram use the ./generate_space_time.sh
script. This will generate the SVG ./dia_space_time.svg.

<A name="toc3-177" title="API Summary" />
### API Summary

This is the API provided by Zlogger 0.x, in alphabetical order.

<A name="toc4-182" title="zlog - zlog actor" />
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
    
    //  Compares the zvector_t's of given logMsg a to logMsg b.
    //  Returns -1 if a < b, otherwiese 1
    ZLOG_EXPORT int
        zlog_compare_log_msg_vc (const char *logMsg_a, const char *logMsg_b);
    
    //  Compares the timestamps's of given logMsg a to logMsg b.
    //  Returns -1 if a < b, otherwiese 1
    ZLOG_EXPORT int
        zlog_compare_log_msg_ts (const char *logMsg_a, const char *logMsg_b);
    
    //  Reads log of source filepath and orders it with given
    //  pointer to compare_function into destination filepath.
    
    ZLOG_EXPORT void
        zlog_order_log (const char *path_src, const char *path_dst, zlistx_comparator_fn *compare_function);
    
    //  Self test of this actor
    ZLOG_EXPORT void
        zlog_test (bool verbose);
```

This is the class self test code:

```c
    char *params1[2] = {"inproc://logger1", "GOSSIP MASTER"};
    zactor_t *zlog = zactor_new (zlog_actor, params1);
    
    char *params2[2] = {"inproc://logger2", "GOSSIP SLAVE"};
    zactor_t *zlog2 = zactor_new (zlog_actor, params2);
    
    char *params3[2] = {"inproc://logger3", "GOSSIP SLAVE"};
    zactor_t *zlog3 = zactor_new (zlog_actor, params3);
    
    /*char *params4[3] = {"inproc://logger4", "logger4", "GOSSIP SLAVE"};*/
    /*zactor_t *zlog4 = zactor_new (zlog_actor, params4);*/
    
    if (verbose) {
        zstr_send (zlog, "VERBOSE");
        zstr_send (zlog2, "VERBOSE");
        zstr_send (zlog3, "VERBOSE");
        /*zstr_send (zlog4, "VERBOSE");*/
    }
    
    zstr_send (zlog, "START");
    zstr_send (zlog2, "START");
    zstr_send (zlog3, "START");
    /*zstr_send (zlog4, "START");*/
    
    //  Give time to interconnect and elect
    zclock_sleep (750);
    
    //  Give time for log collect to happen
    zclock_sleep (12000);
    
    zstr_send (zlog, "STOP");
    zstr_send (zlog2, "STOP");
    zstr_send (zlog3, "STOP");
    /*zstr_send (zlog4, "STOP");*/
    
    //  Give time to disconnect
    zclock_sleep (250);
    
    zactor_destroy (&zlog);
    zactor_destroy (&zlog2);
    zactor_destroy (&zlog3);
    /*zactor_destroy (&zlog4);*/
    
    /*zlog_order_log ("/var/log/vc.log", "ordered_vc1.log");*/
```

<A name="toc4-287" title="zecho - Implements the echo algorithms" />
#### zecho - Implements the echo algorithms

zecho - Implements the echo algorithms which consist of two distinct waves.
        The first wave is to flood the network and build a spanning tree. It
        can be used to inform peers thus it's also called inform wave. The
        second wave is flows from the leaves of the spanning tree back to the
        initiator. It it used to collect things from peers. Collectables are
        e.g. ACKs or arbitrary data.

Please add @discuss section in ../src/zecho.c.

This is the class interface:

```h
    
    //  Process INFORM or COLLECT messages
    typedef void (zecho_process_fn) (
        zecho_t *self, zmsg_t *msg,  void *handler);
    //  Create custom INFORM or COLLECT messages content
    typedef zmsg_t * (zecho_create_fn) (
        zecho_t *self, void *handler);
    
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
    
    //  Sets a handler which is passed to custom collect functions.
    ZLOG_EXPORT void
        zecho_set_collect_handler (zecho_t *self, void *handler);
    
    //  Set a user-defined function to process collect messages from peers; This
    //  function is invoked during the second (incoming) wave.
    ZLOG_EXPORT void
        zecho_set_collect_process (zecho_t *self, zecho_process_fn *process_fn);
    
    //  Set a user-defined function to create a custom message part for the collect
    //  message; This function is invoked during the second (incoming) wave. The
    //  returned message's content is appended to the wave message.
    ZLOG_EXPORT void
        zecho_set_collect_create (zecho_t *self, zecho_create_fn *collect_fn);
    
    //  Sets a handler which is passed to custom inform functions.
    ZLOG_EXPORT void
        zecho_set_inform_handler (zecho_t *self, void *handler);
    
    //  Set a user-defined function to process inform messages from peers; This
    //  function is invoked during the first (outgoing) wave.
    ZLOG_EXPORT void
        zecho_set_inform_process (zecho_t *self, zecho_process_fn *process_fn);
    
    //  Set a user-defined function to create a custom message part for the inform
    //  message; This function is invoked during the first (outgoing) wave. The
    //  returned message's content is appended to the wave message.
    ZLOG_EXPORT void
        zecho_set_inform_create (zecho_t *self, zecho_create_fn *collect_fn);
    
    //  Set a vector clock handle. Echo messages will be prepended with the
    //  vector if not NULL.
    ZLOG_EXPORT void
        zecho_set_clock (zecho_t *self, zvector_t *clock);
    
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
    zecho_set_collect_process (echo1, s_test_zecho_process);
    zecho_set_collect_process (echo2, s_test_zecho_process);
    zecho_set_collect_process (echo3, s_test_zecho_process);
    zecho_set_collect_create (echo1, s_test_zecho_create);
    zecho_set_collect_create (echo2, s_test_zecho_create);
    zecho_set_collect_create (echo3, s_test_zecho_create);
    
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
    
    zyre_event_t *event = NULL;
    
    do {
        event = zyre_event_new (node2);
        if (!streq (zyre_event_type (event), "WHISPER"))
            zyre_event_destroy (&event);
        else
            break;
    } while (1);
    char *type = zmsg_popstr (zyre_event_msg (event));
    assert (streq (type, "ZECHO"));
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
    assert (streq (type, "ZECHO"));
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
    assert (streq (type, "ZECHO"));
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
    assert (streq (type, "ZECHO"));
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

<A name="toc4-512" title="zvector - Implements a dynamic vector clock" />
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
    
    //  Converts the zvector into string representation
    ZLOG_EXPORT char *
        zvector_to_string (zvector_t *self);
    
    //  Converts the zvector into string representation with pid_length
    ZLOG_EXPORT char *
        zvector_to_string_short (zvector_t *self, uint8_t pid_length);
    
    //  Creates a zvector from a given string representation
    ZLOG_EXPORT zvector_t *
        zvector_from_string (char *clock_string);
    
    ZLOG_EXPORT void
        zvector_dump_time_space (zvector_t *self);
    
    //  Compares zvector self to zvector other.
    //  Returns -1 at happened before self, 0 at parallel, 1 at happened after
    //  and 2 when clocks are the same
    ZLOG_EXPORT int
        zvector_compare_to (zvector_t *zv_self, zvector_t *zv_other);
    
    //  Log informational message - low priority. Prepends the current VC.
    ZLOG_EXPORT void
        zvector_info (zvector_t *self, char *format, ...);
    
    //  Duplicates the given zvector, returns a freshly allocated dulpicate.
    ZLOG_EXPORT  zvector_t *
        zvector_dup (zvector_t *self);
    
    //  Prints the zvector for debug purposes
    ZLOG_EXPORT void
        zvector_print (zvector_t *self);
    
    //  Self test of this class
    ZLOG_EXPORT void
        zvector_test (bool verbose);
    
```

This is the class self test code:

```c
    //  TEST: create/destroy zvector_t
    zvector_t *test1_self = zvector_new ("1000");
    assert (test1_self);
    zvector_destroy (&test1_self);
    
    //  TEST: for converting a zvector to stringrepresentation and from
    //        string represenstation to zvector.
    zvector_t *test2_self = zvector_new ("1000");
    assert (test2_self);
    
    //  Inserting some clocks & values
    zvector_event (test2_self);
    unsigned long *test2_inserted_value1 = (unsigned long *) zmalloc (sizeof (unsigned long));
    *test2_inserted_value1 = 7;
    zhashx_insert (test2_self->clock, "1001", test2_inserted_value1);
    unsigned long *test2_inserted_value2 = (unsigned long *) zmalloc (sizeof (unsigned long));
    *test2_inserted_value2 = 11;
    zhashx_insert (test2_self->clock, "1002", test2_inserted_value2);
    
    char *test2_string = zvector_to_string (test2_self);
    assert (streq (test2_string, "VC:3;own:1000;1001,7;1000,1;1002,11;"));
    
    zvector_t *test2_generated = zvector_from_string (test2_string);
    assert ( *(unsigned long *) zhashx_lookup (test2_generated->clock, "1000") == 1 );
    assert ( *(unsigned long *) zhashx_lookup (test2_generated->clock, "1001") == 7 );
    assert ( *(unsigned long *) zhashx_lookup (test2_generated->clock, "1002") == 11 );
    
    zstr_free (&test2_string);
    zvector_destroy (&test2_self);
    zvector_destroy (&test2_generated);
    
    //  TEST: events
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
    
    //  TEST: recv test
    zvector_t *test4_self_clock = zvector_new ("1000");
    char *test4_sender_clock1_stringRep = zsys_sprintf ("%s", "VC:2;own:1001;1000,5;1001,10;");
    char *test4_sender_clock2_stringRep = zsys_sprintf ("%s", "VC:2;own:1002;1000,20;1002,30;");
    assert (test4_self_clock);
    
    //  Receive sender clock 1 and add key-value pairs to own clock
    zmsg_t *test4_msg1 = zmsg_new ();
    zmsg_pushstr (test4_msg1, test4_sender_clock1_stringRep);
    zvector_recv (test4_self_clock, test4_msg1);
    zmsg_destroy (&test4_msg1);
    assert ( *(unsigned long *) zhashx_lookup (test4_self_clock->clock, "1000") == 5 );
    assert ( *(unsigned long *) zhashx_lookup (test4_self_clock->clock, "1001") == 10 );
    
    //  Receive sender clock 2 and add key-value pairs to own clock
    test4_msg1 = zmsg_new ();
    zmsg_pushstr (test4_msg1, test4_sender_clock2_stringRep);
    zvector_recv (test4_self_clock, test4_msg1);
    zmsg_destroy (&test4_msg1);
    assert ( *(unsigned long *) zhashx_lookup (test4_self_clock->clock, "1000") == 20 );
    assert ( *(unsigned long *) zhashx_lookup (test4_self_clock->clock, "1001") == 10 );
    assert ( *(unsigned long *) zhashx_lookup (test4_self_clock->clock, "1002") == 30 );
    
    zstr_free (&test4_sender_clock1_stringRep);
    zstr_free (&test4_sender_clock2_stringRep);
    zvector_destroy (&test4_self_clock);
    
    // TEST: send prepare
    zvector_t *test5_self = zvector_new ("1000");
    assert (test5_self);
    zvector_event (test5_self);
    zmsg_t *test5_zmsg = zmsg_new ();
    zmsg_pushstr (test5_zmsg, "test");
    
    zvector_send_prepare (test5_self, test5_zmsg);
    char *test5_clock_string = zmsg_popstr (test5_zmsg);
    zvector_t *test5_unpacked_clock = zvector_from_string (test5_clock_string);
    char *test5_unpacked_string = zmsg_popstr (test5_zmsg);
    assert (streq (test5_unpacked_string, "test"));
    assert ( *(unsigned long *) zhashx_lookup (test5_unpacked_clock->clock, "1000") == 2 );
    
    zstr_free (&test5_clock_string);
    zstr_free (&test5_unpacked_string);
    zmsg_destroy (&test5_zmsg);
    zvector_destroy (&test5_self);
    zvector_destroy (&test5_unpacked_clock);
    
    // TEST: compare
    char *test6_self_stringrep = zsys_sprintf ("%s", "VC:3;own:p2;p1,2;p2,2;p3,2;");
    char *test6_before_stringrep1 = zsys_sprintf ("%s", "VC:2;own:p3;p1,1;p3,2;");
    char *test6_before_stringrep2 = zsys_sprintf ("%s", "VC:2;own:p2;p1,2;p2,1;");
    char *test6_before_stringrep3 = zsys_sprintf ("%s", "VC:1;own:p1;p1,1;");
    char *test6_parallel_stringrep1 = zsys_sprintf ("%s", "VC:1;own:p1;p1,3;");
    char *test6_parallel_stringrep2 = zsys_sprintf ("%s", "VC:2;own:p3;p1,1;p3,3;");
    char *test6_parallel_stringrep3 = zsys_sprintf ("%s", "VC:2;own:p1;p1,4;p3,3;");
    char *test6_after_stringrep1 = zsys_sprintf ("%s", "VC:3;own:p2;p1,2;p2,3;p3,2;");
    char *test6_after_stringrep2 = zsys_sprintf ("%s", "VC:3;own:p3;p1,2;p2,3;p3,4;");
    zvector_t *test6_self = zvector_from_string (test6_self_stringrep);
    zvector_t *test6_before1 = zvector_from_string (test6_before_stringrep1);
    zvector_t *test6_before2 = zvector_from_string (test6_before_stringrep2);
    zvector_t *test6_before3 = zvector_from_string (test6_before_stringrep3);
    zvector_t *test6_parallel1 = zvector_from_string (test6_parallel_stringrep1);
    zvector_t *test6_parallel2 = zvector_from_string (test6_parallel_stringrep2);
    zvector_t *test6_parallel3 = zvector_from_string (test6_parallel_stringrep3);
    zvector_t *test6_after1 = zvector_from_string (test6_after_stringrep1);
    zvector_t *test6_after2 = zvector_from_string (test6_after_stringrep2);
    
    assert (zvector_compare_to (test6_self, test6_before1) == 1);
    assert (zvector_compare_to (test6_self, test6_before2) == 1);
    assert (zvector_compare_to (test6_self, test6_before3) == 1);
    assert (zvector_compare_to (test6_self, test6_parallel1) == 0);
    assert (zvector_compare_to (test6_self, test6_parallel2) == 0);
    assert (zvector_compare_to (test6_self, test6_parallel3) == 0);
    assert (zvector_compare_to (test6_self, test6_after1) == -1);
    assert (zvector_compare_to (test6_self, test6_after2) == -1);
    
    zstr_free (&test6_self_stringrep);
    zstr_free (&test6_before_stringrep1);
    zstr_free (&test6_before_stringrep2);
    zstr_free (&test6_before_stringrep3);
    zstr_free (&test6_parallel_stringrep1);
    zstr_free (&test6_parallel_stringrep2);
    zstr_free (&test6_parallel_stringrep3);
    zstr_free (&test6_after_stringrep1);
    zstr_free (&test6_after_stringrep2);
    
    zvector_destroy (&test6_self);
    zvector_destroy (&test6_before1);
    zvector_destroy (&test6_before2);
    zvector_destroy (&test6_before3);
    zvector_destroy (&test6_parallel1);
    zvector_destroy (&test6_parallel2);
    zvector_destroy (&test6_parallel3);
    zvector_destroy (&test6_after1);
    zvector_destroy (&test6_after2);
    
    
```

<A name="toc4-724" title="bakery - Bakery with zlogger support" />
#### bakery - Bakery with zlogger support

bakery - Bakery with zlogger support

Please add @discuss section in ../src/bakery.c.

This is the class interface:

Please add @interface section in ../src/bakery.c.

This is the class self test code:

Please add @selftest section in ../src/bakery.c.


<A name="toc3-740" title="Hints to Contributors" />
### Hints to Contributors

Zlogger is a nice, neat library, and you may not immediately appreciate why. Read the CLASS style guide please, and write your code to make it indistinguishable from the rest of the code in the library. That is the only real criteria for good style: it's invisible.

Don't include system headers in source files. The right place for these is CZMQ.

Do read your code after you write it and ask, "Can I make this simpler?" We do use a nice minimalist and yet readable style. Learn it, adopt it, use it.

Before opening a pull request read our [contribution guidelines](https://zenon.cs.hs-rm.de/causality-logger/zlogger/blob/master/README.md).
Thanks!

<A name="toc3-752" title="This Document" />
### This Document

_This documentation was generated from zlogger/README.txt using [Gitdown](https://github.com/zeromq/gitdown)_
