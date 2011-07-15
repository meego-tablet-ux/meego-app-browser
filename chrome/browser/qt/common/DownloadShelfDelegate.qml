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
import MeeGo.Components 0.1

Component {
    id: downloadDelegate
    Rectangle {
        id: mainLevelItem
        // dangerous: 0, paused: 1, downloading: 2
        // cancelled: 3, finished: 4
        function isDangerous() { return dl_status == 0; }
        function isPaused() { return dl_status == 1; }
        function isDownloading() { return dl_status == 2; }
        function isCancelled() { return dl_status == 3; }
        function isFinished() { return dl_status == 4; }

        Component {
            id: spinnerComponent
            Spinner {
                continuousSpinning: true;
                spinning: isDownloading();
            }
        }

        states: [
            State {
                name: "dangerous"
                when: dl_status == 0
                PropertyChanges { target:dangerousItem; sourceComponent: dangerComponent; }
                PropertyChanges  { target:infoContainer; height: 0; opacity: 0;}
                AnchorChanges { target: divide; anchors.top: dangerousItem.bottom; }
            },
            State {
                name: "downloading"
                when: dl_status == 2
                PropertyChanges { target:spinner; sourceComponent: spinnerComponent; }
            }
        ]

        height: childrenRect.height
        width: parent.width
        color: "#fafafa"

        ///// top margin element
        Item { height: 15; id: spacer }

        ///// Date container; relative date above, actual date below
        Item {
            id: dateContainer
            width: 185
            anchors.left: parent.left
            anchors.leftMargin: 10
            anchors.top: spacer.bottom
            visible: dl_show_date
            Text {
                id: textualDate
                color: "black"
                width: parent.width
                text: dl_relative_date
            }
            Text {
                id: numericDate
                color: "gray"
                anchors.top: textualDate.bottom
                width: parent.width
                text: dl_date
            }
        }

        ///// Mimetype image
        Item {
            id: imageContainer
            anchors.left: dateContainer.right
            width: 50
            anchors.top: spacer.bottom
            Image {
                id: mimetypeImage
                height: 32
                width: 32
                fillMode: Image.Stretch
                source: {
                    if (isDownloading()) "";
                    else if (dl_type == 1) "image://themedimage/images/mimetypes/64x64/document";
                    else if (dl_type == 2) "image://themedimage/images/mimetypes/64x64/video";
                    else if (dl_type == 3) "image://themedimage/images/mimetypes/64x64/image";
                    else if (dl_type == 4) "image://themedimage/images/mimetypes/64x64/audio";
                    else "image://themedimage/images/mimetypes/64x64/archive";
                }
                Loader { id: spinner; anchors.centerIn: parent }
            }
        }

        ///// Container for the actual item info
        Item {
            id: infoContainer
            anchors.left: imageContainer.right
            anchors.top: spacer.bottom
            height: childrenRect.height
            // Title of the item (filename)
            Text {
                id: titleText
                text: dl_title
                font.pixelSize: 18
                font.underline: { isFinished() ? true : false ; }
                color: { isFinished() ? "#2FA7D4" : "black"; }  // blue : black
                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        downloadItemContainer.model.openDownloadItem(index)
                    }
                }
            }
            Text {
                anchors.left: titleText.right
                anchors.leftMargin: 5
                font.pixelSize: 18
                visible: isPaused() || isCancelled()
                color: "gray"
                text: "("+dl_progress+")"
            }
            // Progress text
            Text {
                id: progressText
                font.pixelSize: 18
                visible: isDownloading()
                text: dl_progress
                color: "gray"
                anchors.top: titleText.bottom
            }
            // URL
            Text {
                id: urlText
                text: dl_url
                elide: Text.ElideRight
                color: "#409030" // green
                anchors.top: { isDownloading() ? progressText.bottom : titleText.bottom; }
            }

            // Cancel, Resume, Retry, Pause, Discard "buttons" for the item
            Item {
                id: actionLinks
                anchors.top: urlText.bottom
                anchors.topMargin: 20
                width: 200
                height: childrenRect.height
                Text {
                    id: firstActionLink
                    font.pixelSize: 18
                    font.underline: true
                    text: {
                        if( isPaused() ) downloadControlResume;
                        else if (isFinished()) downloadControlRemove;
                        else if (isCancelled()) downloadControlRetry;
                        else if (isDownloading()) downloadControlPause;
                        else if (isDangerous()) "";
                    }
                    color: { isFinished() ? "red" : "gray"; }
                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            if(isPaused())
                                downloadItemContainer.model.resumeDownloadItem(index);
                            else if (isFinished())
                                downloadItemContainer.model.deleteDownloadItem(index);
                            else if (isCancelled())
                                downloadItemContainer.model.retryDownloadItem(index);
                            else if (isDownloading())
                                downloadItemContainer.model.pauseDownloadItem(index);
                        }
                    }
                }
                Text {
                    id: secondActionLink
                    font.pixelSize: 18
                    font.underline: true
                    anchors.left: firstActionLink.right
                    anchors.leftMargin: 16
                    visible: !isFinished()
                    text: { isCancelled() ? downloadControlRemove : downloadControlCancel }
                    color: { isCancelled() ? "red" : "gray"; }
                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            if (isCancelled()) downloadItemContainer.model.deleteDownloadItem(index);
                            else downloadItemContainer.model.cancelDownloadItem(index);
                        }
                    }
                }
            }
        }

        ///// Dangerous Download
        Connections {
            target: dangerousItem.item
            onSave: { downloadItemContainer.model.saveDownloadItem(index); }
            onDiscard: { downloadItemContainer.model.discardDownloadItem(index); }
        }

        Loader {
            id: dangerousItem
            anchors.left: imageContainer.right
            anchors.top: spacer.bottom
        }
        Component {
            id: dangerComponent
            DangerousItem {
                warning_text: downloadDangerDescPre + dl_title + downloadDangerDescPos
                save_text: downloadControlSave
                discard_text: downloadControlDiscard
                title_text: dl_title
                height: childrenRect.height
                opacity: 1
            }
        }

        // Divider between items
        DefaultDivider {
            id: divide
            anchors { left: parent.left; right: parent.right; top: infoContainer.bottom; topMargin: 30; }
        }
    }
}

