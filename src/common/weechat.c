/* ############################################################################
 * ###          ___       __         ______________        _____            ###
 * ###          __ |     / /___________  ____/__  /_______ __  /_           ###
 * ###          __ | /| / /_  _ \  _ \  /    __  __ \  __ `/  __/           ###
 * ###          __ |/ |/ / /  __/  __/ /___  _  / / / /_/ // /_             ###
 * ###          ____/|__/  \___/\___/\____/  /_/ /_/\__,_/ \__/             ###
 * ###                                                                      ###
 * ###             WeeChat - Wee Enhanced Environment for Chat              ###
 * ###                 Fast & light environment for Chat                    ###
 * ###                                                                      ###
 * ###                By FlashCode <flashcode@flashtux.org>                 ###
 * ###                                                                      ###
 * ###                     http://weechat.flashtux.org                      ###
 * ###                                                                      ###
 * ############################################################################
 *
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

/* weechat.c: core functions for WeeChat */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <signal.h>

#ifdef HAVE_GNUTLS
#include <gnutls/gnutls.h>
#endif

#ifdef HAVE_LANGINFO_CODESET
#include <langinfo.h>
#endif

#include "weechat.h"
#include "backtrace.h"
#include "command.h"
#include "fifo.h"
#include "log.h"
#include "session.h"
#include "utf8.h"
#include "util.h"
#include "weeconfig.h"
#include "../irc/irc.h"
#include "../gui/gui.h"

#ifdef PLUGINS
#include "../plugins/plugins.h"
#endif


char *weechat_argv0 = NULL;   /* WeeChat binary file name (argv[0])         */
char *weechat_session = NULL; /* WeeChat session file (for /upgrade cmd)    */
time_t weechat_start_time;    /* WeeChat start time (used by /uptime cmd)   */
int quit_weechat;             /* = 1 if quit request from user... why ? :'( */
int sigsegv = 0;              /* SIGSEGV received?                          */
char *weechat_home = NULL;    /* WeeChat home dir. (default: ~/.weechat)    */

char *local_charset = NULL;   /* local charset, for ex.: ISO-8859-1, UTF-8  */

int server_cmd_line;          /* at least 1 server on WeeChat command line  */
int auto_connect;             /* enabled by default (cmd option to disable) */
int auto_load_plugins;        /* enabled by default (cmd option to disable) */

#ifdef HAVE_GNUTLS
gnutls_certificate_credentials gnutls_xcred;   /* gnutls client credentials */
#endif


/*
 * weechat_display_usage: display WeeChat usage
 */

void
weechat_display_usage (char *exec_name)
{
    printf ("\n");
    printf (_("%s (c) Copyright 2003-2006, compiled on %s %s\n"
              "Developed by FlashCode <flashcode@flashtux.org> - %s"),
            PACKAGE_STRING, __DATE__, __TIME__, WEECHAT_WEBSITE);
    printf ("\n\n");
    printf (_("Usage: %s [options ...]\n" \
              "   or: %s [irc[6][s]://[nickname[:password]@]irc.example.org[:port][/channel][,channel[...]]"),
            exec_name, exec_name);
    printf ("\n\n");
    printf (_("  -a, --no-connect        disable auto-connect to servers at startup\n"
              "  -c, --config            display config file options\n"
              "  -d, --dir <path>        set WeeChat home directory (default: ~/.weechat)\n"
              "  -f, --key-functions     display WeeChat internal functions for keys\n"
              "  -h, --help              this help\n"
              "  -i, --irc-commands      display IRC commands\n"
              "  -k, --keys              display WeeChat default keys\n"
              "  -l, --license           display WeeChat license\n"
              "  -p, --no-plugin         don't load any plugin at startup\n"
              "  -v, --version           display WeeChat version\n"
              "  -w, --weechat-commands  display WeeChat commands\n"));
    printf("\n");
}

/*
 * weechat_display_config_options: display config options
 */

void
weechat_display_config_options ()
{
    int i, j, k;
    
    printf (_("WeeChat configuration options (<weechat_home>/weechat.rc):\n\n"));
    for (i = 0; i < CONFIG_NUMBER_SECTIONS; i++)
    {
        if (weechat_options[i])
        {
            j = 0;
            while (weechat_options[i][j].option_name)
            {
                printf ("* %s:\n",
                    weechat_options[i][j].option_name);
                switch (weechat_options[i][j].option_type)
                {
                    case OPTION_TYPE_BOOLEAN:
                        printf (_("  . type: boolean\n"));
                        printf (_("  . values: 'on' or 'off'\n"));
                        printf (_("  . default value: '%s'\n"),
                                (weechat_options[i][j].default_int == BOOL_TRUE) ?
                                "on" : "off");
                        break;
                    case OPTION_TYPE_INT:
                        printf (_("  . type: integer\n"));
                        printf (_("  . values: between %d and %d\n"),
                                weechat_options[i][j].min,
                                weechat_options[i][j].max);
                        printf (_("  . default value: %d\n"),
                                weechat_options[i][j].default_int);
                        break;
                    case OPTION_TYPE_INT_WITH_STRING:
                        printf (_("  . type: string\n"));
                        printf (_("  . values: "));
                        k = 0;
                        while (weechat_options[i][j].array_values[k])
                        {
                            printf ("'%s'",
                                    weechat_options[i][j].array_values[k]);
                            if (weechat_options[i][j].array_values[k + 1])
                                printf (", ");
                            k++;
                        }
                        printf ("\n");
                        printf (_("  . default value: '%s'\n"),
                                (weechat_options[i][j].default_string) ?
                                weechat_options[i][j].default_string : _("empty"));
                        break;
                    case OPTION_TYPE_COLOR:
                        printf (_("  . type: color\n"));
                        printf (_("  . values: Curses or Gtk color\n"));
                        printf (_("  . default value: '%s'\n"),
                                (weechat_options[i][j].default_string) ?
                                weechat_options[i][j].default_string : _("empty"));
                        break;
                    case OPTION_TYPE_STRING:
                        printf (_("  . type: string\n"));
                        printf (_("  . values: any string\n"));
                        printf (_("  . default value: '%s'\n"),
                                (weechat_options[i][j].default_string) ?
                                weechat_options[i][j].default_string : _("empty"));
                        break;
                }
                printf (_("  . description: %s\n"),
                        _(weechat_options[i][j].long_description));
                printf ("\n");
                j++;
            }
        }
    }
}

/*
 * weechat_display_commands: display WeeChat and/or IRC commands
 */

void
weechat_display_commands (int weechat_cmd, int irc_cmd)
{
    int i;
    
    if (weechat_cmd)
    {
        printf (_("%s internal commands:\n"), PACKAGE_NAME);
        printf ("\n");
        for (i = 0; weechat_commands[i].command_name; i++)
        {
            printf ("* %s", weechat_commands[i].command_name);
            if (weechat_commands[i].arguments &&
                weechat_commands[i].arguments[0])
                printf ("  %s\n\n", _(weechat_commands[i].arguments));
            else
                printf ("\n\n");
            printf ("%s\n\n", _(weechat_commands[i].command_description));
            if (weechat_commands[i].arguments_description &&
                weechat_commands[i].arguments_description[0])
                printf ("%s\n\n",
                    _(weechat_commands[i].arguments_description));
        }
    }
    
    if (irc_cmd)
    {
        printf (_("IRC commands:\n"));
        printf ("\n");
        for (i = 0; irc_commands[i].command_name; i++)
        {
            if (irc_commands[i].cmd_function_args ||
                irc_commands[i].cmd_function_1arg)
            {
                printf ("* %s", irc_commands[i].command_name);
                if (irc_commands[i].arguments &&
                    irc_commands[i].arguments[0])
                    printf ("  %s\n\n", _(irc_commands[i].arguments));
                else
                    printf ("\n\n");
                printf ("%s\n\n", _(irc_commands[i].command_description));
                if (irc_commands[i].arguments_description &&
                    irc_commands[i].arguments_description[0])
                    printf ("%s\n\n",
                        _(irc_commands[i].arguments_description));
            }
        }
    }
}

/*
 * weechat_display_key_functions: display WeeChat key functions
 */

void
weechat_display_key_functions ()
{
    int i;
    
    printf (_("Internal key functions:\n"));
    printf ("\n");
    i = 0;
    while (gui_key_functions[i].function_name)
    {
        printf ("* %s: %s\n",
                gui_key_functions[i].function_name,
                _(gui_key_functions[i].description));
        i++;
    }
}

/*
 * weechat_display_keys: display WeeChat default keys
 */

void
weechat_display_keys ()
{
    t_gui_key *ptr_key;
    char *expanded_name;
    
    printf (_("%s default keys:\n"), PACKAGE_NAME);
    printf ("\n");
    for (ptr_key = gui_keys; ptr_key; ptr_key = ptr_key->next_key)
    {
        expanded_name = gui_keyboard_get_expanded_name (ptr_key->key);
        printf ("* %s => %s\n",
                (expanded_name) ? expanded_name : ptr_key->key,
                (ptr_key->function) ? gui_keyboard_function_search_by_ptr (ptr_key->function) : ptr_key->command);
        if (expanded_name)
            free (expanded_name);
    }
}

/*
 * weechat_parse_args: parse command line args
 */

void
weechat_parse_args (int argc, char *argv[])
{
    int i;
    t_irc_server server_tmp;

    weechat_argv0 = strdup (argv[0]);
    weechat_session = NULL;
    weechat_home = NULL;
    server_cmd_line = 0;
    auto_connect = 1;
    auto_load_plugins = 1;
    
    for (i = 1; i < argc; i++)
    {
        if ((strcmp (argv[i], "-a") == 0)
            || (strcmp (argv[i], "--no-connect") == 0))
            auto_connect = 0;
        else if ((strcmp (argv[i], "-c") == 0)
            || (strcmp (argv[i], "--config") == 0))
        {
            weechat_display_config_options ();
            weechat_shutdown (EXIT_SUCCESS, 0);
        }
        else if ((strcmp (argv[i], "-d") == 0)
            || (strcmp (argv[i], "--dir") == 0))
        {
            if (i + 1 < argc)
                weechat_home = strdup (argv[++i]);
            else
            {
                fprintf (stderr,
                         _("%s missing argument for --dir option\n"),
                         WEECHAT_ERROR);
                weechat_shutdown (EXIT_FAILURE, 0);
            }
        }
        else if ((strcmp (argv[i], "-f") == 0)
            || (strcmp (argv[i], "--key-functions") == 0))
        {
            weechat_display_key_functions ();
            weechat_shutdown (EXIT_SUCCESS, 0);
        }
        else if ((strcmp (argv[i], "-h") == 0)
                || (strcmp (argv[i], "--help") == 0))
        {
            weechat_display_usage (argv[0]);
            weechat_shutdown (EXIT_SUCCESS, 0);
        }
        else if ((strcmp (argv[i], "-i") == 0)
            || (strcmp (argv[i], "--irc-commands") == 0))
        {
            weechat_display_commands (0, 1);
            weechat_shutdown (EXIT_SUCCESS, 0);
        }
        else if ((strcmp (argv[i], "-k") == 0)
            || (strcmp (argv[i], "--keys") == 0))
        {
            weechat_display_keys ();
            weechat_shutdown (EXIT_SUCCESS, 0);
        }
        else if ((strcmp (argv[i], "-l") == 0)
                 || (strcmp (argv[i], "--license") == 0))
        {
            printf ("\n%s%s", WEE_LICENSE);
            weechat_shutdown (EXIT_SUCCESS, 0);
        }
        else if ((strcmp (argv[i], "-p") == 0)
                 || (strcmp (argv[i], "--no-plugin") == 0))
            auto_load_plugins = 0;
        else if (strcmp (argv[i], "--session") == 0)
        {
            if (i + 1 < argc)
                weechat_session = strdup (argv[++i]);
            else
            {
                fprintf (stderr,
                         _("%s missing argument for --session option\n"),
                         WEECHAT_ERROR);
                weechat_shutdown (EXIT_FAILURE, 0);
            }
        }
        else if ((strcmp (argv[i], "-v") == 0)
                 || (strcmp (argv[i], "--version") == 0))
        {
            printf (PACKAGE_VERSION "\n");
            weechat_shutdown (EXIT_SUCCESS, 0);
        }
        else if ((strcmp (argv[i], "-w") == 0)
            || (strcmp (argv[i], "--weechat-commands") == 0))
        {
            weechat_display_commands (1, 0);
            weechat_shutdown (EXIT_SUCCESS, 0);
        }
        else if ((ascii_strncasecmp (argv[i], "irc", 3) == 0))
        {
            if (server_init_with_url (argv[i], &server_tmp) < 0)
            {
                fprintf (stderr, _("%s invalid syntax for IRC server ('%s'), ignored\n"),
                         WEECHAT_WARNING, argv[i]);
            }
            else
            {
                if (!server_new (server_tmp.name, server_tmp.autoconnect,
                                 server_tmp.autoreconnect,
                                 server_tmp.autoreconnect_delay,
                                 1, server_tmp.address, server_tmp.port,
                                 server_tmp.ipv6, server_tmp.ssl,
                                 server_tmp.password, server_tmp.nick1,
                                 server_tmp.nick2, server_tmp.nick3,
                                 NULL, NULL, NULL, NULL, 0,
                                 server_tmp.autojoin, 1, NULL, NULL,
                                 NULL, NULL))
                    fprintf (stderr, _("%s unable to create server ('%s'), ignored\n"),
                             WEECHAT_WARNING, argv[i]);
                server_destroy (&server_tmp);
                server_cmd_line = 1;
            }
        }
        else
        {
            fprintf (stderr,
                     _("%s unknown parameter '%s', ignored\n"),
                     WEECHAT_WARNING, argv[i]);
        }
    }
}

/*
 * weechat_create_dir: create a directory
 *                     return: 1 if ok (or directory already exists)
 *                             0 if error
 */

int
weechat_create_dir (char *directory)
{
    if (mkdir (directory, 0755) < 0)
    {
        /* exit if error (except if directory already exists) */
        if (errno != EEXIST)
        {
            fprintf (stderr, _("%s cannot create directory \"%s\"\n"),
                     WEECHAT_ERROR, directory);
            return 0;
        }
    }
    return 1;
}

/*
 * weechat_create_home_dirs: create WeeChat directories
 */

void
weechat_create_home_dirs ()
{
    char *ptr_home;
    int dir_length;
    struct stat statinfo;

    if (!weechat_home)
    {
        ptr_home = getenv ("HOME");
        if (!ptr_home)
        {
            fprintf (stderr, _("%s unable to get HOME directory\n"),
                     WEECHAT_ERROR);
            weechat_shutdown (EXIT_FAILURE, 0);
        }
        dir_length = strlen (ptr_home) + 10;
        weechat_home =
            (char *) malloc (dir_length * sizeof (char));
        if (!weechat_home)
        {
            fprintf (stderr, _("%s not enough memory for home directory\n"),
                     WEECHAT_ERROR);
            weechat_shutdown (EXIT_FAILURE, 0);
        }
        snprintf (weechat_home, dir_length, "%s%s.weechat", ptr_home,
                  DIR_SEPARATOR);
    }

    /* if home already exists, it has to be a directory */
    if (stat (weechat_home, &statinfo) == 0)
    {
        if (!S_ISDIR (statinfo.st_mode))
        {
            fprintf (stderr, _("%s home (%s) is not a directory\n"),
                     WEECHAT_ERROR, weechat_home);
            weechat_shutdown (EXIT_FAILURE, 0);
        }
    }
    
    /* create home directory; error is fatal */
    if (!weechat_create_dir (weechat_home))
    {
        fprintf (stderr, _("%s unable to create \"%s\" directory\n"),
                 WEECHAT_ERROR, weechat_home);
        weechat_shutdown (EXIT_FAILURE, 0);
    }
}

/*
 * weechat_create_config_dirs: create config directories (read from config file)
 */

void
weechat_create_config_dirs ()
{
    char *dir1, *dir2;
    
    /* create logs directory" */
    dir1 = weechat_strreplace (cfg_log_path, "~", getenv ("HOME"));
    dir2 = weechat_strreplace (dir1, "%h", weechat_home);
    if (weechat_create_dir (dir2))
    {
        if (strcmp (dir2, getenv ("HOME")) != 0)
            chmod (dir2, 0700);
    }
    else
        fprintf (stderr, _("%s unable to create \"%s\" directory\n"),
                 WEECHAT_WARNING, dir2);
    if (dir1)
        free (dir1);
    if (dir2)
        free (dir2);
    
    /* create DCC download directory */
    dir1 = weechat_strreplace (cfg_dcc_download_path, "~", getenv ("HOME"));
    dir2 = weechat_strreplace (dir1, "%h", weechat_home);
    if (weechat_create_dir (dir2))
    {
        if (strcmp (dir2, getenv ("HOME")) != 0)
            chmod (dir2, 0700);
    }
    else
        fprintf (stderr, _("%s unable to create \"%s\" directory\n"),
                 WEECHAT_WARNING, dir2);
    if (dir1)
        free (dir1);
    if (dir2)
        free (dir2);
}

/*
 * weechat_init_vars: initialize some variables
 */

void
weechat_init_vars ()
{
    /* start time, used by /uptime command */
    weechat_start_time = time (NULL);
    
    /* init received messages queue */
    recv_msgq = NULL;
    msgq_last_msg = NULL;
    
    /* init gnutls */
#ifdef HAVE_GNUTLS
    gnutls_global_init ();
    gnutls_certificate_allocate_credentials (&gnutls_xcred);
    gnutls_certificate_set_x509_trust_file (gnutls_xcred, "ca.pem", GNUTLS_X509_FMT_PEM);
#endif
}

/*
 * weechat_config_read: read WeeChat config file
 */

void
weechat_config_read ()
{
    switch (config_read ())
    {
        case 0: /* read ok */
            break;
        case -1: /* config file not found */
            if (config_create_default () < 0)
                exit (EXIT_FAILURE);
            if (config_read () != 0)
                exit (EXIT_FAILURE);
            break;
        default: /* other error (fatal) */
            server_free_all ();
            exit (EXIT_FAILURE);
    }
}

/*
 * weechat_welcome_message: display WeeChat welcome message - yeah!
 */

void
weechat_welcome_message ()
{
    if (cfg_look_startup_logo)
    {
        gui_printf (NULL,
                    "%s   ___       __         ______________        _____ \n"
                    "%s   __ |     / /___________  ____/__  /_______ __  /_\n"
                    "%s   __ | /| / /_  _ \\  _ \\  /    __  __ \\  __ `/  __/\n"
                    "%s   __ |/ |/ / /  __/  __/ /___  _  / / / /_/ // /_  \n"
                    "%s   ____/|__/  \\___/\\___/\\____/  /_/ /_/\\__,_/ \\__/  \n",
                    GUI_COLOR(COLOR_WIN_CHAT_NICK),
                    GUI_COLOR(COLOR_WIN_CHAT_NICK),
                    GUI_COLOR(COLOR_WIN_CHAT_NICK),
                    GUI_COLOR(COLOR_WIN_CHAT_NICK),
                    GUI_COLOR(COLOR_WIN_CHAT_NICK));
    }
    if (cfg_look_weechat_slogan && cfg_look_weechat_slogan[0])
    {
        gui_printf (NULL, _("%sWelcome to %s%s%s, %s\n"),
                    (cfg_look_startup_logo) ? "      " : "",
                    GUI_COLOR(COLOR_WIN_CHAT_CHANNEL),
                    PACKAGE_NAME,
                    GUI_NO_COLOR,
                    cfg_look_weechat_slogan);
    }
    if (cfg_look_startup_version)
    {
        gui_printf (NULL, "%s%s%s%s, %s %s %s\n",
                    (cfg_look_startup_logo) ? "    " : "",
                    GUI_COLOR(COLOR_WIN_CHAT_CHANNEL),
                    PACKAGE_STRING,
                    GUI_NO_COLOR,
                    _("compiled on"), __DATE__, __TIME__);
    }
    if (cfg_look_startup_logo ||
        (cfg_look_weechat_slogan && cfg_look_weechat_slogan[0]) ||
        cfg_look_startup_version)
        gui_printf (NULL,
                    "%s-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-\n",
                    GUI_COLOR(COLOR_WIN_CHAT_NICK));
    
    weechat_log_printf ("%s (%s %s %s)\n",
                        PACKAGE_STRING, _("compiled on"), __DATE__, __TIME__);
}

/*
 * weechat_shutdown: shutdown WeeChat
 */

void
weechat_shutdown (int return_code, int crash)
{
    if (weechat_argv0)
        free (weechat_argv0);
    fifo_remove ();
    if (weechat_home)
        free (weechat_home);
    weechat_log_close ();
    if (local_charset)
        free (local_charset);
    alias_free_all ();
    
#ifdef HAVE_GNUTLS
    gnutls_certificate_free_credentials (gnutls_xcred);
    gnutls_global_deinit();
#endif
    
    if (crash)
        abort();
    else
        exit (return_code);
}

/*
 * weechat_dump: write dump to WeeChat log file
 */

void
weechat_dump (int crash)
{
    t_irc_server *ptr_server;
    t_irc_channel *ptr_channel;
    t_irc_nick *ptr_nick;
    t_gui_window *ptr_window;
    t_gui_buffer *ptr_buffer;
    
    /* prevent reentrance */
    if (sigsegv)
        exit (EXIT_FAILURE);
    
    if (crash)
    {
        sigsegv = 1;
        weechat_log_printf ("Very bad, WeeChat is crashing (SIGSEGV received)...\n");
    }
    
    weechat_log_printf ("\n");
    if (crash)
    {
        weechat_log_printf ("******             WeeChat CRASH DUMP              ******\n");
        weechat_log_printf ("****** Please send this file to WeeChat developers ******\n");
        weechat_log_printf ("******    and explain when this crash happened     ******\n");
    }
    else
    {
        weechat_log_printf ("******            WeeChat dump request             ******\n");
    }
    
    for (ptr_server = irc_servers; ptr_server; ptr_server = ptr_server->next_server)
    {
        weechat_log_printf ("\n");
        server_print_log (ptr_server);
        
        for (ptr_channel = ptr_server->channels; ptr_channel;
            ptr_channel = ptr_channel->next_channel)
        {
            weechat_log_printf ("\n");
            channel_print_log (ptr_channel);
            
            for (ptr_nick = ptr_channel->nicks; ptr_nick;
                ptr_nick = ptr_nick->next_nick)
            {
                weechat_log_printf ("\n");
                nick_print_log (ptr_nick);
            }
            
        }
    }
    
    dcc_print_log ();
    
    gui_panel_print_log ();
    
    weechat_log_printf ("\n");
    weechat_log_printf ("[windows/buffers]\n");
    weechat_log_printf ("  => windows:\n");
    for (ptr_window = gui_windows; ptr_window; ptr_window = ptr_window->next_window)
    {
        weechat_log_printf ("       0x%X\n", ptr_window);
    }
    weechat_log_printf ("  => buffers:\n");
    for (ptr_buffer = gui_buffers; ptr_buffer; ptr_buffer = ptr_buffer->next_buffer)
    {
        weechat_log_printf ("       0x%X\n", ptr_buffer);
    }
    weechat_log_printf ("  => current window = 0x%X\n", gui_current_window);
    
    for (ptr_window = gui_windows; ptr_window; ptr_window = ptr_window->next_window)
    {
        weechat_log_printf ("\n");
        gui_window_print_log (ptr_window);
    }
    
    for (ptr_buffer = gui_buffers; ptr_buffer; ptr_buffer = ptr_buffer->next_buffer)
    {
        weechat_log_printf ("\n");
        gui_buffer_print_log (ptr_buffer);
    }
    
    weechat_log_printf ("\n");
    ignore_print_log ();
    
    weechat_log_printf ("\n");
    weechat_log_printf ("******                 End of dump                 ******\n");
    weechat_log_printf ("\n");
}

/*
 * weechat_sigsegv: SIGSEGV handler: save crash log to <weechat_home>/weechat.log and exit
 */

void
weechat_sigsegv ()
{
    weechat_dump (1);
    dcc_end ();
    server_free_all ();
    gui_main_end ();

    fprintf (stderr, "\n");
    fprintf (stderr, "*** Very bad! WeeChat is crashing (SIGSEGV received)\n");
    if (!weechat_log_crash_rename ())
        fprintf (stderr, "*** Full crash dump was saved to %s/weechat.log file.\n",
                 weechat_home);
    fprintf (stderr, "***\n");
    fprintf (stderr, "*** Please help WeeChat developers to fix this bug:\n");
    fprintf (stderr, "***   1. If you have a core file, please run:  gdb weechat-curses core\n");
    fprintf (stderr, "***      then issue \"bt\" command and send result to developers\n");
    fprintf (stderr, "***      To enable core files with bash shell: ulimit -c 10000\n");
    fprintf (stderr, "***   2. Otherwise send backtrace (below) and weechat.log\n");
    fprintf (stderr, "***      (be careful, private info may be in this file since\n");
    fprintf (stderr, "***      part of chats are displayed, so remove lines if needed)\n\n");
    
    weechat_backtrace ();
    
    /* shutdown with error code */
    weechat_shutdown (EXIT_FAILURE, 1);
}

/*
 * main: WeeChat startup
 */

int
main (int argc, char *argv[])
{
#ifdef ENABLE_NLS
    setlocale (LC_ALL, "");             /* initialize gettext               */
    bindtextdomain (PACKAGE, LOCALEDIR);
    textdomain (PACKAGE);
#endif
    
#ifdef HAVE_LANGINFO_CODESET
    local_charset = strdup (nl_langinfo (CODESET));
#endif
    
    signal (SIGINT, SIG_IGN);           /* ignore SIGINT signal             */
    signal (SIGQUIT, SIG_IGN);          /* ignore SIGQUIT signal            */
    signal (SIGPIPE, SIG_IGN);          /* ignore SIGPIPE signal            */
    signal (SIGSEGV, weechat_sigsegv);  /* crash dump when SIGSEGV received */
    gui_main_pre_init (&argc, &argv);   /* pre-initiliaze interface         */
    weechat_init_vars ();               /* initialize some variables        */
    gui_keyboard_init ();               /* init keyb. (default key bindings)*/
    weechat_parse_args (argc, argv);    /* parse command line args          */
    weechat_create_home_dirs ();        /* create WeeChat directories       */
    weechat_log_init ();                /* init log file                    */
    command_index_build ();             /* build cmd index for completion   */
    weechat_config_read ();             /* read configuration               */
    weechat_create_config_dirs ();      /* create config directories        */
    utf8_init ();                       /* init UTF-8 in WeeChat            */
    gui_main_init ();                   /* init WeeChat interface           */
    fifo_create ();                     /* FIFO pipe for remote control     */
    if (weechat_session)
        session_load (weechat_session); /* load previous session if asked   */
    weechat_welcome_message ();         /* display WeeChat welcome message  */
#ifdef PLUGINS
    plugin_init (auto_load_plugins);    /* init plugin interface(s)         */
#endif
    
    server_auto_connect (auto_connect,  /* auto-connect to servers          */
                         server_cmd_line);
    
    gui_main_loop ();                   /* WeeChat main loop                */
    
#ifdef PLUGINS    
    plugin_end ();                      /* end plugin interface(s)          */
#endif
    server_disconnect_all ();           /* disconnect from all servers      */
    if (cfg_look_save_on_exit)
        (void) config_write (NULL);     /* save config file                 */
    command_index_free ();              /* free commands index              */
    dcc_end ();                         /* remove all DCC                   */
    server_free_all ();                 /* free all servers                 */
    gui_main_end ();                    /* shut down WeeChat GUI            */
    weechat_shutdown (EXIT_SUCCESS, 0); /* quit WeeChat (oh no, why?)       */
    
    return EXIT_SUCCESS;                /* make gcc happy (never executed)  */
}
