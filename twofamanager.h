#ifndef TWOFAMANAGER_H
#define TWOFAMANAGER_H

#include <QWidget>

#include <QWidget>
#include <QListWidget>
#include <QLabel>
#include <QTimer>
#include <QPushButton>
#include <QProgressBar>
#include <QMap>
#include"securitymanager.h"
#include<QDragEnterEvent>
#include<QDropEvent>
#include<QSettings>
#include<QSystemTrayIcon>
#include<QStandardPaths>
#include<QDir>
#include<QApplication>

struct TwoFAEntry {
    QString name;
    QString secret;
    QString issuer;
};

class TwoFAManager : public QWidget
{
    Q_OBJECT

public:
    explicit TwoFAManager(QWidget *parent = nullptr);
    ~TwoFAManager();

private slots:
    void onAddAccount();
    void onDeleteAccount();
    void onCopyCode();
    void onSelectionChanged();
    void updateCurrentCode();
    void onMasterPasswordVerified(const QString& masterPassword);
    void onEditAccount();
    void factoryReset();
    void exportAccounts();
    void importAccounts();

private:
    void setupUI();
    void loadData();
    void saveData();
    void updateProgressBar();
    QSettings settings;
    QListWidget* m_accountList;
    QLabel* m_currentCodeLabel;
    QLabel* m_timeRemainingLabel;
    QPushButton* m_deleteBtn;
    QPushButton* m_copyBtn;
    QTimer* m_updateTimer;
    QProgressBar* m_timeProgressBar;  // ← Add this member too
    QPushButton* addBtn;
    QPushButton *editButton;
    QPushButton *importButton;

    QMap<QString, TwoFAEntry> m_entries;
    SecurityManager *m_securityManager = nullptr;
    void setupMenus();
    QString m_currentMasterPassword = ""; // Store during session

protected:
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event) override;
    void closeEvent(QCloseEvent* event) override;

private:
    void processDroppedImage(const QString& filePath);
    bool extractSecretFromQRCode(const QString& imagePath, QString& outSecret, QString& outIssuer);
private slots:
    void showPrefilledAddDialog(const QString& secret, const QString& issuer);
private:
    QAction* m_minimizeToTrayAction;
    bool m_minimizeToTray;

public:
    bool shouldMinimizeToTray() const { return m_minimizeToTray; }

private slots:
    void onToggleMinimizeToTray(bool enabled);

private:
    QSystemTrayIcon* m_trayIcon;
    QMenu* m_trayMenu;
    void setupSystemTray();
    QString appDirPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + QDir::separator()
            + QApplication::applicationName();
private slots:
        void onTrayIconActivated(QSystemTrayIcon::ActivationReason reason);
        void onShowFromTray();
        void onExitFromTray();
        void importQRCode();

};


#endif // TWOFAMANAGER_H
