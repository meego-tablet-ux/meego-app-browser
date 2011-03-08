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
    id: downloadItemContainer
    width: parent.width
    height:  parent.height
    property int itemHeight: 110
    property variant model

    Component {
      id: downloadDelegate
      Rectangle {
        id: downloadItem
        property bool pressed: false
        width: parent.width
        height: itemHeight
        states: [
          State {
            name: "canceledAndCompleted"
            when: s == 3 || s == 4
            PropertyChanges  { target:buttonLoader; sourceComponent: completedAndCanceledButtons; anchors.topMargin: 10;}
          },
          State {
            name: "inprocess"
            when: s == 2
            PropertyChanges  { target:buttonLoader; sourceComponent: inprocessButtons; anchors.topMargin: 10;}
          },
          State {
            name: "paused"
            when: s == 1
            PropertyChanges  { target:buttonLoader; sourceComponent: pausedButtons; anchors.topMargin: 10;}
          },
          State {
            name: "dangeous"
            when: s == 0
            PropertyChanges { target:buttonLoader; sourceComponent: dangeousButtons; anchors.top: dangeousWarning.bottom;}
            PropertyChanges  { target:fileurl; height: 0; opacity: 0;}
            PropertyChanges  { target: downloadItemInfo; height: 0; opacity: 0;}
            PropertyChanges  { target:dangeousWarning;  opacity: 1;}
          }
        ]
        Image {
          id: downloadItemBackground
          anchors.fill: parent
          fillMode: Image.Stretch
          source: "image://theme/bg_application_p"
        }
 
        Text {
            id: downloadItemdate 
            color: "black"
            elide: Text.ElideRight
            font.pixelSize: 20
            width: 120
            text: { show_date == 1? downloadDate:""}
        }
        Image {
          id: favicon
          anchors.left: downloadItemdate.right
          anchors.leftMargin: 10
          width: height
          height: parent.height/2
          anchors.verticalCenter: parent.verticalCenter 
          fillMode: Image.Stretch
          source: {
            if (type == 1) "image://theme/mimetypes/64x64/document";
            else if (type == 2) "image://theme/mimetypes/64x64/video";
            else if (type == 3) "image://theme/mimetypes/64x64/image";
            else if (type == 4) "image://theme/mimetypes/64x64/audio";
            else "image://theme/mimetypes/64x64/archive";
          }
          BusyIndicator { anchors.centerIn: parent; switcher: s == 2 }
        }
        Rectangle {
          id: infoContainer
          height: itemHeight
          width: downloadItem.width - downloadItemdate.width - favicon.width 
          anchors.left: favicon.right
          anchors.leftMargin: 10 
          anchors.right: parent.right
          color: "white"
          border.color: "gray"
          border.width: 2
          radius: 5
          Rectangle {
            id: downloadItemInfo
            height: 20 
       //     width: parent.width
            anchors.top: parent.top
            anchors.topMargin: 5
            anchors.left: parent.left
            anchors.leftMargin: 5
            anchors.right: parent.right
            anchors.rightMargin: 5
            Text {
              id: filename 
              color: { s == 4? "blue":"black"}
              elide: Text.ElideRight
              width: parent.width - downloadStatus.width
              font.pixelSize: 20
              text: title
              anchors.left: parent.left
          //    anchors.leftMargin: 5
              MouseArea {
                anchors.fill: parent
                onClicked: {
                  downloadItemContainer.model.openDownloadItem(index);
                }
              }
            }
            Text {
              id: downloadStatus
              color: "gray"
              font.pixelSize: 20
              anchors.right: parent.right
          //    anchors.rightMargin: 5
              text: progress
            }
          }
          Text {
            id: fileurl 
            anchors.top:downloadItemInfo.bottom
            anchors.topMargin: 10
            anchors.leftMargin: 5
            color: "black"
            elide: Text.ElideRight
            width: downloadItem.width - downloadItemdate.width - favicon.width
            font.pixelSize: 20
            text: url
          }
          Text {
            id: dangeousWarning
            anchors.top: parent.top
            width: parent.width
            color: "red"
            font.pixelSize: 20
            text: downloadDangerDescPre + title + downloadDangerDescPos
            opacity: 0;
            wrapMode: Text.WordWrap
          }
          Loader
          {
            id:buttonLoader
            anchors.top: fileurl.bottom
          }
        }
      }
    }
    Component {
      id: inprocessButtons
      Rectangle {
        anchors.topMargin: 10
        anchors.leftMargin:5
        Row {
          anchors.fill: parent
          spacing: 20
          Text {
            id: pauseButton 
            color: "blue"
            font.pixelSize: 20
            font.underline: true            
            text: downloadControlPause
            MouseArea {
              anchors.fill: parent
              onClicked: {
                downloadItemContainer.model.pauseDownloadItem(index);
              }
            }
          }
          Text {
            id: cancelButton
            color: "blue"
            font.pixelSize: 20
            font.underline: true
            text: downloadControlCancel
            MouseArea {
              anchors.fill: parent
              onClicked: {
                downloadItemContainer.model.cancelDownloadItem(index);
              }
            }
          }
       }
     }
   }
   Component {
      id: pausedButtons
      Rectangle {
        anchors.topMargin: 10
        Row {
          anchors.fill: parent
          spacing: 20
          Text {
            id: resumeButton
            color: "blue"
            font.pixelSize: 20
            font.underline: true
            text: downloadControlResume
            MouseArea {
              anchors.fill: parent
              onClicked: {
                downloadItemContainer.model.resumeDownloadItem(index);
              }
            }

          }
          Text {
            id: cancelButton
            color: "blue"
            font.pixelSize: 20
            font.underline: true
            text: downloadControlCancel
            MouseArea {
              anchors.fill: parent
              onClicked: {
                downloadItemContainer.model.cancelDownloadItem(index);
              }
            }
          }
       }
     }
   }
   Component {
      id: completedAndCanceledButtons
      Rectangle {
        anchors.topMargin: 10
        Row {
          anchors.fill: parent
          spacing: 20
          Text {
            id: removeButton
            color: "blue"
            font.pixelSize: 20
            font.underline: true
            text: downloadControlRemove
            MouseArea {
              anchors.fill: parent
              onClicked: {
                downloadItemContainer.model.removeDownloadItem(index);
              }
            }
          }
       }
     }
   }
   Component {
      id: dangeousButtons
      Rectangle {
        Row {
          anchors.fill: parent
          spacing: 20
          TextButton {
            id: saveDangeousButton
            text: downloadControlSave
            onClicked: {
              downloadItemContainer.model.saveDownloadItem(index);        
            }
          }
          TextButton {
            id: discardDangeousButton
            text: downloadControlDiscard
            onClicked: {
              downloadItemContainer.model.discardDownloadItem(index);  
            }
          }
        }
     }
   }

   ListView {
      id: downloadView
      anchors.fill: parent
      anchors.leftMargin: 10
      anchors.rightMargin: 10
//      boundsBehavior: Flickable.StopAtBounds
      spacing: 10
      delegate: downloadDelegate
      model: downloadItemContainer.model
      focus: true
      clip: true
      orientation: ListView.Vertical
      opacity: 1
      states: State {
         name: "Shows"
         when: downloadView.movingVertically
         PropertyChanges { target: verticalScrollBar; opacity: 1; width: 5}
      }
    }
    ScrollBar {
      id: verticalScrollBar
      width: 0; height: downloadView.height
      anchors.right: downloadView.right
      opacity: 0
      orientation: Qt.Vertical
    //  position: downloadView.visibleArea.yPosition
    //  pageSize: downloadView.visibleArea.heightRatio
      position: { downloadView.visibleArea.yPosition>0 ? downloadView.visibleArea.yPosition:0}
      pageSize: { 
        console.log(downloadView.visibleArea.yPosition)
        if (downloadView.visibleArea.yPosition > 0)
          downloadView.visibleArea.heightRatio
        else 
          downloadView.visibleArea.heightRatio*(1+downloadView.visibleArea.yPosition)   
      }
    }
}

