/*
 * Copyright (c) 2012 - 2015 Samsung Electronics Co., Ltd All Rights Reserved
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

var WebHelperClient = {
    /* Web IME Initializer - first call this function with a handler object that contains the
     * callback functions listed below, starting with "on*" prefix, to be acknowledged when
     * input related events occurs. */
    initialize : function (handler) {
        this.handler = handler;
        if (this.impl === null) {
            this.impl = new this.WebHelperClientInternal(this);
        }
        this.impl.activate();
    },

    /* IME -> ISF functions */
    log : function(str) {
        this.impl.log(str);
    },
    commitString : function(str) {
        this.impl.commitString(str);
    },
    updatePreeditString : function(str) {
        this.impl.updatePreeditString(str);
    },
    sendKeyEvent : function(code) {
        this.impl.sendKeyEvent(code);
    },
    sendMouse_KeyEvent : function(code) {
        this.impl.sendMouse_KeyEvent(code);
    },
    sendMouse_MoveEvent : function(code) {
        this.impl.sendMouse_MoveEvent(code);
    },
    sendWheel_MoveEvent : function(code) {
        this.impl.sendWheel_MoveEvent(code);
    },
    sendAir_Input : function(code) {
        this.impl.sendAir_Input(code);
    },
    sendAir_Setting : function(code) {
        this.impl.sendAir_Setting(code);
    },
    forwardKeyEvent : function(code) {
        this.impl.forwardKeyEvent(code);
    },
    setKeyboardSizes : function(portraitWidth, portraitHeight, landscapeWidth, landscapeHeight) {
        this.impl.setKeyboardSizes(portraitWidth, portraitHeight, landscapeWidth, landscapeHeight);
    },

    /* ISF -> IME callbacks */
    onInit : function() {
        if (typeof this.handler.onInit === 'function') {
            this.handler.onInit();
        }
    },
    onExit : function() {
        if (typeof this.handler.onExit === 'function') {
            this.handler.onExit();
        }
    },
    onFocusIn : function(inputContext) {
        if (typeof this.handler.onFocusIn === 'function') {
            this.handler.onFocusIn(inputContext);
        }
    },
    onFocusOut : function(inputContext) {
        if (typeof this.handler.onFocusOut === 'function') {
            this.handler.onFocusOut(inputContext);
        }
    },

    onShow : function(inputContext) {
        if (typeof this.handler.onShow === 'function') {
            this.handler.onShow(inputContext);
        }
    },
    onHide : function(inputContext) {
        if (typeof this.handler.onHide === 'function') {
            this.handler.onHide(inputContext);
        }
    },

    onSetRotation : function(inputContext) {
        if (typeof this.handler.onSetRotation === 'function') {
            this.handler.onSetRotation(inputContext);
        }
    },

    onUpdateCursorPosition : function(inputContext, position) {
        if (typeof this.handler.onUpdateCursorPosition === 'function') {
            this.handler.onUpdateCursorPosition(inputContext, position);
        }
    },

    onSetLanguage : function(language) {
        if (typeof this.handler.onSetLanguage === 'function') {
            this.handler.onSetLanguage(language);
        }
    },

    onSetImdata : function(imdata) {
        if (typeof this.handler.onSetImdata === 'function') {
            this.handler.onSetImdata(imdata);
        }
    },
    onGetImdata : function() {
        var imdata = "";
        if (typeof this.handler.onGetImdata === 'function') {
            imdata = this.handler.onGetImdata();
        }
        this.impl.replyGetImdata(imdata);
    },

    onSetReturnKeyType : function(type) {
        if (typeof this.handler.onSetReturnKeyType === 'function') {
            this.handler.onSetReturnKeyType(type);
        }
    },
    onGetReturnKeyType : function() {
        if (typeof this.handler.onGetReturnKeyType === 'function') {
            this.handler.onGetReturnKeyType();
        }
    },

    onSetReturnKeyDisable : function(disabled) {
        if (typeof this.handler.onSetReturnKeyDisable === 'function') {
            this.handler.onSetReturnKeyDisable(disabled);
        }
    },
    onGetReturnKeyDisable : function() {
        if (typeof this.handler.onGetReturnKeyDisable === 'function') {
            this.handler.onGetReturnKeyDisable();
        }
    },

    onSetLayout : function(layout) {
        if (typeof this.handler.onSetLayout === 'function') {
            this.handler.onSetLayout(layout);
        }
    },
    onGetLayout : function() {
        if (typeof this.handler.onGetLayout === 'function') {
            this.handler.onGetLayout();
        }
    },

    onResetInputContext : function(inputContext) {
        if (typeof this.handler.onResetInputContext === 'function') {
            this.handler.onResetInputContext(inputContext);
        }
    },

    onProcessKeyEvent : function(code, mask, layout) {
        var processed = false;
        if (typeof this.handler.onProcessKeyEvent === 'function') {
            processed = this.handler.onProcessKeyEvent(code, mask, layout);
        }
        this.impl.replyProcessKeyEvent(processed);
    },

    /* Internal implementation, no need to check for keyboard developers */
    impl : null,
    handler : null,
    WebHelperClientInternal: function (client) {

        this.MessageTypes = {
            PLAIN:"plain",
            QUERY:"query",
            REPLY:"reply"
        };

        this.MessageCommands = {
            INIT:"init",
            EXIT:"exit",

            FOCUS_IN:"focus_in",
            FOCUS_OUT:"focus_out",
            SHOW:"show",
            HIDE:"hide",
            SET_ROTATION:"set_rotation",
            UPDATE_CURSOR_POSITION:"update_cursor_position",
            SET_LANGUAGE:"set_language",
            SET_IMDATA:"set_imdata",
            GET_IMDATA:"get_imdata",
            SET_RETURN_KEY_TYPE:"set_return_key_type",
            GET_RETURN_KEY_TYPE:"get_return_key_type",
            SET_RETURN_KEY_DISABLE:"set_return_key_disable",
            GET_RETURN_KEY_DISABLE:"get_return_key_disable",
            SET_LAYOUT:"set_layout",
            GET_LAYOUT:"get_layout",
            RESET_INPUT_CONTEXT:"reset_input_context",
            PROCESS_KEY_EVENT:"process_key_event",

            LOG:"log",
            COMMIT_STRING:"commit_string",
            UPDATE_PREEDIT_STRING:"update_preedit_string",
            SEND_KEY_EVENT:"send_key_event",
            SEND_MOUSE_KEY:"send_mouse_key",
            SEND_MOUSE_MOVE:"send_mouse_move",
            SEND_WHEEL_MOVE:"send_wheel_move",
            SEND_AIR_INPUT:"send_air_input",
            SEND_AIR_SETTING:"send_air_setting",
            FORWARD_KEY_EVENT:"forward_key_event",
            SET_KEYBOARD_SIZES:"set_keyboard_sizes",
            CONNECT:"connect"
        };

        this.log = function(str) {
            if (this.socket !== "undefined") {
                this.socket.send(
                    this.MessageTypes.PLAIN + "|" +
                    this.MessageCommands.LOG + "|" +
                    str);
            }
        };
        this.commitString = function(str) {
            if (this.socket !== "undefined") {
                this.socket.send(
                    this.MessageTypes.PLAIN + "|" +
                    this.MessageCommands.COMMIT_STRING + "|" +
                    str);
            }
        };
        this.updatePreeditString = function(str) {
            if (this.socket !== "undefined") {
                this.socket.send(
                    this.MessageTypes.PLAIN + "|" +
                    this.MessageCommands.UPDATE_PREEDIT_STRING + "|" +
                    str);
            }
        };
        this.sendKeyEvent = function(code) {
            if (this.socket !== "undefined") {
                this.socket.send(
                    this.MessageTypes.PLAIN + "|" +
                    this.MessageCommands.SEND_KEY_EVENT + "|" +
                    code);
            }
        };
        this.sendMouse_KeyEvent = function(code) {
            if (this.socket !== "undefined") {
                this.socket.send(
                    this.MessageTypes.PLAIN + "|" +
                    this.MessageCommands.SEND_MOUSE_KEY + "|" +
                    code);
            }
        };
        this.sendMouse_MoveEvent = function(code) {
            if (this.socket !== "undefined") {
                this.socket.send(
                    this.MessageTypes.PLAIN + "|" +
                    this.MessageCommands.SEND_MOUSE_MOVE + "|" +
                    code);
            }
        };
        this.sendWheel_MoveEvent = function(code) {
            if (this.socket !== "undefined") {
                this.socket.send(
                    this.MessageTypes.PLAIN + "|" +
                    this.MessageCommands.SEND_WHEEL_MOVE + "|" +
                    code);
            }
        };
        this.sendAir_Input = function(code) {
            if (this.socket !== "undefined") {
                this.socket.send(
                    this.MessageTypes.PLAIN + "|" +
                    this.MessageCommands.SEND_AIR_INPUT + "|" +
                    code);
            }
        };
        this.sendAir_Setting = function(code) {
            if (this.socket !== "undefined") {
                this.socket.send(
                    this.MessageTypes.PLAIN + "|" +
                    this.MessageCommands.SEND_AIR_SETTING + "|" +
                    code);
            }
        };

        this.forwardKeyEvent = function(code) {
            if (this.socket !== "undefined") {
                this.socket.send(
                    this.MessageTypes.PLAIN + "|" +
                    this.MessageCommands.FORWARD_KEY_EVENT + "|" +
                    code);
            }
        };
        this.setKeyboardSizes = function(portraitWidth, portraitHeight, landscapeWidth, landscapeHeight) {
            if (this.socket !== "undefined") {
                this.socket.send(
                    this.MessageTypes.PLAIN + "|" +
                    this.MessageCommands.SET_KEYBOARD_SIZES + "|" +
                    portraitWidth + "|" + portraitHeight + "|" +
                    landscapeWidth + "|" + landscapeHeight);
            }
        };
        this.replyGetImdata = function(data) {
            if (this.socket !== "undefined") {
                this.socket.send(
                    this.MessageTypes.REPLY + "|" +
                    this.MessageCommands.GET_IMDATA + "|" +
                    data);
            }
        };
        this.replyProcessKeyEvent = function(ret) {
            if (this.socket !== "undefined") {
                this.socket.send(
                    this.MessageTypes.REPLY + "|" +
                    this.MessageCommands.PROCESS_KEY_EVENT + "|" +
                    ret);
            }
        };
        this.defaultHandler = function (items) {
            if (items[1] === this.MessageCommands.INIT) {
                client.onInit();
            }
            if (items[1] === this.MessageCommands.EXIT) {
                client.onExit();
            }
            if (items[1] === this.MessageCommands.FOCUS_IN) {
                client.onFocusIn(items[2]);
            }
            if (items[1] === this.MessageCommands.FOCUS_OUT) {
                client.onFocusOut(items[2]);
            }

            if (items[1] === this.MessageCommands.SHOW) {
                client.onShow(items[2]);
            }
            if (items[1] === this.MessageCommands.HIDE) {
                client.onHide(items[2]);
            }

            if (items[1] === this.MessageCommands.SET_ROTATION) {
                client.onSetRotation(items[2]);
            }

            if (items[1] === this.MessageCommands.UPDATE_CURSOR_POSITION) {
                client.onUpdateCursorPosition(items[2], items[3]);
            }

            if (items[1] === this.MessageCommands.SET_LANGUAGE) {
                client.onSetLanguage(items[2]);
            }

            if (items[1] === this.MessageCommands.SET_IMDATA) {
                client.onSetImdata(items[2], items[3]);
            }
            if (items[1] === this.MessageCommands.GET_IMDATA) {
                client.onGetImdata();
            }

            if (items[1] === this.MessageCommands.SET_RETURN_KEY_TYPE) {
                client.onSetReturnKeyType(items[2]);
            }
            if (items[1] === this.MessageCommands.GET_RETURN_KEY_TYPE) {
                client.onGetReturnKeyType();
            }

            if (items[1] === this.MessageCommands.SET_RETURN_KEY_DISABLE) {
                client.onSetReturnKeyDisable(items[2]);
            }
            if (items[1] === this.MessageCommands.GET_RETURN_KEY_DISABLE) {
                client.onGetReturnKeyDisable();
            }

            if (items[1] === this.MessageCommands.SET_LAYOUT) {
                client.onSetLayout(items[2]);
            }
            if (items[1] === this.MessageCommands.GET_LAYOUT) {
                client.onGetLayout();
            }

            if (items[1] === this.MessageCommands.RESET_INPUT_CONTEXT) {
                client.onResetInputContext(items[2]);
            }

            if (items[1] === this.MessageCommands.PROCESS_KEY_EVENT) {
                client.onProcessKeyEvent(items[2], items[3], items[4]);
            }
        };

        this.getAppropriateWsUrl = function() {

            var pcol;
            var u = document.URL;


            if (u.substring(0, 5) == "https") {
                pcol = "wss://";
                u = u.substr(8);
            } else {
                pcol = "ws://";
                if (u.substring(0, 4) == "http")
                    u = u.substr(7);
            }

            u = u.split('/');
            return pcol + u[0];
        };

        this.connectWebSocket = function() {
            if (typeof MozWebSocket != "undefined") {
                this.socket =
                    new MozWebSocket(this.getAppropriateWsUrl(), "keyboard-protocol");
            } else {
                this.socket =
                    new WebSocket(this.getAppropriateWsUrl(), "keyboard-protocol");
            }
        };

        this.activate = function() {
            this.connectWebSocket();
            this.registerHandlers(this);
        };

        this.registerHandlers = function(handler) {
            try {
                this.socket.onopen = function() {
                    this.send(
                    handler.MessageTypes.PLAIN + "|" +
                    handler.MessageCommands.CONNECT + "|" +
                    "websocket");
                };

                this.socket.onmessage = function(msg) {
                    var items = msg.data.split("|");
                    handler.defaultHandler(items);
                };

                this.socket.onclose = function(evt) {
                    /* Try to reconnect if disconnected uncleanly */
                    if (evt.wasClean === false) {
                        /*
                        setTimeout((function(handler) {
                            alert("connecting again!");
                            this.connectWebSocket();
                            this.registerHandlers(this);
                        }).call(handler), 500);
                        */
                    }
                };
            } catch(exception) {
                alert(exception);
            }
        };
    }
};
