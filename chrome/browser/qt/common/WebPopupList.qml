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
import MeeGo.Labs.Components 0.1

Item {
    id: container
    property variant model : []
    property int minPopupListWidth : 200
    property int maxPopupListWidth : 600
    property int maxPopupListHeight : 600
    property int currentIndex : 0

    anchors.fill : parent
    visible : false

    function show(x, y) {
        visible = true;
        webPopupListContext.display(x,y);
    }

    function close() {
        visible = false;
    }

    function selectItem(index) {
        model.itemInvoked(index);
        close();
    }

    function canceled() {
        model.uiCanceled();
        close();
    }

    AbstractContext {
        id: webPopupListContext
        function display(x, y) {
            webPopupListContext.mouseX = x;
            webPopupListContext.mouseY = y;
            visible = true;
        }

        onClose: {
            if (container.visible == true) {
                container.canceled();
            }
        }

        content: WebPopupListView {
            model: container.model
            minWidth: minPopupListWidth
            maxWidth: maxPopupListWidth
            maxHeight: maxPopupListHeight
            currentIndex: container.currentIndex
            onTriggered: {
                container.selectItem(index);
            }
        }

    }
}
