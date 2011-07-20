 /****************************************************************************
 **
 ** Copyright (c) 2011 Intel Corporation.
 ** All rights reserved.
 **
 ** Redistribution and use in source and binary forms, with or without
 ** modification, are permitted provided that the following conditions are
 ** met:
 **   * Redistributions of source code must retain the above copyright
 **     notice, this list of conditions and the following disclaimer.
 **   * Redistributions in binary form must reproduce the above copyright
 **     notice, this list of conditions and the following disclaimer in
 **     the documentation and/or other materials provided with the
 **     distribution.
 **   * Neither the name of Intel Corporation nor the names of its
 **     contributors may be used to endorse or promote
 **     products derived from this software without specific prior written
 **     permission.
 **
 ** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 ** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 ** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 ** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 ** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 ** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 ** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 ** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 ** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 ** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 ** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
 **
 ****************************************************************************/

import Qt 4.7
import MeeGo.Components 0.1
import Qt.labs.gestures 2.0

Item {
    id: _farther
    // If we are running using the executable wrapper, then we will have a
    // screenWidth and screenHeight added to our context.  If running via
    // qmlviewer then fallback to a reasonable default
    width: {
        try {
            screenWidth;
        }catch (err) {
            1280;
        }
    }

    height: {
        try {
            screenHeight;
        }catch (err) {
            800;
        }
    }

    property bool landscape: false
    property bool menu_hiden: true
    property int ondata: 0
    property int button_width: 80
    property int button_height: 60
    property int processvalue: 0;
    property int forwardValue: 0;
    property int playValue: 1;
    property int backwardValue: 0;
    property int volumPercValue: 50;
    property int fullScreenValue: 0;
    property int curstimeValue: 0;
    property int fullstimeValue: 200;
    property int transmitValue: fullstimeValue;

    property int vSliderDepth: _farther.width - 630;
    property int aSliderDepth: 60;
    property int vSliderProcess: 0;
    property int aSliderProcess: 30;

    function g_FormatTime(time)
    {
        var hour = parseInt(time/3600);
        var min = parseInt(time/60);
        var sec = parseInt(time%60);

        return hour + (min<10 ? ":0":":") + min + (sec<10 ? ":0":":") + sec;
    }

    function g_SliderPercentageIn(_currentTime, _totalTime, _depth)
    {
        var rate = parseInt(_currentTime*10000/_totalTime);
        var realdepth = parseInt(rate * _depth / 10000);

        return realdepth;
    }

    function g_SliderPercentageOut(_realdepth, _totalTime, _depth)
    {
        return parseInt((_realdepth*_totalTime)/_depth);
    }

    signal clicked

    MouseArea {
        id:  _screen_mouseArea
        anchors.fill: parent
        hoverEnabled: true

        onClicked: { 
            if(playValue){
                menu_hiden = !menu_hiden;
                fmenuObject.setMenuHiden(menu_hiden);

                if(!menu_hiden){
                    _tmp_sleep001.running = true;
                    _tmp_sleep001.interval = 100;
                    _tmp_sleep001.restart();
                }else{
                    _tmp_sleep002.running = true;
                    _tmp_sleep002.interval = 2;
                    _tmp_sleep002.restart();
                }
            }
        }
    }

    Timer {
        id: _tmp_sleep001
        interval: 10
        running: false
        repeat: false
        onTriggered: {
            _total_animation.to = _farther.height - _total.height;
            _total_animation.running = true;
        }
    }

    Timer {
        id: _tmp_sleep002
        interval: 10
        running: false
        repeat: false
        onTriggered: {
            _total.y = _farther.height;
        }
    }

    Timer {
        id: _tmp_sleep003
        interval: 100
        running: false
        repeat: false
        onTriggered: {
            // Flush all
            _tmp_sleep002.running = true;
            _tmp_sleep002.interval = 1;
            _tmp_sleep002.restart();

            fmenuObject.SyncWriteStatus(backwardValue,playValue,
                    forwardValue,0/*Skip*/,volumPercValue,
                    fullScreenValue,9/*Fullscr*/,fullstimeValue,0);

            Qt.quit();
        }
    }

    Item {
        id: _total
        y: _farther.height;
        width: _farther.width; height: 60;

        PropertyAnimation {
            id: _total_animation;
            target: _total;
            easing.type: Easing.OutQuad;
            properties: "y";
            duration: 400;
        }

        BorderImage {
            id: background
            anchors.fill: parent
            source: "image://themedimage/widgets/apps/browser/music_main_panel"
        }

        Component {
            id: _backwardButton
            Item {
                id: _bwIRec
                width: button_width;
                height: button_height;

                Image {
                    smooth: true
                    anchors.centerIn: parent
                    source: (backwardValue>0)? "image://themedimage/widgets/apps/browser/media-backward-active" : "image://themedimage/widgets/apps/browser/media-backward"
                }

                signal clicked
                MouseArea {
                    id:  _bwIRec_mouseArea
                    anchors.fill: parent
                    onClicked: { 
                        _bwIRec.clicked()
                        if(backwardValue){ 
                            backwardValue = 0;
                        }else{ 
                            backwardValue = 1;
                            forwardValue = 0;
                        }

                        // Flush all
                        fmenuObject.SyncWriteStatus(backwardValue,playValue,
                                    forwardValue,0/*Skip*/,volumPercValue,
                                    fullScreenValue,3/*back*/,fullstimeValue,0);

                            //fmenuObject.SyncWriteStatus(
                            //0 /*Backward 1-enable 0-disable*/,
                            //1 /*Play 1-play 0-pause*/,
                            //0 /*Forward 1-enable 0-disable*/,
                            //363 /*Current Video Seconds 0-skip*/,
                            //50 /*Current Volum Percentage*/,
                            //1 /*Full screen mode 0-disable*/,
                            //0 /*AR*/,
                            //3600 /*Total Media Duration Seconds*/, 
                            //0 /*Ext-command*/
                            //)

                    }
                }
            }
        }

        Component {
            id: _playButton

            Item {
                id: _pIRec
                width: button_width;
                height: button_height;
                Image {
                    smooth: true
                    anchors.centerIn: parent
                    source: (playValue>0)? "image://themedimage/widgets/apps/browser/media-pause-active" : "image://themedimage/widgets/apps/browser/media-play-active" ;
                }

                signal clicked
                MouseArea {
                    id:  _pIRec_mouseArea
                    anchors.fill: parent
                    onClicked: { 
                        _pIRec.clicked()
                        if(playValue){ 
                            playValue = 0;
                        }else{ 
                            playValue = 1;
                        }

                        backwardValue = 0;
                        forwardValue = 0;

                        // Flush all
                        fmenuObject.SyncWriteStatus(backwardValue,playValue,
                                forwardValue,0/*Skip*/,volumPercValue,
                                fullScreenValue,!playValue,fullstimeValue,0);
                    }
                }
            }
        }

        Component {
            id: _forwardButton

            Item {
                id: _fwIRec
                width: button_width;
                height: button_height;
                Image {
                    smooth: true
                    anchors.centerIn: parent
                    source: (forwardValue>0)? "image://themedimage/widgets/apps/browser/media-forward-active" : "image://themedimage/widgets/apps/browser/media-forward"
                }

                signal clicked
                MouseArea {
                    id:  _fwIRec_mouseArea
                    anchors.fill: parent
                    onClicked: { 
                        _fwIRec.clicked()
                        if(forwardValue){ 
                            forwardValue = 0;
                        }else{ 
                            backwardValue = 0;
                            forwardValue = 1;
                        }
                        // Flush all
                        fmenuObject.SyncWriteStatus(backwardValue,playValue,
                                forwardValue,0/*Skip*/,volumPercValue,
                                fullScreenValue,2,fullstimeValue,0);
                    }
                }
            }
        }

        Component {
            id: _curDisplayBoard
            Item {
                id: _cDBRec
                width: button_width;
                height: button_height;
                property alias text: _cDBRec_txt.text
                Text {
                    id: _cDBRec_txt
                    anchors.centerIn: parent
                    font.bold: true
                    font.pixelSize: 20
                    text: g_FormatTime(curstimeValue)
                    color: "white"
                }
            }
        }

        Component {
            id: _curvideo_slider
            Item {
                id: _curvideo_slider_basebg
                width: vSliderDepth
                height: 60
                signal clicked
                Item {
                    id: _curvideo_slider_bg
                    width: vSliderDepth
                    height: 20
                    anchors.centerIn: parent
                    BorderImage {
                        id: _curvideo_slider_bgline
                        anchors.fill: parent
                        smooth: true
                        source: "image://themedimage/widgets/apps/browser/navigationBar_l"
                    }
                    BorderImage {
                        id: _curvideo_slider_hdline
                        x: -1
                        width: (vSliderProcess<_curvideo_slider_bg.width)? vSliderProcess : _curvideo_slider_bg.width
                        height: _curvideo_slider_bg.height
                        smooth: true
                        source: "image://themedimage/widgets/apps/browser/progress_fill_2"
                    }
                    Image {
                        id: _curvideo_slider_hdpoint
                        width: 30
                        height: 30
                        x: (_curvideo_slider_hdline.width>0)? (_curvideo_slider_hdline.width - width/2):-width/2
                        smooth: true
                        anchors.verticalCenter:_curvideo_slider_hdline.verticalCenter
                        source: "image://themedimage/widgets/apps/browser/slider-handle"
                    }
                }
                MouseArea {
                    id: _curvideo_slider_mouseArea
                    anchors.fill: parent
                    onClicked: { 
                        vSliderProcess = g_SliderPercentageIn(mouseX, vSliderDepth, vSliderDepth);
                        curstimeValue = g_SliderPercentageOut(vSliderProcess,fullstimeValue,vSliderDepth);
                        _g_guider_timer.idle = 2;
                        fmenuObject.SyncWriteStatus(backwardValue,playValue,
                                forwardValue,curstimeValue/*Sink*/,volumPercValue,
                                fullScreenValue,4/*Seek*/,fullstimeValue,0);
                    }
                }
            }
        }

        Component {
            id: _totalDisplayBoard
            Item {
                id: _tDBRec
                width: button_width;
                height: button_height;
                property alias text: _tDBRec_txt.text
                Text {
                    id: _tDBRec_txt
                    anchors.centerIn: parent
                    font.bold: true
                    font.pixelSize: 20
                    text: g_FormatTime(fullstimeValue)
                    color: "white"
                }
            }
        }

        Component {
            id: _volumButton
            Item {
                id: _volumIRec
                width: button_width;
                height: button_height;
                property int tmpcount: 2
                Image {
                    id: _volumIRec_img
                    smooth: true
                    anchors.centerIn: parent
                    source: (tmpcount>=2)? ((tmpcount==2)? "image://themedimage/widgets/apps/browser/media-volume-2-active" : "image://themedimage/widgets/apps/browser/media-volume-max-active" ) :((tmpcount==0)? "image://themedimage/widgets/apps/browser/media-volume-min-active" : "image://themedimage/widgets/apps/browser/media-volume-1-active" );
                    //source: "image://themedimage/widgets/apps/browser/media-volume-2-active"
                }

                signal clicked
                MouseArea {
                    id:  _volumIRec_mouseArea
                    anchors.fill: parent
                    onClicked: { 
                        _volumIRec.clicked()
                        tmpcount = tmpcount + 1;
                        if(tmpcount==1){
                            volumPercValue = 20;
                        }else if(tmpcount==2){
                            volumPercValue = 50;
                        }else if(tmpcount==3){
                            volumPercValue = 90
                        }else if(tmpcount==0){
                            volumPercValue = 1;
                        }else{
                            volumPercValue = 1;
                            tmpcount = 0 
                        }
                        // Flush all
                        fmenuObject.SyncWriteStatus(backwardValue,playValue,
                                forwardValue,0/*Skip*/,volumPercValue,
                                fullScreenValue,5/*Volume*/,fullstimeValue,0);
                    }
                }
            } 
        }

        Rectangle {
            id: _curaudio_slider
            Item {
                id: _curaudio_slider_bg
                width: 0
                height: aSliderDepth
                x: 45+button_width*5+vSliderDepth ; 
                y: 0
                signal clicked
                BorderImage {
                    id: _curaudio_slider_bgline
                    anchors.fill: parent
                    smooth: true
                    source: "image://themedimage/widgets/apps/browser/volumehead_bg"
                }
                Image {
                    id: _curaudio_slider_hdpoint
                    width: _curaudio_slider_bg.width
                    height:  _curaudio_slider_bg.width
                    y: aSliderProcess;
                    smooth: true
                    source: "image://themedimage/widgets/apps/browser/slider-handle"
                }
                MouseArea {
                    id: _curvaudio_slider_mouseArea
                    anchors.fill: parent
                    onClicked: { 
                        _curaudio_slider_bg.clicked()
                        aSliderProcess = mouseY;
                        volumPercValue = aSliderProcess;
                        fmenuObject.SyncWriteStatus(backwardValue,playValue,
                                forwardValue,0/*Skip*/,volumPercValue,
                                fullScreenValue,5/*Volume*/,fullstimeValue,0);
                    }
                }
            }
        }

        Component {
            id: _fullScreenButton
            Item {
                id: _fullScreenIRec
                width: button_width;
                height: button_height;
                Image {
                    smooth: true
                    anchors.centerIn: parent
                    source: "image://themedimage/widgets/apps/browser/view-fullscreen-active"
                }
                signal clicked
                MouseArea {
                    id:  _fullScreenIRec_mouseArea
                    anchors.fill: parent
                    onClicked: { 
                        _fullScreenIRec.clicked()
                        menu_hiden = true;
                        fmenuObject.setMenuHiden(menu_hiden);

                        _tmp_sleep003.running = true;
                        _tmp_sleep003.interval = 200;
                        _tmp_sleep003.restart();

                    }
                }
            }
        }

        Connections {
            target: fmenuObject
            ignoreUnknownSignals:true;
            onVideoRun: {
                if(current>total) current = total;
                else if(current<0 || total<0) current = 0;

                curstimeValue = current;
                fullstimeValue = total;
                curstimeValue = (curstimeValue<fullstimeValue)? curstimeValue : fullstimeValue 
                vSliderProcess = g_SliderPercentageIn(curstimeValue,fullstimeValue,vSliderDepth);
            }

            onForceControlOutside: {
                // Commands Code to QML from outside!
                menu_hiden = true;
                fmenuObject.setMenuHiden(menu_hiden);

                _tmp_sleep002.running = true;
                _tmp_sleep002.interval = 1;
                _tmp_sleep002.restart();

                Qt.quit();

            }

            onSyncRead: {
                // Check code <<TODO>> functions !
                //console.log("***>>>  ", cback, cplay, cforward, cfullscreen, ctype);
            }
        }

        Timer {
            id: _g_guider_timer
            interval: (!menu_hiden)? 1000 : 5000 ;
            running: true
            repeat: true
            property int idle: 0;
            onTriggered: {
                if(!menu_hiden){
                    if(idle){
                        idle--;
                    }

                    if(playValue&&curstimeValue<fullstimeValue&&!idle){
                        fmenuObject.cMethod();
                    }
                    fmenuObject.setMenuHiden(menu_hiden);
                }
            }
        }

        Loader { sourceComponent: _backwardButton; x: 10; }
        Loader { sourceComponent: _playButton; x: 10+button_width; }
        Loader { sourceComponent: _forwardButton; x: 10+button_width*2; }
        Loader { sourceComponent: _curDisplayBoard; x: 10+button_width*3; }
        Loader { sourceComponent: _curvideo_slider; x: 25+button_width*4; }
        Loader { sourceComponent: _totalDisplayBoard; x: 40+button_width*4+vSliderDepth;}
        Loader { sourceComponent: _volumButton; x: 60+button_width*5+vSliderDepth; }
        Loader { sourceComponent: _fullScreenButton; x: 60+button_width*6+vSliderDepth; }

    }
}
