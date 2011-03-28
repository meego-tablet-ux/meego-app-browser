import Qt 4.7
Image {
    id: indicator 
    property bool switcher: false

    source: "image://theme/browser/busy"; 
    visible: indicator.switcher;
    NumberAnimation on rotation { 
      running: indicator.switcher; 
      from: 0; 
      to: 360; 
      loops: Animation.Infinite; 
      duration: 1500 }
}
