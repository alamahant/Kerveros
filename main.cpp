#include "twofamanager.h"
#include<QObject>
#include <QApplication>
#include<QLocalServer>
#include<QLocalSocket>
#include<QMessageBox>

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
