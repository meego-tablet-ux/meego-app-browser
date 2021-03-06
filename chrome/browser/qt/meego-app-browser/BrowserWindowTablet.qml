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
import MeeGo.Components 0.1 as UX
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
    property bool appmode: is_appmode
    property bool showtoolbar: true
    property bool showbookmarkbar: false
    property bool hasfindbar: false
    
    //align QML panel behaviour, such as BookmarkManager, DownloadManager.
    property bool showbookmarkmanager: false
    property bool showdownloadmanager: false
    property bool showqmlpanel: showbookmarkmanager || showdownloadmanager 
    property string panelstring: ""

    property alias findbar: findBarLoader.item
    property alias ssldialog: sslDialogLoader.item
    property alias selectionhandler: selectionHandlerLoader.item
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
    
    property int webViewBorderSize: 20

    property bool showStartupAnimation: false
    property bool tabChangeFromTabSideBar: false
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
        ignoreUnknownSignals:true;
        onOrientationChanged: {
            if (!scene.orientationLocked)
            {
                scene.orientation = qApp.orientation;
            }
        }
    }

    Connections {
      target: browserWindow
      ignoreUnknownSignals:true;
      onHideAllPanel: {
        if (tabSideBarLoader.item && !tabChangeFromTabSideBar) {
          tabSideBarLoader.source = "";
        } else {
          tabChangeFromTabSideBar = false;
        }
        historyLoader.source = "";
        dialogLoader.source = "";
        selectFileDialogLoader.source = "";
//          popupListLoader.sourceComponent = undefined;
//          contextLoader.sourceComponent = undefined;
//          wrenchmenushown = false;
        bubbleLoader.source = "";
        showbookmarkmanager = false;
        showdownloadmanager = false;
     }
     onShowDownloads: {
       showdownloadmanager = is_show;
       if (is_show){
         if(downloadsLoader.source == "") {
           var mappedPos = scene.mapToItem (outerContent, 0, toolbar.height + statusbar.height)
           downloadsLoader.opacity = 1
           downloadsLoader.source = "Downloads.qml"
           downloadsHolder.initx = mappedPos.x
           downloadsHolder.inity = mappedPos.y
           downloadsLoader.item.showed = true
         }
         showdownloadmanager = true
         panelstring  = downloadTitle
         if (bookmarkManagerLoader.item)
             showbookmarkmanager = false
       } else {
         downloadsLoader.source = "";
         showdownloadmanager = false;
       }
     }
     onShowBookmarks: {
       showbookmarkmanager = is_show;
       if (is_show) {
         if(bookmarkManagerLoader.source=="") {
            var mappedPos = scene.mapToItem (outerContent, 0, toolbar.height + statusbar.height)
            bookmarkManagerLoader.opacity = 1
            bookmarkManagerLoader.source = "BookmarkList.qml"
            //bookmarkManagerLoader.item.portrait = !isLandscapeView()
            bookmarkManagerHolder.initx = mappedPos.x
            bookmarkManagerHolder.inity = mappedPos.y
          }
          showbookmarkmanager = true
          panelstring  = bookmarkManagerTitle
          if (downloadsLoader.item)
             showdownloadmanager = false
       } else {
         bookmarkManagerLoader.source = "" 
         showbookmarkmanager = false
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

    UX.StatusBar {
        id: statusbar
        anchors.top: parent.top
        width: container.width
        height: 30
        z: 10
        MouseArea {
			id: mouseArea
          anchors.fill: parent

          /*onClicked: {
            browserWindow.OrientationStart();
            scene.statusBarTriggered();
          }*/

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
            },
            State{
                name: "unfullscreen"
                when: !scene.fullscreen
                PropertyChanges {
                    target: statusbar
                    height: 30
                    opacity: 1
                }
            }
        ]
        transitions: [
            Transition {
                from: "unfullscreen"
                to: "fullscreen"
                reversible: true
                ParallelAnimation{
                    PropertyAnimation {
                        property: "height"
                        duration: 250
                        easing.type: "OutSine"
                    }
                    PropertyAnimation {
                        property: "opacity"
                        duration: 250
                        easing.type: "OutSine"
                    }
                }
            },
            Transition {
                from: "fullscreen"
                to: "unfullscreen"
                reversible: true
                ParallelAnimation{
                    PropertyAnimation {
                        property: "height"
                        duration: 250
                        easing.type: "OutSine"
                    }
                    PropertyAnimation {
                        property: "opacity"
                        duration: 250
                        easing.type: "OutSine"
                    }
                }
            }
        ]
	}

    Item {
        id: outerContent
        objectName: "outerContent"
        anchors.top: statusbar.bottom
        width: scene.width
        height: scene.height - statusbar.height
        clip:true
        UX.ThemeImage {
            id: background
            anchors.fill: parent
            source: "image://themedimage/widgets/apps/browser/scrolling-background"
        }

        BrowserToolbarTablet {
            id: toolbar
            property int tabSideBarX: getTabSideBarX()
            z: 10
            tabsidebar: tabSideBarLoader
            historypopup: historyLoader.item
        }
        Loader {
          id: bookmarkBarLoader
        }
        Connections {
          target: bookmarkBarModel
          ignoreUnknownSignals:true;
          onShow: {
            if (appmode)
              return;
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
          z: content.z       
        }
        Connections {
          target: infobarContainerModel
          ignoreUnknownSignals:true;
          onShow: {
            if (!appmode)
              infobarLoader.source = "InfoBarContainer.qml"
          }
        }
        Loader {
          id: sslDialogLoader
          anchors.horizontalCenter: innerContent.horizontalCenter
          anchors.verticalCenter: innerContent.verticalCenter
          source: ""
          z: 20
        }
        Connections {
          target: sslDialogModel
          ignoreUnknownSignals:true;
          onShow: {
            sslDialogLoader.source = "SslDialog.qml"
            ssldialog.msgHeadline =  headline
            ssldialog.msgDescription = description
            ssldialog.msgMoreInfo = moreInfo
            ssldialog.msgYesButton = buttonYes
            ssldialog.msgNoButton = buttonNo
            ssldialog.errorType = error;
            ssldialog.visible = true
            ssldialog.focus =  true
            ssldialog.show()
          }
          onHide: {
            sslDialogLoader.source = ""
          }
        }
        Window {
            // Wrapper window item to make TopItem bring in by bookmarklist.qml to work correctly
            id: bookmarkManagerHolder
            property int initx: 0
            property int inity: 0
            x: {!scene.fullscreen ? initx:0}
            y: {!scene.fullscreen ? inity:0}
            z: toolbar.z - 1  
            width: parent.width
            height: {!scene.fullscreen ? parent.height - y: parent.height}
            opacity: {showbookmarkmanager ? 1:0}

            Loader {
                id: bookmarkManagerLoader
                objectName: "bookmarkManagerLoader"
                anchors.fill: parent
                property bool portrait : false
            }
        }
        //Connections {
        //    target: bookmarkBarGridModel
        //    ignoreUnknownSignals:true;
        //    onCloseBookmarkManager: bookmarkManagerLoader.sourceComponent = undefined
        //    onOpenBookmarkManager: {
        //      var mappedPos = scene.mapToItem (outerContent, 0, toolbar.height + statusbar.height)
        //      bookmarkManagerLoader.opacity = 1
        //      bookmarkManagerLoader.source = "BookmarkList.qml"
        //      //bookmarkManagerLoader.item.portrait = !isLandscapeView()
        //      bookmarkManagerHolder.initx = mappedPos.x
        //      bookmarkManagerHolder.inity = mappedPos.y
        //      showbookmarkmanager = true
        //      panelstring  = bookmarkManagerTitle
        //      if (downloadsLoader.item)
        //        showdownloadmanager = false
        //    }
        //}
        //Connections {
        //    target: bookmarkOthersGridModel
        //    ignoreUnknownSignals:true;
        //    onCloseBookmarkManager: bookmarkManagerLoader.sourceComponent = undefined
        //}

        Loader {
            id: selectionHandlerLoader
            source: ""
        }
        Connections {
            target: selectionHandler
            ignoreUnknownSignals:true;
            onShow: {
                selectionHandlerLoader.source = "SelectionHandler.qml"
                selectionhandler.parent = webview
                selectionhandler.z = webview.z + 1
                selectionhandler.anchors.fill = webview
            }
            onHide: {
                selectionHandlerLoader.source = ""
            }
        }

        Loader {
            id: findBarLoader
        }
        Connections {
            target: findBarModel
            ignoreUnknownSignals:true;
            onShow: {
              if (!showqmlpanel) {
                findBarLoader.source = "FindBar.qml"
                findbar.parent = toolbar
                findbar.x = toolbar.x
                findbar.y = toolbar.y
                findbar.height = toolbar.height
                findbar.z = toolbar.z + 1
                hasfindbar = true
                findbar.showfindbar = true
                //findBarModel.positionUpdated(toolbar.x, toolbar.y, toolbar.width, toolbar.height);
                findbar.textentry.textFocus = true
              } else {
                if (hasfindbar) {
                  findbar.showfindbar = false
                  hasfindbar = false
                  findBarLoader.source = ""
                }
                if (downloadsLoader.item)
                  downloadsLoader.item.textFocus = true;
                if (bookmarkManagerLoader.item)
                  bookmarkManagerLoader.item.textFocus = true;
              }
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
            start_x: toolbar.tabSideBarX
            start_y: mappedPos.y
            property int maxSideBarHeight 
            maxSideBarHeight: parent.height - start_y
            property bool up: true
            z: 10
            property bool showItem: source != ""
        }
        Connections {
            target: tabSideBarModel
            ignoreUnknownSignals:true;
            onShow: {
                tabSideBarLoader.source = "TabSideBar.qml" 
            }
            onHide: {
                tabSideBarLoader.source = "" 
            }
        }
        Window {
            // Wrapper window item to make TopItem bring in by Downloads.qml to work correctly
            id: downloadsHolder
            property int initx: 0
            property int inity: 0
            x: {!scene.fullscreen ? initx:0}
            y: {!scene.fullscreen ? inity:0}
            z: toolbar.z - 1 
            width: parent.width
            height: {!scene.fullscreen ? parent.height - y: parent.height}
            opacity: {showdownloadmanager ? 1:0}
            Loader {
              id: downloadsLoader
              anchors.fill: parent
            } 
        }
        Connections {
          target: downloadsObject
          ignoreUnknownSignals:true;
          onShow: {
            var mappedPos = scene.mapToItem (outerContent, 0, toolbar.height + statusbar.height)
            downloadsLoader.opacity = 1
            downloadsLoader.source = "Downloads.qml"
            downloadsHolder.initx = mappedPos.x
            downloadsHolder.inity = mappedPos.y
            downloadsLoader.item.showed = true
            showdownloadmanager = true
            panelstring  = downloadTitle
            if (bookmarkManagerLoader.item)
              showbookmarkmanager = false
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
                BorderImage {
                    id: webViewBorderImage
                    anchors.top: webview.top
                    anchors.topMargin: 0 - webViewBorderSize
                    anchors.bottom: webview.bottom
                    anchors.bottomMargin: 0 - webViewBorderSize
                    anchors.left: webview.left
                    anchors.leftMargin: 0 - webViewBorderSize
                    anchors.right: webview.right
                    anchors.rightMargin: 0 - webViewBorderSize
                    border {left: webViewBorderSize; right: webViewBorderSize; top:webViewBorderSize ; bottom:webViewBorderSize }
                    horizontalTileMode: BorderImage.Repeat
                    verticalTileMode: BorderImage.Repeat
                    source: "image://themedimage/widgets/apps/browser/scrolling-shadow"
                }
            }
        }

        ScrollBar {
            id: innerContentVerticalBar
            width: 8 
            anchors { right: innerContent.right; top: innerContent.top; bottom: innerContent.bottom }
            pageSize: innerContent.visibleArea.heightRatio
            position: innerContent.visibleArea.yPosition
            backgroundOpacity: 1.0
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
            backgroundOpacity: 1.0
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
            ignoreUnknownSignals:true;
            onShowHistoryStack: {
                var mappedPos = scene.mapToItem (outerContent, scene.lastMousePos.mouseX, scene.lastMousePos.mouseY);
                historyLoader.source = "HistoryBar.qml"
                //historyLoader.item.historyBarY = toolbar.y + toolbar.height
                historyLoader.item.showed = true
				historyLoader.item.itemCount = count
                historyLoader.item.fingerX= mappedPos.x
                historyLoader.item.fingerY= mappedPos.y
                historyLoader.item.parent = outerContent
                historyLoader.item.parentWidth = outerContent.width
            }
        }
    }

    Window {
        // this screen layer is used to show items that need fog for whole screen.
        // It must use the Window Item. And It must been set as the parent of those
        // items who eventually use TopItem to detect top level qml window
        id: screenLayer
        objectName: "screenlayer"
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
                target: bookmarkManagerLoader
                portrait: false
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
                target: bookmarkManagerLoader
                portrait: false
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
                target: bookmarkManagerLoader
                portrait: true
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
                target: bookmarkManagerLoader
                portrait: true
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

            SequentialAnimation {
              PropertyAnimation {
                exclude:statusBar
                properties: "transformX,transformY,width,height"
                duration: 500
                easing.type: "OutSine"
              }
              ScriptAction {
                script: {
                  browserWindow.OrientationEnd(scene.orientation);
                }
              }
            }
        }
    ]
}
