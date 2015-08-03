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
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/times.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <privilege-control.h>
#include <sys/resource.h>
#include <sched.h>
#include <pkgmgr-info.h>
#include <scim_panel_common.h>
#include "isf_query_utility.h"

using namespace scim;
using std::cout;
using std::cerr;
using std::endl;

static bool check_socket_frontend (void)
{
    SocketAddress address;
    SocketClient client;

    uint32 magic;

    address.set_address (scim_get_default_socket_frontend_address ());

    if (!client.connect (address))
        return false;

    if (!scim_socket_open_connection (magic,
                                      String ("ConnectionTester"),
                                      String ("SocketFrontEnd"),
                                      client,
                                      1000)) {
        return false;
    }

    return true;
}

static bool check_panel (const String &display)
{
    SocketAddress address;
    SocketClient client;

    uint32 magic;

    address.set_address (scim_get_default_panel_socket_address (display));

    if (!client.connect (address))
        return false;

    if (!scim_socket_open_connection (magic,
        String ("ConnectionTester"),
        String ("Panel"),
        client,
        1000)) {
            return false;
    }

    return true;
}

/**
 * @brief Read data from ime category manifest and insert initial db
 *
 * @param handle    pkgmgrinfo_appinfo_h pointer
 * @param user_data The data to pass to this callback.
 *
 * @return 0 if success, negative value(<0) if fail. Callback is not called if return value is negative.
 *
 * @see _filtered_app_list_cb function in isf_panel_efl.cpp; it's slightly different.
 */
static int _ime_app_list_cb (const pkgmgrinfo_appinfo_h handle, void *user_data)
{
    int ret = 0;
    char *appid = NULL, *pkgid = NULL, *pkgtype = NULL, *exec = NULL, *label = NULL, *path = NULL;
    pkgmgrinfo_pkginfo_h  pkginfo_handle = NULL;
    ImeInfoDB ime_db;

    /* appid */
    ret = pkgmgrinfo_appinfo_get_appid (handle, &appid);
    if (ret == PMINFO_R_OK)
        ime_db.appid = String (appid ? appid : "");
    else {
        ISF_SAVE_LOG ("appid is not available!\n");
        return 0;
    }

    ime_db.iconpath = "";

    /* pkgid */
    ret = pkgmgrinfo_appinfo_get_pkgid (handle, &pkgid);
    if (ret == PMINFO_R_OK)
        ime_db.pkgid = String (pkgid ? pkgid : "");
    else {
        ISF_SAVE_LOG ("pkgid is not available!\n");
        return 0;
    }

    /* exec path */
    ret = pkgmgrinfo_appinfo_get_exec (handle, &exec);
    if (ret == PMINFO_R_OK)
        ime_db.exec = String (exec ? exec : "");
    else {
        ISF_SAVE_LOG ("exec is not available!\n");
        return 0;
    }

    /* label */
    ret = pkgmgrinfo_appinfo_get_label (handle, &label);
    if (ret == PMINFO_R_OK)
        ime_db.label = String (label ? label : "");

    /* get pkgmgrinfo_pkginfo_h */
    ret = pkgmgrinfo_pkginfo_get_pkginfo (pkgid, &pkginfo_handle);
    if (ret == PMINFO_R_OK && pkginfo_handle) {
        /* pkgtype */
        ret = pkgmgrinfo_pkginfo_get_type(pkginfo_handle, &pkgtype);

        if (ret == PMINFO_R_OK)
            ime_db.pkgtype = String(pkgtype ? pkgtype : "");
        else {
            ISF_SAVE_LOG ("pkgtype is not available!");
            pkgmgrinfo_pkginfo_destroy_pkginfo(pkginfo_handle);
            return 0;
        }

        /* pkgrootpath */
        ret = pkgmgrinfo_pkginfo_get_root_path (pkginfo_handle, &path);
    }

    ime_db.languages = "en";
    ime_db.display_lang = "";

    if (ime_db.pkgtype.compare ("rpm") == 0 &&   //1 Inhouse IMEngine ISE(IME)
        ime_db.exec.find ("scim-launcher") != String::npos)  // Some IMEngine's pkgid doesn't have "ise-engine" prefix.
    {
        ime_db.mode = TOOLBAR_KEYBOARD_MODE;
        ime_db.options = 0;
        ime_db.module_path = String (SCIM_MODULE_PATH) + String (SCIM_PATH_DELIM_STRING) + String (SCIM_BINARY_VERSION)
            + String (SCIM_PATH_DELIM_STRING) + String ("IMEngine");
        ime_db.module_name = ime_db.pkgid;
        ime_db.is_enabled = 1;
        ime_db.is_preinstalled = 1;
        ime_db.has_option = 0; // It doesn't matter. No option for IMEngine...
    }
    else {
        ime_db.mode = TOOLBAR_HELPER_MODE;
        if (ime_db.pkgtype.compare ("rpm") == 0) //1 Inhouse Helper ISE(IME)
        {
#ifdef _TV
            ime_db.options = SCIM_HELPER_STAND_ALONE | SCIM_HELPER_NEED_SCREEN_INFO | SCIM_HELPER_AUTO_RESTART | ISM_HELPER_PROCESS_KEYBOARD_KEYEVENT;
#else
            ime_db.options = SCIM_HELPER_STAND_ALONE | SCIM_HELPER_NEED_SCREEN_INFO | SCIM_HELPER_AUTO_RESTART;
#endif
            ime_db.module_path = String (SCIM_MODULE_PATH) + String (SCIM_PATH_DELIM_STRING) + String (SCIM_BINARY_VERSION)
                + String (SCIM_PATH_DELIM_STRING) + String ("Helper");
            ime_db.module_name = ime_db.pkgid;
            ime_db.is_enabled = 1;
            ime_db.is_preinstalled = 1;
            ime_db.has_option = 1;  // Let's assume the inhouse IME always has an option menu.
        }
#ifdef _WEARABLE
        else if (ime_db.pkgtype.compare ("wgt") == 0)    //1 Download Web IME
        {
            ime_db.options = SCIM_HELPER_STAND_ALONE | SCIM_HELPER_NEED_SCREEN_INFO | SCIM_HELPER_AUTO_RESTART
                | SCIM_HELPER_NEED_SPOT_LOCATION_INFO | ISM_HELPER_PROCESS_KEYBOARD_KEYEVENT | ISM_HELPER_WITHOUT_IMENGINE;
            ime_db.module_path = String (SCIM_MODULE_PATH) + String (SCIM_PATH_DELIM_STRING) + String (SCIM_BINARY_VERSION)
                + String (SCIM_PATH_DELIM_STRING) + String ("Helper");
            ime_db.module_name = String ("ise-web-helper-agent");
            if (ime_db.exec.compare (0, 5, "/usr/") == 0) {
                ime_db.is_enabled = 1;
                ime_db.is_preinstalled = 1;
            }
            else {
               ime_db.is_enabled = 0;
               ime_db.is_preinstalled = 0;
            }
            ime_db.has_option = -1; // At this point, we can't know IME has an option (setting) or not; -1 means unknown.
        }
#endif
        else if (ime_db.pkgtype.compare ("tpk") == 0)    //1 Download Native IME
        {
            ime_db.options = SCIM_HELPER_STAND_ALONE | SCIM_HELPER_NEED_SCREEN_INFO | SCIM_HELPER_AUTO_RESTART
                | ISM_HELPER_PROCESS_KEYBOARD_KEYEVENT | ISM_HELPER_WITHOUT_IMENGINE;
            if (path)
                ime_db.module_path = String (path) + String ("/lib");
            else
                ime_db.module_path = String ("/opt/usr/apps/") + ime_db.pkgid + String ("/lib");
            ime_db.module_name = String ("lib") + ime_db.exec.substr (ime_db.exec.find_last_of (SCIM_PATH_DELIM) + 1);
            if (ime_db.exec.compare (0, 5, "/usr/") == 0) {
                ime_db.is_enabled = 1;
                ime_db.is_preinstalled = 1;
            }
            else {
                ime_db.is_enabled = 0;
                ime_db.is_preinstalled = 0;
            }
            ime_db.has_option = -1; // At this point, we can't know IME has an option (setting) or not; -1 means unknown.
        }
        else {
            ISF_SAVE_LOG ("Unsupported pkgtype(%s)\n", ime_db.pkgtype.c_str ());
            if (pkginfo_handle) {
                pkgmgrinfo_pkginfo_destroy_pkginfo (pkginfo_handle);
                pkginfo_handle = NULL;
            }
            return 0;
        }
    }

    ret = isf_db_insert_ime_info (&ime_db);
    if (ret < 1) {
        ISF_SAVE_LOG("isf_db_insert_ime_info failed(%d). appid=%s pkgid=%s\n", ret, ime_db.appid.c_str(), ime_db.pkgid.c_str());
    }

    if (pkginfo_handle) {
        pkgmgrinfo_pkginfo_destroy_pkginfo (pkginfo_handle);
        pkginfo_handle = NULL;
    }

    return 0;
}


/* The broker for launching OSP based IMEs */
// {

#include <Ecore.h>
#include <Ecore_Ipc.h>
#ifdef _MOBILE
#include <vconf.h>
#endif

#ifndef SCIM_HELPER_LAUNCHER_PROGRAM
#define SCIM_HELPER_LAUNCHER_PROGRAM  (SCIM_LIBEXECDIR "/scim-helper-launcher")
#endif

#ifdef _MOBILE
#define ISF_SYSTEM_APPSERVICE_READY_VCONF               "memory/appservice/status"
#define ISF_SYSTEM_APPSERVICE_READY_STATE               2
#endif

static Ecore_Ipc_Server *server = NULL;

static Ecore_Event_Handler *exit_handler = NULL;
static Ecore_Event_Handler *data_handler = NULL;

#ifdef _MOBILE
static bool _appsvc_callback_regist = false;
#endif

static char _ise_name[_POSIX_PATH_MAX + 1] = {0};
static char _ise_uuid[_POSIX_PATH_MAX + 1] = {0};

static void launch_helper (const char *name, const char *uuid);

#ifdef _MOBILE
static void vconf_appservice_ready_changed (keynode_t *node, void *user_data)
{
    int node_value = vconf_keynode_get_int(node);
    if (node && node_value >= ISF_SYSTEM_APPSERVICE_READY_STATE) {
        if (_appsvc_callback_regist) {
            vconf_ignore_key_changed (ISF_SYSTEM_APPSERVICE_READY_VCONF, vconf_appservice_ready_changed);
            _appsvc_callback_regist = false;
        }

        ISF_SAVE_LOG ("vconf_appservice_ready_changed val : %d)\n", node_value);

        launch_helper (_ise_name, _ise_uuid);
    }
}

static bool check_appservice_ready ()
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    int ret = 0;
    int val = 0;
    ret = vconf_get_int (ISF_SYSTEM_APPSERVICE_READY_VCONF, &val);

    ISF_SAVE_LOG ("vconf returned : %d, val : %d)\n", ret, val);

    if (ret == 0) {
        if (val >= ISF_SYSTEM_APPSERVICE_READY_STATE)
            return true;
        else {
            /* Register a call back function for checking system ready */
            if (!_appsvc_callback_regist) {
                if (vconf_notify_key_changed (ISF_SYSTEM_APPSERVICE_READY_VCONF, vconf_appservice_ready_changed, NULL)) {
                    _appsvc_callback_regist = true;
                }
            }

            return false;
        }
    } else {
        /* No OSP support environment */
        return true;
    }
}
#endif

static void launch_helper (const char *name, const char *uuid)
{
    int pid;

#ifdef _MOBILE
    /* If appservice is not ready yet, let's return here */
    if (!check_appservice_ready ()) return;
#endif

    pid = fork ();

    if (pid < 0) return;

    if (pid == 0) {
        if (exit_handler) ecore_event_handler_del (exit_handler);
        if (data_handler) ecore_event_handler_del (data_handler);
        if (server) ecore_ipc_server_del (server);
        ecore_ipc_shutdown ();

        const char *argv [] = { SCIM_HELPER_LAUNCHER_PROGRAM,
            "--daemon",
            "--config", "socket",
            "--display", ":0",
            _ise_name,
            _ise_uuid,
            0};

        ISF_SAVE_LOG ("Exec scim_helper_launcher(%s %s)\n", _ise_name, _ise_uuid);

        execv (SCIM_HELPER_LAUNCHER_PROGRAM, const_cast<char **>(argv));
        exit (-1);
    }
}

static Eina_Bool sig_exit_cb (void *data, int ev_type, void *ev)
{
    ecore_main_loop_quit ();
    return ECORE_CALLBACK_CANCEL;
}

static Eina_Bool handler_client_data (void *data, int ev_type, void *ev)
{
    Ecore_Ipc_Event_Client_Data *e = (Ecore_Ipc_Event_Client_Data *)ev;
    if (!e) return ECORE_CALLBACK_RENEW;

    ISF_SAVE_LOG ("client %p sent [%i] [%i] [%i]\n", e->client, e->major, e->minor, e->size);

    const char *message = "Done";
    if (ecore_ipc_client_send (e->client, 0, 0, 0, 0, 0, message, strlen (message)) == 0) {
        ISF_SAVE_LOG ("ecore_ipc_client_send FAILED!!\n");
    }

    char buffer[_POSIX_PATH_MAX + 1] = {0};
    strncpy (buffer, (char*)(e->data), (e->size > _POSIX_PATH_MAX) ? _POSIX_PATH_MAX : e->size);

    int blank_index = 0;
    for (int loop = 0; loop < (int)strlen (buffer); loop++) {
        if (buffer[loop] == ' ') {
            blank_index = loop;
        }
    }
    buffer[blank_index] = '\0';

    /* Save the name and uuid for future use, just in case appservice was not ready yet */
    strncpy (_ise_name, buffer, _POSIX_PATH_MAX);
    strncpy (_ise_uuid, (char*)(buffer) + blank_index + 1, _POSIX_PATH_MAX);

    launch_helper (_ise_name, _ise_uuid);

    return ECORE_CALLBACK_RENEW;
}

static void run_broker (int argc, char *argv [])
{
    if (!ecore_init ()) {
        ISF_SAVE_LOG ("Failed to init ecore!!\n");
        return;
    }

    if (!ecore_ipc_init ()) {
        ISF_SAVE_LOG ("Failed to init ecore_ipc!!\n");
        ecore_shutdown ();
        return;
    }

    exit_handler = ecore_event_handler_add (ECORE_EVENT_SIGNAL_EXIT, sig_exit_cb, NULL);
    data_handler = ecore_event_handler_add (ECORE_IPC_EVENT_CLIENT_DATA, handler_client_data, NULL);

    server = ecore_ipc_server_add (ECORE_IPC_LOCAL_SYSTEM, "scim-helper-broker", 0, NULL);

    if (server == NULL) {
        ISF_SAVE_LOG ("ecore_ipc_server_add returned NULL!!\n");
    }

    ecore_main_loop_begin ();

    ecore_ipc_server_del (server);
    ecore_ipc_shutdown ();
    ecore_shutdown ();
}
// }

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
    bool socket = true;
    bool manual = false;

    int   new_argc = 0;
    char *new_argv [80];

    /* Display version info */
    cout << "Input Service Manager " << ISF_VERSION << "\n\n";

    /* Get modules list */
    scim_get_frontend_module_list (frontend_list);
    config_list.push_back ("simple");
    config_list.push_back ("socket");

    scim_get_helper_module_list (helper_list);
    if (helper_list.size () < 1) {  // If there is no IME, that is, if ime_info DB is empty...
        pkgmgrinfo_appinfo_filter_h handle;
        int ret = pkgmgrinfo_appinfo_filter_create (&handle);
        if (ret == PMINFO_R_OK) {
            ret = pkgmgrinfo_appinfo_filter_add_string (handle, PMINFO_APPINFO_PROP_APP_CATEGORY, "http://tizen.org/category/ime");
            if (ret == PMINFO_R_OK) {
                ret = pkgmgrinfo_appinfo_filter_foreach_appinfo (handle, _ime_app_list_cb, NULL);
            }
            pkgmgrinfo_appinfo_filter_destroy (handle);
        }

        scim_get_helper_module_list (helper_list);  // Assuming ime_info DB is initialized, try again.
    }

    scim_get_imengine_module_list (engine_list);

    for (it = engine_list.begin (); it != engine_list.end (); it++) {
        all_engine_list.push_back (*it);
        if (*it != "socket")
            load_engine_list.push_back (*it);
    }
    for (it = helper_list.begin (); it != helper_list.end (); it++) {
        all_engine_list.push_back (*it);
        load_engine_list.push_back (*it);
    }

    /* Use x11 FrontEnd as default if available. */
    if (frontend_list.size ()) {
        def_frontend = String ("x11");
        if (std::find (frontend_list.begin (),
                       frontend_list.end (),
                       def_frontend) == frontend_list.end ())
            def_frontend = frontend_list [0];
    }

    /* Use simple Config module as default if available. */
    def_config = scim_global_config_read (SCIM_GLOBAL_CONFIG_DEFAULT_CONFIG_MODULE, String ("simple"));
    if (std::find (config_list.begin (),
                   config_list.end (),
                   def_config) == config_list.end ())
        def_config = config_list [0];

    /* If no Socket Config/IMEngine/FrontEnd modules */
    /* then do not try to start a SocketFrontEnd. */
    if (std::find (frontend_list.begin (), frontend_list.end (), "socket") == frontend_list.end () ||
        std::find (config_list.begin (), config_list.end (), "socket") == config_list.end () ||
        std::find (engine_list.begin (), engine_list.end (), "socket") == engine_list.end ())
        socket = false;

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
            for (it = config_list.begin (); it != config_list.end (); it++) {
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

    ISF_SAVE_LOG ("ppid:%d Starting SCIM......\n", getppid ());

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

    if (!def_frontend.length ()) {
        cerr << "No FrontEnd module is available!\n";
        return -1;
    }

    if (!def_config.length ()) {
        cerr << "No Config module is available!\n";
        return -1;
    }

    /* If you try to use the socket feature manually,
       then let you do it by yourself. */
    if (def_frontend == "socket" || def_config == "socket" ||
        std::find (load_engine_list.begin (), load_engine_list.end (), "socket") != load_engine_list.end ())
        socket = false;

    /* If the socket address of SocketFrontEnd and SocketIMEngine/SocketConfig are different,
       then do not try to start the SocketFrontEnd instance automatically. */
    if (scim_get_default_socket_frontend_address () != scim_get_default_socket_imengine_address () ||
        scim_get_default_socket_frontend_address () != scim_get_default_socket_config_address ())
        socket = false;

    /* Try to start a SocketFrontEnd daemon first. */
    if (socket) {
        ISF_SAVE_LOG ("ppid:%d Now socket frontend......\n", getppid ());

        /* If no Socket FrontEnd is running, then launch one.
           And set manual to false. */
        try {
            if (!check_socket_frontend ()) {
                cerr << "Launching a daemon with Socket FrontEnd...\n";
                char *l_argv [] = { const_cast<char *> ("--stay"), 0 };
                scim_launch (true,
                            def_config,
                            (load_engine_list.size () ? scim_combine_string_list (load_engine_list, ',') : "none"),
                            "socket",
                            l_argv);
                manual = false;
            }
        } catch (scim::Exception &e) {
        }

        /* If there is one Socket FrontEnd running and it's not manual mode,
           then just use this Socket Frontend. */
        try {
            if (!manual) {
                for (int j = 0; j < 100; ++j) {
                    if (check_socket_frontend ()) {
                        def_config = "socket";
                        load_engine_list.clear ();
                        load_engine_list.push_back ("socket");
                        break;
                    }
                    scim_usleep (100000);
                }
            }
        } catch (scim::Exception &e) {}
    }

    cerr << "Launching a process with " << def_frontend << " FrontEnd...\n";

    ISF_SAVE_LOG ("ppid:%d Now default frontend......\n", getppid ());

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

        ISF_SAVE_LOG ("ppid:%d Now checking panel process......\n", getppid ());

        /* When finished launching scim-launcher, let's create panel process also, for the default display :0 */
        try {
            if (!check_panel ("")) {
               cerr << "Launching Panel...\n";
               ISF_SAVE_LOG ("ppid:%d Launching panel process......%s\n", getppid (), def_config.c_str ());

               scim_launch_panel (true, "socket", "", NULL);
            }
        } catch (scim::Exception & e) {
            cerr << e.what () << "\n";
            ISF_SAVE_LOG ("Fail to launch panel. error: %s\n", e.what ());
        }

        run_broker (argc, argv);

        return 0;
    }

    if (daemon)
        cerr << "Failed to launch ISM.\n";
    else
        cerr << "ISM has exited abnormally.\n";

    run_broker (argc, argv);

    return 1;
}

/*
vi:ts=4:ai:nowrap:expandtab
*/
