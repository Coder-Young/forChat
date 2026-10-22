QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = forChat
TEMPLATE = app
RC_ICONS = icon.ico
DESTDIR = ./bin

DEFINES += QT_DEPRECATED_WARNINGS

CONFIG += c++11
include(face_auth/face_auth.pri)

SOURCES += \
        BubbleFrame.cpp \
        ChatItemBase.cpp \
        ChatView.cpp \
        MessageTextEdit.cpp \
        PictureBubble.cpp \
        TextBubble.cpp \
        adduseritem.cpp \
        applyfriend.cpp \
        applyfrienditem.cpp \
        applyfriendlist.cpp \
        applyfriendpage.cpp \
        authenfriend.cpp \
        chatdialog.cpp \
        chatpage.cpp \
        chatuserlist.cpp \
        chatuserwid.cpp \
        clickedbtn.cpp \
        clickedlabel.cpp \
        clickedoncelabel.cpp \
        contactuserlist.cpp \
        conuseritem.cpp \
        customizeedit.cpp \
        customizetextedit.cpp \
        findfaildlg.cpp \
        findsuccessdlg.cpp \
        friendinfopage.cpp \
        friendlabel.cpp \
        global.cpp \
        grouptipitem.cpp \
        httpmgr.cpp \
        imagecropperlabel.cpp \
        invaliditem.cpp \
        lineitem.cpp \
        listitembase.cpp \
        loadingdlg.cpp \
        logindialog.cpp \
        main.cpp \
        mainwindow.cpp \
        registerdialog.cpp \
        resetdialog.cpp \
        searchlist.cpp \
        statelabel.cpp \
        statewidget.cpp \
        tcpmgr.cpp \
        timerbtn.cpp \
        userdata.cpp \
        userinfopage.cpp \
        usermgr.cpp

HEADERS += \
        BubbleFrame.h \
        ChatItemBase.h \
        ChatView.h \
        MessageTextEdit.h \
        PictureBubble.h \
        TextBubble.h \
        adduseritem.h \
        applyfriend.h \
        applyfrienditem.h \
        applyfriendlist.h \
        applyfriendpage.h \
        authenfriend.h \
        chatdialog.h \
        chatpage.h \
        chatuserlist.h \
        chatuserwid.h \
        clickedbtn.h \
        clickedlabel.h \
        clickedoncelabel.h \
        contactuserlist.h \
        conuseritem.h \
        customizeedit.h \
        customizetextedit.h \
        findfaildlg.h \
        findsuccessdlg.h \
        friendinfopage.h \
        friendlabel.h \
        global.h \
        grouptipitem.h \
        httpmgr.h \
        imagecropperdialog.h \
        imagecropperlabel.h \
        invaliditem.h \
        lineitem.h \
        listitembase.h \
        loadingdlg.h \
        logindialog.h \
        mainwindow.h \
        registerdialog.h \
        resetdialog.h \
        searchlist.h \
        singleton.h \
        statelabel.h \
        statewidget.h \
        tcpmgr.h \
        timerbtn.h \
        userdata.h \
        userinfopage.h \
        usermgr.h

FORMS += \
        adduseritem.ui \
        applyfriend.ui \
        applyfrienditem.ui \
        applyfriendpage.ui \
        authenfriend.ui \
        chatdialog.ui \
        chatpage.ui \
        chatuserwid.ui \
        conuseritem.ui \
        findfaildlg.ui \
        findsuccessdlg.ui \
        friendinfopage.ui \
        friendlabel.ui \
        grouptipitem.ui \
        lineitem.ui \
        loadingdlg.ui \
        logindialog.ui \
        mainwindow.ui \
        registerdialog.ui \
        resetdialog.ui \
        userinfopage.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    rc.qrc

DISTFILES += \
    config.ini \
    res/add_friend.png \
    res/add_friend_hover.png \
    res/add_friend_normal.png \
    res/addtip.png \
    res/arowdown.png \
    res/chat_icon.png \
    res/chat_icon_hover.png \
    res/chat_icon_press.png \
    res/chat_icon_select_hover.png \
    res/chat_icon_select_press.png \
    res/close_search.png \
    res/close_transparent.png \
    res/contact_list.png \
    res/contact_list_hover.png \
    res/contact_list_press.png \
    res/female.png \
    res/filedir.png \
    res/filedir_hover.png \
    res/filedir_press.png \
    res/head_1.jpg \
    res/head_2.jpg \
    res/head_3.jpg \
    res/head_4.jpg \
    res/head_5.jpg \
    res/ice.png \
    res/loading.gif \
    res/male.png \
    res/msg_chat_hover.png \
    res/msg_chat_normal.png \
    res/msg_chat_press.png \
    res/red_point.png \
    res/right_tip.png \
    res/search.png \
    res/smile.png \
    res/smile_hover.png \
    res/smile_press.png \
    res/tipclose.png \
    res/unvisible.png \
    res/unvisible_hover.png \
    res/video_chat_hover.png \
    res/video_chat_normal.png \
    res/video_chat_press.png \
    res/visible.png \
    res/visible_hover.png \
    res/voice_chat_hover.png \
    res/voice_chat_normal.png \
    res/voice_chat_press.png

win32:CONFIG(release, debug | release)
{
    #指定要拷贝的文件目录为工程目录下release目录下的所有dll、lib文件，例如工程目录在D:\QT\Test
    #PWD就为D:/QT/Test，DllFile = D:/QT/Test/release/*.dll
    TargetConfig = $${PWD}/config/config.ini
    #将输入目录中的"/"替换为"\"
    TargetConfig = $$replace(TargetConfig, /, \\)
    #将输出目录中的"/"替换为"\"
    OutputDir =  $${OUT_PWD}/$${DESTDIR}
    OutputDir = $$replace(OutputDir, /, \\)
    //执行copy命令
    QMAKE_POST_LINK += copy /Y \"$$TargetConfig\" \"$$OutputDir\"
}


#win32-msvc*:QMAKE_CXXFLAGS += /wd"4819" /utf-8
