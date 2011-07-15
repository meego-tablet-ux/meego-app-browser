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
    id: dItem
    property alias warning_text: dangerousWarning.text
    property alias save_text: saveDangerousButton.text
    property alias discard_text: discardDangerousButton.text
    property alias title_text: titleText.text
    signal save
    signal discard

    Text {
        id: titleText
        anchors.top: parent.top
        font.pixelSize: 18
        font.underline: false
        color: "black";
    }

    Text {
        id: dangerousWarning
        anchors.top: titleText.bottom
        width: parent.width
        color: "red"
        font.pixelSize: 20
        wrapMode: Text.WordWrap
    }
    Item {
        id: dangerousButtons
        anchors.top: dangerousWarning.bottom
        anchors.topMargin: 10
        anchors.left: dangerousWarning.left
        anchors.leftMargin: 5
        height: childrenRect.height
        TextButton {
            id: saveDangerousButton
            width: 150
            onClicked: dItem.save();
        }
        TextButton {
            id: discardDangerousButton
            width: 150
            anchors.left: saveDangerousButton.right
            anchors.leftMargin: 20
            onClicked: dItem.discard();
        }
    }
}
