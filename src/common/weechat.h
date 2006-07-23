/*
 * Copyright (c) 2003-2006 by FlashCode <flashcode@flashtux.org>
 * See README for License detail, AUTHORS for developers list.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */


#ifndef __WEECHAT_H
#define __WEECHAT_H 1

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <sys/time.h>

#ifdef HAVE_GNUTLS
#include <gnutls/gnutls.h>
#endif

#if defined(ENABLE_NLS) && !defined(_)
    #include <locale.h>
    #ifdef HAVE_LIBINTL_H
        #include <libintl.h>
    #else
        #include "../../intl/libintl.h"
    #endif
    #define _(x) gettext(x)
    #ifdef gettext_noop
        #define N_(string) gettext_noop (string)
    #else
        #define N_(string) (string)
    #endif
#endif
#if !defined(_)
    #define _(x) (x)
    #define N_(string) (string)
#endif


#define WEECHAT_COPYRIGHT_DATE "(c) 2003-2006"
#define WEECHAT_WEBSITE "http://weechat.flashtux.org"

#define WEECHAT_ERROR   _("WeeChat Error:")
#define WEECHAT_WARNING _("WeeChat Warning:")

/* log file */

#define WEECHAT_LOG_NAME "weechat.log"

/* license */

#define WEE_LICENSE \
    PACKAGE_STRING " (c) Copyright 2003-2006, compiled on " __DATE__ " " __TIME__ \
    "\nDeveloped by FlashCode <flashcode@flashtux.org> - " WEECHAT_WEBSITE "\n\n" \
    "This program is free software; you can redistribute it and/or modify\n" \
    "it under the terms of the GNU General Public License as published by\n" \
    "the Free Software Foundation; either version 2 of the License, or\n" \
    "(at your option) any later version.\n" \
    "\n", \
    \
    "This program is distributed in the hope that it will be useful,\n" \
    "but WITHOUT ANY WARRANTY; without even the implied warranty of\n" \
    "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n" \
    "GNU General Public License for more details.\n" \
    "\n" \
    "You should have received a copy of the GNU General Public License\n" \
    "along with this program; if not, write to the Free Software\n" \
    "Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA\n\n"
    
/* directory separator, depending on OS */

#ifdef _WIN32
    #define DIR_SEPARATOR       "\\"
    #define DIR_SEPARATOR_CHAR  '\\'
#else
    #define DIR_SEPARATOR       "/"
    #define DIR_SEPARATOR_CHAR  '/'
#endif

/* global variables and functions */

extern char *weechat_argv0;
extern time_t weechat_start_time;
extern int quit_weechat;
extern char *weechat_home;
extern char *local_charset;

#ifdef HAVE_GNUTLS
extern gnutls_certificate_credentials gnutls_xcred;
#endif

extern void weechat_dump (int);
extern void weechat_shutdown (int, int);

#endif /* weechat.h */
