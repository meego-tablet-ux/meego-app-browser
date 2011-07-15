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

Item {

    id: downloadItemContainer
    width: parent.width
    height:  parent.height
    property int itemHeight: 120
    property variant model

    DownloadShelfDelegate {
        id: downloadDelegate    
   }
   ListView {
      id: downloadView
      anchors.fill: parent
      anchors.leftMargin: 25
      anchors.rightMargin: 25
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
      position: { downloadView.visibleArea.yPosition>0 ? downloadView.visibleArea.yPosition:0}
      pageSize: { 
        if (downloadView.visibleArea.yPosition > 0)
          downloadView.visibleArea.heightRatio
        else 
          downloadView.visibleArea.heightRatio*(1+downloadView.visibleArea.yPosition)   
      }
    }
}

