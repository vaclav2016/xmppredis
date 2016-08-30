# XMPP to REDIS bridge

Here is implementation XMPP/REDIS bridge in C. xmppredis will receive/send messages between Jabber and REDIS-queue's (or channel?).

NOTE: This project is not tested for production.

What use case for this project?
For example, this project need if You want send messages from php-page to somebody via Jabber. In this case, You just a place message to REDIS queue.

Another use case - building bridges between social platforms (email etc).

Message format is simple text:

    jid:somebody@jabberserver.org
    message text

For inbound queue - it will be sender jid, for outbound queue - it will be receiver jid.

## How it works ? Example in PHP

It works just via REDIS-commands PUBLISH and SUBSCRIBE:
http://redis.io/commands/publish

This example PHP-script is try to send message to jid somebody@a35a17e05e4z6vxdl.onion:

    <?php

    $jid = "somebody@a35a17e05e4z6vxdl.onion";
    $msg = "Hello, world!";
    $outQueue = "out_testbot";

    $redis = new Redis();
    $redis->pconnect( "127.0.0.1", 6379 );
    $redis->publish( $outQueue, "jid:" . $jid . "\n" . $msg );

    echo "Done!";

    ?>

How act this example?

1. PHP push message to queue 'out_testbot'
2. xmppredis, as 'out_testbot' queue subscriber, pickup message from queue
3. xmppredis parse and send message to somebody@a35a17e05e4z6vxdl.onion

So, if You want receive jabber-messages via xmppredis - You must execute 'SUBSCRIBE in_testbot' in your application


## Run and Configuration

You can run it with:

    $ xmppredis testbot xmppredis.conf

Where testbot - is a section into .conf file with bot profile.
Lets check xmppredis.conf:

    [localhostRedis]

    host=127.0.0.1
    port=6379

    [testbot]

    jid=testbot@not-jabber.org
    password=secret

    redis=localhostRedis

    inbound=in_testbot
    outbound=out_testbot

So, when we run 

    $ xmppredis testbot xmppredis.conf

we mean section [testbot] with bot configuration.

String 'redis=localhostRedis' has the same reference - it is mean to take redis host/port from section [localhostRedis].

So, multiple bot can have reference to the same REDIS server.

## Dependencies and Build

    hiredis
    libstrophe >=8.5

With debian-based linux, you can use:

    $ apt-get install redis-server redis-tools libhiredis-dev libstrophe-dev

Also, may be You want install a PHP-extension:

    $ apt-get install php5-redis

### Build

You need cmake and make to build.

    $ cmake ./
    $ make

## Licensing

Code is licensed with different License:

xmppres.c is licensed under the Boost License, Version 1.0. See each
repository's licensing information for details and exceptions.

http://www.boost.org/LICENSE_1_0.txt

ini library (c) Haipo Yang, is licensed under the MIT License, original repository with sources is https://github.com/haipome/ini

## References

1. Redis - http://redis.io/
2. LibStrophe - https://github.com/strophe/libstrophe
3. Hiredis - https://github.com/redis/hiredis
4. Ini library - https://github.com/haipome/ini
