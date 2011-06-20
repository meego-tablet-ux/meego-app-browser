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

Flickable {

    id: container

    property alias model: repeater.model

    property int minWidth : 200     // The width of the menu should always be between minWidth and maxWidth
    property int maxWidth : 500
    property int currentWidth: minWidth
    property int textMargin : 20

    property bool elideEnabled: false
    property int currentIndex : 0
    property Item currentItem : null
    property int currentItemY : 0
    property int currentItemHeight : 0
    property int realheight : 200
    property int maxHeight : 450

    signal triggered(int index)

    // currentWidth is the current width of the largest text width, clamped between minWidth and maxWidth
    width: currentWidth

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

    function positionCurrentItem() {
        //console.log("container.height " + container.height);
        //console.log("maxHeight " + maxHeight);
        //console.log("currentItemY " + currentItemY);
        //console.log("contentHeight" + contentHeight);
        //console.log("realHeight" + realheight);
        // we try to position currentItem at the center of the layout
        // if that's not possible, scroll the flickable to either end
        // so that the currentItem is positioned as near to the center as posible
        
        if(!container.interactive)
            return;

        // Note: container.height is setted by the invoker
        if (currentItemY < container.height/2 ){
            container.contentY = 0;
        }else if (currentItemY + container.height/2 > realheight ){
            if(realheight - container.height < 0){
                container.contentY = 0
            }else{
                container.contentY = realheight - container.height;
            }
        }else{
            container.contentY = currentItemY - container.height/2
        }
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

                width: repeater.width
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


                Image {
                    id: highLighterImage
                    width: 22
                    height: 18
                    visible : (container.currentIndex == index) ? true : false
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.rightMargin: 20
                    anchors.topMargin: parent.height/2 - highLighterImage.height/2
                    source: "image://themedimage/widgets/apps/browser/tick"
                }
                

                Text {
                    id: textItem
                    x: textMargin
                    width: parent.width - textMargin - 70
                    height: 45

                    verticalAlignment: Text.AlignVCenter

                    color: theme_contextMenuFontColor
                    font.pixelSize: theme_contextMenuFontPixelSize

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
                    source: "image://themedimage/images/menu_item_separator"
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
        }

        // enable text eliding when the correct width was computed
        Component.onCompleted: {
                container.realheight = layout.height
                positionCurrentItem();
                elideEnabled = true;
        }
    }
}
