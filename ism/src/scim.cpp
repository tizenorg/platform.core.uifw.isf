/* ISF is based on SCIM 1.4.7 and extended for supporting more mobile fitable. */

/*
 * Smart Common Input Method
 * 
 * Copyright (c) 2002-2005 James Su <suzhe@tsinghua.org.cn>
 *
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA  02111-1307  USA
 *
 * $Id: scim.cpp,v 1.51 2005/06/15 00:19:08 suzhe Exp $
 *
 */

#define Uses_SCIM_FRONTEND_MODULE
#define Uses_SCIM_IMENGINE_MODULE
#define Uses_SCIM_HELPER_MODULE
#define Uses_SCIM_BACKEND
#define Uses_SCIM_CONFIG_PATH
#define Uses_SCIM_TRANSACTION
#define Uses_C_LOCALE
#define Uses_SCIM_UTILITY
#include "scim_private.h"
#include <scim.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/times.h>
#include <unistd.h>
#include <signal.h>

using namespace scim;
using std::cout;
using std::cerr;
using std::endl;

int main (int argc, char *argv [])
{
    struct tms tiks_buf;
    clock_t clock_start = times (&tiks_buf);

    BackEndPointer       backend;

    std::vector<String>  frontend_list;
    std::vector<String>  config_list;
    std::vector<String>  engine_list;
    std::vector<String>  helper_list;
    std::vector<String>  exclude_engine_list;
    std::vector<String>  load_engine_list;
    std::vector<String>  all_engine_list;

    std::vector<String>::iterator it;

    String def_frontend ("socket");
    String def_config ("simple");

    int  i;
    bool daemon = false;
    bool socket = false;
    bool manual = false;

    int   new_argc = 0;
    char *new_argv [80];

    cout << "Input Service Manager " << ISF_VERSION << "\n\n";

    /* Get modules list */
    frontend_list.push_back ("socket");
    config_list.push_back ("simple");
    config_list.push_back ("socket");

    scim_get_imengine_module_list (engine_list);
    scim_get_helper_module_list   (helper_list);

    for (it = engine_list.begin (); it != engine_list.end (); it++) {
        all_engine_list.push_back (*it);
        if (*it != "socket")
            load_engine_list.push_back (*it);
    }
    for (it = helper_list.begin (); it != helper_list.end (); it++) {
        all_engine_list.push_back (*it);
        load_engine_list.push_back (*it);
    }

    /* parse command options */
    i = 0;
    while (i < argc) {
        if (++i >= argc) break;

        if (String ("-l") == argv [i] ||
            String ("--list") == argv [i]) {

            cout << endl;
            cout << "Available FrontEnd module:\n";
            for (it = frontend_list.begin (); it != frontend_list.end (); it++)
                cout << "    " << *it << endl;

            cout << endl;
            cout << "Available Config module:\n";
            for (it = config_list.begin (); it != config_list.end (); it++)
            {
                if (*it != "dummy")
                    cout << "    " << *it << endl;
            }

            cout << endl;
            cout << "Available ISEngine module:\n";
            for (it = all_engine_list.begin (); it != all_engine_list.end (); it++)
                cout << "    " << *it << endl;

            return 0;
        }

        if (String ("-f") == argv [i] ||
            String ("--frontend") == argv [i]) {
            if (++i >= argc) {
                cerr << "No argument for option " << argv [i-1] << endl;
                return -1;
            }
            def_frontend = argv [i];
            continue;
        }

        if (String ("-c") == argv [i] ||
            String ("--config") == argv [i]) {
            if (++i >= argc) {
                cerr << "No argument for option " << argv [i-1] << endl;
                return -1;
            }
            def_config = argv [i];
            continue;
        }

        if (String ("-h") == argv [i] ||
            String ("--help") == argv [i]) {
            cout << "Usage: " << argv [0] << " [option]...\n\n"
                 << "The options are: \n"
                 << "  -l, --list              List all of available modules.\n"
                 << "  -f, --frontend name     Use specified FrontEnd module.\n"
                 << "  -c, --config name       Use specified Config module.\n"
                 << "  -e, --engines name      Load specified set of ISEngines.\n"
                 << "  -ne,--no-engines name   Do not load those set of ISEngines.\n"
                 << "  -d, --daemon            Run as a daemon.\n"
                 << "  --no-socket             Do not try to start a SocketFrontEnd daemon.\n"
                 << "  -h, --help              Show this help message.\n";
            return 0;
        }

        if (String ("-d") == argv [i] ||
            String ("--daemon") == argv [i]) {
            daemon = true;
            continue;
        }

        if (String ("-e") == argv [i] || String ("-s") == argv [i] ||
            String ("--engines") == argv [i] || String ("--servers") == argv [i]) {
            if (++i >= argc) {
                cerr << "No argument for option " << argv [i-1] << endl;
                return -1;
            }
            load_engine_list.clear ();
            scim_split_string_list (load_engine_list, String (argv [i]), ',');
            manual = true;
            continue;
        }

        if (String ("-ne") == argv [i] || String ("-ns") == argv [i] ||
            String ("--no-engines") == argv [i] || String ("-no-servers") == argv [i]) {
            if (++i >= argc) {
                cerr << "No argument for option " << argv [i-1] << endl;
                return -1;
            }
            scim_split_string_list (exclude_engine_list, String (argv [i]), ',');
            manual = true;
            continue;
        }

        if (String ("--no-socket") == argv [i]) {
            socket = false;
            continue;
        }

        if (String ("--") == argv [i])
            break;

        cerr << "Invalid command line option: " << argv [i] << "\n";
        return -1;
    } /* End of command line parsing. */

    /* Store the rest argvs into new_argv. */
    for (++i; i < argc; ++i) {
        new_argv [new_argc ++] = argv [i];
    }

    new_argv [new_argc] = 0;

    /* Get the imengine module list which should be loaded. */
    if (exclude_engine_list.size ()) {
        load_engine_list.clear ();
        for (i = 0; i < (int)all_engine_list.size (); ++i) {
            if (std::find (exclude_engine_list.begin (),
                           exclude_engine_list.end (),
                           all_engine_list [i]) == exclude_engine_list.end () &&
                all_engine_list [i] != "socket")
                load_engine_list.push_back (all_engine_list [i]);
        }
    }

    cerr << "Launching a process with " << def_frontend << " FrontEnd...\n";

    /* Launch the scim process. */
    if (scim_launch (daemon,
                     def_config,
                     (load_engine_list.size () ? scim_combine_string_list (load_engine_list, ',') : "none"),
                     def_frontend,
                     new_argv) == 0) {
        if (daemon)
            cerr << "ISM has been successfully launched.\n";
        else
            cerr << "ISM has exited successfully.\n";

        gettime (clock_start, "ISM launch time");

        return 0;
    }

    if (daemon)
        cerr << "Failed to launch ISM.\n";
    else
        cerr << "ISM has exited abnormally.\n";

    return 1;
}

/*
vi:ts=4:ai:nowrap:expandtab
*/
