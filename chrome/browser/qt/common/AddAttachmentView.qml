import Qt 4.7
import MeeGo.Labs.Components 0.1
import MeeGo.Media 0.1
import Qt.labs.gestures 2.0

Item {
    id: container

    anchors.fill: parent

    GestureArea {
        anchors.fill: parent
        Tap {}
        TapAndHold {}
        Pan {}
        Pinch {}
        Swipe {}
    }

    function pickerCancelled () {
        console.log ("Cancelled picker");
        selectFileDialogObject.OnPickerCancelled()
    }

    function pickerSelected (uri) {
        console.log (uri + " selected");
        selectFileDialogObject.OnPickerSelected(uri)

        //attachments.append ({"uri": uri});
    }

    function addPicker (pickerComponent) {
        var picker = pickerComponent.createObject (container);
        picker.show ();
        picker.cancel.connect (pickerCancelled);
        picker.selected.connect (pickerSelected);
    }

    Rectangle {
        id: fog

        anchors.fill: parent
        color: theme_dialogFogColor
        opacity: theme_dialogFogOpacity
        Behavior on opacity {
               PropertyAnimation { duration: theme_dialogAnimationDuration }
        }
    }

    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.RightButton|Qt.LeftButton
        onClicked: {
            mouse.accepted = true;
        }
        onPressed: {
             mouse.accepted = true;
        }
        onReleased: {
             mouse.accepted = true;
        }
    }

  
    BorderImage {
        id: fileTypeDialog
        source: "image://theme/notificationBox_bg"

        x: (container.width - width) / 2
        y: (container.height - height) / 2
        height: 80

        Item {
            Column {
                anchors.fill: parent
                Button {
                    title: qsTr ("Photos")
                    width: fileTypeDialog.width
                    height: 40

                    onClicked: {
                        addPicker (photoPicker);
                    }
                }

                Button {
                    title: qsTr ("Movies")
                    width: fileTypeDialog.width
                    height: 40

                    onClicked: {
                        addPicker (moviePicker);
                    }
                }

    /*
                Button {
                    title: qsTr ("Musics")
                    width: parent.width
                    height: 40

                    onClicked: {
                        addPicker (musicPicker);
                    }
                }
*/
            }
        }
    }

    Component {
        id: documentPicker
        Rectangle {
            anchors.fill: parent
            color: "pink"
        }
    }

    Component {
        id: musicPicker
        MusicPicker {
            anchors.fill: parent

            showPlaylists: false
            showAlbums: false

            signal selected (string uri)
            onSongSelected: {
                selected (song);
            }
        }
    }

    Component {
        id: photoPicker
        PhotoPicker {
            anchors.fill: parent

            signal selected (string uri)

            onPhotoSelected: {
                selected (uri);
            }
        }
    }

    Component {
        id: moviePicker
        VideoPicker {
            anchors.fill: parent

            signal selected (string uri)

            onVideoSelected: {
                selected (uri);
            }
        }
    }
}
