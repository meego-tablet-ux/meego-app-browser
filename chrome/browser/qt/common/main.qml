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
import MeeGo.Components 0.1 as UX
import MeeGo.App.BrowserWrapper 1.0

Item {
  id: entrance
  width: screenWidth
  height: screenHeight
  
  BrowserWrapper {
    id: browserWrapper
    Component.onCompleted: {
      // transfer mainWindow(QDeclarativeView) to native code
      browserWrapper.transfer(mainWindow);
    }
  }

  Connections {
    target: qApp
    onOrientationChanged: {
      // Since an application can lock it's orientation, various overlay windows
      // like the task switcher and the status indicator menu observe the published
      // orientation attribute on the top level window of the active application.
      // This is accomplished from QML via the mainWindow.actualOrientation property
      mainWindow.actualOrientation = qApp.orientation;
    }
  }
  Connections {
    target: mainWindow
    onCall: {
      browserWrapper.arguments(parameters);
    }
  }
  Loader {
    id: holder
    source: {screenWidth*25.4/dpiX > 140 && screenHeight*25.4/dpiY > 93? "Tablet.qml":"Handset.qml"}
  }
  Loader {
    id: browserLoader
  }
  Connections {
    target: browserWrapper
    onLoad: {
      browserLoader.source = "BrowserObject.qml"
      browserLoader.item.runMain(parameters, mainWindow);
    }
  }
}
