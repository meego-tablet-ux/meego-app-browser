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
import Qt.labs.gestures 2.0


TimedOverlay {
    containerInterval: 3000
    z: parent.z + 1
    anchors.fill: parent

    BorderImage {
        id: container

        source: "image://theme/navigationBar_l"

        width: parent.width
        height: 60

        anchors.bottom: parent.bottom

        property bool containerVisible: parent.containerVisible
        property bool fullscreen: scene.fullscreen

        states: [
            State {
                name: "show"
                when: containerVisible && fullscreen
                PropertyChanges {
                    target: container
                    height: 50
                    opacity: 0.7
                }
            },
            State {
                name: "hide"
                when: !(containerVisible && fullscreen)
                PropertyChanges {
                    target: container
                    height: 0
                    opacity: 0
                }
            }
        ]

        transitions: [
            Transition {
                from: "show"
                to: "hide"
                PropertyAnimation {
                    properties: "anchors.topMargin,height,opacity"
                    duration: 500
                }
            },
            Transition {
                from: "hide"
                to: "show"
                PropertyAnimation {
                    properties: "anchors.topMargin,height,opacity"
                    duration: 500
                }
            }
        ]

        GestureArea {
            anchors.fill: parent
            Tap {}
            TapAndHold {}
            Pan {}
            Pinch {}
            Swipe {}
        }

        Item {
            width: parent.width
            height: 50
            anchors.top: parent.top
            anchors.topMargin: 5
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 5

            Text {
                id: exitLabel
                text: fullscreenBubbleLabel
                objectName: "exitFullscreenLabel"
                color: "white"
                height: 50
                elide: Text.ElideRight
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignHCenter

                anchors.right: yesButton.left
                anchors.rightMargin: 10
            }

            Button {
                id: yesButton;
                objectName: "exitFullscreenYesButton"
                text: fullscreenBubbleYes
                color: "white"
                height: 40
                width: 100
                bgSourceUp: "image://theme/btn_blue_up"
                bgSourceDn: "image://theme/btn_blue_dn"

                anchors.right: noButton.left
                anchors.rightMargin: 5

                onClicked: {
                    fullscreenBubbleObject.OnYesButton()
                }
            }

            Button {
                id: noButton;
                objectName: "exitFullscreenNoButton"
                text: fullscreenBubbleNo
                color: "white"
                height: 40
                width: 100
                bgSourceUp: "image://theme/btn_red_up"
                bgSourceDn: "image://theme/btn_red_dn"

                anchors.right: parent.right
                anchors.rightMargin: 5

                onClicked: {
                    container.state = "hide"
                }//end of nobutton clicked signal
            }// end of noButton
        }//end of item
    }//end of BorderImage
}
