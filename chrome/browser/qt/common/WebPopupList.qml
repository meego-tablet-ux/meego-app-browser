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
    id: container
    property variant model : []
    property int minPopupListWidth : 200
    property int maxPopupListWidth : 600
    property int maxPopupListHeight : 450
    property int constMaxPopupListHeight : 450
    property int currentIndex : 0
    property Item targetContainer : null
    property Item contentItem: null
    property int screenHeight: 0
    property int contextMenuExtraHeight: 60
    property int maxAllowedHeight: 0
    property int ddd: 85
    property int fff: 0
    anchors.fill : parent
    visible : false

    function show(x, y) {
        if(container.contentItem) container.contentItem.destroy();
            container.contentItem = popupholder.createObject (container);
        visible = true;
        container.contentItem.display(x,y);
    }

    function close() {
        visible = false;
        if(container.contentItem) {
            container.contentItem.destroy();
            container.contentItem = null;
        }
    }

    function selectItem(index) {
        close();
    }

    function canceled() {
        model.uiCanceled();
        close();
    }
     
    Component {
        id : popupholder


        ContextMenu {
            id: webPopupListContext
            targetContainer: container.targetContainer

            property int toolbarHeight: 0
            function display(x, y) {

                console.log("full:" + scene.fullscreen);
                console.log("height:" + toolbarHeight);
                console.log("sh:" + screenHeight);
                var pageSize = screenHeight - toolbarHeight;
                var midOfPage = (screenHeight - toolbarHeight)/2 + toolbarHeight;

                if (y> midOfPage){
                    webPopupListContext.forceFingerMode = 3;
                    if(y - midOfPage + pageSize/2 > contextMenuExtraHeight + constMaxPopupListHeight){
                        maxAllowedHeight = constMaxPopupListHeight;
                    }else{
                        maxAllowedHeight = y - midOfPage + pageSize/2 - contextMenuExtraHeight;
                    }
                }else{
                    webPopupListContext.forceFingerMode = 2;
                    if(midOfPage - y + pageSize/2 > constMaxPopupListHeight + contextMenuExtraHeight){
                        maxAllowedHeight = constMaxPopupListHeight;
                    }else{
                        maxAllowedHeight = midOfPage - y + pageSize/2 - contextMenuExtraHeight;
                    }
                }

                if(maxAllowedHeight > contextMenuContent.realheight){
                    contextMenuContent.height = contextMenuContent.realheight;
                    contextMenuContent.interactive = false;
                }else{
                    contextMenuContent.height = maxAllowedHeight;
                    contextMenuContent.interactive = true;
                }

                webPopupListContext.setPosition(x, y);
                // The invoke sequence is positionCurrentItem() -> display()
                // Since we modified the contextMenuConten.height which positionCurrentItem() used,
                // We need reinvoke the positionCurrentItem()
                contextMenuContent.positionCurrentItem();
                webPopupListContext.show();
            }

            onFogHideFinished: {
                if (container.visible == true) {
                    container.canceled();
                }
            }

          
            content : WebPopupListView {
                id: contextMenuContent
                model: container.model
                minWidth: minPopupListWidth
                maxWidth: maxPopupListWidth
                maxHeight: maxPopupListHeight
                currentIndex: container.currentIndex
                onTriggered: {
                    model.itemInvoked(index);
                    webPopupListContext.hide();
                    container.selectItem(index);
                }
            }
            states: [
                State {
                    name: "fullscreen"
                    when: scene.fullscreen
                    PropertyChanges {
                        target: webPopupListContext
                        toolbarHeight : 0
                    }
                },

                State {
                    name: "unfullscreen"
                    when: !scene.fullscreen
                    PropertyChanges{
                        target: webPopupListContext
                        toolbarHeight: 85
                    }
                }
            ]
        }
    }


    states: [
        State {
            name: "horizontal"
            when: scene.orientation == 1 || scene.orientation == 3
            PropertyChanges {
                target: container
                screenHeight: scene.height
            }
        },

        State {
            name: "vertical"
            when: scene.orientation == 2 || scene.orientation == 0
            PropertyChanges {
                target: container
                screenHeight: scene.width
            }
        }
    ]
}
