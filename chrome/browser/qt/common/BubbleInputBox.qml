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

Item {
    id:inputBox

    property alias text: textInput.text
    property alias textColor: textInput.color
    property alias echoMode: textInput.echoMode
    property alias inputMask: textInput.inputMask
    property alias textFocus: textInput.focus

    signal returnKeyPressed(string text)
    signal textChanged(string text)

    Rectangle {
        id: inputField
        width: inputBox.width
        height: inputBox.height
        radius: 5
        color: "white"

        TextInput {
            id: textInput
            anchors.fill: parent
            anchors.leftMargin: 15
            anchors.topMargin: 13
            anchors.rightMargin: 15
            selectByMouse: true
            color: "white"
            font.pixelSize: 18
            font.family: theme_fontFamily
            focus: false
            echoMode: TextInput.Normal
            autoScroll: focus
            Keys.onReturnPressed: {
                inputBox.returnKeyPressed(textInput.text)
            }
            Keys.onReleased: {
                inputBox.textChanged(textInput.text)
            }
        }
    }
}
