/****************************************************************************
 **
 ** Copyright (c) <2010> Intel Corporation.
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

Flickable {
    id: container

    property alias model: repeater.model

    property int minWidth : 200     // The width of the menu should always be between minWidth and maxWidth
    property int maxWidth : 500
    property int currentWidth: minWidth
    property int textMargin : 16

    property bool elideEnabled: false
    property int currentIndex : 0
    property Item currentItem : null
    property int currentItemY : 0
    property int currentItemHeight : 0

    property int maxHeight : 600

    signal triggered(int index)

    // currentWidth is the current width of the largest text width, clamped between minWidth and maxWidth
    width: currentWidth
    height: (contentHeight < maxHeight) ? contentHeight : maxHeight

    contentHeight: layout.height
    contentWidth: currentWidth

    interactive: (contentHeight > height) ? true : false
    clip: true

    Item {
        id: itemType
        property int optionItem : 0
        property int checkableOptionItem : 1
        property int groupItem : 2
        property int separatorItem : 3
    }

    function positionCurrentItem() {
        //console.log("container.height " + container.height);
        //console.log("maxHeight " + maxHeight);
        //console.log("currentItemY " + currentItemY);

        // we try to position currentItem at the center of the layout
        // if that's not possible, scroll the flickable to either end
        // so that the currentItem is positioned as near to the center as posible
        var bestY = currentItemY - container.height/2

        if (bestY < 0) {
            bestY = 0;
        }

        if (bestY + container.height > contentHeight) {
            bestY = contentHeight - container.height;
        }

        container.contentY = bestY;
    }

    Component.onCompleted: {
       positionCurrentItem();
    }

    Column {
        id: layout
        width: parent.width

        Repeater {
            id: repeater

            width: parent.width
            height: childrenRect.height

            delegate: Item {
                id: delegateThingy

                width: parent.width
                height: textItem.height + seperatorImage.height

                clip: true

                Component.onCompleted: {
                    if (container.currentIndex > index) {
                        currentItemY = currentItemY + height;
                    }

                    if (container.currentIndex == index) {
                        currentItemHeight = height;
                    }
                }

                Rectangle {
                    id : highlighter
                    property int borderWidth : 6
                    radius : 1
                    x : borderWidth
                    width : parent.width - borderWidth*2
                    y : borderWidth
                    height : textItem.height - borderWidth*2

                    border.color:  "#2CACE3"
                    border.width: borderWidth
                    visible :  (container.currentIndex == index) ? true : false

                    // saddly, we need this to cover the rectangle's background color
                    Image {
                        id: backgroundImage
                        width: parent.width
                        height: parent.height
                        source: "image://theme/popupbox_2"
                    }
                }

                Text {
                    id: textItem

                    x: textMargin

                    width: parent.width - textMargin * 2
                    height: paintedHeight + textMargin * 2

                    verticalAlignment: Text.AlignVCenter

                    color: theme_contextMenuFontColor
                    font.pixelSize: theme_contextMenuFontPixelSize
                    font.italic: (type == itemType.groupItem) ? true : false
                    font.weight: (type == itemType.groupItem) ? Font.Black : ((container.currentIndex == index) ? Font.Bold : Font.Normal)

                    // elide has to be turned of to correctly compute the paintedWidth. It is re-enabled after the width computing
                    elide: if(elideEnabled){ Text.ElideRight; }else{ Text.ElideNone; }

                    text: label

                    Component.onCompleted: {
                        // This compares the paintedWidth to minWidth and maxWidth and sets the widgets width accordingly
                        if( paintedWidth + textMargin * 2  > container.currentWidth ){
                            if( paintedWidth + textMargin * 2 > maxWidth )
                                container.currentWidth = maxWidth;
                            else{
                                container.currentWidth = paintedWidth + textMargin * 2;
                            }
                        }
                        if( currentWidth > maxWidth )
                            currentWidth = maxWidth;
                    }
                }

                Image {
                    id: seperatorImage

                    anchors.top: textItem.bottom
                    width: currentWidth

                    visible: index < repeater.count - 1     // Seperator won't be visible for the last item

                    source: "image://theme/menu_item_separator"
                }

                MouseArea {
                    anchors.fill: parent

                    // pressed state for the text entry:
                    onClicked: {
                        if (type == itemType.optionItem) {
                            container.triggered(index)
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
                }
            }

            // enable text eliding when the correct width was computed
            Component.onCompleted: {
                elideEnabled = true;
            }
        }
    }

}
