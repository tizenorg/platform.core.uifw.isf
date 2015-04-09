document.write("<script type='text/javascript' src='ajaxCaller.js'><"+"/script>");
document.write("<script type='text/javascript' src='util.js'><"+"/script>");
//for key_input
            // Key definition
        var KEY_BACKSPACE = 8;
        var KEY_ENTER = 13;
        var KEY_SPACE = 32;
        var KEY_ESCAPE = 27;

        var KEY_MENU = 10001;
        var KEY_HOME = 10002;
        var KEY_BACK = 10003;

        var MOUSE_CLICK = 555;
//        var MOUSE_PRESSED = 556;
//        var MOUSE_RELEASED =557;

        // Callback incremental index
        var id_num = 0;

        var TV_KEY_POWER = 124;
        var TV_KEY_SWITCHMODE = 235;
        var TV_KEY_MENU = 179;
        var TV_KEY_UP = 111;
        var TV_KEY_INFO = 69;
        var TV_KEY_LEFT = 113;
        var TV_KEY_SELECT = 36;
        var TV_KEY_RIGHT = 114;
        var TV_KEY_BACK = 166;
        var TV_KEY_DOWN = 116;
        var TV_KEY_EXIT = 182;
        var TV_KEY_VOL_UP = 123;
        var TV_KEY_MUTE = 121;
        var TV_KEY_CHAN_UP = 112;
        var TV_KEY_VOL_DOWN = 122;
        var TV_KEY_CHAN_LIST = 68;
        var TV_KEY_CHAN_DOWN = 117;

        var multi_touched = 0;
        var touch_started_x = 0;
        var touch_started_y = 0;
        var touch_contentarea_Pressed=0;
        var touch_contentarea_Moved=0;
        var touch_contentarea_pre_x=0;
        var touch_contentarea_pre_y=0;


// for key_input

        // Mouse prior coordinate, click flag
        var pre_x = 0;
        var pre_y = 0;
        var touch_Pressed=0;
        var touch_Moved=0;
        var scroll_pre_x = 0;
        var scroll_pre_y = 0;

        // Callback incremental index
        var id_num = 0;
        var count = 0;

        // Flush timer
        var flush_timeout;
        var FLUSH_TIMEOUT = 2000;
        var FLUSH_ENTER_TIMEOUT = 50;
        var flick_start_time = 0;
        var flick_time = 0;
        // Backup pre string
        var pre_str = "";
        var TMP_CHAR = " ";
        var TMP_TIMESTAMP = 0;
        var EMPTY_CHECKER_TIMEOUT = 10;
        var latest_typed_timestamp = 0;


        function cbAjax(text, headers, callingContext){
            id_num++;
            return true;
        }
        function sendKeyEvent(keyCode){
            var bodyVars = {id:id_num, key:'event', value:keyCode}
            ajaxCaller.postForPlainText("/tizen-temp.htm", bodyVars, cbAjax);
        }
        function sendMouse_KeyEvent(mouseCode) {
            var bodyVars = {id:id_num, key:'mouse_key', value:mouseCode}
            ajaxCaller.postForPlainText("/tizen-temp.htm", bodyVars, cbAjax);
        }

        function sendMouse_MoveEvent(coordinate) {
            var bodyVars = {id:id_num, key:'mouse_move', value:coordinate}
            ajaxCaller.postForPlainText("/tizen-temp.htm", bodyVars, cbAjax);
        }

        function sendWheel_MoveEvent(coordinate) {
            var bodyVars = {id:id_num, key:'wheel_move', value:coordinate}
            ajaxCaller.postForPlainText("/tizen-temp.htm", bodyVars, cbAjax);
        }
        /*
        function sendScroll(direction, count) {
            for (i = 0 ; i < count ; i++) {
                sendMouse_KeyEvent(direction);
            }
        }
*/
        function removeTmpChar(str){
            if (TMP_CHAR.length < 1) return;
            if (str.length >= TMP_CHAR.length) {
                str = str.substring (TMP_CHAR.length);
            }
            return str;
        }

        function sendPreeditStr(str){
            if ($("#search_link_checker").prop("checked")){
                return;
            }
            str = removeTmpChar(str);
            var bodyVars = {id:id_num, key:'preedit', value:str}
            ajaxCaller.postForPlainText("/tizen-temp.htm", bodyVars, cbAjax);
        }

        function sendCommitStr(str){
            str = removeTmpChar(str);
            if ($("#search_link_checker").prop("checked")){
                var bodyVars = {id:id_num, key:'search', value:str}
                ajaxCaller.postForPlainText("/tizen-temp.htm", bodyVars, cbAjax);
            }
            else{
                var bodyVars = {id:id_num, key:'commit', value:str}
                ajaxCaller.postForPlainText("/tizen-temp.htm", bodyVars, cbAjax);
            }
        }

        function sendFlushCurStr(){
            var entry = document.getElementById( "entry" );
            var str = entry.value;
            if(str.length > 0){
                entry.value = TMP_CHAR;
                pre_str = "";
                sendCommitStr (str)
                window.clearInterval(flush_timeout);
            }
        }
        /*
        function printLog(str){
            log_box.value = log_box.value +"\n"+ str;
            var psconsole = $('#log_box');
            psconsole.scrollTop(psconsole[0].scrollHeight - psconsole.height());
        }
        */
        function clearLog(){
            log_box.value = "";log_box.value +"\n"+ str;
        }
        function reArrangeCursorPos(){
            if(latest_typed_timestamp == TMP_TIMESTAMP){
                entry.setSelectionRange(TMP_CHAR.length,TMP_CHAR.length);
            }
            window.clearInterval(empty_checker_timeout);
        }

        $(document).ready(function()    {

          setTimeout(function(){
            window.scrollTo(0, 1);
            }, 0);


            var entry = document.getElementById( "entry" );
            //entry.focus();
            entry.value = TMP_CHAR;
/*
            // 4 arrow keys
            $("#arrow_left").click(function(event) {
                  window.location='tv_mode.htm';
//                sendFlushCurStr();
//                sendKeyEvent(KEY_LEFT);
 //               var entry = document.getElementById( "entry" );
 //               entry.focus();
            });

            $("#arrow_right").click(function(event) {
                sendFlushCurStr();
                sendKeyEvent(KEY_BACK);

            });

            $("#arrow_down").click(function(event) {
                sendFlushCurStr();
                sendKeyEvent(KEY_DOWN);
            });

            $("#arrow_up").click(function(event) {
                sendFlushCurStr();
                sendKeyEvent(KEY_UP);
            });
*/

            // 3 system hw buttons
            $("#sys_btn_menu").click(function(event) {
                sendKeyEvent(KEY_MENU);
            });
            $("#sys_btn_home").click(function(event) {
                sendKeyEvent(KEY_HOME);
            });
            $("#sys_btn_back").click(function(event) {
                sendKeyEvent(KEY_BACK);
            });

            $("#sys_btn_menu2").click(function(event) {
                sendFlushCurStr();
                sendKeyEvent(KEY_MENU);
            });
            $("#sys_btn_home2").click(function(event) {
                sendFlushCurStr();
                sendKeyEvent(KEY_HOME);
            });
            $("#sys_btn_back2").click(function(event) {
                sendFlushCurStr();
                sendKeyEvent(KEY_BACK);
            });

            // Tv remote control buttons
            $("#bt_power").click(function(event) {
                sendKeyEvent(TV_KEY_POWER);
            });
            $("#bt_switchmode").click(function(event) {
                sendKeyEvent(TV_KEY_SWITCHMODE);
            });
            $("#bt_menu").click(function(event) {
                sendKeyEvent(TV_KEY_MENU);
            });
            $("#bt_up").click(function(event) {
                sendKeyEvent(TV_KEY_UP);
            });
            $("#bt_info").click(function(event) {
                sendKeyEvent(TV_KEY_INFO);
            });
            $("#bt_left").click(function(event) {
                sendKeyEvent(TV_KEY_LEFT);
            });
            $("#bt_select").click(function(event) {
                sendKeyEvent(TV_KEY_SELECT);
            });
            $("#bt_right").click(function(event) {
                sendKeyEvent(TV_KEY_RIGHT);
            });
            $("#bt_back").click(function(event) {
                sendKeyEvent(TV_KEY_BACK);
            });
            $("#bt_down").click(function(event) {
                sendKeyEvent(TV_KEY_DOWN);
            });
            $("#bt_exit").click(function(event) {
                sendKeyEvent(TV_KEY_EXIT);
            });
            $("#bt_volume_up").click(function(event) {
                sendKeyEvent(TV_KEY_VOL_UP);
            });
            $("#bt_mute").click(function(event) {
                sendKeyEvent(TV_KEY_MUTE);
            });
            $("#bt_channel_up").click(function(event) {
                sendKeyEvent(TV_KEY_CHAN_UP);
            });
            $("#bt_volume_down").click(function(event) {
                sendKeyEvent(TV_KEY_VOL_DOWN);
            });
            $("#bt_channel_list").click(function(event) {
                sendKeyEvent(TV_KEY_CHAN_LIST);
            });
            $("#bt_channel_down").click(function(event) {
                sendKeyEvent(TV_KEY_CHAN_DOWN);
            });


            content_div.addEventListener("touchstart", function(event) {
                touch_started_x = event.touches[0].pageX;
                touch_started_y = event.touches[0].pageY;
                multi_touched = event.touches.length;
                touch_contentarea_Pressed=1;
                touch_contentarea_Moved=0;
                touch_contentarea_pre_x = event.touches[0].pageX;
                touch_contentarea_pre_y = event.touches[0].pageY;

            });

            content_div.addEventListener("touchmove", function(event) {
                event.preventDefault();
                touch_contentarea_Moved = 1;
                touch_contentarea_pre_x = event.touches[0].pageX;
                touch_contentarea_pre_y = event.touches[0].pageY;
                event.preventDefault();
            });
            content_div.addEventListener("touchend", function(event) {
                if(touch_contentarea_Moved==1 && touch_contentarea_Pressed==1 && multi_touched == 2){
                    if(touch_contentarea_pre_x-touch_started_x == 0){
                        return;
                    }
                    coordinate2= touch_contentarea_pre_x-touch_started_x;
                    if (Math.abs(coordinate2) >=130){
                    $.mobile.changePage('#keymode','flip','reverse');
//                        window.location='key_mode.htm';
                    }
                }
                touch_contentarea_Pressed=0;
                touch_contentarea_Moved=0;
            });


            content_div2.addEventListener("touchstart", function(event) {
                touch_started_x = event.touches[0].pageX;
                touch_started_y = event.touches[0].pageY;
                multi_touched = event.touches.length;
                touch_contentarea_Pressed=1;
                touch_contentarea_Moved=0;
                touch_contentarea_pre_x = event.touches[0].pageX;
                touch_contentarea_pre_y = event.touches[0].pageY;

            });

            content_div2.addEventListener("touchmove", function(event) {
                event.preventDefault();
                touch_contentarea_Moved = 1;
                touch_contentarea_pre_x = event.touches[0].pageX;
                touch_contentarea_pre_y = event.touches[0].pageY;
                event.preventDefault();
            });
            content_div2.addEventListener("touchend", function(event) {
                if(touch_contentarea_Moved==1 && touch_contentarea_Pressed==1 && multi_touched == 2){
                    if(touch_contentarea_pre_x-touch_started_x == 0){
                        return;
                    }
                    coordinate2= touch_contentarea_pre_x-touch_started_x;
                    if (Math.abs(coordinate2) >=130){
                    $.mobile.changePage('#tvmode','flip','reverse');
                    }
                }
                touch_contentarea_Pressed=0;
                touch_contentarea_Moved=0;
            });


            mousepad.addEventListener("touchstart", function(event) {
                //hideTheKeyBoard();
                flick_start_time = new Date;
                pre_x = event.touches[0].pageX;
                pre_y = event.touches[0].pageY;
//                scroll_pre_x = event.touches[0].pageX;
//                scroll_pre_y = event.touches[0].pageY;
                touch_Pressed=1;
                touch_Moved=0;

                if(document.activeElement == document.getElementById( "entry" )) {
                    sendFlushCurStr();
                    document.activeElement.blur();
                }
            });

            mousepad.addEventListener("touchmove", function(event) {

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

                coordinate = (event.touches[0].pageX-pre_x).toString() + "," + (event.touches[0].pageY-pre_y).toString();
                sendMouse_MoveEvent(coordinate);
                pre_x = event.touches[0].pageX;
                pre_y = event.touches[0].pageY;
            });

            mousepad.addEventListener("touchend", function(event) {

                var coordinate, coordinate2;
                var direction = 0, count = 0;
//                var flick_end_time = new Date;
//                var flick_time = flick_end_time - flick_start_time;

                if(touch_Moved==0&&touch_Pressed==1){
                    if(document.activeElement == document.getElementById( "entry" )){
                        sendFlushCurStr();
                    }
                    else{
                        sendMouse_KeyEvent(MOUSE_CLICK);
                    }
                }
                /*
                else{
                    if(touch_Moved==1 && touch_Pressed==1 && flick_time < 200 && multi_touched ==1){
                        //printLog("flick : " + Math.abs(pre_x-scroll_pre_x) +","+Math.abs(pre_y-scroll_pre_y));
                        if((Math.abs(pre_x-scroll_pre_x) > 70) ||(Math.abs(pre_y-scroll_pre_y) < 50)){
                            return;
                        }
                        coordinate = (scroll_pre_x - pre_x).toString() + "," + (scroll_pre_y - pre_y).toString();
                        //sendMouse_MoveEvent(coordinate);

                        coordinate2= pre_y-scroll_pre_y;
                        direction = (coordinate2 > 0) ? MOUSE_SCROLL_UP : MOUSE_SCROLL_DOWN ;
                        //sendScroll(direction, 5);
                    }
                }*/

                 if(document.activeElement == document.getElementById( "entry" )){
                        sendFlushCurStr();
                        touch_Moved=0;
                        touch_Pressed=0;
                    }
                  else {
                        //sendMouse_KeyEvent(MOUSE_RELEASED);
                        touch_Moved=0;
                        touch_Pressed=0;
                  }

            });

            mousescroll.addEventListener("touchstart", function(event) {

                scroll_pre_x = event.touches[0].pageX;
                scroll_pre_y = event.touches[0].pageY;
//                sendMouse_KeyEvent(MOUSE_PRESSED);
            });

            mousescroll.addEventListener("touchmove", function(event) {

                var coordinate;
                event.preventDefault();

                if(scroll_pre_y == 0){
                    scroll_pre_x = event.touches[0].pageX;
                    scroll_pre_y = event.touches[0].pageY;
                    return;
                }
                if(((event.touches[0].pageX-scroll_pre_x == 0) && (event.touches[0].pageY-scroll_pre_y == 0))||(event.touches[0].pageY-scroll_pre_y == 0)){
                    return;
                }
                var coordinate = (event.touches[0].pageX-scroll_pre_x).toString()+","+(event.touches[0].pageY-scroll_pre_y).toString();
                sendWheel_MoveEvent(coordinate);

                scroll_pre_x = event.touches[0].pageX;
                scroll_pre_y = event.touches[0].pageY;
            });

            mousescroll.addEventListener("touchend", function(event) {
                scroll_pre_y=0;
                //sendMouse_KeyEvent(MOUSE_RELEASED);
            });

            // Firefox, Google Chrome, Opera, Safari, Internet Explorer from version 9
            $("#entry").on("input", function(event) {

                window.clearInterval(flush_timeout);
                var cur_timestamp = (new Date).getTime();
                latest_typed_timestamp = cur_timestamp;

                //To prevent multiline field in textarea when the enter key is typed and send keyevent
                if(this.value.substring(this.value.length - 1) == "\n"){
                    this.value = this.value.substring(0, this.value.length - 1);
                    sendFlushCurStr();
                    entry.value = TMP_CHAR;
                    TMP_TIMESTAMP = cur_timestamp;
                    empty_checker_timeout = window.setInterval("reArrangeCursorPos()", EMPTY_CHECKER_TIMEOUT);
                    sendKeyEvent(KEY_ENTER);
                    return false;
                }
                //To prevent removing the first chartor(TMP_CHAR)
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
                pre_str = this.value;
                if(this.value.substring(this.value.length - 1) == " ") {
                    var commitStr = this.value;
                    sendFlushCurStr();
                }
                else{
                    if ($("#search_link_checker").prop("checked")){
                        flush_timeout = window.setInterval("sendFlushCurStr()", FLUSH_TIMEOUT);
                    }
                    sendPreeditStr(this.value);
                }
            });

            // Internet Explorer
            $("#entry").on("propertychange", function(event) {

                window.clearInterval(flush_timeout);
                var cur_timestamp = (new Date).getTime();
                latest_typed_timestamp = cur_timestamp;

                //To prevent multiline field in textarea when the enter key is typed and send keyevent
                if(this.value.substring(this.value.length - 1) == "\n"){
                    this.value = this.value.substring(0, this.value.length - 1);
                    sendFlushCurStr();
                    entry.value = TMP_CHAR;
                    TMP_TIMESTAMP = cur_timestamp;
                    empty_checker_timeout = window.setInterval("reArrangeCursorPos()", EMPTY_CHECKER_TIMEOUT);
                    sendKeyEvent(KEY_ENTER);
                    return false;
                }
                //To prevent removing the first chartor(TMP_CHAR)
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
                pre_str = this.value;
                if(this.value.substring(this.value.length - 1) == " ") {
                    var commitStr = this.value;
                    sendFlushCurStr();
                }
                else{
                    if ($("#search_link_checker").prop("checked")){
                        flush_timeout = window.setInterval("sendFlushCurStr()", FLUSH_TIMEOUT);
                    }
                    sendPreeditStr(this.value);
                }
            });

            $("#entry").on("keydown", function(event) {

                if(event.keyCode == 17 || event.keyCode == 67) return;

                //To enable back space key continually, even there is no charactor for Note2, S3 web browser
                if(this.value == TMP_CHAR && event.keyCode == KEY_BACKSPACE) {
                    sendKeyEvent(event.keyCode);

                //To remove the last preedit charator, when tap the backspace key
                }else if(this.value.length > TMP_CHAR.length && TMP_CHAR == this.value.substring(0, this.value.length - 1) && event.keyCode == KEY_BACKSPACE){
                    sendKeyEvent(event.keyCode);
                }
            });
        });
