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
  // Represent a history stack view, showing thumbnails of visual history
  // Call c++ methods to open the clicked page and decide whether to show
  // historybar and listen to signals from c++ to update its showness
  id: container
  // default height is 150
  property int rectHeight: 200
  //width: parent.width
  height: rectHeight
  anchors.left: parent.left
  anchors.right: parent.right

  // default margin is 5
  property int commonMargin: 5

  anchors.leftMargin: commonMargin
  anchors.rightMargin: commonMargin

  // delegate to show thumbnail of each web page in the history list
  Component {
    id: historyItemDelegate
    Item {
      id: innerItem

      height: historyView.itemHeight 
      width: historyView.itemWidth  

      // flag to indicate whether the current item is the highlighted one in the list
      property bool isCurrent: false
      Rectangle {
        id: historyItem
        height: parent.height - 4 * commonMargin
        //color: "#383838"
        border.width: 4
        border.color: "grey"

        width: parent.width - 2 * commonMargin
        anchors.fill: parent
        anchors.margins: commonMargin

        anchors {
            leftMargin: commonMargin
            rightMargin: commonMargin
            topMargin: commonMargin * 2
            bottomMargin: commonMargin * 2
        }

        Image {
            id: bg
            anchors.fill: parent
            source: "image://themedimage/images/browser/bg_favouritesoverlay"
        }

        Rectangle {
            id: image 
            height: parent.height * 0.7 - 2 * commonMargin    
            width: parent.width - 4 * commonMargin

            anchors.top: parent.top
            anchors.left: parent.left
            anchors.leftMargin: 2 * commonMargin
            anchors.topMargin: 2 * commonMargin
            Image {                        
              id: thumbImage               
              source: thumbSrc             
              fillMode: Image.PreserveAspectFit
              smooth: true
              anchors.fill: parent
            }
        }
            Text {
              id: historyTitle
              height: parent.height * 0.3   
              width: parent.width 
              anchors.bottom: parent.bottom
              text: title
              clip: true
              //anchors.fill: parent
              color: "white"
              elide: Text.ElideRight
              verticalAlignment: Text.AlignVCenter
              horizontalAlignment: Text.AlignHCenter
              wrapMode: Text.NoWrap
            }
          MouseArea {
            anchors.fill: parent
            onClicked: {
              console.log("click page " + index)
              // set current item is the highlighted one, and after some time,
              // open the page
              historyView.currentIndex = index
              timer.running = true
            }
          }
          // timer to delay some time to show the new clicked thumbnail
          Timer {
            id: timer
            interval: 100
            running: false
            onTriggered: {
              console.log("timer is ended")
              historyView.model.openPage(index)
            }
          }
      }
      // highlighted the selected current thumbnail with a border
      states: State {
        name: "highlight"
        when: innerItem.ListView.isCurrentItem
        PropertyChanges {
          target: historyItem
          radius: 2
          border.width: 6
          border.color: "#2CACE3"
        }
      }
    }
  }
  ListView {
    id: historyView
    anchors.fill: parent
    spacing: 1
    delegate: historyItemDelegate
    model: historyStackModel
    focus: true
    clip: true
    currentIndex: 0
    property real itemHeight: height
    property real itemWidth: height * 1.2
    orientation: ListView.Horizontal
    // only interactive when items are full of container
    interactive: itemWidth * count < container.width ? false : true
    // control the scrollbar
    states: State {
       name: "ShowBars"
       when: historyView.movingHorizontally
       PropertyChanges { target: horizontalScrollBar; opacity: 1; height: 5}
    }
  }
  ScrollBar {
    id: horizontalScrollBar
    width: historyView.width; height: 0
    anchors.bottom: parent.bottom
    opacity: 0
    orientation: Qt.Horizontal
    position: historyView.visibleArea.xPosition
    pageSize: historyView.visibleArea.widthRatio
  }
  Connections {
    target: historyStackModel
    onCurrent: {
        historyView.currentIndex = index
    }
  }
  signal hideOverlay
  signal showOverlay
  Connections {
    target: historyStackModel
    onShowHistory: {
        showOverlay()
    }
    onHideHistory: {
        hideOverlay()
    }
  }
}
