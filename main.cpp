#include "twofamanager.h"
#include<QObject>
#include <QApplication>
#include<QLocalServer>
#include<QLocalSocket>
#include<QMessageBox>
/*
int main(int argc, char *argv[])
{
    QApplication::setApplicationName("Kerveros");
    QApplication::setOrganizationName("Alamahant");
    QApplication::setApplicationVersion("1.0.0");

    QApplication a(argc, argv);


    // Create a unique server name for this app
    QString serverName = "Kerveros_2FA_App";
    QLocalSocket socket;

    qDebug() << "=== SINGLE INSTANCE DEBUG ===";
       qDebug() << "Trying to connect to server:" << serverName;
    socket.connectToServer(serverName);

    // Try to connect to existing instance
    if (socket.waitForConnected(500)) {
        // Another instance is running - bring it to front and exit
        QMessageBox::information(nullptr, "Already Running",
                                "Kerveros is already running.\nCheck your system tray or taskbar.");
        socket.disconnectFromServer();
        return 0;
    }

    // No other instance - create server and run app
    QLocalServer server;
    if (!server.listen(serverName)) {
        // Couldn't create server - maybe permissions issue, but continue anyway
        qWarning() << "Could not create single instance server:" << server.errorString();
    }


    TwoFAManager manager;

    manager.show();
    return a.exec();
}
*/


int main(int argc, char *argv[])
{
    QApplication::setApplicationName("Kerveros");
    QApplication::setOrganizationName("Alamahant");
    QApplication::setApplicationVersion("1.0.0");

    QApplication a(argc, argv);

    // Create a unique server name for this app
    QString serverName = "Kerveros_2FA_App";
    QLocalSocket socket;

    qDebug() << "=== SINGLE INSTANCE DEBUG ===";
    qDebug() << "Trying to connect to server:" << serverName;

    socket.connectToServer(serverName);

    // Try to connect to existing instance
    if (socket.waitForConnected(500)) {
        qDebug() << "SUCCESS: Connected to existing instance - another Kerveros is running";
        QMessageBox::information(nullptr, "Already Running",
                                "Kerveros is already running.\nCheck your system tray or taskbar.");
        socket.disconnectFromServer();
        return 0;
    } else {
        qDebug() << "FAILED: No existing instance found. Socket error:" << socket.error() << "-" << socket.errorString();

        // CLEAN UP STALE SERVER FILE
        qDebug() << "Cleaning up stale server file...";
        QLocalServer::removeServer(serverName);
    }

    // No other instance - create server and run app
    QLocalServer server;
    if (!server.listen(serverName)) {
        qDebug() << "ERROR: Could not create single instance server:" << server.errorString();
        qWarning() << "Could not create single instance server:" << server.errorString();
    } else {
        qDebug() << "SUCCESS: Server created and listening on:" << serverName;
        qDebug() << "Server full name:" << server.fullServerName();
        qDebug() << "Server is listening:" << server.isListening();
    }

    TwoFAManager manager;
    qDebug() << "TwoFAManager created, about to show window...";

    manager.show();
    qDebug() << "Window shown, entering event loop...";

    int result = a.exec();
    qDebug() << "Event loop ended with code:" << result;

    return result;
}
