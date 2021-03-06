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
#include <ctype.h>
#include <pwstor.h>
#include "inilib/inilib.h"
#include "cervixes.h"
#include <mysql/mysql.h>

SVCNick *_nick_first;
SVCMemo *_memo_first;
MYSQL *dbhandle;

int init_database()
{
	my_bool reconnect;

	_nick_first = (SVCNick *)NULL;
	_memo_first = (SVCMemo *)NULL;

	dbhandle = (MYSQL *)NULL;
	mysql_library_init(0, (char **)NULL, (char **)NULL);
	dbhandle = mysql_init((MYSQL *)NULL);

	mysql_options(dbhandle, MYSQL_READ_DEFAULT_GROUP, "client");
	reconnect = 1;
	mysql_options(dbhandle, MYSQL_OPT_RECONNECT, &reconnect);
	mysql_ssl_set(dbhandle, (char *)NULL, (char *)NULL, "ca.crt", (char *)NULL, "DHE-DSS-AES128-SHA");
	errno = 0;
	if (!mysql_real_connect(DB, NULL, NULL, NULL, "information_schema", 0, NULL, CLIENT_REMEMBER_OPTIONS|CLIENT_COMPRESS))
	{
		fprintf(stdout, "mysql_real_connect(information_schema): %s\n", mysql_error(DB));
		mysql_close(dbhandle);
		return(-1);
	}
}

int deinit_database()
{
	if (!dbhandle) { return(-1); }
	mysql_close(dbhandle);
	mysql_library_end();
	return(0);
}

int database_wr()
{

}
