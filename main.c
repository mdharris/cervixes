/*
** Copyright (c) 2012, Matt Harris
** All rights reserved.
** 
** Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
** 
**    Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
**    Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other 
**    materials provided with the distribution.
** 
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF 
** MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
** INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF 
** THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
** 
*/

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <pwstor.h>
#include <signal.h>
#include "inilib/inilib.h"
#include <msocket.h>
#include "cervixes.h"

extern char *optarg;

uint16_t *status;

static int8_t init_caps(void);

int main(int argc, char **argv)
{
	time_t last;

	status = 0;
	signal(SIGTSTP, SIG_IGN);
	signal(SIGHUP, sh_reboot);
	signal(SIGINT, sh_die);
	signal(SIGKILL, sh_die);
	signal(SIGTERM, sh_die);

	init_caps();
	init_to();
	if (lms_init((config->debugmode > 0) ? 1 : 0) != 0)
	{
		return(1);
	}
	init_database();
	status |= STATUS_INITIALIZED;

	while (!(status & STATUS_QUITTING))
	{
		if (time(NULL) > last)
		{
			to_check();
			last = time(NULL);
		}
		lms_
		/* milisecond level granularity should be sane. */
		usleep(1000);
	}

	return(0);
}
