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
import QtMobility.sensors 1.1
import Qt.labs.gestures 2.0

Item {
    id: scene

    // If we are running using the executable wrapper, then we will have a
    // screenWidth and screenHeight added to our context.  If running via
    // qmlviewer then fallback to a reasonable default
    width: {
        try
        {
            screenWidth;
        }
        catch (err)
        {
            1280;
        }
    }
    height: {
        try
        {
            screenHeight;
        }
        catch (err)
        {
            800;
        }
    }

    // public
    property alias content: innerContent
    property alias webview: webview 
    property alias container: outerContent
    property alias screenlayer: screenLayer

    property bool fullscreen: is_fullscreen
    property bool showtoolbar: true
    property bool showbookmarkbar: false
    property bool hasfindbar: false
    property alias findbar: findBarLoader.item
    property alias showsearch: toolbar.showsearch
    property alias toolbarheight: toolbar.height
    property alias wrenchmenushown: toolbar.wrenchmenushown
    property alias statusBar: statusbar

    property color backgroundColor: theme_backgroundColor
    property color sidepanelBackgroundColor: theme_sidepanelBackgroundColor

    signal search(string needle)

    property alias crumbMaxWidth: toolbar.crumbMaxWidth
    property alias crumbHeight: toolbar.crumbHeight
    property alias crumbTrail: toolbar.crumbTrail
    signal crumbTriggered(variant payload)

    signal statusBarTriggered()

    property bool showStartupAnimation: false

    // We support four orientations:
    // 0 - Right Uup
    // 1 - Top Up
    // 2 - Left Up
    // 3 - Top Down
    property int orientation: qApp.orientation
    property bool orientationLocked: false
    onOrientationLockedChanged: {
        if (!orientationLocked)
        {
            orientation = qApp.orientation;
        }
    }

    // Disable orientation transitions on startup
    property bool orientationTransitionEnabled: false
    Timer {
        running: true
        repeat: false
        interval: 1000
        onTriggered: scene.orientationTransitionEnabled = true;
    }

    Connections {
        target: qApp
        onOrientationChanged: {
            if (!scene.orientationLocked)
            {
                scene.orientation = qApp.orientation;
            }
        }
    }

    // private
    Item {
        id: privateData
        property int transformX: 0
        property int transformY: 0
        property int angle: 0
    }
/*
    ApplicationsModel {
        id: appsModel
    }
    WindowModel {
        id: windowModel
    }
*/
    // This function returns if the view is in landscape or inverted landscape
    function isLandscapeView() {
      if (screenHeight <= screenWidth)
        return orientation == 1 || orientation == 3;
      else 
        return orientation == 0 || orientation == 2;
    }

    function showModalDialog (source) {
        if (typeof (source) == "string") {
            dialogLoader.source = source;
        } else {
            dialogLoader.sourceComponent = source;
        }
    }

    transform: [
        Rotation {
            id: rotationTransform
            angle: privateData.angle
        },
        Translate {
            id: translateTransform
            x: privateData.transformX
            y: privateData.transformY
        }
    ]
    transformOrigin: Item.TopLeft

    StatusBar {
        id: statusbar
        anchors.top: parent.top
        width: container.width
        height: 30
        z: 10
        MouseArea {
			id: mouseArea
            anchors.fill: parent
			//onClicked: scene.statusBarTriggered()

			property int firstY: 0
			property int firstX: 0
			property bool triggered: false

			onPressed: {
				triggered = false;
				firstY = mouseY;
				firstX = mouseX;
			}
			onMousePositionChanged: {
				if(!triggered && mouseArea.pressed) {
					if(Math.abs(mouseX - firstX) < Math.abs(mouseY - firstY)) {
						if( mouseArea.mouseY - mouseArea.firstY > 10) {
							mainWindow.triggerSystemUIMenu()
							triggered = true
						}

					}

				}
			}
        }
        states: [
            State {
                name: "fullscreen"
				when: scene.fullscreen
                PropertyChanges {
                    target: statusbar
                    height: 0
                    opacity: 0
                }
            }
        ]
        transitions: [
            Transition {
                from: ""
                to: "fullscreen"
                reversible: true
                PropertyAnimation {
                    properties: "height"
                    duration: 500
                    easing.type: "OutSine"
                }
            }
        ]
	}

    Item {
        id: outerContent
        anchors.top: statusbar.bottom
        width: scene.width
        height: scene.height - statusbar.height
        clip:true
        Image {
            id: background
            anchors.fill: parent
            fillMode: Image.Tile
            source: "image://themedimage/images/bg_application_p"
        }

        BrowserToolbarTablet {
            id: toolbar
            z: 10
            tabsidebar: tabSideBarLoader
            historypopup: historyLoader.item
        }
        Loader {
          id: bookmarkBarLoader
        }
        Connections {
          target: bookmarkBarModel
          onShow: {
            var mappedPos = scene.mapToItem (outerContent, 0, toolbar.height + statusbar.height)
            bookmarkBarLoader.source = "BookmarkBar.qml"
            bookmarkBarLoader.item.x = mappedPos.x 
            bookmarkBarLoader.item.y = mappedPos.y
            scene.showbookmarkbar = 1
            bookmarkBarLoader.item.parent = outerContent
          }
          onHide: {
            scene.showbookmarkbar = 0
          }
        }
        Loader {
          id: infobarLoader
          anchors.left: outerContent.left
          anchors.top: toolbar.bottom
          anchors.topMargin: bookmarkBarLoader.height
          width: parent.width
          z: 5
        }
        Connections {
          target: infobarContainerModel
          onShow: {
            infobarLoader.source = "InfoBarContainer.qml"
          }
        }

        Loader {
            id: bookmarkManagerLoader
        }
        Connections {
            target: bookmarkBarListModel
            onCloseBookmarkManager: bookmarkManagerLoader.sourceComponent = undefined
            onOpenBookmarkManager: {
                bookmarkManagerLoader.source = "BookmarkList.qml"
                bookmarkManagerLoader.item.parent = outerContent 
                bookmarkManagerLoader.item.z = toolbar.z + 1
            }
        }
        Connections {
            target: bookmarkOthersListModel
            onCloseBookmarkManager: bookmarkManagerLoader.sourceComponent = undefined
        }

        Loader {
            id: findBarLoader
            source: "FindBarNull.qml"
        }
        Connections {
            target: findBarModel
            onShow: {
                findBarLoader.source = "FindBar.qml"
                findbar.parent = innerContent 
//                findbar.width = isLandscapeView()? outerContent.width / 2 - 20 : outerContent.width - 20

                var mappedPos = scene.mapToItem (outerContent, 0, toolbar.height + statusbar.height + infobarLoader.height + bookmarkBarLoader.height)
                var ix = innerContent.width / 2
                var ih = 50
                findbar.x = ix
                findbar.width = ix
                findbar.height = ih
                findbar.z = innerContent.z + 1
                findbar.showfindbar = true
                hasfindbar = true
                findBarModel.positionUpdated(ix, mappedPos.y, ix, ih);
            }
        }
        
        Loader {
            id: tabSideBarLoader
            objectName: "tabSideBarLoader"
            anchors.fill: parent

            property variant mappedPos
            mappedPos : scene.mapToItem (parent, scene.lastMousePos.mouseX, scene.lastMousePos.mouseY);
            property int start_x
            property int start_y
            start_x: mappedPos.x
            start_y: mappedPos.y
            property int maxSideBarHeight 
            maxSideBarHeight: parent.height - start_y
            property bool up: true
            z: 10
            property bool showItem: source != ""
        }
        Connections {
            target: tabSideBarModel
            onShow: {
                tabSideBarLoader.source = "TabSideBar.qml" 
            }
            onHide: {
                tabSideBarLoader.source = "" 
            }
        }

        Loader {
            id: downloadsLoader
        }

        Connections {
          target: downloadsObject
          onShow: {
            downloadsLoader.source = "Downloads.qml"
            downloadsLoader.item.showed = true
            downloadsLoader.item.parent = outerContent
          }
        }

        Flickable {
            id: innerContent
            anchors.left: outerContent.left
            anchors.top: infobarLoader.bottom
            height:outerContent.height - toolbar.height - infobarLoader.height - bookmarkBarLoader.height
            width: outerContent.width
            contentWidth: webview.width
            contentHeight: webview.height
            objectName: "innerContent"
            boundsBehavior: Flickable.DragOverBounds
            clip:true
            Item {
                id: webview
                objectName: "webView"
            }
        }

        ScrollBar {
            id: innerContentVerticalBar
            width: 8 
            anchors { right: innerContent.right; top: innerContent.top; bottom: innerContent.bottom }
            pageSize: innerContent.visibleArea.heightRatio
            position: innerContent.visibleArea.yPosition
            opacity: 0
            states:  
                State {
                    name: "ShowScrollBar"
                    when: innerContent.movingVertically
                    PropertyChanges { target: innerContentVerticalBar; opacity: 1}
                }
        }

        ScrollBar {
            id: innerContentHorizontalBar
            height: 8; orientation: Qt.Horizontal
            anchors { right: innerContent.right; rightMargin: 8; left: innerContent.left; bottom: innerContent.bottom }
            pageSize: innerContent.visibleArea.widthRatio
            position: innerContent.visibleArea.xPosition
            opacity: 0
            states:  
                State {
                    name: "ShowScrollBar"
                    when: innerContent.movingHorizontally
                    PropertyChanges { target: innerContentHorizontalBar; opacity: 1}
                }
        }


        Loader {
            id: historyLoader
        }

        Connections {
            target: browserToolbarModel
            onShowHistoryStack: {
                var mappedPos = scene.mapToItem (outerContent, scene.lastMousePos.mouseX, scene.lastMousePos.mouseY);
                historyLoader.source = "HistoryBar.qml"
                //historyLoader.item.historyBarY = toolbar.y + toolbar.height
                historyLoader.item.showed = true
                historyLoader.item.fingerX= mappedPos.x
                historyLoader.item.fingerY= mappedPos.y
                historyLoader.item.parent = outerContent
            }
        }
    }

    Item {
        // this screen layer is used to show items that need fog for whole screen.
        id: screenLayer
        width: { 
          if (privateData.angle == 90 ||privateData.angle == -90) {
            return scene.height
          } else {
            return scene.width
          }
        }

        height: { 
          if (privateData.angle == 90 ||privateData.angle == -90) {
            return scene.width
          } else {
            return scene.height
          }
        }
        z: 10
    }

    states: [
        State {
            name: "landscape"
            when: scene.orientation == 1
            PropertyChanges {
                target: statusbar
            //    mode: 0
            }
            PropertyChanges {
                target: outerContent
                width: scene.width
                height: scene.height - statusbar.height
            }
            PropertyChanges {
                target: privateData
                angle: 0
                transformX: 0
            }
            PropertyChanges {
                target: findbar
                width: scene.width / 2
                x: scene.width / 2
            }
        },

        State {
            name: "invertedlandscape"
            when:scene.orientation == 3
            PropertyChanges {
                target: statusbar
            //    mode: 0
            }
            PropertyChanges {
                target: outerContent
                width: scene.width
                height: scene.height - statusbar.height
            }
            PropertyChanges {
                target: privateData
                angle: 180
                transformX: scene.width
                transformY: scene.height
            }
            PropertyChanges {
                target: findbar
                width: scene.width / 2
                x: scene.width / 2
            }
        },

        State {
            name: "portrait"
            when: scene.orientation == 0
            PropertyChanges {
                target: statusbar
            //    mode: 1
            }
            PropertyChanges {
                target: outerContent
                width: scene.height
                height: scene.width - statusbar.height
            }
            PropertyChanges {
                target: privateData
                angle: 90
                transformX: scene.width
            }
            PropertyChanges {
                target: findbar
                width: scene.height / 2
                x: scene.height / 2
            }
        },

        State {
            name: "invertedportrait"
            when:scene.orientation == 2
            PropertyChanges {
                target: statusbar
              //  mode: 1
            }
            PropertyChanges {
                target: outerContent
                width: scene.height
                height: scene.width - statusbar.height
            }
            PropertyChanges {
                target: privateData
                angle: -90
                transformY: scene.height
            }
            PropertyChanges {
                target: findbar
                width: scene.height / 2
                x: scene.height / 2
            }
        }
    ]
    transitions: [
        Transition {
            from: "*"
            to: "*"
            reversible: true

            RotationAnimation{
                direction: RotationAnimation.Shortest
            }

            PropertyAction {
                target: statusBar;
                property: "width";
                value: scene.width
            }

            PropertyAnimation {
                exclude:statusBar
                properties: "transformX,transformY,width,height"
                duration: 500
                easing.type: "OutSine"
            }
        }
    ]


}
