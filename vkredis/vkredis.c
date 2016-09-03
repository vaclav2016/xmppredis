/*
(с) 2016 Copyright by vaclav2016, https://github.com/vaclav2016/xmppredis/

Boost Software License - Version 1.0 - August 17th, 2003

Permission is hereby granted, free of charge, to any person or organization
obtaining a copy of the software and accompanying documentation covered by
this license (the "Software") to use, reproduce, display, distribute,
execute, and transmit the Software, and to prepare derivative works of the
Software, and to permit third-parties to whom the Software is furnished to
do so, all subject to the following:

The copyright notices in the Software and this entire statement, including
the above license grant, this restriction and the following disclaimer,
must be included in all copies of the Software, in whole or in part, and
all derivative works of the Software, unless such copies or derivative
works are solely in the form of machine-executable object code generated by
a source language processor.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <error.h>
#include <errno.h>
#include <pthread.h>
#include "curl-client.h"
#include <hiredis/hiredis.h>
#include "ini.h"

int main(int argc, char **argv) {
	printf("\033[34m\r");
	printf("oooooo     oooo oooo        ooooooooo.                   .o8   o8o           \n");
	printf(" `888.     .8'  `888        `888   `Y88.                \"888   `\"'           \n");
	printf("  `888.   .8'    888  oooo   888   .d88'  .ooooo.   .oooo888  oooo   .oooo.o \n");
	printf("   `888. .8'     888 .8P'    888ooo88P'  d88' `88b d88' `888  `888  d88(  \"8 \n");
	printf("    `888.8'      888888.     888`88b.    888ooo888 888   888   888  `\"Y88b.  \n");
	printf("     `888'       888 `88b.   888  `88b.  888    .o 888   888   888  o.  )88b \n");
	printf("      `8'       o888o o888o o888o  o888o `Y8bod8P' `Y8bod88P\" o888o 8""888P' ");
	printf("\033[37m\n");
	printf("(с) 2016 Copyright by \033[36mvaclav2016\033[37m, https://github.com/vaclav2016/xmppredis/\n");
	printf("\033[31mBoost License, Version 1.0, http://www.boost.org/LICENSE_1_0.txt\033[37m\n");

//	char *page = url_get("http://mirror.amsiohosting.net/releases.ubuntu.com/16.04.1/ubuntu-16.04.1-desktop-amd64.iso");
	char *page = url_get("https://google.com/");
//	printf(page);
	free(page);
}
