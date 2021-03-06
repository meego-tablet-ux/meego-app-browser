/***************************************************************************
 **
 ** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
 ** All rights reserved.
 ** Contact: Nokia Corporation (qt-info@nokia.com)
 **
 ** This file is part of the examples of the Qt Toolkit.
 **
 ** $QT_BEGIN_LICENSE:BSD$
 ** You may use this file under the terms of the BSD license as follows:
 **
 ** "Redistribution and use in source and binary forms, with or without
 ** modification, are permitted provided that the following conditions are
 ** met:
 **   * Redistributions of source code must retain the above copyright
 **     notice, this list of conditions and the following disclaimer.
 **   * Redistributions in binary form must reproduce the above copyright
 **     notice, this list of conditions and the following disclaimer in
 **     the documentation and/or other materials provided with the
 **     distribution.
 **   * Neither the name of Nokia Corporation and its Subsidiary(-ies) nor
 **     the names of its contributors may be used to endorse or promote
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
 ** $QT_END_LICENSE$
 **
 ****************************************************************************/
 import Qt 4.7

 Item {
     id: scrollBar

     // The properties that define the scrollbar's state.
     // position and pageSize are in the range 0.0 - 1.0.  They are relative to the
     // height of the page, i.e. a pageSize of 0.5 means that you can see 50%
     // of the height of the view.
     // orientation can be either Qt.Vertical or Qt.Horizontal
     property real position
     property real pageSize
     property variant orientation : Qt.Vertical
     property real backgroundOpacity: 0.3
     property real scrollOpacity: 0.7 
     // A light, semi-transparent background
     Rectangle {
         id: background
         anchors.fill: parent
         radius: orientation == Qt.Vertical ? (width/2 - 1) : (height/2 - 1)
         color: "gray"
         opacity: backgroundOpacity
     }

     // Size the bar to the required size, depending upon the orientation.
     Rectangle {
         x: orientation == Qt.Vertical ? 1 : scrollBarPos(scrollBar.width)
         y: orientation == Qt.Vertical ? scrollBarPos(scrollBar.height) : 1
         width: orientation == Qt.Vertical ? (parent.width-2) : scrollBarLen(scrollBar.width)
         height: orientation == Qt.Vertical ? scrollBarLen(scrollBar.height) : (parent.height-2)
         radius: orientation == Qt.Vertical ? (width/2 - 1) : (height/2 - 1)
         color: "white"
         opacity: scrollOpacity
     }

     function scrollBarPos(len) {
         var rad = orientation == Qt.Vertical ? (width/2 - 1) : (height/2 - 1)
         var maxPos = len - rad*2 - 1
         var ret = scrollBar.position * (len-2) + 1
         if ( ret < 1 ) {             // Not exceeding top
            ret = 1
         } else if ( ret > maxPos ) {  // Not exceeding bottom
            ret = maxPos
         }
         return ret
     }

     function scrollBarLen(len) {
        var rad = orientation == Qt.Vertical ? (width/2 - 1) : (height/2 - 1) // radius of bar
        var pos = scrollBar.position * (len-2) + 1 // position of bar, normally
        var ret = scrollBar.pageSize * (len-2) // size of bar, normally
        if ( pos < 0 ) {  // shrink size of bar when page moves out of top
            if ( (pos + ret - rad*2 - 1) < 0 ) {  // min size of bar at the top
                ret = rad*2
            } else {
                ret += pos
            }
        } else if ( pos > (len - rad*2 - 1) ) {  // min size of bar in the bottom
            ret = rad*2
        } else if ( pos > (len - ret - 1) ) {  // shrink size of bar when page moves out of bottom
            ret = len - pos - 1
        }
        return ret
     }
 }

