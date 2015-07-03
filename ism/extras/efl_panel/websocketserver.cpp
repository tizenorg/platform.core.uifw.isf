/*
 * Copyright (c) 2012 - 2016 Samsung Electronics Co., Ltd All Rights Reserved
 *
 * Licensed under the Apache License, Version 2.0 (the License);
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#ifdef CMAKE_BUILD
#include "lws_config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#include <dlog.h>
#include <feedback.h>

#include "websocketserver.h"
#include "remote_input.h"

#include <syslog.h>

#include <signal.h>

#include <libwebsockets.h>

pthread_t g_ws_server_thread = (pthread_t)NULL;
pthread_mutex_t g_ws_server_mutex = PTHREAD_MUTEX_INITIALIZER;

pthread_cond_t g_ws_query_condition = PTHREAD_COND_INITIALIZER;
pthread_mutex_t g_ws_query_mutex = PTHREAD_MUTEX_INITIALIZER;

bool g_ws_server_exit = false;
struct libwebsocket_context *g_ws_server_context = NULL;

WebSocketServer* WebSocketServer::m_current_instance = NULL;
static Remote_Input* remote_input_impl= NULL;

int force_exit = 0;

extern unsigned int preedit_from_remote;
//extern void flush_immengine_by_remote();

enum demo_protocols {
    /* always first */
    PROTOCOL_HTTP = 0,

    PROTOCOL_KEYBOARD,

    /* always last */
    MAX_PROTOCOL_COUNT
};
struct serveable {
    const char *urlpath;
    const char *mimetype;
};

static const struct serveable whitelist[] = {
    { "/favicon.ico", "image/x-icon" },
    { "/libwebsockets.org-logo.png", "image/png" },

    { "/tv.htm", "text/html" },
    { "/air.htm", "text/html" },
    { "/airsetting.htm", "text/html" },
    { "/tv2.htm", "text/html" },
    { "/game_mode.htm", "text/html" },
    { "/airinput.htm", "text/html" },
    { "/ajaxCaller.js", "text/javascript" },
    { "/remote_input.js", "text/javascript" },
    { "/remote_input.css", "text/css" },
    { "/remote_input_game.css", "text/css" },
    { "/util.js", "text/javascript" },
    { "/jquery-2.0.2.min.js", "text/javascript" },
    { "/web-helper-client.js", "text/javascript" },
    { "/web_index.html", "text/html" },
    { "/style.css", "text/css" },
    { "/test.htm", "text/html" },
    { "/testpage.htm", "text/html" },
    { "/avahi.htm", "text/html" },

    { "/imgs/chlist_pressed.png", "image/png" },
    { "/imgs/channel.png", "image/png" },
    { "/imgs/channel_up.png", "image/png" },
    { "/imgs/channel_up_pressed.png", "image/png" },
    { "/imgs/channel_down.png", "image/png" },
    { "/imgs/channel_down_pressed.png", "image/png" },
    { "/imgs/back.png", "image/png" },
    { "/imgs/back_pressed.png", "image/png" },
    { "/imgs/apps.png", "image/png" },
    { "/imgs/apps_pressed.png", "image/png" },
    { "/shortcut_icon.png", "image/png" },
    { "/imgs/chlist.png", "image/png" },
    { "/imgs/exit_pressed.png", "image/png" },
    { "/imgs/exit.png", "image/png" },
    { "/imgs/info_pressed.png", "image/png" },
    { "/imgs/info.png", "image/png" },
    { "/imgs/menu_pressed.png", "image/png" },
    { "/imgs/menu.png", "image/png" },
    { "/imgs/modeswitcher_pressed.png", "image/png" },
    { "/imgs/modeswitcher_to_mouse.png", "image/png" },
    { "/imgs/modeswitcher_to_tv.png", "image/png" },
    { "/imgs/modeswitcher.png", "image/png" },
    { "/imgs/mouse_panel_bg.png", "image/png" },
    { "/imgs/mute_pressed.png", "image/png" },
    { "/imgs/mute.png", "image/png" },
    { "/imgs/power_pressed.png", "image/png" },
    { "/imgs/power.png", "image/png" },
    { "/imgs/remote_keyboard_logo.png", "image/png" },
    { "/imgs/remotekeyboard_bg.png", "image/png" },
    { "/imgs/remotekeyboard_remocon_bg.png", "image/png" },
    { "/imgs/remotekeyboard_remocon_bg3.png", "image/png" },
    { "/imgs/return_pressed.png", "image/png" },
    { "/imgs/return.png", "image/png" },
    { "/imgs/scrollbar.png", "image/png" },
    { "/imgs/source_pressed.png", "image/png" },
    { "/imgs/source.png", "image/png" },
    { "/imgs/volume_down_pressed.png", "image/png" },
    { "/imgs/volume_down.png", "image/png" },
    { "/imgs/volume_up_pressed.png", "image/png" },
    { "/imgs/volume_up.png", "image/png" },
    { "/imgs/volume.png", "image/png" },
    { "/imgs/air_bt.png", "image/png" },
    { "/imgs/air_bt_pressed.png", "image/png" },
    { "/imgs/reset_bt.png", "image/png" },
    { "/imgs/reset_bt_pressed.png", "image/png" },
    { "/imgs/air_panel_bg.png", "image/png" },
    { "/imgs/a_bt.png", "image/png" },
    { "/imgs/a_bt_pressed.png", "image/png" },
    { "/imgs/b_bt.png", "image/png" },
    { "/imgs/b_bt_pressed.png", "image/png" },
    { "/imgs/c_bt.png", "image/png" },
    { "/imgs/c_bt_pressed.png", "image/png" },

    { "/imgs/game_a.png", "image/png" },
    { "/imgs/game_a_pressed.png", "image/png" },
    { "/imgs/game_b.png", "image/png" },
    { "/imgs/game_b_pressed.png", "image/png" },
    { "/imgs/game_c.png", "image/png" },
    { "/imgs/game_c_pressed.png", "image/png" },
    { "/imgs/game_d.png", "image/png" },
    { "/imgs/game_d_pressed.png", "image/png" },
    { "/imgs/game_up.png", "image/png" },
    { "/imgs/game_up_pressed.png", "image/png" },
    { "/imgs/game_down.png", "image/png" },
    { "/imgs/game_down_pressed.png", "image/png" },
    { "/imgs/game_left.png", "image/png" },
    { "/imgs/game_left_pressed.png", "image/png" },
    { "/imgs/game_right.png", "image/png" },
    { "/imgs/game_right_pressed.png", "image/png" },
    { "/imgs/game_menu.png", "image/png" },
    { "/imgs/game_menu_pressed.png", "image/png" },
    { "/imgs/game_exit.png", "image/png" },
    { "/imgs/game_exit_pressed.png", "image/png" },
    { "/imgs/game_title.png", "image/png" },
    { "/imgs/game_logo.png", "image/png" },
    { "/imgs/game_background.png", "image/png" },
    { "/imgs/game_air.png", "image/png" },
    { "/imgs/game_air_pressed.png", "image/png" },
    { "/imgs/game_set.png", "image/png" },
    { "/imgs/game_set_pressed.png", "image/png" },
    { "/imgs/game_select.png", "image/png" },
    { "/imgs/game_select_pressed.png", "image/png" },

    /* last one is the default served if no match */
    { "/tv.htm", "text/html" },
};
#define LOCAL_RESOURCE_PATH "/usr/share/scim/remote-input"

/*
 * We take a strict whitelist approach to stop ../ attacks
 */

struct per_session_data__http {
    int fd;
};

/* this protocol server (always the first one) just knows how to do HTTP */

static int callback_http(struct libwebsocket_context *context,
        struct libwebsocket *wsi,
        enum libwebsocket_callback_reasons reason, void *user,
                               void *in, size_t len)
{
    LOGD(" ");
#if 0
    char client_name[128];
    char client_ip[128];
#endif
    char buf[256];
    int n, m;
    unsigned char *p;
    static unsigned char buffer[4096];
    struct stat stat_buf;
    struct per_session_data__http *pss = (struct per_session_data__http *)user;
#ifdef EXTERNAL_POLL
    int fd = (int)(long)in;
#endif

    switch (reason) {
    case LWS_CALLBACK_HTTP:

        /* check for the "send a big file by hand" example case */

        if (!strcmp((const char *)in, "/leaf.jpg")) {

            /* well, let's demonstrate how to send the hard way */

            p = buffer;

            pss->fd = open(LOCAL_RESOURCE_PATH"/leaf.jpg", O_RDONLY);
            if (pss->fd < 0)
                return -1;

            fstat(pss->fd, &stat_buf);

            /*
             * we will send a big jpeg file, but it could be
             * anything.  Set the Content-Type: appropriately
             * so the browser knows what to do with it.
             */

            p += sprintf((char *)p,
                "HTTP/1.0 200 OK\x0d\x0a"
                "Server: libwebsockets\x0d\x0a"
                "Content-Type: image/jpeg\x0d\x0a"
                    "Content-Length: %u\x0d\x0a\x0d\x0a",
                    (unsigned int)stat_buf.st_size);

            /*
             * send the http headers...
             * this won't block since it's the first payload sent
             * on the connection since it was established
             * (too small for partial)
             */

            n = libwebsocket_write(wsi, buffer,
                   p - buffer, LWS_WRITE_HTTP);

            if (n < 0) {
                close(pss->fd);
                return -1;
            }
            /*
             * book us a LWS_CALLBACK_HTTP_WRITEABLE callback
             */
            libwebsocket_callback_on_writable(context, wsi);
            break;
        }

        /* if not, send a file the easy way */

        for (n = 0; n < (sizeof(whitelist) / sizeof(whitelist[0]) - 1); n++)
            if (in && strcmp((const char *)in, whitelist[n].urlpath) == 0)
                break;

        sprintf(buf, LOCAL_RESOURCE_PATH"%s", whitelist[n].urlpath);

        if (libwebsockets_serve_http_file(context, wsi, buf, whitelist[n].mimetype))
            return -1; /* through completion or error, close the socket */

        /*
         * notice that the sending of the file completes asynchronously,
         * we'll get a LWS_CALLBACK_HTTP_FILE_COMPLETION callback when
         * it's done
         */

        break;

    case LWS_CALLBACK_HTTP_FILE_COMPLETION:
//      lwsl_info("LWS_CALLBACK_HTTP_FILE_COMPLETION seen\n");
        /* kill the connection after we sent one file */
        return -1;

    case LWS_CALLBACK_HTTP_WRITEABLE:
        /*
         * we can send more of whatever it is we were sending
         */

        do {
            n = read(pss->fd, buffer, sizeof buffer);
            /* problem reading, close conn */
            if (n < 0)
                goto bail;
            /* sent it all, close conn */
            if (n == 0)
                goto bail;
            /*
             * because it's HTTP and not websocket, don't need to take
             * care about pre and postamble
             */
            m = libwebsocket_write(wsi, buffer, n, LWS_WRITE_HTTP);
            if (m < 0)
                /* write failed, close conn */
                goto bail;
            if (m != n)
                /* partial write, adjust */
                lseek(pss->fd, m - n, SEEK_CUR);

        } while (!lws_send_pipe_choked(wsi));
        libwebsocket_callback_on_writable(context, wsi);
        break;

bail:
        close(pss->fd);
        return -1;

    /*
     * callback for confirming to continue with client IP appear in
     * protocol 0 callback since no websocket protocol has been agreed
     * yet.  You can just ignore this if you won't filter on client IP
     * since the default uhandled callback return is 0 meaning let the
     * connection continue.
     */

    case LWS_CALLBACK_FILTER_NETWORK_CONNECTION:
#if 0
        libwebsockets_get_peer_addresses(context, wsi, (int)(long)in, client_name,
                 sizeof(client_name), client_ip, sizeof(client_ip));

        fprintf(stderr, "Received network connect from %s (%s)\n",
                            client_name, client_ip);
#endif
        /* if we returned non-zero from here, we kill the connection */
        break;

#ifdef EXTERNAL_POLL
    /*
     * callbacks for managing the external poll() array appear in
     * protocol 0 callback
     */

    case LWS_CALLBACK_ADD_POLL_FD:

        if (count_pollfds >= max_poll_elements) {
            lwsl_err("LWS_CALLBACK_ADD_POLL_FD: too many sockets to track\n");
            return 1;
        }

        fd_lookup[fd] = count_pollfds;
        pollfds[count_pollfds].fd = fd;
        pollfds[count_pollfds].events = (int)(long)len;
        pollfds[count_pollfds++].revents = 0;
        break;

    case LWS_CALLBACK_DEL_POLL_FD:
        if (!--count_pollfds)
            break;
        m = fd_lookup[fd];
        /* have the last guy take up the vacant slot */
        pollfds[m] = pollfds[count_pollfds];
        fd_lookup[pollfds[count_pollfds].fd] = m;
        break;

    case LWS_CALLBACK_SET_MODE_POLL_FD:
        pollfds[fd_lookup[fd]].events |= (int)(long)len;
        break;

    case LWS_CALLBACK_CLEAR_MODE_POLL_FD:
        pollfds[fd_lookup[fd]].events &= ~(int)(long)len;
        break;
#endif

    default:
        break;
    }

    return 0;
}


/* keyboard protocol */

struct per_session_data__keyboard {
    int session_id;
    int valid;
};

static int
callback_keyboard(struct libwebsocket_context *context,
struct libwebsocket *wsi,
    enum libwebsocket_callback_reasons reason,
    void *user, void *in, size_t len);

/* list of supported protocols and callbacks */
static struct libwebsocket_protocols protocols[] = {
    /* first protocol must always be HTTP handler */
    {
        "http-only",        /* name */
        callback_http,      /* callback */
        sizeof(struct per_session_data__http),  /* per_session_data_size */
        0,          /* max frame size / rx buffer */
    },
    {
        "keyboard-protocol",
        callback_keyboard,
        sizeof(struct per_session_data__keyboard),
        80,
    },
    { NULL, NULL, 0, 0 } /* terminator */
};

static int
callback_keyboard(struct libwebsocket_context *context,
            struct libwebsocket *wsi,
            enum libwebsocket_callback_reasons reason,
                           void *user, void *in, size_t len)
{
    LOGD(" %d",reason);
    static int last_session_id = 0;
    const int bufsize = 512;
    int n = 0;
    unsigned char buf[LWS_SEND_BUFFER_PRE_PADDING + bufsize +
                          LWS_SEND_BUFFER_POST_PADDING];
    unsigned char *p = &buf[LWS_SEND_BUFFER_PRE_PADDING];
    struct per_session_data__keyboard *pss = (struct per_session_data__keyboard *)user;
    WebSocketServer *agent = WebSocketServer::get_current_instance();
    switch (reason) {

    case LWS_CALLBACK_ESTABLISHED:
        pss->session_id = ++last_session_id;
        LOGD("LWS_CALLBACK_ESTABLISHED : %d", pss->session_id);
        //pss->valid = false;
        libwebsocket_callback_on_writable_all_protocol(&protocols[PROTOCOL_KEYBOARD]);
        break;

    case LWS_CALLBACK_CLOSED:
        LOGD("LWS_CALLBACK_CLOSED : %d", pss->session_id);
        break;

    case LWS_CALLBACK_SERVER_WRITEABLE:
        LOGD("LWS_CALLBACK_SERVER_WRITEABLE");
        if (agent) {
            /* We allow data tranmission only if this client is guaranteed to be valid */
                pthread_mutex_lock(&g_ws_server_mutex);
                std::queue<ISE_MESSAGE>& messages = agent->get_send_message_queue();
                while (messages.size() > 0) {
                    ISE_MESSAGE &message = messages.front();
                    //n = sprintf((char *)p, "%s %s", message.command.c_str(), message.value.c_str());
                    std::string str = CISEMessageSerializer::serialize(message);
                    LOGD("SEND_WEBSOCKET_MESSAGE : %s", str.c_str());
                    n = snprintf((char *)p, bufsize, "%s", str.c_str());
                    /* too small for partial */
                    n = libwebsocket_write(wsi, p, n, LWS_WRITE_TEXT);
                    messages.pop();
                }
                pthread_mutex_unlock(&g_ws_server_mutex);

                if (n < 0) {
                    lwsl_err("ERROR %d writing to di socket\n", n);
                    return -1;
                }
        }
        break;

    case LWS_CALLBACK_RECEIVE:
        LOGD("LWS_CALLBACK_RECEIVE");
        if (in) {
            std::string str = (const char *)in;
            LOGD("Receive MSG :|%s|", str.c_str());
            ISE_MESSAGE message = CISEMessageSerializer::deserialize(str);
/*
            if (message.command.compare(ISE_MESSAGE_COMMAND_STRINGS[ISE_MESSAGE_COMMAND_LOGIN]) == 0) {
                if (message.values.at(0).compare(CMagicKeyManager::get_magic_key()) == 0) {
                    LOGD("LOGIN successful, validating client");
                    pss->valid = true;
                } else {
                    LOGD("LOGIN failed, invalidating client");
                    pss->valid = false;
                }
            }
*/
                pthread_mutex_lock(&g_ws_server_mutex);
                std::queue<ISE_MESSAGE>& messages = agent->get_recv_message_queue();
                messages.push(message);
                pthread_mutex_unlock(&g_ws_server_mutex);

                const char *recved_message = "recved";
                ecore_pipe_write(agent->get_recv_message_pipe(), recved_message, strlen(recved_message));

                /* If we received reply message, let's send signal to wake up our main thread */
                if (message.type.compare(ISE_MESSAGE_TYPE_STRINGS[ISE_MESSAGE_TYPE_REPLY]) == 0) {
                    pthread_mutex_lock(&g_ws_query_mutex);
                    pthread_cond_signal(&g_ws_query_condition);
                    pthread_mutex_unlock(&g_ws_query_mutex);
                }
        }

        break;
    default:
        break;
    }

    return 0;
}

void *process_ws_server(void *data)
{
    LOGD(" ");
    unsigned int oldus = 0;

    while (!force_exit && !g_ws_server_exit) {
        struct timeval tv;
        gettimeofday(&tv, NULL);

        /*
         * This provokes the LWS_CALLBACK_SERVER_WRITEABLE for every
         * live websocket connection using the DUMB_INCREMENT protocol,
         * as soon as it can take more packets (usually immediately)
         */

        /*
        if (((unsigned int)tv.tv_usec - oldus) > 50000) {
            libwebsocket_callback_on_writable_all_protocol(&protocols[PROTOCOL_KEYBOARD]);
            oldus = tv.tv_usec;
        }
        */

        /*
         * If libwebsockets sockets are all we care about,
         * you can use this api which takes care of the poll()
         * and looping through finding who needed service.
         *
         * If no socket needs service, it'll return anyway after
         * the number of ms in the second argument.
         */

        libwebsocket_service(g_ws_server_context, 50);
    }
    return NULL;
}

void log_func(int level, const char *line)
{
    LOGD(" ");

}

WebSocketServer::WebSocketServer()
{
    LOGD(" ");
    if (m_current_instance != NULL) {
        LOGD("WARNING : m_current_instance is NOT NULL");
    }
    m_current_instance = this;
    m_recv_message_pipe = NULL;
}

WebSocketServer::~WebSocketServer()
{
    LOGD(" ");
    if (m_current_instance == this) {
        m_current_instance = NULL;
    }

    if (m_recv_message_pipe) {
        ecore_pipe_del(m_recv_message_pipe);
        m_recv_message_pipe = NULL;
    }
}

static void recv_message_pipe_handler(void *data, void *buffer, unsigned int nbyte)
{
    LOGD(" ");
    WebSocketServer *agent = WebSocketServer::get_current_instance();
    if (agent) {
        agent->process_recved_messages();
    }
}

bool WebSocketServer::init()
{
    LOGD(" ");
    bool ret = true;

    struct lws_context_creation_info info;

    memset(&info, 0, sizeof info);
    info.port = 7172;

    /* tell the library what debug level to emit and to send it to syslog */
    int debug_level = LLL_DEBUG;
    lws_set_log_level(debug_level, log_func);

    info.iface = NULL;
    info.protocols = protocols;
#ifndef LWS_NO_EXTENSIONS
    info.extensions = libwebsocket_get_internal_extensions();
#endif
    info.ssl_cert_filepath = NULL;
    info.ssl_private_key_filepath = NULL;
    info.gid = -1;
    info.uid = -1;
    info.options = 0;

    feedback_initialize(); // Initialize feedback

    /* The WebSocket server is running on a separate thread, and let the thread send a message
        through this pipe to guarantee thread safety */
    m_recv_message_pipe = ecore_pipe_add(recv_message_pipe_handler, NULL);

    /* Let's retry creating server context for a certain number of times */
    const int max_retry_num = 10;
    int retry_num = 0;

    do {
        g_ws_server_context = libwebsocket_create_context(&info);
        LOGD("libwebsocket context : %p", g_ws_server_context);
        sleep(1);
    } while (g_ws_server_context == NULL && retry_num++ < max_retry_num);

    pthread_mutex_init(&g_ws_server_mutex, NULL);

    pthread_mutex_init(&g_ws_query_mutex, NULL);
    pthread_cond_init(&g_ws_query_condition, NULL);

    if (g_ws_server_context) {
        pthread_create(&g_ws_server_thread, NULL, &process_ws_server, NULL);
    } else {
        ret = false;
    }

    on_init();

    remote_input_impl = Remote_Input::get_instance();

    return ret;
}

bool WebSocketServer::exit()
{
    LOGD(" ");
    on_exit();

    g_ws_server_exit = true;

    feedback_deinitialize(); // Deinitialize feedback

    if (m_recv_message_pipe) {
        ecore_pipe_del(m_recv_message_pipe);
        m_recv_message_pipe = NULL;
    }

    if(g_ws_server_thread) {
        pthread_join(g_ws_server_thread, NULL);
    }

    pthread_cond_destroy(&g_ws_query_condition);
    pthread_mutex_destroy(&g_ws_query_mutex);

    pthread_mutex_destroy(&g_ws_server_mutex);

    libwebsocket_context_destroy(g_ws_server_context);

    return true;
}

void WebSocketServer::signal(int sig)
{
    LOGD(" ");
    force_exit = 1;
}

template<class T>
std::string to_string(T i)
{
    std::stringstream ss;
    std::string s;
    ss << i;
    s = ss.str();

    return s;
}

void WebSocketServer::on_init()
{
    LOGD(" ");
    ISE_MESSAGE message;
    message.type = ISE_MESSAGE_TYPE_STRINGS[ISE_MESSAGE_TYPE_PLAIN];
    message.command = ISE_MESSAGE_COMMAND_STRINGS[ISE_MESSAGE_COMMAND_INIT];

    pthread_mutex_lock(&g_ws_server_mutex);
    m_send_message_queue.push(message);
    pthread_mutex_unlock(&g_ws_server_mutex);

    libwebsocket_callback_on_writable_all_protocol(&protocols[PROTOCOL_KEYBOARD]);
}

void WebSocketServer::on_exit()
{
    LOGD(" ");
    ISE_MESSAGE message;
    message.type = ISE_MESSAGE_TYPE_STRINGS[ISE_MESSAGE_TYPE_PLAIN];
    message.command = ISE_MESSAGE_COMMAND_STRINGS[ISE_MESSAGE_COMMAND_EXIT];

    pthread_mutex_lock(&g_ws_server_mutex);
    m_send_message_queue.push(message);
    pthread_mutex_unlock(&g_ws_server_mutex);

    libwebsocket_callback_on_writable_all_protocol(&protocols[PROTOCOL_KEYBOARD]);
}

void WebSocketServer::on_focus_in(int ic)
{
    LOGD(" ");
    ISE_MESSAGE message;
    message.type = ISE_MESSAGE_TYPE_STRINGS[ISE_MESSAGE_TYPE_PLAIN];
    message.command = ISE_MESSAGE_COMMAND_STRINGS[ISE_MESSAGE_COMMAND_FOCUS_IN];
    message.values.push_back(to_string(ic));

    pthread_mutex_lock(&g_ws_server_mutex);
    m_send_message_queue.push(message);
    pthread_mutex_unlock(&g_ws_server_mutex);

    libwebsocket_callback_on_writable_all_protocol(&protocols[PROTOCOL_KEYBOARD]);
}

void WebSocketServer::on_focus_out(int ic)
{
    LOGD(" ");
    ISE_MESSAGE message;
    message.type = ISE_MESSAGE_TYPE_STRINGS[ISE_MESSAGE_TYPE_PLAIN];
    message.command = ISE_MESSAGE_COMMAND_STRINGS[ISE_MESSAGE_COMMAND_FOCUS_OUT];
    message.values.push_back(to_string(ic));

    pthread_mutex_lock(&g_ws_server_mutex);
    m_send_message_queue.push(message);
    pthread_mutex_unlock(&g_ws_server_mutex);

    libwebsocket_callback_on_writable_all_protocol(&protocols[PROTOCOL_KEYBOARD]);
}

void WebSocketServer::on_show(int ic)
{
    LOGD(" ");
    ISE_MESSAGE message;
    message.type = ISE_MESSAGE_TYPE_STRINGS[ISE_MESSAGE_TYPE_PLAIN];
    message.command = ISE_MESSAGE_COMMAND_STRINGS[ISE_MESSAGE_COMMAND_SHOW];
    message.values.push_back(to_string(ic));

    pthread_mutex_lock(&g_ws_server_mutex);
    m_send_message_queue.push(message);
    pthread_mutex_unlock(&g_ws_server_mutex);

    libwebsocket_callback_on_writable_all_protocol(&protocols[PROTOCOL_KEYBOARD]);

    LOGD("put into send message buffer");
}

void WebSocketServer::on_hide(int ic)
{
    LOGD(" ");
    ISE_MESSAGE message;
    message.type = ISE_MESSAGE_TYPE_STRINGS[ISE_MESSAGE_TYPE_PLAIN];
    message.command = ISE_MESSAGE_COMMAND_STRINGS[ISE_MESSAGE_COMMAND_HIDE];
    message.values.push_back(to_string(ic));

    pthread_mutex_lock(&g_ws_server_mutex);
    m_send_message_queue.push(message);
    pthread_mutex_unlock(&g_ws_server_mutex);

    libwebsocket_callback_on_writable_all_protocol(&protocols[PROTOCOL_KEYBOARD]);
}

void WebSocketServer::on_set_rotation(int degree)
{
    LOGD(" ");
    ISE_MESSAGE message;
    message.type = ISE_MESSAGE_TYPE_STRINGS[ISE_MESSAGE_TYPE_PLAIN];
    message.command = ISE_MESSAGE_COMMAND_STRINGS[ISE_MESSAGE_COMMAND_SET_ROTATION];
    message.values.push_back(to_string(degree));

    pthread_mutex_lock(&g_ws_server_mutex);
    m_send_message_queue.push(message);
    pthread_mutex_unlock(&g_ws_server_mutex);

    libwebsocket_callback_on_writable_all_protocol(&protocols[PROTOCOL_KEYBOARD]);
}

void WebSocketServer::on_update_cursor_position(int ic, int cursor_pos)
{
    LOGD(" ");
    ISE_MESSAGE message;
    message.type = ISE_MESSAGE_TYPE_STRINGS[ISE_MESSAGE_TYPE_PLAIN];
    message.command = ISE_MESSAGE_COMMAND_STRINGS[ISE_MESSAGE_COMMAND_UPDATE_CURSOR_POSITION];
    message.values.push_back(to_string(ic));
    message.values.push_back(to_string(cursor_pos));

    pthread_mutex_lock(&g_ws_server_mutex);
    m_send_message_queue.push(message);
    pthread_mutex_unlock(&g_ws_server_mutex);

    libwebsocket_callback_on_writable_all_protocol(&protocols[PROTOCOL_KEYBOARD]);
}

void WebSocketServer::on_set_language(unsigned int language)
{
    LOGD(" ");
    ISE_MESSAGE message;
    message.type = ISE_MESSAGE_TYPE_STRINGS[ISE_MESSAGE_TYPE_PLAIN];
    message.command = ISE_MESSAGE_COMMAND_STRINGS[ISE_MESSAGE_COMMAND_SET_LANGUAGE];

    bool found = false;
    for (unsigned int loop = 0;loop < sizeof(ISE_LANGUAGE_TYPES) / sizeof(ISE_TYPE_VALUE_STRING);loop++) {
        if (language == ISE_LANGUAGE_TYPES[loop].type_value) {
            message.values.push_back(ISE_LANGUAGE_TYPES[loop].type_string);
            found = true;
        }
    }

    if (found) {
        pthread_mutex_lock(&g_ws_server_mutex);
        m_send_message_queue.push(message);
        pthread_mutex_unlock(&g_ws_server_mutex);

        libwebsocket_callback_on_writable_all_protocol(&protocols[PROTOCOL_KEYBOARD]);
    }
}

void WebSocketServer::on_set_imdata(char *buf, unsigned int len)
{
    LOGD(" ");
    ISE_MESSAGE message;
    message.type = ISE_MESSAGE_TYPE_STRINGS[ISE_MESSAGE_TYPE_PLAIN];
    message.command = ISE_MESSAGE_COMMAND_STRINGS[ISE_MESSAGE_COMMAND_SET_IMDATA];
    message.values.push_back(buf);

    pthread_mutex_lock(&g_ws_server_mutex);
    m_send_message_queue.push(message);
    pthread_mutex_unlock(&g_ws_server_mutex);

    libwebsocket_callback_on_writable_all_protocol(&protocols[PROTOCOL_KEYBOARD]);
}

void WebSocketServer::on_get_imdata(char **buf, unsigned int *len)
{
    LOGD(" ");
    ISE_MESSAGE message;
    message.type = ISE_MESSAGE_TYPE_STRINGS[ISE_MESSAGE_TYPE_QUERY];
    message.command = ISE_MESSAGE_COMMAND_STRINGS[ISE_MESSAGE_COMMAND_GET_IMDATA];

    pthread_mutex_lock(&g_ws_server_mutex);
    m_send_message_queue.push(message);
    pthread_mutex_unlock(&g_ws_server_mutex);

    libwebsocket_callback_on_writable_all_protocol(&protocols[PROTOCOL_KEYBOARD]);

    wait_for_reply_message();

    std::vector<std::string> values;
    /* Check if we received reply for GET_IMDATA message */
    if (process_recved_messages_until_reply_found(
        ISE_MESSAGE_COMMAND_STRINGS[ISE_MESSAGE_COMMAND_GET_IMDATA], values)) {
            if (values.size() > 0 && buf && len) {
                int string_length = values.at(0).length();
                (*buf) = new char[string_length + 1];
                if (*buf) {
                    strncpy(*buf, values.at(0).c_str(), string_length);
                    /* Make sure this is a null-terminated string */
                    *(*buf + string_length) = '\0';
                    *len = string_length;
                }
            }
    } else {
        LOGD("process_recved_messages_until_reply_found returned FALSE");
    }
    /* Now process the rest in the recv buffer */
    process_recved_messages();
}

void WebSocketServer::on_set_return_key_type(unsigned int type)
{
    LOGD(" ");
    ISE_MESSAGE message;
    message.type = ISE_MESSAGE_TYPE_STRINGS[ISE_MESSAGE_TYPE_PLAIN];
    message.command = ISE_MESSAGE_COMMAND_STRINGS[ISE_MESSAGE_COMMAND_SET_RETURN_KEY_TYPE];

    bool found = false;
    for (unsigned int loop = 0;loop < sizeof(ISE_RETURN_KEY_TYPES) / sizeof(ISE_TYPE_VALUE_STRING);loop++) {
        if (type == ISE_RETURN_KEY_TYPES[loop].type_value) {
            message.values.push_back(ISE_RETURN_KEY_TYPES[loop].type_string);
            found = true;
        }
    }

    if (found) {
        pthread_mutex_lock(&g_ws_server_mutex);
        m_send_message_queue.push(message);
        pthread_mutex_unlock(&g_ws_server_mutex);

        libwebsocket_callback_on_writable_all_protocol(&protocols[PROTOCOL_KEYBOARD]);
    }
}

void WebSocketServer::on_get_return_key_type(unsigned int *type)
{
    LOGD(" ");
    ISE_MESSAGE message;
    message.type = ISE_MESSAGE_TYPE_STRINGS[ISE_MESSAGE_TYPE_QUERY];
    message.command = ISE_MESSAGE_COMMAND_STRINGS[ISE_MESSAGE_COMMAND_GET_RETURN_KEY_TYPE];

    pthread_mutex_lock(&g_ws_server_mutex);
    m_send_message_queue.push(message);
    pthread_mutex_unlock(&g_ws_server_mutex);

    libwebsocket_callback_on_writable_all_protocol(&protocols[PROTOCOL_KEYBOARD]);

    wait_for_reply_message();

    /* Since we are accessing recved buffer, lock the server mutex again */
    pthread_mutex_lock(&g_ws_server_mutex);
    std::vector<std::string> values;
    /* Check if we received reply for GET_RETURN_KEY_TYPE message */
    if (process_recved_messages_until_reply_found(
        ISE_MESSAGE_COMMAND_STRINGS[ISE_MESSAGE_COMMAND_GET_RETURN_KEY_TYPE], values)) {
            if (type) {
                for (unsigned int loop = 0;loop < sizeof(ISE_RETURN_KEY_TYPES) / sizeof(ISE_TYPE_VALUE_STRING);loop++) {
                    if (values.at(0).compare(ISE_RETURN_KEY_TYPES[loop].type_string) == 0) {
                        *type = ISE_RETURN_KEY_TYPES[loop].type_value;
                    }
                }
            }
    }
    /* Now process the rest in the recv buffer */
    process_recved_messages();
    pthread_mutex_unlock(&g_ws_server_mutex);
}

void WebSocketServer::on_set_return_key_disable(unsigned int disabled)
{
    LOGD(" ");
    ISE_MESSAGE message;
    message.type = ISE_MESSAGE_TYPE_STRINGS[ISE_MESSAGE_TYPE_PLAIN];
    message.command = ISE_MESSAGE_COMMAND_STRINGS[ISE_MESSAGE_COMMAND_SET_RETURN_KEY_DISABLE];

    bool found = false;
    for (unsigned int loop = 0;loop < sizeof(ISE_TRUEFALSE_TYPES) / sizeof(ISE_TYPE_VALUE_STRING);loop++) {
        if (disabled == ISE_TRUEFALSE_TYPES[loop].type_value) {
            message.values.push_back(ISE_TRUEFALSE_TYPES[loop].type_string);
            found = true;
        }
    }

    if (found) {
        pthread_mutex_lock(&g_ws_server_mutex);
        m_send_message_queue.push(message);
        pthread_mutex_unlock(&g_ws_server_mutex);

        libwebsocket_callback_on_writable_all_protocol(&protocols[PROTOCOL_KEYBOARD]);
    }
}

void WebSocketServer::on_get_return_key_disable(unsigned int *disabled)
{
    LOGD(" ");
    ISE_MESSAGE message;
    message.type = ISE_MESSAGE_TYPE_STRINGS[ISE_MESSAGE_TYPE_QUERY];
    message.command = ISE_MESSAGE_COMMAND_STRINGS[ISE_MESSAGE_COMMAND_GET_RETURN_KEY_DISABLE];

    pthread_mutex_lock(&g_ws_server_mutex);
    m_send_message_queue.push(message);
    pthread_mutex_unlock(&g_ws_server_mutex);

    libwebsocket_callback_on_writable_all_protocol(&protocols[PROTOCOL_KEYBOARD]);

    wait_for_reply_message();

    /* Since we are accessing recved buffer, lock the server mutex again */
    pthread_mutex_lock(&g_ws_server_mutex);
    std::vector<std::string> values;
    /* Check if we received reply for GET_RETURN_KEY_DISABLE message */
    if (process_recved_messages_until_reply_found(
        ISE_MESSAGE_COMMAND_STRINGS[ISE_MESSAGE_COMMAND_GET_RETURN_KEY_DISABLE], values)) {
            if (disabled) {
                for (unsigned int loop = 0;loop < sizeof(ISE_TRUEFALSE_TYPES) / sizeof(ISE_TYPE_VALUE_STRING);loop++) {
                    if (values.at(0).compare(ISE_TRUEFALSE_TYPES[loop].type_string) == 0) {
                        *disabled = ISE_TRUEFALSE_TYPES[loop].type_value;
                    }
                }
            }
    }
    /* Now process the rest in the recv buffer */
    process_recved_messages();
    pthread_mutex_unlock(&g_ws_server_mutex);
}

void WebSocketServer::on_set_layout(unsigned int layout)
{
    LOGD(" ");
    ISE_MESSAGE message;
    message.type = ISE_MESSAGE_TYPE_STRINGS[ISE_MESSAGE_TYPE_PLAIN];
    message.command = ISE_MESSAGE_COMMAND_STRINGS[ISE_MESSAGE_COMMAND_SET_LAYOUT];

    bool found = false;
    for (unsigned int loop = 0;loop < sizeof(ISE_LAYOUT_TYPES) / sizeof(ISE_TYPE_VALUE_STRING);loop++) {
        if (layout == ISE_LAYOUT_TYPES[loop].type_value) {
            message.values.push_back(ISE_LAYOUT_TYPES[loop].type_string);
            found = true;
        }
    }

    if (found) {
        pthread_mutex_lock(&g_ws_server_mutex);
        m_send_message_queue.push(message);
        pthread_mutex_unlock(&g_ws_server_mutex);

        libwebsocket_callback_on_writable_all_protocol(&protocols[PROTOCOL_KEYBOARD]);
    }
}

void WebSocketServer::on_get_layout(unsigned int *layout)
{
    LOGD(" ");
    ISE_MESSAGE message;
    message.type = ISE_MESSAGE_TYPE_STRINGS[ISE_MESSAGE_TYPE_QUERY];
    message.command = ISE_MESSAGE_COMMAND_STRINGS[ISE_MESSAGE_COMMAND_GET_LAYOUT];

    pthread_mutex_lock(&g_ws_server_mutex);
    m_send_message_queue.push(message);
    pthread_mutex_unlock(&g_ws_server_mutex);

    libwebsocket_callback_on_writable_all_protocol(&protocols[PROTOCOL_KEYBOARD]);

    wait_for_reply_message();

    /* Since we are accessing recved buffer, lock the server mutex again */
    pthread_mutex_lock(&g_ws_server_mutex);
    std::vector<std::string> values;
    /* Check if we received reply for GET_LAYOUT message */
    if (process_recved_messages_until_reply_found(
        ISE_MESSAGE_COMMAND_STRINGS[ISE_MESSAGE_COMMAND_GET_LAYOUT], values)) {
            if (layout) {
                for (unsigned int loop = 0;loop < sizeof(ISE_LAYOUT_TYPES) / sizeof(ISE_TYPE_VALUE_STRING);loop++) {
                    if (values.at(0).compare(ISE_LAYOUT_TYPES[loop].type_string) == 0) {
                        *layout = ISE_LAYOUT_TYPES[loop].type_value;
                    }
                }
            }
    }
    /* Now process the rest in the recv buffer */
    process_recved_messages();
    pthread_mutex_unlock(&g_ws_server_mutex);
}

void WebSocketServer::on_reset_input_context(int ic)
{
    LOGD(" ");
    ISE_MESSAGE message;
    message.type = ISE_MESSAGE_TYPE_STRINGS[ISE_MESSAGE_TYPE_PLAIN];
    message.command = ISE_MESSAGE_COMMAND_STRINGS[ISE_MESSAGE_COMMAND_RESET_INPUT_CONTEXT];
    message.values.push_back(to_string(ic));

    pthread_mutex_lock(&g_ws_server_mutex);
    m_send_message_queue.push(message);
    pthread_mutex_unlock(&g_ws_server_mutex);

    libwebsocket_callback_on_writable_all_protocol(&protocols[PROTOCOL_KEYBOARD]);
}

void WebSocketServer::on_process_key_event(unsigned int code, unsigned int mask, unsigned int layout, unsigned int *ret)
{
    LOGD(" ");
    ISE_MESSAGE message;
    message.type = ISE_MESSAGE_TYPE_STRINGS[ISE_MESSAGE_TYPE_QUERY];
    message.command = ISE_MESSAGE_COMMAND_STRINGS[ISE_MESSAGE_COMMAND_PROCESS_KEY_EVENT];
    message.values.push_back(to_string(code));
    message.values.push_back(to_string(mask));
    message.values.push_back(to_string(layout));

    pthread_mutex_lock(&g_ws_server_mutex);
    m_send_message_queue.push(message);
    pthread_mutex_unlock(&g_ws_server_mutex);

    libwebsocket_callback_on_writable_all_protocol(&protocols[PROTOCOL_KEYBOARD]);

    wait_for_reply_message();

    /* Since we are accessing recved buffer, lock the server mutex again */
    pthread_mutex_lock(&g_ws_server_mutex);
    std::vector<std::string> values;
    /* Check if we received reply for PROCESS_KEY_EVENT message */
    if (process_recved_messages_until_reply_found(
        ISE_MESSAGE_COMMAND_STRINGS[ISE_MESSAGE_COMMAND_PROCESS_KEY_EVENT], values)) {
            if (ret) {
                for (unsigned int loop = 0;loop < sizeof(ISE_TRUEFALSE_TYPES) / sizeof(ISE_TYPE_VALUE_STRING);loop++) {
                    if (values.at(0).compare(ISE_TRUEFALSE_TYPES[loop].type_string) == 0) {
                        *ret = ISE_TRUEFALSE_TYPES[loop].type_value;
                    }
                }
            }
    }
    /* Now process the rest in the recv buffer */
    process_recved_messages();
    pthread_mutex_unlock(&g_ws_server_mutex);
}

WebSocketServer* WebSocketServer::get_current_instance()
{
    LOGD(" ");
    return m_current_instance;
}

std::queue<ISE_MESSAGE>& WebSocketServer::get_send_message_queue()
{
    LOGD(" ");
    return m_send_message_queue;
}

std::queue<ISE_MESSAGE>& WebSocketServer::get_recv_message_queue()
{
    LOGD(" ");
    return m_recv_message_queue;
}

Ecore_Pipe* WebSocketServer::get_recv_message_pipe()
{
    LOGD(" ");
    return m_recv_message_pipe;
}

void WebSocketServer::wait_for_reply_message()
{
    LOGD(" ");
    /* Let's wait for at most REPLY_TIMEOUT */
    struct timeval now;
    struct timespec timeout;
    gettimeofday(&now, NULL);
    timeout.tv_sec = now.tv_sec + REPLY_TIMEOUT.tv_sec;
    timeout.tv_nsec = (now.tv_usec + REPLY_TIMEOUT.tv_usec) * 1000;
    pthread_mutex_lock(&g_ws_query_mutex);
    pthread_cond_timedwait(&g_ws_query_condition, &g_ws_query_mutex, &timeout);
    pthread_mutex_unlock(&g_ws_query_mutex);

}

void WebSocketServer::process_recved_messages()
{
    LOGD(" ");
    pthread_mutex_lock(&g_ws_server_mutex);

    while (m_recv_message_queue.size() > 0) {
        ISE_MESSAGE &message = m_recv_message_queue.front();

        handle_recved_message(message);

        m_recv_message_queue.pop();
    }

    pthread_mutex_unlock(&g_ws_server_mutex);
}

bool WebSocketServer::process_recved_messages_until_reply_found(std::string command, std::vector<std::string> &values)
{
    LOGD(" ");

    bool ret = false;

    pthread_mutex_lock(&g_ws_server_mutex);

    while (ret == false && m_recv_message_queue.size() > 0) {
        ISE_MESSAGE &message = m_recv_message_queue.front();

        if (message.command.compare(command) == 0 &&
            message.type.compare(ISE_MESSAGE_TYPE_STRINGS[ISE_MESSAGE_TYPE_REPLY]) == 0) {
            ret = true;
            values = message.values;
        }
        handle_recved_message(message);

        m_recv_message_queue.pop();
    }

    pthread_mutex_unlock(&g_ws_server_mutex);

    return ret;
}

void WebSocketServer::handle_recved_message(ISE_MESSAGE &message)
{
    LOGD(" ");
    LOGD("Received message : %s, %s, %s", message.type.c_str(), message.command.c_str() , message.values.at(0).c_str());
    /*FIXME delte login
    if (message.command.compare(ISE_MESSAGE_COMMAND_STRINGS[ISE_MESSAGE_COMMAND_LOGIN]) == 0) {
        libwebsocket_callback_on_writable_all_protocol(&protocols[PROTOCOL_KEYBOARD]);
    }
    */

    if (remote_input_impl == NULL) {
        remote_input_impl = Remote_Input::get_instance();
    }

    remote_input_impl->handle_websocket_message(message);
}
