# XMPP to REDIS bridge

Here is implementation XMPP/REDIS bridge in C.
xmppredis will receive/send messages between xmpp and redis-queue's.

Message format is text:

    jid:somebody@jabberserver.org
    message text

## Configuration

See xmppredis.sample.conf file. One configuration file could contain multiple bot's account.

## Run

    $ xmppredis testbot xmppredis.conf

Where testbot - is a section into .conf file with bot profile.

## Dependecies

    hiredis
    libstrophe >=8.5

With debian-based linux, you can use:

    $ apt-get install redis-server redis-tools libhiredis-dev libstrophe-dev

## Build

You need cmake and make to build.

    cmake
    make

## Licensing

Code is licensed with different License:

xmppres.c is licensed under the Boost License, Version 1.0. See each
repository's licensing information for details and exceptions.

http://www.boost.org/LICENSE_1_0.txt

ini.c / ini.h is licensed under the MIT License
