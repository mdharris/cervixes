/*
 * Copyright (c) 2008
 *      Matt Harris.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      "This product includes software developed by Matt Harris."
 * 4. Neither the name of the Mr. Harris nor the names of his contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 * 5. Any modifications and/or variations in the end-product which you are 
 *    distributing from the original source code are clearly noted in
 *    the standard end-user documentation distributed with any package
 *    containing this software in either source or binary form, as well
 *    as on any internet sites or media on which this software is included.
 *
 * THIS SOFTWARE IS PROVIDED BY Mr. Harris AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL Mr. Harris OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#include <msocket-internal.h>

static time_t _lms_loop_lastrun = 0;


/*
 * lms_loop() is our main event loop
 *
 */
int lms_loop()
{
	if (_lms_loop_lastrun < time(NULL))
	{
		lms_socket_housekeeping();
		lms_dns_cleancache();
#ifdef LMS_THROTTLE_ENABLE
		lms_throttle_expire();
#endif
		_lms_loop_lastrun = time(NULL);
	}

	event_base_loop(lms_mux_evtb, EVLOOP_ONCE|EVLOOP_NONBLOCK);

	return(0);
}

/*
 * lms_init() initializes all components of libmsocket
 *
 */
int lms_init(unsigned char print)
{
	if (lms_socket_init() < 0)
	{
		if (print > 0)
		{
			fprintf(stdout, "MSocket initialization failed: %s\n", strerror(errno));
		}
		return(-1);
	}
	if (lms_mux_init() < 0)
	{
		if (print > 0)
		{
			fprintf(stdout, "MUX initialization failed: %s\n", strerror(errno));
		}
		return(-1);
	}
	if (lms_dns_init() < 0)
	{
		if (print > 0)
		{
			fprintf(stdout, "DNS initialization failed: %s\n", strerror(errno));
		}
		return(-1);
	}
	if (lms_ssl_init() < 0)
	{
		if (print > 0)
		{
			fprintf(stdout, "SSL initialization failed: %s\n", strerror(errno));
		}
		return(-1);
	}

	return(0);
}

/*
 * lms_version_int() returns the version of libmsocket in an integer format
 *
 */
int lms_version_int()
{
	return(LMS_VERSION_INT);
}

/*
 * lms_version() returns the version of libmsocket
 *
 */
char *lms_version()
{
	return(LMS_VERSION);
}
