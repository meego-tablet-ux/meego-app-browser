/****************************************************************************
 **
 ** Copyright (c) 2010 Intel Corporation.
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

Item {
    signal triggered(int index)

    property int itemMargin: 20
    property int textRightSideSpace: 70

    width: textItem.width
    height: textItem.height + seperatorImage.height
    clip: true

    Image {
        id: highlighterImage
        width: 22
        height: 18
        visible : currentIndex == index
        anchors.verticalCenter: parent.verticalCenter
        anchors.right: parent.right
        anchors.rightMargin: itemMargin
        source: "image://themedimage/widgets/apps/browser/tick"
    } // Image (highlighter)

    Text {
        id: textItem
        anchors { left: parent.left; top: parent.top; right: parent.right }
        anchors.rightMargin: textRightSideSpace
        anchors.leftMargin: itemMargin
        height: 45
        verticalAlignment: Text.AlignVCenter
        horizontalAlignment: Text.AlignLeft
        color: theme_contextMenuFontColor
        font.pixelSize: theme_contextMenuFontPixelSize

        // elide has to be turned of to correctly compute the paintedWidth. It is re-enabled after the width computing
        elide: elideEnabled ? Text.ElideRight :Text.ElideNone
        text: label

        Component.onCompleted: {
            // This compares the paintedWidth to minWidth and maxWidth and sets the widgets width accordingly
            var requiredWidth = itemMargin + paintedWidth + textRightSideSpace
            if ( requiredWidth  > currentWidth ){
                if( requiredWidth > maxWidth )
                    currentWidth = maxWidth;
                else{
                    currentWidth = requiredWidth;
                }
            }
            if( currentWidth > maxWidth )
                currentWidth = maxWidth;
        } // onCompleted
    } // Text

    Image {
        id: seperatorImage
        anchors { top: textItem.bottom; left: parent.left }
        width: parent.width
        visible: index < repeater.count - 1     // Seperator won't be visible for the last item
        source: "image://themedimage/images/menu_item_separator"
    } // Image (separator)

    MouseArea {
        anchors.fill: parent

        // pressed state for the text entry:
        onClicked: {
            if (type == itemType.optionItem) {
                triggered(index)
            }
        }
        onPressed: {
            //if (container.currentIndex != index) {
            //    highlighter.visible = true
            //}
        }
        onReleased: {
            //if (container.currentIndex != index) {
            //    highlighter.visible = false
            //}
        }
    } // MouseArea
}
