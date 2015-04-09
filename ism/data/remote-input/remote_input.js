document.write("<script type='text/javascript' src='ajaxCaller.js'><"+"/script>");
document.write("<script type='text/javascript' src='util.js'><"+"/script>");
document.write("<script type='text/javascript' src='jquery-2.0.2.min.js'><"+"/script>");
document.write("<script type='text/javascript' src='jquery.mobile-1.3.1.min.js'><"+"/script>");




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
           //addJavascript('ajaxCaller.js');
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
        function inputModeChanged(from, to) {
                event.preventDefault();
                //setHandMode(to);
                var from_div = document.getElementById(from);
                from_div.style.display = "none";
                var to_div = document.getElementById(to);
                to_div.style.display = "block";
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
        function sendevent(form){
            //alert(form);
            if (form == "sys_btn_modechange") {
                inputModeChanged("tvmode", "keymode");
            }
            else if (form == "sys_btn_home"){
                sendKeyEvent(KEY_HOME);
            }
            else if (form == "sys_btn_back"){
                sendKeyEvent(KEY_BACK);
            }
            else if (form == "sys_btn_modechange2"){
                inputModeChanged("keymode", "tvmode");
            }
            else if (form == "sys_btn_home2"){
                sendKeyEvent(KEY_HOME);
            }
            else if (form == "sys_btn_back2"){
                sendKeyEvent(KEY_BACK);
            }
            else if (form == "bt_power"){
                sendKeyEvent(TV_KEY_POWER);
            }
            else if (form == "bt_switchmode"){
                sendKeyEvent(TV_KEY_SWITCHMODE);
            }
            else if (form == "bt_menu"){
                sendKeyEvent(KEY_MENU);
            }
            else if (form == "bt_info"){
                sendKeyEvent(TV_KEY_INFO);
            }
            else if (form == "bt_select"){
                sendKeyEvent(TV_KEY_SELECT);
            }
            else if (form == "bt_back"){
                sendKeyEvent(TV_KEY_BACK);
            }
            else if (form == "bt_exit"){
                sendKeyEvent(TV_KEY_EXIT);
            }
            else if (form == "bt_volume_up"){
               // sendKeyEvent(TV_KEY_VOL_UP);
                inputModeChanged('tvmode', 'keymode');
            }
            else if (form == "bt_mute"){
                sendKeyEvent(TV_KEY_MUTE);
            }
            else if (form == "bt_channel_up"){
                sendKeyEvent(TV_KEY_CHAN_UP);
            }
            else if (form == "bt_volume_down"){
                sendKeyEvent(TV_KEY_VOL_DOWN);
            }
            else if (form == "bt_channel_list"){
                sendKeyEvent(TV_KEY_CHAN_LIST);
            }
            else if (form == "bt_channel_down"){
                sendKeyEvent(TV_KEY_CHAN_DOWN);
            }
        }

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
                }
                //////별/해야됨
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
*/
