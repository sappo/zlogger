
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
*  <a href="#toc4-155"> - Implements the echo algorithms</a>

**<a href="#toc3-301">Hints to Contributors</a>**

**<a href="#toc3-312">This Document</a>**

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

Test by running the `zyre_selftest` command:
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

This is the API provided by Zlogger 2.x, in alphabetical order.

<A name="toc4-155" title=" - Implements the echo algorithms" />
####  - Implements the echo algorithms

zecho - Implements the echo algorithms

Please add @discuss section in ../src/zecho.c.

This is the class interface:

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
    ZLOG_EXPORT void
        zecho_recv (zecho_t *self, zyre_event_t *token);
    
    //  Self test of this class
    ZLOG_EXPORT void
        zecho_test (bool verbose);
    

This is the class self test code:

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
    
    //  Join topology
    zyre_join (node1, "GLOBAL");
    zyre_join (node2, "GLOBAL");
    zyre_join (node2, "LOCAL");
    zyre_join (node3, "LOCAL");
    
    //  Give time for them to interconnect
    zclock_sleep (500);
    
    zyre_dump (node1);
    zclock_sleep (150);
    zyre_dump (node2);
    zclock_sleep (150);
    zyre_dump (node3);
    zclock_sleep (150);
    
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
    zecho_recv (echo2, event);
    
    do {
        event = zyre_event_new (node3);
        if (!streq (zyre_event_type (event), "WHISPER"))
            zyre_event_destroy (&event);
        else
            break;
    } while (1);
    zecho_recv (echo3, event);
    
    do {
        event = zyre_event_new (node2);
        if (!streq (zyre_event_type (event), "WHISPER"))
            zyre_event_destroy (&event);
        else
            break;
    } while (1);
    zecho_recv (echo2, event);
    
    do {
        event = zyre_event_new (node1);
        if (!streq (zyre_event_type (event), "WHISPER"))
            zyre_event_destroy (&event);
        else
            break;
    } while (1);
    zecho_recv (echo1, event);
    
    // Print result
    zecho_print (echo1);
    zecho_print (echo2);
    zecho_print (echo3);
    
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
    


<A name="toc3-301" title="Hints to Contributors" />
### Hints to Contributors

Zlogger is a nice, neat library, and you may not immediately appreciate why. Read the CLASS style guide please, and write your code to make it indistinguishable from the rest of the code in the library. That is the only real criteria for good style: it's invisible.

Don't include system headers in source files. The right place for these is CZMQ.

Do read your code after you write it and ask, "Can I make this simpler?" We do use a nice minimalist and yet readable style. Learn it, adopt it, use it.

Before opening a pull request read our [contribution guidelines](https://github.com/zeromq/zlogger/blob/master/CONTRIBUTING.md). Thanks!

<A name="toc3-312" title="This Document" />
### This Document

_This documentation was generated from zlogger/README.txt using [Gitdown](https://github.com/zeromq/gitdown)_
