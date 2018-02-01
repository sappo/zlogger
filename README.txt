.set GIT=https://zenon.cs.hs-rm.de/causality-logger/zlogger
.sub 0MQ=ØMQ

# Zlogger

## Contents

.toc 3

## Overview

### Scope and Goals

### Ownership and License

The contributors are listed in AUTHORS. This project uses the MPL v2 license, see LICENSE.

Zlogger uses the [C4.1 (Collective Code Construction Contract)](http://rfc.zeromq.org/spec:22) process for contributions.

Zlogger uses the [CLASS (C Language Style for Scalabilty)](http://rfc.zeromq.org/spec:21) guide for code style.

To report an issue, use the [Zlogger issue tracker](https://zenon.cs.hs-rm.de/causality-logger/zlogger/issues) at
gitlab.com.

## Using Zlogger

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

### Linking with an Application

Include `zlogger.h` in your application and link with libzlogger. Here is a typical gcc link command:

    gcc -lzlogger -lzyre -lczmq -lzmq myapp.c -o myapp

### Election Protocol

* Peers can be either in *ELECTION* or *PASSIVE* mode.
* When a peer joins the network it enter *PASSIVE* mode and it shall be given
  some time to discover other peers.
* After the initial discovery phase a peer will enter *ELECTION* mode and start
  a challenge to become leader by sending a **ELECTION** message thereby setting
  its own ID a potential leader ID.
* If a peer in *PASSIVE* mode receives a **ELECTION** message it switches to
  *ELECTION* mode and the challenger's ID becomes the potentials leader ID.
  Further it will forward the **ELECTION** message to all known peers but the
  one it received the message from.
* If a peer in *ELECTION* mode receives a **ELECTION** message and the
  challenger's ID is lower than the potential leaders the challenger will be
  become the potential leader. Further it will forward the **ELECTION** message
  to all known peers but the one it received the message from.
* If a peer in *ELECTION* mode receives a **ELECTION** message and the
  challengers ID is higher than the potential leaders one it discard the
  message.
* If a peer received **ELECTION** messages from all known peers. The peer will
  send a **LEADER** message to all known peers with the ID of the leader.
* If a peer receives a **LEADER** message with the leader's ID the peer will set
  this ID as its leader ID and forwards the **LEADER** message to all known
  peers.
* If a peer received **LEADER** message from all known peers the election is
  finished and it switches to *PASSIVE* mode. In case the leaders ID equals his
  own ID this peer promote itsself to leader, otherwise it will become
  a follower.

### Demo

The demo scripts are available in demo/ folder. Please refer to demo/README.md.

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
script. This will generate the SVG ./dia_space_time.svg. Make sure you have
graphviz installed on your system i.e.

    sudo apt-get install graphviz

### API Summary

This is the API provided by Zlogger 0.x, in alphabetical order.

.pull doc/zlog.doc
.pull doc/zecho.doc
.pull doc/zvector.doc
.pull doc/bakery.doc

### Hints to Contributors

Zlogger is a nice, neat library, and you may not immediately appreciate why. Read the CLASS style guide please, and write your code to make it indistinguishable from the rest of the code in the library. That is the only real criteria for good style: it's invisible.

Don't include system headers in source files. The right place for these is CZMQ.

Do read your code after you write it and ask, "Can I make this simpler?" We do use a nice minimalist and yet readable style. Learn it, adopt it, use it.

Before opening a pull request read our [contribution guidelines](https://zenon.cs.hs-rm.de/causality-logger/zlogger/blob/master/README.md).
Thanks!

### This Document
