import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Effects
import 'themes'
import 'assets'

ApplicationWindow {
    id: root;
    width: 1280; height: 720;
    minimumWidth: 900; minimumHeight: 600;
    visible: true;
    color: Theme.defBackground;
    title: qsTr("Reim Pass — Manager");
    onClosing: Qt.quit()

    property string currentPage: "passwords";

    // Sidebar
    Rectangle {
        id: sidebarBackground;
        
        visible: true;
        width: 350; height: root.height;
        z: 3;

        color: Theme.panelBackground;

        Image {
            id: reimPassLogo;

            width: 24; height: 55; 
            x: 12; y: 12;

            source: Icons.logo;
            sourceSize.width: width; sourceSize.height: height;

            layer.enabled: true;
            layer.effect: MultiEffect {
                colorization: 1.0;
                colorizationColor: '#f13d64';
            }
        }

        Text {
            id: titleText;
            x: 42; y: 22;

            font.family: "DejaVu Math TeX Gyre"
            font.pointSize: 24; font.bold: true;

            text: "Reim Pass";
            color: "white";
        }

        Column {
            topPadding: 80;
            anchors.fill: parent;

            spacing: 2;

            SidebarButton {
                id: passwordsButton;

                selected: root.currentPage === "passwords";
                onClicked: root.currentPage = "passwords";

                Text {
                    anchors.fill: parent;
                    anchors.leftMargin: 12;
                    verticalAlignment: Text.AlignVCenter;

                    text: "Passwords";
                    font.pointSize: 16;
                    color: Theme.lightText;
                }
            }
            SidebarButton {
                id: settingsButton;

                selected: root.currentPage === "settings";
                onClicked: root.currentPage = "settings";

                Text {
                    anchors.fill: parent;
                    anchors.leftMargin: 12;
                    verticalAlignment: Text.AlignVCenter;

                    text: "Settings";
                    font.pointSize: 16;
                    color: Theme.lightText;
                }
            }
        }
    }

    Rectangle {
        id: footer;

        width: root.width - sidebarBackground.width; height: 24;
        x: sidebarBackground.width; y: root.height - height; z: 2;

        color: Theme.panel2Background;

        Text {
            id: footerText;

            anchors.fill: parent;
            anchors.rightMargin: 4;
            horizontalAlignment: Text.AlignRight;
            verticalAlignment: Text.AlignVCenter;

            text: "Reim Pass FOSS Password Manager | Made by Jaheim6002 | 2.0.0v GUI";
            color: Theme.grayText;
        }
    }

    //
    // Content
    //

    // Searcher
    Rectangle {
        id: searcher;

        width: root.width - sidebarBackground.width; height:80;
        x: sidebarBackground.width; z: 2;

        color: Theme.panel2Background;

        TextField {
            id: searchField;

            width: parent.width - (parent.height - height); height: 44;
            x: (parent.height - height) / 2; y: (parent.height - height) / 2;
            placeholderText: qsTr("Search...");
            color: Theme.lightText;
            leftPadding: 12;
            selectByMouse: true;
            visible: true;

            background: Rectangle {
                radius: 8;
                color: Theme.buttonBackground;
                border.width: 0.5; border.color: searchField.activeFocus ? Theme.selectedBorder : Theme.containerBorder;
            }
        }
    }

    // Selected Data
    Rectangle {
        id: selectedData;

        width: root.width / 2.25; height: root.height;
        x: root.width - width + 1; z: 1

        color: Theme.defBackground;
        border.width: 0.5; border.color: Theme.containerBorder;
    }

    // Listed Data
    Rectangle {
        id: listedData;

        width: root.width - (sidebarBackground.width + selectedData.width); height: root.height;
        x: sidebarBackground.width;

        color: Theme.defBackground;
    }
}
