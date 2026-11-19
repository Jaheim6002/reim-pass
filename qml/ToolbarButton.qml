import QtQuick
import 'themes'

// A custom button for the sidebar

Rectangle {
    id: root;

    width: height; height: 20;

    property bool selected: false;
    signal clicked();

    color: selected ? Theme.panelSelectedBackground
            : mouseArea.containsMouse ? Theme.panelHoveredBackground
            : Theme.panelBackground;

    MouseArea {
        id: mouseArea;

        anchors.fill: parent;
        hoverEnabled: true;

        onClicked: root.clicked();
    }
}
