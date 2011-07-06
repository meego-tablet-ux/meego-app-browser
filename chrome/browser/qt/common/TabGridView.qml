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
  // Represent the tab list view, show thumbnails of each tab
  // Call c++ methods to open/close pages
  property variant model: tabSideBarModel
  property int maxHeight: 600
  property int animDuration: 500

  // Number of columns in vertical mode or number of rows in horizontal mode
  property int maxColumns: ( scene.isLandscapeView() && gridView.count > 2 ) ? 2: 1
  // Vertical/horizontal mode toggle
  property bool horizontalMode: scene.isLandscapeView()

  anchors.left: parent.left
  anchors.leftMargin: 5
  anchors.rightMargin: 5
  clip: true

  width: columns() * gridView.cellWidth + 10
  height: rows() * gridView.cellHeight

  Behavior on height {
      NumberAnimation { duration: animDuration; easing.type: Easing.OutQuad }
  }
  Behavior on width {
      NumberAnimation { duration: animDuration; easing.type: Easing.OutQuad }
  }

  GridView {
    id: gridView
    cellHeight: 136
    cellWidth:  192
    anchors.fill: parent
    boundsBehavior: Flickable.StopAtBounds

    model: tabSideBarModel
    delegate: TabGridDelegate {}

    // Hide tab list after allowing width and height behaviors to complete animating
    function hideTabsLater() {
        hideTabsAnim.running = true
    }
    SequentialAnimation {
        id: hideTabsAnim
        PauseAnimation { duration: animDuration }
        ScriptAction { script: { tabSideBarModel.hideSideBar() } }
    }
  }

  function columns() {
    return !horizontalMode ? Math.min( gridView.count, maxColumns ) :
                             Math.ceil( gridView.count / rows() )
  }
  function rows() {
    return horizontalMode ? Math.min( gridView.count, maxColumns ) :
                            Math.ceil( gridView.count / columns() )
  }

  Connections {
    target: tabSideBarModel
    onSelectTab: {
        gridView.currentIndex = index
    }
  }
}
