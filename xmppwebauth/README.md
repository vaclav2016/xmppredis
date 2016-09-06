# XmppWebAuth

This subproject demostrate how to have web authorization without keeping password into database.

* [php/login.php](php/login.php) - sample login page
* [php/page.php](php/page.php) - protected page, which show different content for anonymous/authrized users
* [php/logout.php](php/logout.php) - logout page

**Require runned XmppRedis !**

## Authorization scheme for web site without keeping password into database

This project allow build authrization scheme for your site without keeping password into DB:

* Customer of your site enter his jabber JID on web-form.

![Authorization scheme for web site without keeping password into database Step 1](step1.png)

* Script receive JID, generate random password, store pair JID/password at user-session and send password to customer's JID. When I say "user-session", I dont mean 'cookies'.

* Customer receive password and enter him on web-form.

![Authorization scheme for web site without keeping password into database Step 2](step2.png)

* Script receive password from web-form and check with session-stored password.
* In case when password is right, your customer ID (login) is entered JID.

![Authorization scheme for web site without keeping password into database Step 3](step3.png)

So, nobody can steal your passwords database.

Also, in this case, target for bruteforce attacks will be customer's jabber service instead your site.

## Donate

I maintain this project in my free time, if it helped you please support my work via Bitcoins, thanks a lot! 

Bitcoin wallet is `1QAfNs5Utygt2XQoV3YCykzHs63S3AfEJ3`

## Contacts

Please, fill free contact to me via jabber: vaclav2016@jabber.cz

## Licensing

(c) 2016 Copyright Vaclav2016 https://github.com/vaclav2016, jabber id vaclav2016@jabber.cz

is licensed under the Boost License, Version 1.0.

<http://www.boost.org/LICENSE_1_0.txt>
