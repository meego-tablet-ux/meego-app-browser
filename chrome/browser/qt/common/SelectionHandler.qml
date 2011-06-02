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

Item {
    id: handler
    property int startx;
    property int starty;
    property int endx;
    property int endy;

    property double scale: 1.0;

    property bool in_pinch_gesture : false;
    property double pinch_scale : 1;
    property int pinch_offset_x: 0;
    property int pinch_offset_y: 0;

    property int selection_height : 14;

    Item {
        id: startAnchor
        x: handler.startx * handler.scale + handler.pinch_offset_x;
        y: handler.starty * handler.scale + handler.pinch_offset_y;
    }

    Item {
        id: endAnchor
        x: handler.endx * handler.scale + handler.pinch_offset_x;
        y: handler.endy * handler.scale + handler.pinch_offset_y;
    }

    Image {
        id: start
        scale: handler.scale*handler.pinch_scale/1.5
        fillMode: Image.PreserveAspectFit
        anchors.horizontalCenter: startAnchor.horizontalCenter
        anchors.horizontalCenterOffset: 0
        anchors.verticalCenter : startAnchor.verticalCenter
        anchors.verticalCenterOffset : (selection_height-8)*scale
        source: "image://themedimage/widgets/common/text-selection/text-selection-marker-start"
    }

    Image {
        id: end
        scale: handler.scale*handler.pinch_scale/1.5
        fillMode: Image.PreserveAspectFit
        anchors.horizontalCenter: endAnchor.horizontalCenter
        anchors.horizontalCenterOffset: -1*scale
        anchors.verticalCenter : endAnchor.verticalCenter
        anchors.verticalCenterOffset : (-selection_height+8)*scale
        source: "image://themedimage/widgets/common/text-selection/text-selection-marker-end"
    }

    Connections {
        target: selectionHandler;
        onSet:
        {
            handler.startx = start_x;
            handler.starty = start_y;
            handler.endx = end_x;
            handler.endy = end_y;
        }
        onSetScale:
        {
            handler.scale = scale_;
        }
        onSetPinchOffset:
        {
            handler.pinch_offset_x = offset_x_;
            handler.pinch_offset_y = offset_y_;
        }
        onSetPinchState:
        {
            handler.in_pinch_gesture = in_pinch_;
        }
        onSetHandlerHeight:
        {
            handler.selection_height = height_;
        }
    }

}
