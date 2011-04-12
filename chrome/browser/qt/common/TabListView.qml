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
  // Represent the tab list view, show thumbnails of each tab
  // Call c++ methods to open/close pages
  id: container
  property variant model
  property int maxHeight: 600

  property int innertabHeight: 40
  property int titleHeight: 50

  property int tabTitleHeight: 30
  property int countSize: 40
  property int thumbnailMargin: 5
  property int counterSpacing: 5

  property int tabHeight: width/1.5


  width: parent.width
  height: tabSideBarListView.count * tabHeight > maxHeight? maxHeight:tabSideBarListView.count*tabHeight

  // default margin is 4
  property int commonMargin : 4

  clip: true

  Component {
    id: tabDelegate
    Rectangle {
      id: tabContainer
//      color: "#383838"
      border.width: 4
      border.color: "grey"
      width: parent.width
      height: width/1.5

      anchors {
         leftMargin: commonMargin
         rightMargin: commonMargin
         topMargin: commonMargin * 2
         bottomMargin: commonMargin * 2
      }


      //background
      Image {
          id: overlay
          anchors.fill: parent
          source: "image://theme/browser/bg_favouritesoverlay"
      }

      // thumbnail of the tab
      Rectangle {
        id: thumb
        width: parent.width - 8* commonMargin
        height: parent.height * 0.7 - 2* commonMargin

        anchors.top: parent.top
        anchors.left: parent.left

        anchors.leftMargin: 4* commonMargin
        anchors.topMargin: 2* commonMargin

        Image {
          anchors.fill: parent
          fillMode: Image.PreserveAspectFit
          smooth: true
          source: "image://tabsidebar/thumbnail_" + index + "_" + thumbnail
        }
      }

      // title of the tab
      Text {
        id: tabtitle
        height: parent.height * 0.3
        width: parent.width

        anchors.bottom : parent.bottom
        anchors.topMargin: -commonMargin
        anchors.left: parent.left
        anchors.leftMargin: 4*commonMargin
        anchors.right: parent.right
        anchors.rightMargin: 4*commonMargin

        verticalAlignment: Text.AlignVCenter
        horizontalAlignment: Text.AlignHCenter

        //font.pixelSize: height * 0.6
        //font.bold: true
	font.pixelSize: theme_fontPixelSizeNormal
        elide: Text.ElideRight
        text: title
        color: "white"
      }

      Item {
        id: close
        z: 1
        height: 60
        width: height
        anchors.right: parent.right
        Image {
          id: closeIcon
          anchors.centerIn: parent
          source: "image://theme/icn_close_up"
          property bool pressed: false
          states: [
            State {
              name: "pressed"
              when: closeIcon.pressed
              PropertyChanges {
                target: closeIcon
                source: "image://theme/icn_close_dn"
              }
            }
          ]
        }

        MouseArea {
          anchors.fill: parent
          onClicked: tabSideBarModel.closeTab(index)
          onPressed: closeIcon.pressed = true
          onReleased: closeIcon.pressed = false
        }
      } // Item of close

      MouseArea {
        anchors.fill: parent
        onClicked: tabSideBarModel.go(index)
      }

      states: [
        State {
            name: "highlight"
            when: index == tabSideBarListView.currentIndex
            PropertyChanges {
              target: tabContainer
              radius: 2
              border.width: 5
              border.color: "#2CACE3"
            }
        } 
      ]

      ListView.onRemove: SequentialAnimation {
         PropertyAction { target: tabContainer; property: "ListView.delayRemove"; value: true }
         NumberAnimation { target: tabContainer; property: "height"; to: 0; duration: 500; easing.type: Easing.InOutQuad }

         // Make sure delayRemove is set back to false so that the item can be destroyed
         PropertyAction { target: tabContainer; property: "ListView.delayRemove"; value: false }
      }

    } // Rectangle of tabContainer
  } // Component


  ListView {
    id: tabSideBarListView
    
    anchors.fill: parent
    anchors.margins: 4

    interactive: container.height < container.maxHeight ? false:true
    spacing: 5

    contentY: tabHeight*currentIndex

    model: tabSideBarModel
    delegate: tabDelegate

  }


  Connections {
    target: tabSideBarModel
    onSelectTab: {
        tabSideBarListView.currentIndex = index
    }
  }

}
