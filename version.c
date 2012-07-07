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
#include <stdin.h>
#include <string.h>
#include "cervixes.h"

#define CERVIXES_VERSION	"0.1"

char *cervixes_version()
{
	char *v;
	size_t len;

	len = 10;	/* "cervixes-" and \0 */
	len += strlen(CERVIXES_VERSION);

	v = (char *)malloc(len);
	memset(v, 0, len);
	snprintf(v, len, "cervixes-%s", CERVIXES_VERSION);

	return(v);
}

/*
#define CAP_CAP         0x00001
#define CAP_QS          0x00002
#define CAP_EX          0x00004
#define CAP_CHW         0x00008
#define CAP_IE          0x00010
#define CAP_EOB         0x00020
#define CAP_KLN         0x00040
#define CAP_GLN         0x00080
#define CAP_TS6         0x00100
#define CAP_ZIP         0x00200
#define CAP_ENC         0x00400
#define CAP_KNOCK       0x00800
#define CAP_TB          0x01000
#define CAP_UNKLN       0x02000
#define CAP_HOPS        0x04000
#define CAP_CLUSTER     0x08000
#define CAP_ENCAP       0x10000
#define CAP_TBURST      0x20000
*/

char *cervixes_capstr(uint32_t caps)
{
	char *c;
	size_t len;

	len = 1;

	c = (char *)malloc(len);
	memset(c, 0, len);
	snprintf(c, len, "");

	return(c);
}
