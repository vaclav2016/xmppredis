# XMPP to REDIS bridge

Here is implementation XMPP/REDIS bridge in C. xmppredis will receive/send messages between Jabber and REDIS-queue's.

What use case for this project?
For example, this project need if You want send messages from php-page to somebody via Jabber. In this case, You just a place message to REDIS queue.

Another use case - building bridges between social platforms (and email etc).

So, lets meet more people, be social ! ;)

Message format is simple text:

    jid:somebody@jabberserver.org
    message text

For inbound queue - it will be sender jid, for outbound queue - it will be receiver jid.

## Configuration

See xmppredis.sample.conf file. One configuration file could contain multiple bot's account.

## Run

    $ xmppredis testbot xmppredis.conf

Where testbot - is a section into .conf file with bot profile.

## How it works ? Example in PHP

It works just via REDIS-commands PUBLISH and SUBSCRIBE:
http://redis.io/commands/publish

This example PHP-script is try to send message to jid somebody@a35a17e05e4z6vxdl.onion:

    <?php

    $jid = "somebody@a35a17e05e4z6vxdl.onion";
    $msg = "Hello, world!";

    $outQueue = "out_testbot";

    $redis = new Redis();
    $redis->pconnect('127.0.0.1', 6379);
    $redis->publish($outQueue, "jid:".$jid."\n".$msg);

    echo "Done!";

    ?>

## Dependecies

    hiredis
    libstrophe >=8.5

With debian-based linux, you can use:

    $ apt-get install redis-server redis-tools libhiredis-dev libstrophe-dev

Also, may be You want install a PHP-extension:

    $ apt-get install php5-redis

## Build

You need cmake and make to build.

    $ cmake ./
    $ make

## Licensing

Code is licensed with different License:

xmppres.c is licensed under the Boost License, Version 1.0. See each
repository's licensing information for details and exceptions.

http://www.boost.org/LICENSE_1_0.txt

ini.c / ini.h is licensed under the MIT License

## References

1. Redis - http://redis.io/
2. LibStrophe - https://github.com/strophe/libstrophe
2. Hiredis - https://github.com/redis/hiredis
