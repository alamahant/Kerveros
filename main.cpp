#include "twofamanager.h"
#include<QObject>
#include <QApplication>
#include<QLocalServer>
#include<QLocalSocket>
#include<QMessageBox>
#include<QLockFile>
#include<QStandardPaths>
#include<QProcess>

/*
int main(int argc, char *argv[])
{
    QApplication::setApplicationName("Kerveros");
    QApplication::setOrganizationName("Alamahant");
    QApplication::setApplicationVersion("1.0.0");

    QApplication a(argc, argv);

    // Absolute socket path for single-instance check
    QString socketPath = QStandardPaths::writableLocation(QStandardPaths::RuntimeLocation)
                         + "/Kerveros.sock";

    // Try to connect to an existing instance
    QLocalSocket socket;
    socket.connectToServer(socketPath);

    if (socket.waitForConnected(500)) {
        // Another instance is running
        QMessageBox::information(nullptr, "Already Running",
                                 "Kerveros is already running.\nCheck your system tray or taskbar.");
        socket.disconnectFromServer();
        return 0;
    } else {
        // Clean up any stale socket
        QLocalServer::removeServer(socketPath);
    }

    // Create server for this instance
    QLocalServer server;
    if (!server.listen(socketPath)) {
        qWarning() << "Could not create single instance server:" << server.errorString();
        // Optional: continue anyway
    }

    TwoFAManager manager;
    manager.show();

    return a.exec();
}
*/
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


    socket.connectToServer(serverName);

    // Try to connect to existing instance
    if (socket.waitForConnected(500)) {
        QMessageBox::information(nullptr, "Already Running",
                                "Kerveros is already running.\nCheck your system tray or taskbar.");
        socket.disconnectFromServer();
        return 0;
    } else {

        // CLEAN UP STALE SERVER FILE
        QLocalServer::removeServer(serverName);
    }

    // No other instance - create server and run app
    QLocalServer server;

    if (!server.listen(serverName)) {
        qWarning() << "Could not create single instance server:" << server.errorString();
    } else {
    }

    TwoFAManager manager;

    manager.show();

    int result = a.exec();

    return result;
}
*/


int main(int argc, char *argv[])
{

    QApplication::setApplicationName("Kerveros");
    QApplication::setOrganizationName("Alamahant");
    QApplication::setApplicationVersion("1.0.0");

    QApplication a(argc, argv);

    // Use QLockFile for single instance check
    //QString tempDir = QDir::tempPath();
    QString tempDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);

    QLockFile lockFile(tempDir + "/Kerveros_2FA_App.lock");

    if (!lockFile.tryLock(100)) {
        QMessageBox::information(nullptr, "Already Running",
                                "Kerveros is already running.\nCheck your system tray or taskbar.");
        return 0;
    }

    TwoFAManager manager;
    manager.show();
    return a.exec();

}

