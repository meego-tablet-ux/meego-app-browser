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
    id: webPopupListView
    property alias model: repeater.model

    property int minWidth : 200     // The width of the menu should always be between minWidth and maxWidth
    property int maxWidth : 500
    // currentWidth is the current width of the largest text width, clamped between minWidth and maxWidth
    property int currentWidth: minWidth

    property bool elideEnabled: false
    property int currentIndex : 0
    property Item currentItem : null
    property int realheight : 200
    property int maxHeight : 450
    property alias interactive: container.interactive

    signal triggered(int index)

    width: currentWidth

    onWidthChanged: {
        // Clamp to [minWidth,maxWidth]
        currentWidth = Math.min( maxWidth, Math.max( minWidth, width ) )
    }


    function positionCurrentItem() {

        if (!container.interactive)
            return

        var singleItemHeight = container.contentHeight / repeater.count
        var currentItemPos = currentIndex * singleItemHeight
        var maxPosWithinViewport = container.height - singleItemHeight

        if ( maxPosWithinViewport <= 0 )
            return

        // Position the selected item to the middle of viewport
        container.contentY = currentItemPos - maxPosWithinViewport/2

        // Clamp scroll position to range [ 0, contentHeight-viewportHeight ] to avoid awkward over/under scrolling
        container.contentY = Math.max( container.contentY, 0 )
        container.contentY = Math.min( container.contentY, container.contentHeight - container.height )

    } // positionCurrentItem()


    Flickable {
        id: container

        anchors.fill: parent
        contentHeight: realheight
        contentWidth: currentWidth
        clip: true

        Item {
            id: itemType
            property int optionItem : 0
            property int checkableOptionItem : 1
            property int groupItem : 2
            property int separatorItem : 3
        }

        Column {
            id: itemColumn
            width: parent.width

            Repeater {
                id: repeater
                width: parent.width
                height: childrenRect.height
                delegate: WebPopupListDelegate {
                    width: repeater.width
                    Connections {
                        // Forward triggered signal from delegate
                        onTriggered: { webPopupListView.triggered( index ) }
                    }
                }
            } // Repeater

            // enable text eliding when the correct width was computed
            Component.onCompleted: {
                    realheight = itemColumn.height
                    positionCurrentItem();
                    elideEnabled = true;
            }
        } // Column

    } // Flickable

    // Top and bottom shadows item (visible when list is scrollable)
    // TODO: replace gradients with actual graphics when we get them
    Item {
        property int shadowSize: 6
        opacity:  0.333
        visible: container.contentHeight > container.height
        anchors.fill:  parent
        Rectangle {
            height: parent.shadowSize
            anchors { top: parent.top; left: parent.left; right: parent.right }
            gradient: Gradient {
                GradientStop { position: 0.0; color: "black" }
                GradientStop { position: 1.0; color: "transparent" }
            }
        }
        Rectangle {
            height: parent.shadowSize
            anchors { bottom: parent.bottom; left: parent.left; right: parent.right }
            gradient: Gradient {
                GradientStop { position: 0.0; color: "transparent" }
                GradientStop { position: 1.0; color: "black" }
            }
        }
    } // Item (shadows)
}
