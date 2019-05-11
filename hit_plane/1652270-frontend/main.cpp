#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QtQml>
#include <QFont>
#include <QFontInfo>
#include <QFontDatabase>
#include <QDir>
#include "BTP.h"

int main(int argc, char *argv[])
{
    qDebug() << QDir::currentPath();

    QLocale currLocale(QLocale("zh_CN"));
    QLocale::setDefault(currLocale);

    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

    QGuiApplication app(argc, argv);

    QFontDatabase::addApplicationFont(":/materialdesignicons-webfont.ttf");

    QFont::insertSubstitution("Droid Sans Mono", "华文细黑");
    QFont font("Droid Sans Mono");

    font.setStyleHint(QFont::StyleHint::SansSerif);
    qDebug() << QFontInfo(font).family();
    app.setFont(font);

    QQmlApplicationEngine engine;

    qmlRegisterUncreatableType<BTP>("sj.bombtheplane", 1, 0, "BTP", "No reason");
    qmlRegisterUncreatableType<Friend>("sj.bombtheplane", 1, 0, "Friend", "No reason");
    qmlRegisterUncreatableType<FriendsModel>("sj.bombtheplane", 1, 0, "FriendsModel", "No reason");
    qmlRegisterUncreatableType<BTPMapModel>("sj.bombtheplane", 1, 0, "BTPMapModel", "No reason");
    qmlRegisterUncreatableMetaObject(StateNS::staticMetaObject, "sj.bombtheplane", 1, 0, "StateNS", "No reason");
    qmlRegisterUncreatableMetaObject(MsgTypeNS::staticMetaObject, "sj.bombtheplane", 1, 0, "MsgTypeNS", "No reason");
    qmlRegisterUncreatableMetaObject(GridNS::staticMetaObject, "sj.bombtheplane", 1, 0, "GridNS", "No reason");

    BTP &btp = *new BTP;
    //engine.setObjectOwnership(&im, QQmlEngine::JavaScriptOwnership);

    engine.rootContext()->setContextProperty("btp", &btp);

    engine.load(QUrl(QStringLiteral("qrc:/main.qml")));
    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
}
