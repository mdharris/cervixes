dnl NGCast configure.ac
dnl 

AC_PREREQ(2.57)

AC_COPYRIGHT([Copyright 2008 Matt D. Harris - See LICENSE for more details])

AC_INIT([libmsocket],[0.4])

AC_CONFIG_HEADER(localconfig.h)

AC_PREFIX_DEFAULT(/usr/local)

OLD_CFLAGS="$CFLAGS"

AC_PROG_CC
AC_LANG(C)

CFLAGS="$OLD_CFLAGS"

AC_SUBST(STDOUT)

AC_ARG_ENABLE(debug,
[AC_HELP_STRING([--enable-debug],[Enable compile-time and run-time debugging])],
[cf_debug="1"], 
[cf_debug="0"])
AC_ARG_ENABLE(throttling,
[AC_HELP_STRING([--enable-throttling],[Enable throttling interface])],
[AC_DEFINE([LMS_THROTTLE_ENABLE], [1])],
[unusedvariable="0"])

AC_ARG_WITH(openssl,
[AC_HELP_STRING([--with-openssl=DIR],[Specify OpenSSL path.])],
[cf_enable_openssl=$enableval],
[cf_enable_openssl="auto"])
AC_ARG_WITH(libevent,
[AC_HELP_STRING([--with-libevent=DIR],[Specify libevent path.])],
[cf_enable_libevent=$enableval],
[cf_enable_libevent="auto"])

AC_ISC_POSIX
AC_C_INLINE
AC_C_CONST
AC_C_VOLATILE
AC_C_BIGENDIAN
AC_PROG_INSTALL
AC_PROG_MAKE_SET
AC_PATH_PROG(TEST, [test], [test])

AC_HEADER_STDC
AC_CHECK_HEADER([stdint.h])
AC_CHECK_HEADERS([sys/param.h sys/endian.h netinet/in.h])
AC_CHECK_SIZEOF(long long)
AC_CHECK_SIZEOF(long)
AC_CHECK_SIZEOF(int)
AC_CHECK_SIZEOF(short)
AC_CHECK_FUNCS([mmap realloc reallocf gmtime_r localtime_r usleep])

AC_CHECK_HEADER([readpassphrase.h], [
	AC_CHECK_FUNCS(readpassphrase, AC_DEFINE(HAVE_READPASSPHRASE, 1, [Define to 1 if readpassphrase() is available.]))],
	[AC_CHECK_FUNCS(getpass, AC_DEFINE(HAVE_GETPASS, 1, [Define to 1 if getpass() is available.]))
])

if test "$ac_cv_sizeof_long_long" = 8; then
  AC_CHECK_TYPE(uint64_t, unsigned long long)
  AC_CHECK_TYPE(int64_t, long long)
else
  AC_MSG_ERROR([Your system doesn't appear to have support for 64 bit integers.])
fi

AC_MSG_CHECKING([for entropy device])
if test -c /dev/urandom ; then
  AC_DEFINE(LMS_ENTROPYSOURCE, "/dev/urandom", [Path to entropy device])
  AC_MSG_RESULT([/dev/urandom])
else
  if test -c /dev/random ; then
    AC_DEFINE(LMS_ENTROPYSOURCE, "/dev/random", [Path to entropy device])
    AC_MSG_RESULT([/dev/random])
  else
    AC_DEFINE(LMS_ENTROPYSOURCE, "", [Path to entropy device])
    AC_MSG_ERROR([no suitable entropy source was found.])
  fi
fi


dnl OpenSSL support

AC_MSG_CHECKING(for OpenSSL)

if test "$cf_enable_openssl" != "no" ; then
  cf_openssl_basedir=""
  if test "$cf_enable_openssl" != "auto" &&
  test "$cf_enable_openssl" != "yes" ; then
    cf_openssl_basedir="`echo ${cf_enable_openssl} | sed 's/\/$//'`"
  else
    for dirs in /usr/local/ssl /usr/pkg /usr/local /usr/local/openssl /usr/sfw /opt/sfw /usr; do
      if test -f "${dirs}/include/openssl/opensslv.h" ; then
	cf_openssl_basedir="${dirs}"
	break
      fi
    done
    unset dirs
  fi
  if test ! -z "$cf_openssl_basedir"; then
    if test -f "${cf_openssl_basedir}/include/openssl/opensslv.h" ; then
      SSL_INCLUDES="-I${cf_openssl_basedir}/include"
      SSL_LIBS="-L${cf_openssl_basedir}/lib"
    else
      cf_openssl_basedir=""
    fi
  fi

  if test ! -z "$cf_openssl_basedir"; then
    AC_MSG_RESULT($cf_openssl_basedir)
    cf_enable_openssl="yes"
  else
    AC_MSG_ERROR([Could not find OpenSSL: please use --with-openssl=/path/to/openssl])
  fi
  unset cf_openssl_basedir
else
  cf_enable_openssl="no"
  AC_MSG_RESULT([not found.])
fi

CPPFLAGS="$CPPFLAGS $SSL_INCLUDES"
CFLAGS="$CFLAGS $SSL_INCLUDES"
LIBS="$LIBS $SSL_LIBS"
AC_CHECK_LIB(ssl, SSL_library_init)
AC_CHECK_LIB(crypto, RAND_seed)
if test "$cf_enable_openssl" != no; then
	AC_MSG_CHECKING(for OpenSSL 0.9.7 or above)
	AC_RUN_IFELSE(
		AC_LANG_PROGRAM(
	    	[#include <openssl/opensslv.h>
		#include <stdlib.h>],
		[[if (OPENSSL_VERSION_NUMBER >= 0x00907000)
		exit(0); else exit(1);]]),
	cf_openssl_version_ok=yes,
	cf_openssl_version_ok=no,
	cf_openssl_version_ok=no)

	if test "$cf_openssl_version_ok" = yes; then
		AC_MSG_RESULT(found)
	else
		AC_MSG_RESULT([no - OpenSSL 0.9.8d or later is recommended, but OpenSSL 0.9.7 and above should work.])
		cf_enable_openssl='no'
		AC_MSG_ERROR([No OpenSSL found.])
	fi
fi

dnl End OpenSSL detection


dnl libevent

AC_MSG_CHECKING(for libevent)

if test "$cf_enable_libevent" != "no" ; then
  cf_libevent_basedir=""
  if test "$cf_enable_libevent" != "auto" &&
  test "$cf_enable_libevent" != "yes" ; then
    cf_libevent_basedir="`echo ${cf_enable_libevent} | sed 's/\/$//'`"
  else
    for dirs in /usr/local /usr/pkg /usr/sfw /opt/sfw /usr; do
      if test -f "${dirs}/include/event.h" ; then
	cf_libevent_basedir="${dirs}"
	break
      fi
    done
    unset dirs
  fi
  if test ! -z "$cf_libevent_basedir"; then
    if test -f "${cf_libevent_basedir}/include/event.h" ; then
      LIBEVENT_INCLUDES="-I${cf_libevent_basedir}/include"
      LIBEVENT_LIBS="-L${cf_libevent_basedir}/lib"
    else
      cf_libevent_basedir=""
    fi
  fi

  if test ! -z "$cf_libevent_basedir"; then
    AC_MSG_RESULT($cf_libevent_basedir)
    cf_enable_libevent="yes"
  else
    AC_MSG_ERROR([Could not find libevent: please use --with-libevent=/path/to/libevent])
  fi
  unset cf_libevent_basedir
else
  cf_enable_libevent="no"
  AC_MSG_RESULT([not found.])
fi

CPPFLAGS="$CPPFLAGS $LIBEVENT_INCLUDES"
CFLAGS="$CFLAGS $LIBEVENT_INCLUDES"
LIBS="$LIBS $LIBEVENT_LIBS"

AC_CHECK_LIB(event, event_init)
if test "$ac_cv_lib_event_event_init" = yes; then
	AC_CHECK_HEADER(event.h, , [AC_MSG_ERROR([Could not find event.h])])
else
	AC_MSG_ERROR(not found)
fi

dnl end libevent detection

if test "$prefix" = "NONE"; then 
  PREFIX=$ac_default_prefix
  AC_DEFINE_UNQUOTED(MY_PREFIX, "$ac_default_prefix", [Prefix where the framework is to be installed.])
else
  prefix=`echo $prefix | sed 's/\/$//'`
  PREFIX=$prefix
  AC_DEFINE_UNQUOTED(MY_PREFIX, "$prefix", [Prefix where the framework is to be installed.])
fi

if test "$cf_debug" = "1"; then
  DFLAGS="-Wall -Wno-long-long -Wno-unused-function -Wno-unused-variable -Wno-unused-value -ggdb"
  AC_DEFINE([LMS_DEBUG], [1])
else
  DFLAGS=""
fi
AC_SUBST(DFLAGS)

AC_SUBST(CFLAGS)
AC_SUBST(LIBS)
AC_SUBST(PREFIX)
AC_CONFIG_FILES(Makefile)

AC_OUTPUT
