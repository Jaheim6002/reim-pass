import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Effects
import 'themes'
import 'assets'

ApplicationWindow {
    width: 600; height: 480;
    visible: true;
    color: Theme.defBackground;
    title: qsTr("Reim Pass — Sign In");

    Text {
        id: titleText;
        text: "Reim Pass";
        font.family: "DejaVu Math TeX Gyre"
        y: 40;
        anchors.horizontalCenter: parent.horizontalCenter;
        anchors.horizontalCenterOffset: 30;
        font.pointSize: 42; font.bold: true;
        color: Theme.headerText;

        Image {
            width: 48; height: 110; 
            x: -60; y: -20;

            source: Icons.logo;
            sourceSize.width: width; sourceSize.height: height;

            layer.enabled: true;
            layer.effect: MultiEffect {
                colorization: 1.0;
                colorizationColor: Theme.headerText;
            }
        }

        Text {
            id: subtitleText;
            text: "FOSS Password Manager";
            font.family: "DejaVu Math TeX Gyre"
            y: 60;
            anchors.horizontalCenter: parent.horizontalCenter;
            font.pointSize: 16;
            color: Theme.lightText;
        }
    }

    TextField {
        id: firstPassphrase;
        width: 300; height: 44;
        y: 150;
        anchors.horizontalCenter: parent.horizontalCenter;
        placeholderText: qsTr("Enter Passphrase");
        echoMode: TextInput.Password;
        color: Theme.lightText;
        leftPadding: 12;
        selectByMouse: true;

        background: Rectangle {
            radius: 8;
            color: Theme.buttonBackground;
            border.width: 0.5; border.color: firstPassphrase.activeFocus ?  Theme.selectedBorder : Theme.containerBorder;
        }
    }

    TextField {
        id: secondPassphrase;
        width: 300; height: 44;
        y: 200;
        anchors.horizontalCenter: parent.horizontalCenter;
        placeholderText: qsTr("Enter it once again");
        echoMode: TextInput.Password;
        color: Theme.lightText;
        leftPadding: 12;
        selectByMouse: true;
        visible: true;

        background: Rectangle {
            radius: 8;
            color: Theme.buttonBackground;
            border.width: 0.5; border.color: secondPassphrase.activeFocus ? Theme.selectedBorder : Theme.containerBorder;
        }
    }

    Button {
        id: logInButton;
        y: 300;
        anchors.horizontalCenter: parent.horizontalCenter;
        text: "Sign in"; 
        //onClicked: ;

        background: Rectangle {
            radius: 8;
            color: Theme.buttonBackground;
            border.width: 0.5; border.color: logInButton.pressed ? Theme.selectedBorder : Theme.containerBorder;
        }
    }


    footer: Rectangle {
        height: 24;
        color: Theme.panelBackground;

        Text {
            text: "Reim Pass FOSS Password Manager | Made by Jaheim6002 | 2.0.0v GUI";
            color: Theme.lightText;
            leftPadding: 4; y: 4;
            anchors.horizontalCenter: parent.horizontalCenter;
        }
    }
}
