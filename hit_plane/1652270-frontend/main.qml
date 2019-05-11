import QtQuick 2.11
import QtQuick.Controls 2.4
import QtQuick.Controls.Material 2.3
import QtQuick.Controls.Styles 1.4
import QtQuick.Layouts 1.3
import QtGraphicalEffects 1.12
import QtQuick.Dialogs 1.3

import sj.bombtheplane 1.0

ApplicationWindow {
    id: mainWindow
    visible: true
    width: 480
    height: 720
    title: qsTr("炸飞机")

    QtObject {
        id: util
        readonly property int dirLEFT : -1
        readonly property int dirRIGHT : 1
        readonly property int dirUP : -10
        readonly property int dirDOWN : 10

        readonly property int iH1 : 6
        readonly property int iH2 : 8
        readonly property int iH3 : 10

        readonly property int iT1 : 6 + 1
        readonly property int iT2 : 8 + 1
        readonly property int iT3 : 10 + 1

        property int deploying : iH1

        property var deployAblePlanes : [
            getPlaneBody(btp.headTails[deploying], util.dirLEFT),
            getPlaneBody(btp.headTails[deploying], util.dirRIGHT),
            getPlaneBody(btp.headTails[deploying], util.dirUP),
            getPlaneBody(btp.headTails[deploying], util.dirDOWN)
        ]

        function towards(start, direct /*10,-10,1,-1*/) {
            var end = start + direct;
            if (end > 99 || end < 0
                    || parseInt(start / 10) === 9 && direct === dirDOWN
                    || parseInt(start / 10) === 0 && direct === dirUP
                    || parseInt(start % 10) === 9 && direct === dirRIGHT
                    || parseInt(start % 10) === 0 && direct === dirLEFT
            ) {
                return btp.Coord_None;
            }
            return end;
        }

        function left(direct) {
            switch(direct) {
            case dirLEFT:
                return dirDOWN;
            case dirRIGHT:
                return dirUP;
            case dirUP:
                return dirLEFT;
            case dirDOWN:
                return dirRIGHT;
            }
        }

        function getPlaneBody(head, tailOrient) {
            var result = [head];
            var curr = head;

            var i;
            for (i = 0; i < 3; i++) {
                curr = towards(curr, tailOrient);
                if (i !== 2)
                    result.push(curr);
                if (curr === btp.Coord_None || btp.map.get(curr) & (GridNS.Body | GridNS.Head | GridNS.Tail)) {
                    return [btp.Coord_None];
                }
            }

            var tail = curr;
            var currDirect = left(tailOrient);

            curr = towards(curr, currDirect);
            result.push(curr);
            if (curr === btp.Coord_None || btp.map.get(curr) & (GridNS.Body | GridNS.Head | GridNS.Tail)) {
                return [btp.Coord_None];
            }

            curr = tail;
            currDirect = -left(tailOrient);

            curr = towards(curr, currDirect);
            result.push(curr);
            if (curr === btp.Coord_None || btp.map.get(curr) & (GridNS.Body | GridNS.Head | GridNS.Tail)) {
                return [btp.Coord_None];
            }


            curr = towards(head, tailOrient);
            currDirect = left(tailOrient);

            for (i = 0; i < 2; i++) {
                curr = towards(curr, currDirect);
                result.push(curr);
                if (curr === btp.Coord_None || btp.map.get(curr) & (GridNS.Body | GridNS.Head | GridNS.Tail)) {
                    return [btp.Coord_None];
                }
            }

            curr = towards(head, tailOrient);
            currDirect = -left(tailOrient);

            for (i = 0; i < 2; i++) {
                curr = towards(curr, currDirect);
                result.push(curr);
                if (curr === btp.Coord_None || btp.map.get(curr) & (GridNS.Body | GridNS.Head | GridNS.Tail)) {
                    return [btp.Coord_None];
                }
            }

            result.push(tail);
            return result;
        }

        function origColor(type) {
            return (type & (GridNS.Body | GridNS.Head | GridNS.Tail)) ?
               (type & GridNS.Head) ?
                   (type & GridNS.Hit) ?
                       Material.Red
                   :
                       Material.Green
               : (type & GridNS.Tail) ?
                   (type & GridNS.Hit) ?
                       Material.Orange
                   :
                       Material.Green
               : (type & GridNS.Hit) ?
                   Material.Orange
               :
                   Material.Teal
           : (type & GridNS.Hit) ?
               Material.Yellow
           :
               Material.Grey
        }
    }



    Component.onCompleted: {
        console.log("初始化 BTP 对象");
        btp.init();
    }

    readonly property bool inPortrait: mainWindow.width < mainWindow.height

    Popup {

        id: accInvitPopup
        parent: Overlay.overlay

        x: Math.round((parent.width - width) / 2)
        y: Math.round((parent.height - height) / 2)
        z: 20   // Critical
        width: 300
        height: accInvitColumn.height + 60

        modal: true
        closePolicy: Popup.NoAutoClose

        visible: (btp.state & StateNS.P2WaitAcc)

        ColumnLayout{

            id: accInvitColumn


            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top

            anchors.leftMargin: 30
            anchors.rightMargin: 30
            anchors.topMargin: 10

            spacing: 15

            Label {
                text:
                      qsTr("有人邀请你玩游戏")
                font.pointSize: 18
                wrapMode: "WrapAnywhere"
                Layout.preferredWidth: 200
            }

            Rectangle {
                Layout.preferredHeight: 10
            }

            Label {
                text: btp.popupHint[BTP.CPopupAccInvit]
            }

            Rectangle {
                Layout.preferredHeight: 10
            }

            RowLayout {
                Layout.alignment: Layout.Center

                Button {
                    id: accInvitRejectButton
                    Layout.alignment: Layout.Center
                    text: qsTr("拒绝")
                    onClicked: {
                        btp.triggerEvent(BTP.EvRefuseInvit, null);
                    }
                }

                Button {
                    id: accInvitAcceptButton
                    Layout.alignment: Layout.Center
                    text: qsTr("接受")
                    onClicked: {
                        btp.triggerEvent(BTP.EvAcceptInvit, null);
                    }
                }
            }

        }

    }

    Popup {

        id: initiaInvitPopup
        parent: Overlay.overlay

        x: Math.round((parent.width - width) / 2)
        y: Math.round((parent.height - height) / 2)
        width: 300
        height: initiaInvitColumn.height + 60

        modal: true
        closePolicy: Popup.NoAutoClose

        visible: ((btp.uiState & BTP.TPopupMask) ==  BTP.TPopupInitiaInvit)

        ColumnLayout{

            id: initiaInvitColumn

            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top

            anchors.leftMargin: 30
            anchors.rightMargin: 30
            anchors.topMargin: 10

            spacing: 15

            Label {
                text:
                      qsTr("确定邀请TA玩游戏？")
                font.pointSize: 18
                wrapMode: "WrapAnywhere"
                Layout.preferredWidth: 200
            }

            Rectangle {
                Layout.preferredHeight: 10
            }

            Label {
                text: btp.popupHint[BTP.TPopupInitiaInvit]
            }

            Rectangle {
                Layout.preferredHeight: 10
            }

            RowLayout {
                Layout.alignment: Layout.Center

                Button {
                    id: initiaInvitCancel
                    Layout.alignment: Layout.Center
                    text: qsTr("取消")
                    onClicked: {
                        btp.uiState &= ~BTP.TPopupMask;
                    }
                }

                Button {
                    id: initiaInvitOK
                    Layout.alignment: Layout.Center
                    text: qsTr("确定")
                    onClicked: {
                        btp.triggerEvent(BTP.EvInvite, [friendsListView.currentIndex]);
                        btp.uiState &= ~BTP.TPopupMask;
                    }
                }
            }

        }

    }

    Popup {

        id: loginPopup
        parent: Overlay.overlay

        x: Math.round((parent.width - width) / 2)
        y: Math.round((parent.height - height) / 2)
        z: 20   // Critical
        width: 300
        height: loginPopupColumn.height + 60

        modal: true
        closePolicy: Popup.NoAutoClose

        visible: (btp.state & (StateNS.CliPasswordError | StateNS.CliUsernameError)) || ( (btp.state & StateNS.Logout) && btp.username === "") && !(btp.uiState & BTP.TPopupError)

        // onVisibleChanged: {usernameField.text = ""; passwordField.text = "";}

        ColumnLayout{

            id: loginPopupColumn


            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top

            anchors.leftMargin: 30
            anchors.rightMargin: 30
            anchors.topMargin: 10

            spacing: 15

            Label {
                text: (btp.username === "") ?
                      qsTr("请登录") :
                    btp.state & (StateNS.CliUsernameError) ?
                      qsTr("用户名错误.") :
                    btp.state & (StateNS.CliPasswordError) ?
                      qsTr("密码错误.") : qsTr("")
                font.pointSize: 18
            }

            Rectangle {
                Layout.preferredHeight: 10
            }

            TextField {
                id: usernameField
                text: "fengshun"

                Layout.alignment: Layout.Center
                placeholderText: qsTr("用户名")
            }

            TextField {
                id: passwordField
                text: "1652270"

                Layout.alignment: Layout.Center
                placeholderText: qsTr("密码")

                echoMode: TextInput.Password

                onAccepted: {
                    loginButton.onClicked();
                }
            }

            Rectangle {
                height: 10
            }

            Button {
                id: loginButton
                Layout.alignment: Layout.Center
                text: qsTr("登录")
                onClicked: {
                    btp.username = usernameField.text;
                    btp.password = passwordField.text;
                    btp.triggerEvent(BTP.EvLogin, [usernameField.text, passwordField.text]);
                }
            }

        }

    }

    Popup {

        id: waitRespBusy
        parent: Overlay.overlay

        x: Math.round((parent.width - width) / 2)
        y: Math.round((parent.height - height) / 2)
        z: 20   // Critical
        width: 100
        height: 100

        background: null

        modal: true
        closePolicy: Popup.NoAutoClose

        visible: (btp.state & (StateNS.CliWaitLogin))

        BusyIndicator {
            anchors.centerIn: parent
            running: true
        }

    }

    Popup {

        id: waitAccBusy
        parent: Overlay.overlay

        x: Math.round((parent.width - width) / 2)
        y: Math.round((parent.height - height) / 2)
        z: 20   // Critical
        width: 300
        height: waitAccBusyColumn.height + 60

        modal: true
        closePolicy: Popup.NoAutoClose

        visible: (btp.state & StateNS.P1WaitAcc)

        ColumnLayout{

            id: waitAccBusyColumn


            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top

            anchors.leftMargin: 30
            anchors.rightMargin: 30
            anchors.topMargin: 10


            spacing: 15

            Label {
                text:
                      qsTr("等待TA作出回应…")
                font.pointSize: 18
                wrapMode: "WrapAnywhere"
                Layout.preferredWidth: 200
            }

            BusyIndicator {
                Layout.alignment: Qt.AlignCenter
                running: true
            }
        }
    }

    Popup {

        id: waitDeployBusy
        parent: Overlay.overlay

        x: Math.round((parent.width - width) / 2)
        y: Math.round((parent.height - height) / 2)
        z: 20   // Critical
        width: 300
        height: waitDeployBusyColumn.height + 60

        modal: true
        closePolicy: Popup.NoAutoClose

        visible: (btp.state & (StateNS.WaitDeploy))

        ColumnLayout{

            id: waitDeployBusyColumn


            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top

            anchors.leftMargin: 30
            anchors.rightMargin: 30
            anchors.topMargin: 10

            spacing: 15

            Label {
                text:
                      qsTr("等待TA部署完毕…")
                font.pointSize: 18
                wrapMode: "WrapAnywhere"
                Layout.preferredWidth: 200
            }

            BusyIndicator {
                Layout.alignment: Qt.AlignCenter
                running: true
            }
        }
    }

    Popup {

        id: opActionPopup
        parent: Overlay.overlay

        x: Math.round((parent.width - width) / 2)
        y: Math.round((parent.height - height) / 2)
        z: 10   // Trivial
        width: 300
        height: opActionPopupColumn.height + 60

        modal: true
        closePolicy: Popup.NoAutoClose

        visible: ((btp.uiState & BTP.TPopupMask) === BTP.TPopupOpAction)

        ColumnLayout{

            id: opActionPopupColumn

            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top

            anchors.leftMargin: 30
            anchors.rightMargin: 30
            anchors.topMargin: 10

            spacing: 15

            Label {
                text:
                      qsTr("对手操作")
                font.pointSize: 18
                wrapMode: "WrapAnywhere"
                Layout.preferredWidth: 200
            }

            Rectangle {
                Layout.preferredHeight: 10
            }

            Label {
                text: btp.popupHint[BTP.TPopupOpAction]
                Layout.preferredWidth: 200
                wrapMode: "WrapAnywhere"
            }

            Rectangle {
                Layout.preferredHeight: 10
            }

            RowLayout {
                Layout.alignment: Layout.Center

                Button {
                    id: opActionOK
                    Layout.alignment: Layout.Center
                    text: qsTr("了解")
                    onClicked: {
                        btp.uiState &= ~BTP.TPopupMask;
                    }
                }
            }

        }

    }

    Popup {

        id: actionResultPopup
        parent: Overlay.overlay

        x: Math.round((parent.width - width) / 2)
        y: Math.round((parent.height - height) / 2)
        z: 10   // Trivial
        width: 300
        height: actionResultPopupColumn.height + 60

        modal: true
        closePolicy: Popup.NoAutoClose

        visible: ((btp.uiState & BTP.TPopupMask) === BTP.TPopupActionResult)

        ColumnLayout{

            id: actionResultPopupColumn

            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top

            anchors.leftMargin: 30
            anchors.rightMargin: 30
            anchors.topMargin: 10

            spacing: 15

            Label {
                text:
                      qsTr("结果")
                font.pointSize: 18
                wrapMode: "WrapAnywhere"
                Layout.preferredWidth: 200
            }

            Rectangle {
                Layout.preferredHeight: 10
            }

            Label {
                text: btp.popupHint[BTP.TPopupActionResult]
                Layout.preferredWidth: 200
                wrapMode: "WrapAnywhere"
            }

            Rectangle {
                Layout.preferredHeight: 10
            }

            RowLayout {
                Layout.alignment: Layout.Center

                Button {
                    Layout.alignment: Layout.Center
                    text: qsTr("了解")
                    onClicked: {
                        btp.uiState &= ~BTP.TPopupMask;
                    }
                }
            }

        }

    }

    Popup {

        id: deployErrorPopup
        parent: Overlay.overlay

        x: Math.round((parent.width - width) / 2)
        y: Math.round((parent.height - height) / 2)
        z: 10   // Trivial
        width: 300
        height: deployErrorPopupColumn.height + 60

        modal: true
        closePolicy: Popup.NoAutoClose

        visible: ((btp.uiState & BTP.TPopupMask) === BTP.TPopupDeployError)

        ColumnLayout{

            id: deployErrorPopupColumn

            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top

            anchors.leftMargin: 30
            anchors.rightMargin: 30
            anchors.topMargin: 10

            spacing: 15

            Label {
                text:
                      qsTr("部署失败")
                font.pointSize: 18
                wrapMode: "WrapAnywhere"
                Layout.preferredWidth: 200
            }

            Rectangle {
                Layout.preferredHeight: 10
            }

            Label {
                text: qsTr("你的部署有错误。请再试一次。")
                Layout.preferredWidth: 200
                wrapMode: "WrapAnywhere"
            }

            Rectangle {
                Layout.preferredHeight: 10
            }

            RowLayout {
                Layout.alignment: Layout.Center

                Button {
                    id: deployErrorOk
                    Layout.alignment: Layout.Center
                    text: qsTr("了解")
                    onClicked: {
                        btp.uiState &= ~BTP.TPopupMask;
                    }
                }
            }

        }

    }

    Popup {

        id: chatPopup
        parent: Overlay.overlay

        x: Math.round((parent.width - width) / 2)
        y: Math.round((parent.height - height) / 2)
        z: 10   // Trivial
        width: 300
        height: chatPopupColumn.height + 60

        modal: true
        closePolicy: Popup.NoAutoClose

        visible: ((btp.uiState & BTP.TPopupMask) === BTP.TPopupChat)

        ColumnLayout{

            id: chatPopupColumn

            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top

            anchors.leftMargin: 30
            anchors.rightMargin: 30
            anchors.topMargin: 10

            spacing: 15

            Label {
                text:
                      btp.opName + qsTr("说：")
                font.pointSize: 18
                wrapMode: "WrapAnywhere"
                Layout.preferredWidth: 200
            }

            Rectangle {
                Layout.preferredHeight: 10
            }

            Label {
                text: btp.popupHint[BTP.TPopupChat]
                Layout.preferredWidth: 200
                wrapMode: "WrapAnywhere"
            }

            Rectangle {
                Layout.preferredHeight: 10
            }

            RowLayout {
                Layout.alignment: Layout.Center

                Button {
                    id: chatOK
                    Layout.alignment: Layout.Center
                    text: qsTr("了解")
                    onClicked: {
                        btp.uiState &= ~BTP.TPopupMask;
                    }
                }
            }

        }

    }

    Popup {

        id: invitResultPopup
        parent: Overlay.overlay

        x: Math.round((parent.width - width) / 2)
        y: Math.round((parent.height - height) / 2)
        z: 10    // Trivial
        width: 300
        height: invitResultPopupColumn.height + 60

        modal: true
        closePolicy: Popup.NoAutoClose

        visible: ((btp.uiState & BTP.TPopupMask) === BTP.TPopupInvitResult)

        ColumnLayout{

            id: invitResultPopupColumn

            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top

            anchors.leftMargin: 30
            anchors.rightMargin: 30
            anchors.topMargin: 10

            spacing: 15

            Label {
                text:
                      qsTr("邀请结果")
                font.pointSize: 18
                wrapMode: "WrapAnywhere"
                Layout.preferredWidth: 200
            }

            Rectangle {
                Layout.preferredHeight: 10
            }

            Label {
                text: btp.popupHint[BTP.TPopupInvitResult]
                Layout.preferredWidth: 200
                wrapMode: "WrapAnywhere"
            }

            Rectangle {
                Layout.preferredHeight: 10
            }

            RowLayout {
                Layout.alignment: Layout.Center

                Button {
                    id: invitResultOK
                    Layout.alignment: Layout.Center
                    text: qsTr("了解")
                    onClicked: {
                        btp.uiState &= ~BTP.TPopupMask;
                    }
                }
            }

        }

    }

    Popup {

        id: winLosePopup
        parent: Overlay.overlay

        x: Math.round((parent.width - width) / 2)
        y: Math.round((parent.height - height) / 2)
        z: 10    // Trivial
        width: 300
        height: winLosePopupColumn.height + 60

        modal: true
        closePolicy: Popup.NoAutoClose

        visible: ((btp.uiState & BTP.TPopupMask) === BTP.TPopupWinLose)

        ColumnLayout{

            id: winLosePopupColumn

            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top

            anchors.leftMargin: 30
            anchors.rightMargin: 30
            anchors.topMargin: 10

            spacing: 15

            Label {
                text:
                      qsTr("游戏结束")
                font.pointSize: 18
                wrapMode: "WrapAnywhere"
                Layout.preferredWidth: 200
            }

            Rectangle {
                Layout.preferredHeight: 10
            }

            Label {
                text: btp.popupHint[BTP.TPopupWinLose]
                Layout.preferredWidth: 200
                wrapMode: "WrapAnywhere"
            }

            Rectangle {
                Layout.preferredHeight: 10
            }

            RowLayout {
                Layout.alignment: Layout.Center

                Button {
                    Layout.alignment: Layout.Center
                    text: qsTr("了解")
                    onClicked: {
                        btp.uiState &= ~BTP.TPopupMask;
                    }
                }
            }

        }

    }

    Popup {

        id: errorPopup
        parent: Overlay.overlay

        x: Math.round((parent.width - width) / 2)
        y: Math.round((parent.height - height) / 2)
        z: 20    // Trivial
        width: 300
        height: errorPopupColumn.height + 40

        modal: true
        closePolicy: Popup.NoAutoClose

        visible: ((btp.uiState & BTP.TPopupMask) === BTP.TPopupError)

        ColumnLayout {

            id: errorPopupColumn


            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top

            anchors.leftMargin: 30
            anchors.rightMargin: 30
            anchors.topMargin: 10

            spacing: 40

            Label {
                text: qsTr("错误")
                font.pointSize: 18
            }

            Label {
                text: btp.popupHint[BTP.TPopupError]
                Layout.preferredWidth: 200
                wrapMode: "WrapAnywhere"
            }

            Button {
                Layout.alignment: Qt.AlignCenter
                text: qsTr("确定")
                onClicked: {
                    btp.uiState &= ~BTP.TPopupMask;
                    btp.afterError();
                }

            }



        }

    }

    footer: ToolBar {
        id: statusBar
        height: 32
        z: 30

            Label {
                id: socketStateLabel

                text: btp.socketStateText
                font.pointSize: 6

                anchors.left: parent.left
                anchors.verticalCenter: parent.verticalCenter
                width: parent.width * 0.7
            }

            Label {
                text: btp.getStateText(btp.state)
                font.pointSize: 6

                anchors.verticalCenter: parent.verticalCenter
                anchors.right: parent.right

            }

        Component.onCompleted: {

        }
    }

    Drawer {
        id: drawer
        y: 0

        width: Math.min(height / 2, mainWindow.width)
        height: mainWindow.height - statusBar.height

        modal: inPortrait
        interactive: inPortrait
        visible: inPortrait ? true : true


        ListView {
            id: friendsListView
            anchors.fill: parent
            headerPositioning: ListView.PullBackHeader
            header: ToolBar {
                id: drawerHeader
                z: 2
                width: parent.width
                height: mainWindow.height / 3
                anchors.top: mainWindow.top

                contentHeight: logo.height

                Image {
                    id: logo
                    //anchors.top: parent.top
                    anchors.verticalCenter: parent.verticalCenter
                    width: parent.width
                    height: Math.min(-friendsListView.contentY, parent.height * 0.7)
                    source: "images/logo.png"
                    fillMode: Image.PreserveAspectFit
                }

                MenuSeparator {
                    parent: drawerHeader
                    width: parent.width
                    anchors.verticalCenter: parent.bottom
                }

                Label {
                    id: currUsername
                    text: (btp.state & StateNS.Logout) ? qsTr("未登录") : qsTr(btp.username)
                    anchors.left: parent.left
                    anchors.bottom: parent.bottom
                    font.pointSize: 24
                    anchors.margins: 20

                }

                DropShadow {
                    anchors.fill: currUsername
                    horizontalOffset: 3
                    verticalOffset: 3
                    radius: 8.0
                    samples: 17
                    color: "#80000000"
                    source: currUsername
                }

                RoundButton {
                    text: "\uf342"
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    anchors.margins: 10
                    font.family: "Material Design Icons"
                    ToolTip {
                        text: qsTr("登录/切换用户")
                        visible: hovered
                        delay: 500
                    }

                    Material.background: Material.Red

                    onClicked: {
                        btp.triggerEvent(BTP.EvLogout, null)
                    }
                }

                Label {
                    anchors.top: parent.bottom
                    anchors.topMargin: 30
                    anchors.left: parent.left
                    anchors.leftMargin: 30
                    anchors.horizontalCenter: parent.horizontalCenter

                    text: qsTr("貌似当前没有好友上线哦")
                    color: "grey"
                    visible: friendsListView.model.count === 0
                }

            }

            currentIndex: -1

            onCurrentIndexChanged: {
                btp.currentFriendIndex = currentIndex
            }

            model: btp.friends

            delegate: ItemDelegate {
                id: thisFriend

                text: username + qsTr("(") + btp.getStateText_friendly_int(currState) + qsTr(")")

                width: parent.width

                //: thisFriend.ListView.view.currentIndex === index ? Material.LightBlue : isGroup ? Material.LightGreen : Material.Grey;

                onClicked: {
                    console.log(currState);
                    ListView.view.currentIndex = index;
                    if (btp.state & StateNS.Idle)
                        btp.uiState |= BTP.TPopupInitiaInvit;
                    if (inPortrait) {
                        drawer.close()
                    }
                }

                Component.onCompleted: {
                }
            }

            ScrollIndicator.vertical: ScrollIndicator{}
        }
    }

    Drawer {
        id: gameDrawer
        y: 0
        width: Math.min(height / 2, mainWindow.width)
        height: mainWindow.height - statusBar.height

        interactive: ((!(drawer.position > 0.99)) || !inPortrait)
        edge: Qt.RightEdge

        ToolBar {
            id: gameGuessLogBar
            width: parent.width
            height: 80
            z: 4


            Label {
                id: gameGuessLogTitle
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.leftMargin: 40
                anchors.topMargin: 25
                anchors.bottomMargin: 30
                font.pointSize: 15
                text: qsTr("游戏日志")

                color: Material.foreground
            }
        }

        Flickable {
            id: gameGuessLogFlickable
            contentHeight: gameGuessLogItem.height
            clip: true
            anchors.top: gameGuessLogBar.bottom
            anchors.bottom: parent.bottom
            width: parent.width

            ScrollIndicator.vertical: ScrollIndicator {
            }

            Item {
                id: gameGuessLogItem
                anchors.left: parent.left
                anchors.right: parent.right

                anchors.leftMargin: 10
                anchors.rightMargin: 10

                height: childrenRect.height



                ColumnLayout {
                    id: gameGuessLogColumnLayout
                    width: parent.width
                    anchors.top: parent.top
                    anchors.topMargin: 20
                    Label {
                        id: gameGuessLog
                        Layout.preferredWidth: gameGuessLogColumnLayout.width

                        font.pointSize: 11
                        lineHeight: 2

                        wrapMode: Text.WrapAnywhere
                        text: btp.gameLog

                    }
                }
            }


        }
    }

    Rectangle {
        id: mainArea

        anchors.topMargin: 0
        width: inPortrait ? parent.width : parent.width - drawer.width
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom

        color: "#e6e6e6"

        property double bottomRatio : 0.15
        property double bottomMaxHeight : 56

        ToolBar {
            z: 9
            id: bottomBar
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.bottom: parent.bottom
            anchors.left: parent.left

            height: bottomBarRect.height

            Component.onCompleted: {
            }

            Rectangle {
                id: bottomBarRect

                color: "transparent"

                width: parent.width
                height: mainArea.bottomMaxHeight > mainArea.bottomRatio * mainArea.height ? mainArea.bottomRatio * mainArea.height : mainArea.bottomMaxHeight
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.bottom: parent.bottom



                //https://cdn.materialdesignicons.com/3.2.89/
                ToolButton {
                    id: switchMapbutton
                    text: "\uf019"
                    font.pointSize: height * 0.3
                    width: parent.width / 2
                    anchors.left: parent.left

                    height: mainArea.bottomMaxHeight > mainArea.bottomRatio * mainArea.height ? mainArea.bottomRatio * mainArea.height : mainArea.bottomMaxHeight
                    anchors.bottom: parent.bottom

                    font.family: "Material Design Icons"
                    // Material.background: Material.Red

                    onClicked: {
                        btp.switchMap();
                    }

                    Component.onCompleted: {

                    }
                }

                ToolButton {
                    id: guessButton
                    text: (btp.state & (StateNS.Deploy|StateNS.OpDeployed)) ? "\uf450" : (btp.uiState & BTP.MapModeGuess) ?  "\uf690" : "\ufa76"
                    font.pointSize: height * 0.3
                    width: parent.width / 2
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom

                    height: mainArea.bottomMaxHeight > mainArea.bottomRatio * mainArea.height ? mainArea.bottomRatio * mainArea.height : mainArea.bottomMaxHeight
                    font.family: "Material Design Icons"
                    // Material.background: Material.Red

                    onClicked: {
                        if (btp.state & (StateNS.Deploy|StateNS.OpDeployed)) {
                            btp.setHeadTails(util.iH1, btp.Coord_None);
                            btp.setHeadTails(util.iH2, btp.Coord_None);
                            btp.setHeadTails(util.iH3, btp.Coord_None);
                            btp.setHeadTails(util.iT1, btp.Coord_None);
                            btp.setHeadTails(util.iT2, btp.Coord_None);
                            btp.setHeadTails(util.iT3, btp.Coord_None);
                            btp.map.clear();
                            btp.currMap.clear();
                        }
                        else
                            if (btp.uiState & BTP.MapModeGuess) {
                                btp.switchToBomb();
                            } else {
                                btp.switchToGuess();
                            }
                    }
                }

            }

            RoundButton {

                id: confirmButton
                text: (btp.state & (StateNS.Deploy|StateNS.OpDeployed)) ? "\uf12c" : (btp.uiState & BTP.MapModeGuess) ? "\ufa76" : "\uf690"
                font.family: "Material Design Icons"
                font.pointSize: height * 0.35
                width: (mainArea.bottomMaxHeight > mainArea.bottomRatio * mainArea.height ? mainArea.bottomRatio * mainArea.height : mainArea.bottomMaxHeight) * 1.6
                height: width
                anchors.verticalCenter: bottomBarRect.top
                anchors.horizontalCenter: bottomBarRect.horizontalCenter
                Material.background: Material.Yellow

                onClicked: {
                    if (btp.state & (StateNS.Deploy|StateNS.OpDeployed)) {
                        if (btp.headTails[6] !== BTP.Coord_None &&
                                btp.headTails[7] !== BTP.Coord_None &&
                                btp.headTails[8] !== BTP.Coord_None &&
                                btp.headTails[9] !== BTP.Coord_None &&
                                btp.headTails[10] !== BTP.Coord_None &&
                                btp.headTails[11] !== BTP.Coord_None)
                            btp.triggerEvent(BTP.EvDeploy, null);
                    } else
                        if ((btp.uiState & BTP.MapModeBomb)) {
                            if (btp.mapPos1 !== btp.Coord_None)
                                btp.triggerEvent(BTP.EvBomb, null/* TODO */);
                        } else {
                            if (btp.mapPos2 !== btp.Coord_None && btp.mapPos1 !== btp.Coord_None)
                                btp.triggerEvent(BTP.EvGuess, null/* TODO */);
                        }
                }

                enabled: true
            }
        }

        ToolBar {
            id: topBar
            width: parent.width
            height: 54
            z: 4

            anchors.top: parent.top

            ToolButton {
                id: menuButton
                anchors.left: parent.left
                text: "\uf35c"
                font.family: "Material Design Icons"
                visible: inPortrait

                onClicked: {
                    drawer.open();
                }
            }

            Label {
                id: topBarTitleLabel
                anchors.left: menuButton.right
                anchors.verticalCenter: parent.verticalCenter
                text: mainWindow.title
                font.pointSize: 13
            }

            ToolButton {
                id: quitGameButton
                anchors.right: parent.right
                text: "\ufa47"
                font.family: "Material Design Icons"

                onClicked: {
                    btp.triggerEvent(BTP.EvQuitGame, null)
                }
            }

            Component.onCompleted: {
                //console.log(friendUsername.height + 20)
            }

            //height: friendUsername.height + 20
        }

        Rectangle {
            id: gameField

            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: topBar.bottom
            anchors.bottom: bottomBar.top
            // anchors.topMargin: 10
            // anchors.bottomMargin: 10

            //contentWidth: width
            //contentHeight: gameFieldColumn.height

            clip: true

            ColumnLayout {
                id: gameFieldColumnLayout
                spacing: 2

                 Item {
                    id: mapRect
                    z: 0
                    Layout.preferredWidth: gameField.width
                    Layout.preferredHeight: gameField.height * 0.80

                    GridView {
                        id: mapGridView
                        interactive: false

                        width: parent.width * 0.95
                        height: parent.height * 0.95

                        anchors.centerIn: parent

                        cellWidth: width / 10
                        cellHeight: height / 10

                        model: btp.currMap

                        property int refresh : 0

                        delegate: Rectangle {
                            width: mapGridView.cellWidth
                            height: mapGridView.cellHeight
                            Button {
                                down: highlighted
                                highlighted: mapGridView.refresh,
                                (btp.state & (StateNS.Deploy | StateNS.OpDeployed)) ?
                                    (function(){
                                        var currPlaneList;
                                        var posInPlaneList;
                                        var i;

                                        if (btp.uiState & BTP.MapCampTheirSide)
                                            return false;

                                        if (btp.headTails[util.iH1] === btp.Coord_None) {
                                            return false;
                                        }
                                        if (btp.headTails[util.iT1] === btp.Coord_None) {
                                            if (index ===  btp.headTails[util.iH1]) {
                                                return true;
                                            }

                                            return false;
                                        }
                                        if (btp.headTails[util.iH2] === btp.Coord_None) {
                                            return false;
                                        }
                                        if (btp.headTails[util.iT2] === btp.Coord_None) {

                                            if (index ===  btp.headTails[util.iH2]) {
                                                return true;
                                            }
                                            return false;
                                        }

                                        if (btp.headTails[util.iH3] === btp.Coord_None) {
                                            return false;
                                        }
                                        if (btp.headTails[util.iT3] === btp.Coord_None) {

                                            if (index ===  btp.headTails[util.iH3]) {
                                                return true;
                                            }


                                            return false;
                                        }

                                        return false;
                                    })()
                                :
                                ((btp.state & (StateNS.MyTurn)) && (btp.uiState & BTP.MapModeBomb)) ?
                                     (function(){

                                         if (btp.uiState & BTP.MapCampOurSide)
                                             return false;

                                         if (btp.mapPos1 === btp.Coord_None) {
                                             return false;
                                         }

                                         if (index === btp.mapPos1) {
                                             return true;
                                         }

                                         return false;

                                     })()
                                :
                                ((btp.state & (StateNS.MyTurn)) && (btp.uiState & BTP.MapModeGuess)) ?
                                          (function(){

                                              if (btp.uiState & BTP.MapCampOurSide)
                                                  return false;

                                              if (btp.mapPos1 === btp.Coord_None) {
                                                  return false;
                                              }

                                              if (btp.mapPos2 === btp.Coord_None) {
                                                  if (index === btp.mapPos1) {
                                                      return true;
                                                  }
                                                  return false;
                                              }

                                              if (index === btp.mapPos1) {
                                                  return true;
                                              }

                                              if (index === btp.mapPos2) {
                                                  return true;
                                              }

                                              return false;

                                          })()
                                : false

                                text: label
                                font.pointSize: 8

                                property int origColor : util.origColor(type)

                                Material.background:
                                    mapGridView.refresh,

                                    (btp.state & (StateNS.Deploy | StateNS.OpDeployed)) ?
                                        (function(){
                                            var currPlaneList;
                                            var posInPlaneList;
                                            var i;

                                            if (btp.uiState & BTP.MapCampTheirSide)
                                                return origColor;

                                            if (btp.headTails[util.iH1] === btp.Coord_None) {
                                                return origColor;
                                            }
                                            if (btp.headTails[util.iT1] === btp.Coord_None) {
                                                if (index ===  btp.headTails[util.iH1]) {
                                                    return util.origColor(GridNS.Head | GridNS.Selected);
                                                }
                                                for (i in util.deployAblePlanes) {
                                                    currPlaneList = util.deployAblePlanes[i];
                                                    posInPlaneList = currPlaneList.indexOf(index);
                                                    if (posInPlaneList === currPlaneList.length - 1) {
                                                        return util.origColor(GridNS.Tail);
                                                    }
                                                    if (posInPlaneList >= 0) {
                                                        return util.origColor(GridNS.Body);
                                                    }
                                                }

                                                return origColor;
                                            }
                                            if (btp.headTails[util.iH2] === btp.Coord_None) {
                                                return origColor;
                                            }
                                            if (btp.headTails[util.iT2] === btp.Coord_None) {

                                                if (index ===  btp.headTails[util.iH2]) {
                                                    return util.origColor(GridNS.Head | GridNS.Selected);
                                                }
                                                for (i in util.deployAblePlanes) {
                                                    currPlaneList = util.deployAblePlanes[i];
                                                    posInPlaneList = currPlaneList.indexOf(index);
                                                    if (posInPlaneList === currPlaneList.length - 1) {
                                                        return util.origColor(GridNS.Tail);
                                                    }
                                                    if (posInPlaneList >= 0) {
                                                        return util.origColor(GridNS.Body);
                                                    }
                                                }

                                                return origColor;
                                            }

                                            if (btp.headTails[util.iH3] === btp.Coord_None) {
                                                return origColor;
                                            }
                                            if (btp.headTails[util.iT3] === btp.Coord_None) {

                                                if (index ===  btp.headTails[util.iH3]) {
                                                    return util.origColor(GridNS.Head | GridNS.Selected);
                                                }
                                                for (i in util.deployAblePlanes) {
                                                    currPlaneList = util.deployAblePlanes[i];
                                                    posInPlaneList = currPlaneList.indexOf(index);
                                                    if (posInPlaneList === currPlaneList.length - 1) {
                                                        return util.origColor(GridNS.Tail);
                                                    }
                                                    if (posInPlaneList >= 0) {
                                                        return util.origColor(GridNS.Body);
                                                    }
                                                }

                                                return origColor;
                                            }

                                            return origColor;
                                        })()
                                    :
                                    ((btp.state & (StateNS.MyTurn)) && (btp.uiState & BTP.MapModeBomb)) ?
                                         (function(){

                                             if (btp.uiState & BTP.MapCampOurSide)
                                                 return origColor;

                                             if (btp.mapPos1 === btp.Coord_None) {
                                                 return origColor;
                                             }

                                             if (index === btp.mapPos1) {
                                                 return util.origColor(type | GridNS.Selected);
                                             }

                                             return origColor;

                                         })()
                                    :
                                    ((btp.state & (StateNS.MyTurn)) && (btp.uiState & BTP.MapModeGuess)) ?
                                              (function(){

                                                  if (btp.uiState & BTP.MapCampOurSide)
                                                      return origColor;

                                                  if (btp.mapPos1 === btp.Coord_None) {
                                                      return origColor;
                                                  }

                                                  if (btp.mapPos2 === btp.Coord_None) {
                                                      if (index === btp.mapPos1) {
                                                          return util.origColor(type | GridNS.Selected | GridNS.Head);
                                                      }
                                                      return origColor;
                                                  }

                                                  if (index === btp.mapPos1) {
                                                      return util.origColor(type | GridNS.Selected | GridNS.Head);
                                                  }

                                                  if (index === btp.mapPos2) {
                                                      return util.origColor(type | GridNS.Selected | GridNS.Tail);
                                                  }

                                                  return origColor;

                                              })()
                                    : origColor

                                Material.elevation: 0
                                anchors.left: parent.left
                                anchors.right: parent.right
                                anchors.verticalCenter: parent.verticalCenter
                                height: parent.height * 1.5

                                onClicked: {



                                    (btp.state & (StateNS.Deploy | StateNS.OpDeployed)) ?
                                        (function(){
                                            var currPlaneList;
                                            var posInPlaneList;
                                            var i;
                                            var j;

                                            if (btp.uiState & BTP.MapCampTheirSide) {
                                                return;
                                            }

                                            if (btp.headTails[util.iH1] === btp.Coord_None) {
                                                btp.setHeadTails(util.iH1, index);
                                                util.deploying = util.iH1;
                                                return;
                                            }
                                            if (btp.headTails[util.iT1] === btp.Coord_None) {
                                                for (i in util.deployAblePlanes) {
                                                    currPlaneList = util.deployAblePlanes[i];
                                                    posInPlaneList = currPlaneList.indexOf(index);
                                                    if (posInPlaneList === currPlaneList.length - 1) {

                                                        for (j = 1; j < currPlaneList.length - 1; j++) {
                                                            btp.map.setGrid_i(currPlaneList[j], GridNS.Body);
                                                        }
                                                        btp.map.setGrid_i(currPlaneList[0], GridNS.Head);
                                                        btp.map.setGrid_i(currPlaneList[currPlaneList.length - 1], GridNS.Tail);
                                                        btp.setHeadTails(util.iT1, index);
                                                        return;
                                                    }
                                                }
                                                btp.setHeadTails(util.iH1, btp.Coord_None);
                                                return;
                                            }
                                            if (btp.headTails[util.iH2] === btp.Coord_None) {
                                                btp.setHeadTails(util.iH2, index);
                                                util.deploying = util.iH2;
                                                return;
                                            }
                                            if (btp.headTails[util.iT2] === btp.Coord_None) {
                                                for (i in util.deployAblePlanes) {
                                                    currPlaneList = util.deployAblePlanes[i];
                                                    posInPlaneList = currPlaneList.indexOf(index);
                                                    if (posInPlaneList === currPlaneList.length - 1) {

                                                        for (j = 1; j < currPlaneList.length - 1; j++) {
                                                            btp.map.setGrid_i(currPlaneList[j], GridNS.Body);
                                                        }
                                                        btp.map.setGrid_i(currPlaneList[0], GridNS.Head);
                                                        btp.map.setGrid_i(currPlaneList[currPlaneList.length - 1], GridNS.Tail);

                                                        btp.setHeadTails(util.iT2, index);
                                                        return;
                                                    }
                                                }
                                                btp.setHeadTails(util.iH2, btp.Coord_None);
                                                return;
                                            }

                                            if (btp.headTails[util.iH3] === btp.Coord_None) {
                                                btp.setHeadTails(util.iH3, index);
                                                util.deploying = util.iH3;
                                                return;
                                            }
                                            if (btp.headTails[util.iT3] === btp.Coord_None) {
                                                for (i in util.deployAblePlanes) {
                                                    currPlaneList = util.deployAblePlanes[i];
                                                    posInPlaneList = currPlaneList.indexOf(index);
                                                    if (posInPlaneList === currPlaneList.length - 1) {
                                                        for (j = 1; j < currPlaneList.length - 1; j++) {
                                                            btp.map.setGrid_i(currPlaneList[j], GridNS.Body);
                                                        }
                                                        btp.map.setGrid_i(currPlaneList[0], GridNS.Head);
                                                        btp.map.setGrid_i(currPlaneList[currPlaneList.length - 1], GridNS.Tail);
                                                        btp.setHeadTails(util.iT3, index);
                                                        return;
                                                    }
                                                }
                                                btp.setHeadTails(util.iH3, btp.Coord_None);
                                                return;
                                            }

                                            ;
                                        })()
                                    :
                                    ((btp.state & (StateNS.MyTurn)) && (btp.uiState & BTP.MapModeBomb)) ?
                                         (function(){

                                             if (btp.uiState & BTP.MapCampOurSide) {
                                                 return;
                                             }

                                             if (btp.mapPos1 === btp.Coord_None) {
                                                 btp.mapPos1 = index;
                                                 return;
                                             }

                                             if (btp.mapPos1 === index) {
                                                 btp.mapPos1 = btp.Coord_None;
                                             } else {
                                                 btp.mapPos1 = index;
                                             }

                                         })()
                                    :
                                    ((btp.state & (StateNS.MyTurn)) && (btp.uiState & BTP.MapModeGuess)) ?
                                              (function(){

                                                  if (btp.uiState & BTP.MapCampOurSide) {
                                                      return;
                                                  }

                                                  if (btp.mapPos1 === btp.Coord_None) {
                                                      btp.mapPos1 = index;
                                                      return;
                                                  }

                                                  if (btp.mapPos2 === btp.Coord_None) {
                                                      btp.mapPos2 = index;
                                                      return;
                                                  }

                                                  btp.mapPos1 = btp.Coord_None;
                                                  btp.mapPos2 = btp.Coord_None;

                                              })()
                                    : null

                                    mapGridView.refresh++;
                                    btp.syncMap();
                                }
                            }
                        }
                    }
                }

                Label {
                    id: mapHint1


                    Layout.preferredHeight: 0
                    Layout.alignment: Qt.AlignLeft
                    Layout.leftMargin: 20
                    Layout.preferredWidth: gameField.width * 0.3


                    wrapMode: Text.WrapAnywhere
                    text:
                        (btp.state & StateNS.Deploy) ?
                            "我方部署"
                        : (btp.state & StateNS.WaitDeploy) ?
                            "等待对方部署"
                        : (btp.state & StateNS.OpDeployed) ?
                            "我方部署(对方完成)"
                        : (btp.state & StateNS.MyTurn) ?
                            "我方行动"
                        : (btp.state & StateNS.WaitOp) ?
                            "对方行动"
                        : ((btp.state & StateNS.CliWaitRDeploy) || (btp.state & StateNS.CliWaitRDeployWhenOpDeployed)) ?
                            "提交部署"
                        : ((btp.state & StateNS.CliWaitRBomb) || (btp.state & StateNS.CliWaitRGuess)) ?
                            "提交行动"
                        :
                            ""


                    font.pointSize: 18

                }

                Label {
                    id: mapHint2


                    Layout.preferredHeight: 0
                    Layout.alignment: Qt.AlignRight
                    Layout.rightMargin: 20
                    Layout.preferredWidth: gameField.width * 0.55

                    wrapMode: Text.WrapAnywhere
                    text:
                              (btp.state & StateNS.Deploy) ?
                                  "请切换到我方地图，点击地图完成我方飞机部署。"
                              : (btp.state & StateNS.WaitDeploy) ?
                                  "对方部署中……"
                              : (btp.state & StateNS.OpDeployed) ?
                                  "请切换到我方地图，点击地图完成我方飞机部署。对方部署完毕。"
                              : (btp.state & StateNS.MyTurn) ?
                                  "请切换到对方地图，使用炸弹或准星完成“炸”/“猜飞机”操作。"
                              : (btp.state & StateNS.WaitOp) ?
                                  "等待对方行动……"
                              : ((btp.state & StateNS.CliWaitRDeploy) || (btp.state & StateNS.CliWaitRDeployWhenOpDeployed)) ?
                                  "等待服务器响应……"
                              : ((btp.state & StateNS.CliWaitRBomb) || (btp.state & StateNS.CliWaitRGuess)) ?
                                  "等待服务器响应……"
                              :
                                  ""


                    font.pointSize: 12

                }

            }

            Label {
                id: mapCampLabel

                height: gameField.height
                anchors.left: parent.left
                anchors.leftMargin: 20
                anchors.top: parent.top
                anchors.topMargin: gameField.height * 0.02

                wrapMode: Text.WrapAnywhere
                text: (btp.uiState & BTP.MapCampOurSide) ? qsTr("我方") : qsTr("对方")
                font.pointSize: gameField.width * 0.2
                font.bold: true
                font.italic: true

                color: Qt.rgba(0, 0, 0, 0.2)

                layer.enabled: true
                layer.effect: DropShadow {
                    verticalOffset: gameField.width * 0.05
                    horizontalOffset: gameField.width * 0.05
                    color: "#40000000"
                    radius: 1
                    samples: 3
                }


            }

        }
    }
}
