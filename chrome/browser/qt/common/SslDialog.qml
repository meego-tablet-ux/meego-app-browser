import Qt 4.7
import MeeGo.Components 0.1


Item {
  id: container
  anchors.fill: parent
  visible: false
  focus: false

  property string msgHeadline: ""
  property string msgDescription: ""
  property string msgMoreInfo: ""
  property string msgYesButton: ""
  property string msgNoButton: ""
  ModalDialog {
    id: myDialog

    title : container.msgHeadline
    buttonWidth: 300
    buttonHeight: 35
    showCancelButton: true
    showAcceptButton: true
    cancelButtonText: msgNoButton
    acceptButtonText: msgYesButton
    
    acceptButtonImage: "image://themedimage/widgets/common/button/button-negative"
    acceptButtonImagePressed: "image://themedimage/widgets/common/button/button-negative-pressed"
    cancelButtonImage: "image://themedimage/widgets/common/button/button-default"
    cancelButtonImagePressed: "image://themedimage/widgets/common/button/button-default-pressed"

    content: Rectangle {
      id: myContent
      width: 600
      height: 400
      TextEdit {
        id: description
        anchors.fill: parent
        anchors.margins: 20
        wrapMode: TextEdit.Wrap
        readOnly: true
        font.pixelSize: 15
        color: "#383838"
        text: container.msgDescription
      }
    }

    // handle signals:
    onAccepted: {
      sslDialogModel.yesButtonClicked()
    }
    onRejected: {
      sslDialogModel.noButtonClicked()
    }
  }
  Connections {
    target: sslDialogModel
    onShow: {
      container.msgHeadline =  headline
      container.msgDescription = description
      //container.msgMoreInfo = moreInfo
      container.msgNoButton = buttonNo
      if(!error) {
        container.msgYesButton = buttonYes
        myDialog.showAcceptButton  = true
      }
      else
        myDialog.showAcceptButton  = false
      container.visible = true
      container.focus =  true
      myDialog.show()
    }
    onHide: {
      container.visible = false
      myDialog.hide()
    }
  }
}
