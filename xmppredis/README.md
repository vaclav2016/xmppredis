# XMPP to REDIS bridge

Here is implementation [XMPP](http://xmpp.org/) / [REDIS](http://redis.io/) bridge in C. XmppRedis will receive/send messages between Jabber and [REDIS](http://redis.io/)-queue's (or channel?). Sorry, I am novice in C and [REDIS](http://redis.io/), so I use word from Java Enterprise world - Queue.

**NOTE:** This project is not tested for production.

What use case for this sub-project?

* For hardware/languages without support asynchronous execution (like multithreading/pthreads or something like a Java JMS) - PHP language, Arduino hardware etc.
* For hardware/languages without rich hardware/libraries - Arduino, STM32 etc. For example: libopenssl may be is too big for Arduino, but notification exchange is require strong crypto-protection. Another example: I am happy use  [Smack](http://www.igniterealtime.org/projects/smack/) library, but have too many instances of Java Runtime Environment (JRE) is not good idea for my ARM'based servers - [C.H.I.P.](http://getchip.com/) and [Raspberry PI](https://www.raspberrypi.org/).
* Building bridges between social platforms or internet protocols (jabber <-> email, skype <-> jabber etc). This bridges will be usefull for business (notificaion from your server to backoffice and customers) or for keeping your privacy (in case, when you place your bridge into [TOR](https://www.torproject.org/)).

In this cases, you just a place message to [REDIS](http://redis.io/) queue.

Also, remember - queue can have more then one subscriber, so you can play with subscribers combination.

Message format is simple text:

    jid:somebody@jabberserver.org
    message text

For inbound queue - it will be sender JID, for outbound queue - it will be receiver JID.

## How it works ? Example in PHP

It works just via [REDIS](http://redis.io/)-commands [PUBLISH](http://redis.io/commands/publish) and [SUBSCRIBE](http://redis.io/commands/subscribe):

This example PHP-script is try to send message to JID somebody@a35a17e05e4z6vxdl.onion:

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

1. PHP push message to queue `out_testbot` in [REDIS](http://redis.io/)
2. XmppRedis, as `out_testbot`-queue subscriber, pickup message from queue
3. XmppRedis parse and send message to `somebody@a35a17e05e4z6vxdl.onion`

So, if you want receive jabber-messages via xmppredis - you must execute `SUBSCRIBE in_testbot` in your application.

## XmppRedis run and configuration

You can run it with:

    $ xmppredis testbot xmppredis.conf

Where `testbot` - is a section into .conf file with bot profile.
Lets check `xmppredis.conf`:

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

we mean section `[testbot]` with bot configuration.

String `redis=localhostRedis` has the same reference - it is mean to take [REDIS](http://redis.io/) host/port from section `[localhostRedis]`.

So, multiple bot can have reference to the same [REDIS](http://redis.io/)-server.

## Donate

I maintain this project in my free time, if it helped you please support my work via Bitcoins, thanks a lot!

Bitcoin wallet is `1QAfNs5Utygt2XQoV3YCykzHs63S3AfEJ3`

## Dependencies

    hiredis
    libstrophe >= 0.9.0
    ini

With debian-based linux, you can use:

    $ apt-get install redis-server redis-tools libhiredis-dev libstrophe-dev

Also, may be you want install a PHP-extension:

    $ apt-get install php5-redis

Download and install lib ini:

    $ git clone https://github.com/vaclav2016/ini/
    $ cd ini
    $ cmake ./
    $ make
    $ sudo make install
    $ cd ..

Now, you can download and build xmppredis:

    $ git clone https://github.com/vaclav2016/xmppredis
    $ cd xmppredis/xmppredis
    $ cmake ./
    $ make
    $ sudo make install
    $ cd ..

## Contacts

Please, fill free contact to me via jabber: vaclav2016@jabber.cz

## Licensing

(с) 2016 Copyright by vaclav2016, <https://github.com/vaclav2016/xmppredis/>

is licensed under the Boost License, Version 1.0.

<http://www.boost.org/LICENSE_1_0.txt>

## References

1. Redis - <http://redis.io/>
2. LibStrophe - <https://github.com/strophe/libstrophe>
3. Hiredis - <https://github.com/redis/hiredis>
4. Ini library - <https://github.com/vaclav2016/ini>
