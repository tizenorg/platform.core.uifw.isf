document.write("<script type='text/javascript' src='ajaxCaller.js'><"+"/script>");
document.write("<script type='text/javascript' src='util.js'><"+"/script>");
document.write("<script type='text/javascript' src='jquery-2.0.2.min.js'><"+"/script>");

var openInput;

if(!openInput) openInput = {};

openInput.TV_KEY_BACKSPACE = 8;
openInput.TV_KEY_ENTER = 13;
openInput.TV_KEY_SPACE = 32;
openInput.TV_KEY_ESCAPE = 27;
openInput.TV_KEY_SELECT = 36;
openInput.TV_KEY_POWER = 124;
openInput.TV_KEY_MENU = 10001;
openInput.TV_KEY_HOME = 10002;
openInput.TV_KEY_BACK = 10003;
openInput.TV_KEY_UP = 111;
openInput.TV_KEY_DOWN = 116;
openInput.TV_KEY_LEFT = 113;
openInput.TV_KEY_RIGHT = 114;
openInput.TV_KEY_SWITCHMODE = 235;
openInput.TV_KEY_CHANNEL_LIST = 68;
openInput.TV_KEY_INFO = 69;
openInput.TV_KEY_EXIT = 182;
openInput.TV_KEY_CHANNEL_UP = 112;
openInput.TV_KEY_CHANNEL_DOWN = 117;
openInput.TV_KEY_MUTE = 121;
openInput.TV_KEY_VOLUME_DOWN = 122;
openInput.TV_KEY_VOLUME_UP = 123;

openInput.initialize = function(_app_id) {
    if (this.impl === undefined) {
        this.handler = _app_id;
        this.impl = new WebHelperClientInternal(this);
    }
    
    this.impl.activate();
};

function WebHelperClientInternal(client) {
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

    this.commitStr = function(str) {
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

//TV
openInput.tv = {};

(function(window)
{   
    var MOUSE_CLICK = 555;

    var pre_x = 0;
    var pre_y = 0;
    var touch_Pressed=0;
    var touch_Moved=0;
    var scroll_pre_x = 0;
    var scroll_pre_y = 0;

    var id_num = 0;
    var count = 0;

    var flush_timeout;
    var cal_flush_timeout;
    var CAL_FLUSH_TIMEOUT = 3000;
    var FLUSH_TIMEOUT = 3000;
    var FLUSH_ENTER_TIMEOUT = 50;
    var click_start_time = 0;

    var pre_str = "";
    var pre_pre_str = "";
    var TMP_CHAR = " ";
    var TMP_TIMESTAMP = 0;
    var EMPTY_CHECKER_TIMEOUT = 10;
    var latest_typed_timestamp = 0;
    var cur_air_mode = 0;
    var cur_reset_mode = 0;
    var cur_touch_mode = 0;
    var forceEnable_air = 0;
    var touchmove_delta;
    var air_delta;
    var gry_basic_a = -0.043;
    var gry_basic_b = 0.005;
    var gry_basic_g = -0.002;
    var gry_sum_a = 0;
    var gry_sum_b = 0;
    var gry_sum_g = 0;
    var gry_sum_count = 0;
    var progress = ["",".","..","...","...."];
    var progress_count=0;
    var cur_mode = 0;
    
    var entry_flag = 0;
    var entry_id = "";

    openInput.tv.sendKeyEvent = function(_code)
    {
        event.preventDefault();
        openInput.impl.sendKeyEvent(_code);
    };

    function sendMouse_MoveEvent(_coordinate) {
        event.preventDefault();
        if (event.touches.length < 2) {
            openInput.impl.sendMouse_MoveEvent(_coordinate);
        }
    }

    function sendMouse_KeyEvent(_mouseCode) {
        event.preventDefault();
        if (event.touches.length < 2) {
            openInput.impl.sendMouse_KeyEvent(_mouseCode);
        }
    }

    function check_entry() {
        if(document.activeElement == document.getElementById(entry_id))
            return 1;
        else
            return 0;
    }
    
    openInput.tv.setMousepad = function(_mousepad_id) {
        $(document).on("touchstart", "#"+_mousepad_id, function() {
            event.preventDefault();
            click_start_time = new Date;
            pre_x = event.touches[0].pageX;
            pre_y = event.touches[0].pageY;
            touch_Pressed=1;
            touch_Moved=0;

            if (check_entry()) {
                sendFlushCurStr();
                touch_Pressed=0;
                document.getElementById(entry_id).blur();
                return;
            }
            /*
            if(document.activeElement == document.getElementById(entry_id)) {
                sendFlushCurStr();
                touch_Pressed=0;
                return;
            }*/
        });

        $(document).on("touchmove", "#"+_mousepad_id, function() {
            event.preventDefault();
            
            var coordinate;
            touch_Moved=1;
            event.preventDefault();
            if((pre_x==0) && (pre_y==0)) {
                pre_x = event.touches[0].pageX;
                pre_y = event.touches[0].pageY;
                return;
            }
            if((event.touches[0].pageX-pre_x == 0) && (event.touches[0].pageY-pre_y == 0)) {
                return;
            }

            if(check_entry()) {
                touch_Moved=0;
                touch_Pressed=0;
                document.getElementById(entry_id).blur();
                return;
            }
            touchmove_delta = parseInt(Math.sqrt(((event.touches[0].pageX-pre_x) * (event.touches[0].pageX-pre_x)) + ((event.touches[0].pageY-pre_y)*(event.touches[0].pageY-pre_y))));

            coordinate = (event.touches[0].pageX-pre_x).toString() + "," + (event.touches[0].pageY-pre_y).toString();

            sendMouse_MoveEvent(coordinate);
            
            pre_x = event.touches[0].pageX;
            pre_y = event.touches[0].pageY;
        });

        $(document).on("touchend", "#"+_mousepad_id, function() {
            event.preventDefault();
            
            var coordinate, coordinate2;
            var direction = 0, count = 0;
            var click_end_time = new Date;
            var click_time = click_end_time - click_start_time;

            if(check_entry()){
                sendFlushCurStr();
                //hide_keypad();
                touch_Moved=0;
                touch_Pressed=0;
                document.getElementById(entry_id).blur();
            }
            else if(touch_Pressed==1 && click_time < 350){
                    sendMouse_KeyEvent(MOUSE_CLICK);
                    touch_Moved=0;
                    touch_Pressed=0;
            }

            cur_touch_mode = 0;
        });
    };
    
    function sendWheel_MoveEvent (_coordinate) {
        event.preventDefault();
        if (event.touches.length < 2) {
            openInput.impl.sendWheel_MoveEvent(_coordinate);
        }
    }

    openInput.tv.setMousescroll = function(_mousescroll_id, _type) {
        $(document).on("touchstart", "#"+_mousescroll_id, function() {
            scroll_pre_x = event.touches[0].pageX;
            scroll_pre_y = event.touches[0].pageY;

            if(check_entry()) {
                sendFlushCurStr();
                document.getElementById(entry_id).blur();
               return;
            }
        });

        $(document).on("touchmove", "#"+_mousescroll_id, function() {
            event.preventDefault();
            var coordinate;

            if(check_entry()) {
                sendFlushCurStr();
                document.getElementById(entry_id).blur();
                return;
            }
            if(scroll_pre_y == 0){
                scroll_pre_x = event.touches[0].pageX;
                scroll_pre_y = event.touches[0].pageY;
                return;
            }
            if(((event.touches[0].pageX-scroll_pre_x == 0) && (event.touches[0].pageY-scroll_pre_y == 0))||(event.touches[0].pageY-scroll_pre_y == 0)){
                return;
            }

            if (_type === 1)
                var coordinate = ((event.touches[0].pageX-scroll_pre_x)*-1).toString()+","+((event.touches[0].pageY-scroll_pre_y)*-1).toString();
            else
                var coordinate = (event.touches[0].pageX-scroll_pre_x).toString()+","+(event.touches[0].pageY-scroll_pre_y).toString();
            
            if (Math.abs((event.touches[0].pageY-scroll_pre_y)) >= 3){
              sendWheel_MoveEvent(coordinate);
            }
            scroll_pre_x = event.touches[0].pageX;
            scroll_pre_y = event.touches[0].pageY;
        });

        $(document).on("touchend", "#"+_mousescroll_id, function() {
            scroll_pre_y=0;
        });
    };

    function removeTmpChar(str){
        if (TMP_CHAR.length < 1) return;
        if (str.length >= TMP_CHAR.length) {
            str = str.substring (TMP_CHAR.length);
        }
        return str;
    }

    function sendCommitStr(str) {
        str = removeTmpChar(str);
        event.preventDefault();
        openInput.impl.commitStr(str);
    }

    function sendPreeditStr(str) {
        str = removeTmpChar(str);
        event.preventDefault();
        openInput.impl.updatePreeditString(str);
    }
    
    function sendFlushCurStr() {
        var entry = document.getElementById(entry_id);
        var str = entry.value;
        if (str.length > 0) {
            entry.value = TMP_CHAR;
            pre_str = "";
            pre_pre_str = "";
            sendCommitStr(str);
            window.clearInterval(flush_timeout);
        }
    }
    
    function hide_keypad() {
        var entry = document.getElementById(entry_id);
        entry.blur();
        Android.hideKeyboard();
        
        setTimeout(function() {
            window.scrollTo(0,1);
            var entry = document.getElementById(entry_id);
            entry.focus();
            entry.value = TMP_CHAR;
        }, 300);
    }

    openInput.tv.setEntry = function(_entry_id) {
        if (entry_flag != 0)
            return;
        
        entry_id = _entry_id;
        entry_flag = 1;
        $(document).ready(function() {
            var entry = document.getElementById(entry_id);
            entry.value = TMP_CHAR;
            
            $(document).on("input", "#"+entry_id, function() {
                window.clearInterval(flush_timeout);
                var cur_timestamp = (new Date).getTime();
                latest_typed_timestamp = cur_timestamp;
                
                //To prevent multiline field in textarea when the enter key is typed and send commit string and enter key event
                if(this.value.substring(this.value.length - 1) == "\n"){
                    this.value = this.value.substring(0, this.value.length - 1);
                    sendFlushCurStr(entry_id);
                    entry.value = TMP_CHAR;
                    TMP_TIMESTAMP = cur_timestamp;
                    empty_checker_timeout = window.setInterval("reArrangeCursorPos()", EMPTY_CHECKER_TIMEOUT);
                    openInput.tv.sendKeyEvent(openInput.TV_KEY_ENTER);
                    return false;
                }
                
                //To prevent removing the TMP_CHAR(first) character for the all time
                if(TMP_CHAR.substring(0, TMP_CHAR.length - 1) == this.value){
                    entry.value = TMP_CHAR;
                    TMP_TIMESTAMP = cur_timestamp;
                    empty_checker_timeout = window.setInterval("reArrangeCursorPos()", EMPTY_CHECKER_TIMEOUT);
                    return false;
                }
                
                //To prevent duplicating preedit string error for Note2, S3 web browser
                if(pre_str == this.value){
                    return;
                }
                
                /*To prevent wrong preedit string error for Tizen phone, when next character is made in CJK
                  correct event must happen like [pre_str:가 for 간 -> preedit:간 ->  pre_str: 간 for 가나 -> preedit:가나 ]
                  wrong event happen from tizen  [pre_str:가 for 간 -> preedit:간 -> preedit:가나 -> pre_str: 가 for 가나 -> preedit: 가 -> pre_str: 간 for  가 ] */
                if(pre_pre_str == this.value) {
                    return;
                }
                pre_pre_str = pre_str;
                pre_str = this.value;
                
                //send commit string when the space key is typed
                if(this.value.substring(this.value.length - 1) == " ") {
                    sendFlushCurStr(entry_id);
                } else {
                    flush_timeout = window.setInterval("sendFlushCurStr()", FLUSH_TIMEOUT);
                    sendPreeditStr(this.value);
                }
            });
    
            $(document).on("propertychange", "#"+entry_id, function() {
                alert("document_propertychange");
            });
    
            $(document).on("keydown", "#"+entry_id, function() {
                //To prevent Ctrl + C event (keycode 17:Ctrl, 67:C)
                if(event.keyCode == 17 || event.keyCode == 67) return;
                
                //To enable back space key continually, even there is no charactor for Note2, S3 web browser
                if(this.value == TMP_CHAR && event.keyCode == openInput.TV_KEY_BACKSPACE) {
                    openInput.tv.sendKeyEvent(event.keyCode);
                } else if(this.value.length > TMP_CHAR.length && TMP_CHAR == this.value.substring(0, this.value.length - 1) 
                    && event.keyCode == openInput.TV_KEY_BACKSPACE) {
                    openInput.tv.sendKeyEvent(event.keyCode);
                }
            });
        });
    };
    
    window.openInput.tv = openInput.tv;
})(window);


//air conditioner
openInput.air = {};

(function(window)
{   
    openInput.air.sendKeyEvent = function(_code)
    {
        event.preventDefault();
        openInput.impl.sendKeyEvent(_code);
    };
    
    window.openInput.air = openInput.air;
})(window);