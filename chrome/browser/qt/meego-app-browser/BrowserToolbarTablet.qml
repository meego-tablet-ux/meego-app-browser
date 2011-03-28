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

Item {
    id: container
    width: parent.width
    height: 50
    anchors.top: parent.top
    opacity: 1
    onCrumbTriggered: scene.crumbTriggered(payload)
    onSearch: scene.search(needle)
    onAppSwitcherTriggered: mainWindow.showTaskSwitcher()

    property alias locationBar: omniboxcontainer
    property alias tabsidebar: tabButton.tabSidebar
    property alias historypopup: backForwardButton.popup
    property alias wrenchmenushown: wrenchButton.shown

    property bool showsearch: true
    // to comply with window
    property int crumbTrail: 0
    property int crumbMaxWidth: 160
    property int crumbHeight: height

    property int buttonWidth: 65
    property int buttonHeight: container.height

    signal crumbTriggered(variant payload)
    signal search(string needle)
    signal appSwitcherTriggered()

    states: [
        State {
            name: "show"
            when: scene.showtoolbar && !scene.fullscreen
            PropertyChanges {
                target: toolbar
                height: 50
                opacity: 1
            }
        },
        State {
            name: "hide"
            when: !scene.showtoolbar || scene.fullscreen
            PropertyChanges {
                target: toolbar
                height: 0
                opacity: 0
            }
        }
    ]

    transitions: [
        Transition {
            from: "*"
            to: "*"
            PropertyAnimation {
                properties: "anchors.topMargin,height,opacity"
                duration: 500
            }
        }
    ]

    BorderImage {
        id: background
        source: "image://meegotheme/images/titlebar_l"
        width: parent.width
        height: 50

        anchors.leftMargin: 10
        anchors.rightMargin: 10

        BackForwardButton {
            id: backForwardButton
            height: buttonHeight
            width: buttonWidth
            anchors.left: parent.left
            // change its icons to listen to signals from browserToolbarModel
            Connections {
                target: browserToolbarModel
                onUpdateBfButton: {
                    backForwardButton.kind = kind
                    backForwardButton.active = active
                }
            }
        }

        Image {
            id: divider1
            anchors.left: backForwardButton.right
            height: buttonHeight - 3
            source: "image://meegotheme/widgets/common/toolbar/toolbar-item-separator"
        }

        ReloadButton {
            id: reloadButton
            anchors.left: divider1.right
            height: buttonHeight
            width: buttonWidth

            Connections {
                target: browserToolbarModel
                onUpdateReloadButton: {
                    reloadButton.loading = is_loading
                }
            }
        }

        Image {
            id: divider2
            anchors.left: reloadButton.right
            height: buttonHeight - 3
            source: "image://meegotheme/widgets/common/toolbar/toolbar-item-separator"
        }

        Omnibox {
            id: omniboxcontainer
            popupHeight: innerContent.height/2
            z: 10
            anchors.right: divider3.left
            anchors.left: divider2.right
            onActiveFocusChanged: {
                if (activeFocus == true){
                    omniboxcontainer.anchors.left = divider1.right
		    omniboxcontainer.anchors.right = divider4.left
                } else {
                    omniboxcontainer.anchors.left = divider2.right
		    omniboxcontainer.anchors.right = divider3.left
                }
            }
        }

        Image {
            id: divider3
            anchors.right: tabButton.left
            height: buttonHeight - 3
            source: "image://meegotheme/widgets/common/toolbar/toolbar-item-separator"
        }

        TabButton {
            id: tabButton
            anchors.right: divider4.left
            height: buttonHeight
            width: buttonWidth
            popupDirection: 1
        }

        Image {
            id: divider4
            anchors.right: wrenchButton.left
            height: buttonHeight - 3
            source: "image://meegotheme/widgets/common/toolbar/toolbar-item-separator"
        }

        WrenchButton {
            id: wrenchButton
            height: buttonHeight
            width: buttonWidth
            anchors.right: parent.right
            anchors.rightMargin: 20
            popupDirection: 1
            topWidget: scene
        }
    }
}
