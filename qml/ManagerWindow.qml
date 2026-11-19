import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Effects
import ReimPass

ApplicationWindow {
    id: root;
    width: 1280; height: 720;
    minimumWidth: 900; minimumHeight: 600;
    visible: true;
    color: Theme.defBackground;
    title: qsTr("Reim Pass — Manager");
    onClosing: Qt.quit()

    property string currentPage: "passwords";

    BrowseData {
        id: bridge;
    }

    Component.onCompleted: {
        bridge.SelectList();
    }

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

                Image {
                    id: keyicon

                    width: 32; height: width;
                    x: 12;

                    anchors.verticalCenter: parent.verticalCenter;
                    sourceSize.width: width; sourceSize.height: height;

                    source: Icons.key;

                    layer.enabled: true;
                    layer.effect: MultiEffect {
                        colorization: 1.0;
                        brightness: 1.0;
                        colorizationColor: Theme.lightText;
                    }
                }

                Text {
                    anchors.fill: parent;
                    anchors.leftMargin: 12 + keyicon.x + keyicon.width;
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

                Image {
                    id: cogicon

                    width: 32; height: width;
                    x: 12;

                    anchors.verticalCenter: parent.verticalCenter;
                    sourceSize.width: width; sourceSize.height: height;

                    source: Icons.cog;

                    layer.enabled: true;
                    layer.effect: MultiEffect {
                        colorization: 1.0;
                        brightness: 1.0;
                        colorizationColor: Theme.lightText;
                    }
                }

                Text {
                    anchors.fill: parent;
                    anchors.leftMargin: 12 + cogicon.x + cogicon.width;
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
        x: root.width - width + 1; z: 1;

        color: Theme.defBackground;
        border.width: 0.5; border.color: Theme.containerBorder;

        // Toolbar
        Rectangle {
            id: toolbar;

            width: parent.width; height: 20;
            y: searcher.height;

            color: Theme.panel2Background;

            Row {
                width: parent.width;
                spacing: 2;

                ToolbarButton {
                    id: addData;

                    onClicked: {
                        listedDataList.currentIndex = -1;
                        siteField.visible = true; loginField.visible = true; passwordField.visible = true;
                        addButton.visible = true;
                    }

                    Image {
                        id: addicon

                        width: parent.width - 5; height: width;

                        anchors.verticalCenter: parent.verticalCenter;
                        anchors.horizontalCenter: parent.horizontalCenter;
                        sourceSize.width: width; sourceSize.height: height;

                        source: Icons.add;

                        layer.enabled: true;
                        layer.effect: MultiEffect {
                            colorization: 1.0;
                            brightness: 1.0;
                            colorizationColor: Theme.lightText;
                        }
                    }
                }

                ToolbarButton {
                    id: deleteData;

                    property int selectedId;

                    visible: if (listedDataList.currentIndex == -1) {
                        false;
                    } else {
                        true;
                    }

                    onClicked: {
                        listedDataList.currentIndex = -1;
                        deleteWarning.visible = true;
                        deleteButton.visible = true;
                        deleteButton.selectedId = selectedId;
                    }

                    Image {
                        id: binicon

                        width: parent.width - 5; height: width;

                        anchors.verticalCenter: parent.verticalCenter;
                        anchors.horizontalCenter: parent.horizontalCenter;
                        sourceSize.width: width; sourceSize.height: height;

                        source: Icons.bin;

                        layer.enabled: true;
                        layer.effect: MultiEffect {
                            colorization: 1.0;
                            brightness: 1.0;
                            colorizationColor: Theme.lightText;
                        }
                    }
                }

                ToolbarButton {
                    id: editData;

                    property int selectedId;

                    visible: if (listedDataList.currentIndex == -1) {
                        false;
                    } else {
                        true;
                    }

                    onClicked: {
                        listedDataList.currentIndex = -1;
                        siteField.visible = true; loginField.visible = true; passwordField.visible = true;
                        editButton.visible = true;
                        editButton.selectedId = selectedId;
                    }

                    Image {
                        id: editicon

                        width: parent.width - 5; height: width;

                        anchors.verticalCenter: parent.verticalCenter;
                        anchors.horizontalCenter: parent.horizontalCenter;
                        sourceSize.width: width; sourceSize.height: height;

                        source: Icons.edit;

                        layer.enabled: true;
                        layer.effect: MultiEffect {
                            colorization: 1.0;
                            brightness: 1.0;
                            colorizationColor: Theme.lightText;
                        }
                    }
                }
            }
        }

        TextArea {
            id: siteText;

            y: searcher.height + toolbar.height + 20; 
            width: parent.width - 40; height: 35;
            anchors.horizontalCenter: parent.horizontalCenter;
            readOnly: true;
            visible: if (listedDataList.currentIndex == -1) {
                false;
            } else {
                true;
            }

            text: "Empty"

            background: Rectangle {
                radius: 8;
                width: parent.width;
                anchors.horizontalCenter: parent.horizontalCenter;
                anchors.verticalCenter: parent.verticalCenter;
                color: Theme.buttonBackground;
                border.width: 0.5; border.color: Theme.containerBorder;
            }
        }

        TextArea {
            id: loginText;

            y: searcher.height + toolbar.height + 40 + height; 
            width: parent.width - 40; height: 35;
            anchors.horizontalCenter: parent.horizontalCenter;
            readOnly: true;
            visible: if (listedDataList.currentIndex == -1) {
                false;
            } else {
                true;
            }

            text: "Empty"

            background: Rectangle {
                radius: 8;
                width: parent.width;
                anchors.horizontalCenter: parent.horizontalCenter;
                anchors.verticalCenter: parent.verticalCenter;
                color: Theme.buttonBackground;
                border.width: 0.5; border.color: Theme.containerBorder;
            }
        }

        TextArea {
            id: passwordText;

            y: searcher.height + toolbar.height + 60 + height * 2; 
            width: parent.width - 40; height: 35;
            anchors.horizontalCenter: parent.horizontalCenter;
            readOnly: true;
            visible: if (listedDataList.currentIndex == -1) {
                false;
            } else {
                true;
            }

            text: "Empty"

            background: Rectangle {
                radius: 8;
                width: parent.width;
                anchors.horizontalCenter: parent.horizontalCenter;
                anchors.verticalCenter: parent.verticalCenter;
                color: Theme.buttonBackground;
                border.width: 0.5; border.color: Theme.containerBorder;
            }
        }

        TextField {
            id: siteField;

            y: searcher.height + toolbar.height + 20; 
            width: parent.width - 40; height: 35;
            anchors.horizontalCenter: parent.horizontalCenter;
            visible: false;

            selectByMouse: true;
            placeholderText: qsTr("Site");

            background: Rectangle {
                radius: 8;
                width: parent.width;
                anchors.horizontalCenter: parent.horizontalCenter;
                anchors.verticalCenter: parent.verticalCenter;
                color: Theme.buttonBackground;
                border.width: 0.5; border.color: Theme.containerBorder;
            }
        }

        TextField {
            id: loginField;

            y: searcher.height + toolbar.height + 40 + height; 
            width: parent.width - 40; height: 35;
            anchors.horizontalCenter: parent.horizontalCenter;
            visible: false;

            selectByMouse: true;
            placeholderText: qsTr("Login");

            background: Rectangle {
                radius: 8;
                width: parent.width;
                anchors.horizontalCenter: parent.horizontalCenter;
                anchors.verticalCenter: parent.verticalCenter;
                color: Theme.buttonBackground;
                border.width: 0.5; border.color: Theme.containerBorder;
            }
        }

        TextField {
            id: passwordField;

            y: searcher.height + toolbar.height + 60 + height * 2; 
            width: parent.width - 40; height: 35;
            anchors.horizontalCenter: parent.horizontalCenter;
            visible: false;

            echoMode: TextInput.Password;
            selectByMouse: true;
            placeholderText: qsTr("Password");

            background: Rectangle {
                radius: 8;
                width: parent.width;
                anchors.horizontalCenter: parent.horizontalCenter;
                anchors.verticalCenter: parent.verticalCenter;
                color: Theme.buttonBackground;
                border.width: 0.5; border.color: Theme.containerBorder;
            }
        }

        Text {
            id: deleteWarning;
            y: searcher.height + toolbar.height + 20; 
            anchors.horizontalCenter: parent.horizontalCenter;
            visible: false;

            text: "Warning: Deleting login data is IRREVERSIBLE!"

            color: '#f13d64';
        }

        Button {
            id: addButton;
            y: searcher.height + toolbar.height + 80 + passwordField.height * 3; 
            anchors.horizontalCenter: parent.horizontalCenter;
            visible: false;

            text: "Add";

            onClicked: {
                if (siteField.text == "" || loginField.text == "" || passwordField.text == "") {
                    return;
                } else {
                    siteField.visible = false; loginField.visible = false; passwordField.visible = false; 
                    addButton.visible = false;
                    bridge.InsertData(siteField.text, loginField.text, passwordField.text);
                    bridge.SelectList();
                }
            }

            background: Rectangle {
                radius: 8;
                width: 64; height: 28;
                anchors.horizontalCenter: parent.horizontalCenter;
                anchors.verticalCenter: parent.verticalCenter;
                color: Theme.buttonBackground;
                border.width: 0.5; border.color: parent.pressed ? Theme.selectedBorder : Theme.containerBorder;
            }
        }

        Button {
            id: deleteButton;
            y: searcher.height + toolbar.height + 60; 
            anchors.horizontalCenter: parent.horizontalCenter;
            visible: false;

            property int selectedId;
            
            text: "Delete";

            onClicked: {
                deleteWarning.visible = false;
                deleteButton.visible = false;
                bridge.DeleteData(selectedId);
                bridge.SelectList();
            }

            background: Rectangle {
                radius: 8;
                width: 64; height: 28;
                anchors.horizontalCenter: parent.horizontalCenter;
                anchors.verticalCenter: parent.verticalCenter;
                color: Theme.buttonBackground;
                border.width: 0.5; border.color: parent.pressed ? Theme.selectedBorder : Theme.containerBorder;
            }
        }

        Button {
            id: editButton;
            y: searcher.height + toolbar.height + 80 + passwordField.height * 3; 
            anchors.horizontalCenter: parent.horizontalCenter;
            visible: false;

            property int selectedId;
            
            text: "Edit";

            onClicked: {
                if (siteField.text == "" || loginField.text == "" || passwordField.text == "") {
                    return;
                } else {
                    siteField.visible = false; loginField.visible = false; passwordField.visible = false; 
                    editButton.visible = false;
                    bridge.UpdateData(selectedId, siteField.text, loginField.text, passwordField.text);
                    bridge.SelectList();
                }
            }

            background: Rectangle {
                radius: 8;
                width: 64; height: 28;
                anchors.horizontalCenter: parent.horizontalCenter;
                anchors.verticalCenter: parent.verticalCenter;
                color: Theme.buttonBackground;
                border.width: 0.5; border.color: parent.pressed ? Theme.selectedBorder : Theme.containerBorder;
            }
        }
    }

    // Listed Data
    Rectangle {
        id: listedData;

        width: root.width - (sidebarBackground.width + selectedData.width); height: root.height;
        x: sidebarBackground.width;

        color: Theme.defBackground;

        ListView {
            id: listedDataList
            y: searcher.height;
            width: parent.width; height: parent.height - searcher.height - footer.height;
            clip: true;
            model: bridge.list;
            currentIndex: -1; // Nothing selected by default

            ScrollBar.vertical: ScrollBar { }

            delegate: Rectangle {
                required property int index;
                required property int selectedId;
                required property string site;
                required property string login;

                width: listedDataList.width;
                height: 40;
                color: ListView.isCurrentItem        ? Theme.defSelectedBackground
                     : listedDataMouse.containsMouse ? Theme.defHoveredBackground
                     :                                 Theme.defBackground

                Text {
                    text: parent.site + " | " + parent.login;
                    color: Theme.lightText;
                    anchors.verticalCenter: parent.verticalCenter;
                    leftPadding: 12;
                }

                MouseArea {
                    id: listedDataMouse;
                    anchors.fill: parent;
                    hoverEnabled: true;
                    onClicked: {
                        siteField.visible = false; loginField.visible = false; passwordField = false;
                        addButton.visible = false; editButton.visible = false; deleteButton.visible = false;
                        deleteWarning.visible = false;
                        listedDataList.currentIndex = parent.index;
                        bridge.SelectPassword(parent.selectedId);
                        editData.selectedId = parent.selectedId;
                        deleteData.selectedId = parent.selectedId;
                        siteText.text = bridge.site;
                        loginText.text = bridge.login;
                        passwordText.text = bridge.password;
                    }
                }
            }
        }
    }
}
