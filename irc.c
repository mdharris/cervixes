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
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include "cervixes.h"

struct _Capab
{
	uint32_t i;
	char name[32];
};
typedef struct _Capab IRCCap;

IRCCap *_cap_first;

int init_caps()
{
	/*
         * capsstr[CAP_CAP] = "";
         * capsstr[CAP_QS] = "QS";
         * capsstr[CAP_EX] = "EX";
         * capsstr[CAP_CHW] = "CHW";
         * capsstr[CAP_IE] = "IE";
         * capsstr[CAP_EOB] = "EOB";
         * capsstr[CAP_KLN] = "KLN";
         * capsstr[CAP_GLN] = "GLN";
         * capsstr[CAP_TS6] = "TS6";
         * capsstr[CAP_ZIP] = "ZIP";
         * capsstr[CAP_ENC] = "ENC";
         * capsstr[CAP_KNOCK] = "KNOCK";
         * capsstr[CAP_TB] = "TB";
         * capsstr[CAP_UNKLN] = "UNKLN";
         * capsstr[CAP_HOPS] = "HOPS";
         * capsstr[CAP_CLUSTER] = "CLUSTER";
         * capsstr[CAP_ENCAP] = "ENCAP";
         * capsstr[CAP_TBURST] = "TBURST";
	 */
	_cap_first = (IRCCap *)NULL;

	return(0);
}

int cap_add(uint32_t idx, char *txt)
{
	IRCCap *new;
	IRCCap *cptr;

	if (!txt)
	{
		return(-1);
	}

	if (cap_findidx(idx) || cap_findtxt(txt))
	{
	}

	if (!_cap_first)
	{
	}
}

int user_add()
{

}

int user_rem()
{

}

int server_add()
{

}

int server_rem()
{

}
