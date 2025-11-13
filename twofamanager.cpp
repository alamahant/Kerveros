#include "twofamanager.h"
#include"simple2fa.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QFrame>
#include <QDialog>
#include <QLineEdit>
#include <QDialogButtonBox>
#include <QMessageBox>
#include <QApplication>
#include <QClipboard>
#include <QSettings>
#include <QProgressBar>
#include <QGroupBox>
#include <QSplitter>
#include<QMenuBar>
#include<QMimeData>
#include<QByteArray>
#include<QProcess>
#include<QUrlQuery>
#include<QStatusBar>
#include<QFile>
#include<QFileDialog>
#include"helpmenudialog.h"
#include"donationdialog.h"

TwoFAManager::TwoFAManager(QWidget *parent)
    :QWidget(parent, Qt::Window)
    , m_accountList(nullptr)
    , m_currentCodeLabel(nullptr)
    , m_timeRemainingLabel(nullptr)
    , m_deleteBtn(nullptr)
    , m_copyBtn(nullptr)
    , m_updateTimer(nullptr)
    , m_timeProgressBar(nullptr)
    , m_securityManager(new SecurityManager(this))
    , m_minimizeToTray(settings.value("settings/minimizeToTray", false).toBool())
{

    QDir().mkpath(appDirPath);
    setAcceptDrops(true);  // ENABLE DRAG & DROP
    connect(m_securityManager, &SecurityManager::masterPasswordVerified,
                this, &TwoFAManager::onMasterPasswordVerified);

    connect(m_securityManager, &SecurityManager::factoryResetRequested, this, &TwoFAManager::factoryReset);
    setAttribute(Qt::WA_DeleteOnClose);

    setWindowTitle("Kerveros - 2FA Code Manager");
    setWindowIcon(QIcon(":/favicon_io/android-chrome-512x512.png"));
    resize(450, 350);

    setupUI();
    setupMenus();
    m_securityManager->checkPasswordOnStartup();

    loadData();
    // Force reset progress bar on startup
    m_timeProgressBar->setValue(Simple2FA::getTimeRemaining());
    // Start the update timer
    m_updateTimer = new QTimer(this);
    connect(m_updateTimer, &QTimer::timeout, this, &TwoFAManager::updateCurrentCode);
    m_updateTimer->start(1000); // Update every second

    updateCurrentCode();
    setupSystemTray();
}

TwoFAManager::~TwoFAManager()
{
    if (m_updateTimer) {
        m_updateTimer->stop();
    }
    saveData();
}

void TwoFAManager::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Create splitter for resizable sections
    QSplitter* splitter = new QSplitter(Qt::Vertical, this);

    // === ACCOUNTS SECTION ===
    QGroupBox* accountsGroup = new QGroupBox("2FA Accounts");
    QVBoxLayout* accountsLayout = new QVBoxLayout(accountsGroup);

    m_accountList = new QListWidget;
    m_accountList->setMinimumHeight(150);
    accountsLayout->addWidget(m_accountList);

    // Account buttons
    QHBoxLayout* accountButtonsLayout = new QHBoxLayout;
    addBtn = new QPushButton("Add Account", this);
    addBtn->setToolTip("Manually add  account from token");

    importButton = new QPushButton("Import",this);
    importButton->setToolTip("Import account from QR code screenshot");
    m_deleteBtn = new QPushButton("Delete", this);
    m_deleteBtn->setToolTip("Delete Account");
    m_deleteBtn->setEnabled(false);
    editButton = new QPushButton("Edit");
    editButton->setToolTip("Edit Account");
    editButton->setEnabled(false);
    accountButtonsLayout->addWidget(addBtn);
    accountButtonsLayout->addStretch();

    accountButtonsLayout->addWidget(importButton);
    accountButtonsLayout->addStretch();

    accountButtonsLayout->addWidget(m_deleteBtn);
    accountButtonsLayout->addStretch();

    accountButtonsLayout->addWidget(editButton);
    accountButtonsLayout->addStretch();

    //accountButtonsLayout->addStretch();
    accountsLayout->addLayout(accountButtonsLayout);

    splitter->addWidget(accountsGroup);

    // === CODE DISPLAY SECTION ===
    QGroupBox* codeGroup = new QGroupBox("Current Code");
    QVBoxLayout* codeLayout = new QVBoxLayout(codeGroup);

    // Code display frame
    QFrame* codeFrame = new QFrame;
    codeFrame->setFrameStyle(QFrame::StyledPanel);
    codeFrame->setStyleSheet(
        "QFrame { background-color: #f8f9fa; border-radius: 8px; padding: 10px; }"
        );
    QVBoxLayout* codeFrameLayout = new QVBoxLayout(codeFrame);

    m_currentCodeLabel = new QLabel("------");
    m_currentCodeLabel->setAlignment(Qt::AlignCenter);
    m_currentCodeLabel->setStyleSheet(
        "font-size: 32px; font-weight: bold; color: #2196F3; "
        "font-family: 'Courier New', monospace; letter-spacing: 4px;"
        );
    codeFrameLayout->addWidget(m_currentCodeLabel);

    // Time remaining
    m_timeRemainingLabel = new QLabel("Select an account");
    m_timeRemainingLabel->setAlignment(Qt::AlignCenter);
    m_timeRemainingLabel->setStyleSheet("font-size: 12px; color: #666;");
    codeFrameLayout->addWidget(m_timeRemainingLabel);

    // Progress bar for time remaining
    m_timeProgressBar = new QProgressBar;
    m_timeProgressBar->setRange(0, 30);
    m_timeProgressBar->setValue(30);
    m_timeProgressBar->setTextVisible(false);
    m_timeProgressBar->setStyleSheet(
        "QProgressBar { border: 1px solid #ddd; border-radius: 4px; background: #f0f0f0; }"
        "QProgressBar::chunk { background: qlineargradient(x1:0, y1:0, x2:1, y2:0, "
        "stop:0 #4CAF50, stop:0.7 #FFC107, stop:1 #F44336); border-radius: 3px; }"
        );
    codeFrameLayout->addWidget(m_timeProgressBar);

    codeLayout->addWidget(codeFrame);

    // Copy button
    m_copyBtn = new QPushButton("Copy Code to Clipboard");
    m_copyBtn->setEnabled(false);
    m_copyBtn->setStyleSheet(
        "QPushButton { font-weight: bold; padding: 8px; }"
        "QPushButton:enabled { background-color: #4CAF50; color: white; }"
        );
    codeLayout->addWidget(m_copyBtn);

    splitter->addWidget(codeGroup);
    splitter->setSizes({200, 150});

    mainLayout->addWidget(splitter);

    // === CONNECTIONS ===
    connect(addBtn, &QPushButton::clicked, this, &TwoFAManager::onAddAccount);
    connect(m_deleteBtn, &QPushButton::clicked, this, &TwoFAManager::onDeleteAccount);
    connect(editButton, &QPushButton::clicked, this, &TwoFAManager::onEditAccount);

    connect(m_copyBtn, &QPushButton::clicked, this, &TwoFAManager::onCopyCode);
    connect(m_accountList, &QListWidget::currentTextChanged, this, &TwoFAManager::onSelectionChanged);
    connect(importButton, &QPushButton::clicked, this, &TwoFAManager::importQRCode);
}

void TwoFAManager::onAddAccount()
{
    QDialog dialog(this);
    dialog.setWindowTitle("Add 2FA Account");
    dialog.resize(400, 180);

    QFormLayout* layout = new QFormLayout(&dialog);

    QLineEdit* nameEdit = new QLineEdit;
    nameEdit->setPlaceholderText("e.g., GitHub, Google, Discord");

    QLineEdit* secretEdit = new QLineEdit;
    secretEdit->setPlaceholderText("Paste the secret key from the website");
    secretEdit->setEchoMode(QLineEdit::Password);

    QLineEdit* issuerEdit = new QLineEdit;
    issuerEdit->setPlaceholderText("Company name (optional)");


    QLineEdit* groupEdit = new QLineEdit;
    groupEdit->setPlaceholderText("Group name (optional)");
    groupEdit->setToolTip("You can create groups like 'Work', 'Personal' etc");

    QLineEdit* commentsEdit = new QLineEdit;
    commentsEdit->setPlaceholderText("Comments (optional)");
    commentsEdit->setToolTip("Any comments you wish to include");

    layout->addRow("Account Name:", nameEdit);
    layout->addRow("Secret Key:", secretEdit);
    layout->addRow("Issuer:", issuerEdit);
    layout->addRow("Group", groupEdit);
    layout->addRow("Comments", commentsEdit);

    QLabel *notiflabel = new QLabel(this);
    QString notifText = QString("NOTE: Drag and Drop QR Code screenshots to easily add accounts!\n"
                                "NOTE:Screenshots should be placed in '%1'").arg(appDirPath);
    notiflabel->setText(notifText);
    layout->addRow(notiflabel);
    QDialogButtonBox* buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    layout->addWidget(buttons);

    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted) {
        QString name = nameEdit->text().trimmed();
        QString secret = secretEdit->text().trimmed().toUpper().remove(' ');
        QString issuer = issuerEdit->text().trimmed();
        QString group = groupEdit->text().trimmed();
        QString comments = commentsEdit->text().trimmed();

        if (name.isEmpty()) {
            QMessageBox::warning(this, "Invalid Input", "Please enter an account name.");
            return;
        }

        if (!Simple2FA::isValidSecret(secret)) {
            QMessageBox::warning(this, "Invalid Secret",
                                 "Please enter a valid secret key. It should contain only letters A-Z and numbers 2-7.");
            return;
        }

        if (m_entries.contains(name)) {
            QMessageBox::warning(this, "Duplicate Account",
                                 "An account with this name already exists.");
            return;
        }

        TwoFAEntry entry;
        entry.name = name;
        entry.secret = secret;
        entry.issuer = issuer;
        entry.group = group;
        entry.comments = comments;

        m_entries[name] = entry;
        m_accountList->addItem(name);
        m_accountList->setCurrentRow(m_accountList->count() - 1);

        saveData();
        updateCurrentCode();
    }

}

void TwoFAManager::onDeleteAccount()
{
    QString currentItem = m_accountList->currentItem() ?
                              m_accountList->currentItem()->text() : "";

    if (currentItem.isEmpty()) {
        return;
    }

    QMessageBox::StandardButton reply = QMessageBox::question(this,
                                                              "Delete Account",
                                                              QString("Are you sure you want to delete the 2FA account '%1'?").arg(currentItem),
                                                              QMessageBox::Yes | QMessageBox::No,
                                                              QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        m_entries.remove(currentItem);
        delete m_accountList->takeItem(m_accountList->currentRow());
        //saveData();
        // Remove from QSettings

              settings.beginGroup("TwoFA");
              settings.beginGroup(currentItem);
              settings.remove(""); // Remove this account's settings
              settings.endGroup();
              settings.endGroup();
        updateCurrentCode();
    }
}

void TwoFAManager::onCopyCode()
{
    QString code = m_currentCodeLabel->text();
    if (code != "------" && code != "000000") {
        QApplication::clipboard()->setText(code);

        // Visual feedback
        QString originalStyle = m_copyBtn->styleSheet();
        m_copyBtn->setText("Copied!");
        m_copyBtn->setStyleSheet(originalStyle + "background-color: #2196F3;");

        QTimer::singleShot(1500, [this, originalStyle]() {
            m_copyBtn->setText("Copy Code to Clipboard");
            m_copyBtn->setStyleSheet(originalStyle);
        });
    }
}

void TwoFAManager::onSelectionChanged()
{
    bool hasSelection = m_accountList->currentItem() != nullptr;

    if (hasSelection) {
            QString accountName = m_accountList->currentItem()->text();
            if (m_entries.contains(accountName)) {
                const TwoFAEntry& entry = m_entries[accountName];

                // Standard 12 asterisks for all secrets
                /*
                QString tooltip = QString("Account: %1\nSecret: ************\nIssuer: %2")
                    .arg(entry.name)
                    .arg(entry.issuer.isEmpty() ? "Not specified" : entry.issuer);
                */

                QString tooltip = QString("Account: %1\n"
                                         "Secret: ************\n"
                                         "Issuer: %2\n"
                                         "Group: %3\n"
                                         "Comments: %4")
                                  .arg(entry.name)
                                  .arg(entry.issuer.isEmpty() ? "Not specified" : entry.issuer)
                                  .arg(entry.group.isEmpty() ? "Not specified" : entry.group)
                                  .arg(entry.comments.isEmpty() ? "None" : entry.comments);

                m_accountList->currentItem()->setToolTip(tooltip);
            }
        } else {
            m_accountList->setToolTip("");
        }

    m_deleteBtn->setEnabled(hasSelection);
    m_copyBtn->setEnabled(hasSelection);
    editButton->setEnabled(hasSelection);
    updateCurrentCode();
}

void TwoFAManager::updateCurrentCode()
{
    QString currentItem = m_accountList->currentItem() ?
                              m_accountList->currentItem()->text() : "";

    if (currentItem.isEmpty() || !m_entries.contains(currentItem)) {
        m_currentCodeLabel->setText("------");
        m_timeRemainingLabel->setText("Select an account");
       // m_timeProgressBar->setValue(30);
        m_timeProgressBar->setValue(Simple2FA::getTimeRemaining());

        return;
    }

    const TwoFAEntry& entry = m_entries[currentItem];
    QString code = Simple2FA::generateCode(entry.secret);
    int timeRemaining = Simple2FA::getTimeRemaining();

    m_currentCodeLabel->setText(code);
    m_timeRemainingLabel->setText(QString("Refreshes in %1 seconds").arg(timeRemaining));
    m_timeProgressBar->setValue(timeRemaining);

    updateProgressBar();
}

void TwoFAManager::updateProgressBar()
{
    int timeRemaining = Simple2FA::getTimeRemaining();
    m_timeProgressBar->setValue(timeRemaining);

    // Change color based on time remaining
    QString color;
    if (timeRemaining > 20) {
        color = "#4CAF50"; // Green
    } else if (timeRemaining > 10) {
        color = "#FFC107"; // Yellow
    } else {
        color = "#F44336"; // Red
    }

    m_timeProgressBar->setStyleSheet(
        QString("QProgressBar { border: 1px solid #ddd; border-radius: 4px; background: #f0f0f0; }"
                "QProgressBar::chunk { background: %1; border-radius: 3px; }").arg(color)
        );
}

void TwoFAManager::loadData()
{

    settings.beginGroup("TwoFA");

    QStringList accounts = settings.childGroups();
    for (const QString& account : accounts) {
        settings.beginGroup(account);

        TwoFAEntry entry;
        entry.name = account;
        //entry.secret = settings.value("secret").toString();
        //encryption

        QString storedSecret = settings.value("secret").toString();

               // DECRYPT only if we have a master password
               if (!m_currentMasterPassword.isEmpty()) {
                   entry.secret = Simple2FA::decryptSecret(storedSecret, m_currentMasterPassword);
               } else {
                   entry.secret = storedSecret; // Use plain text if no password protection
               }


        entry.issuer = settings.value("issuer").toString();
        entry.group = settings.value("group").toString();
        entry.comments = settings.value("comments").toString();

        if (!entry.secret.isEmpty() && Simple2FA::isValidSecret(entry.secret)) {
            m_entries[account] = entry;
            m_accountList->addItem(account);
        }

        settings.endGroup();
    }

    settings.endGroup();
}


void TwoFAManager::saveData()
{

    settings.beginGroup("TwoFA");
    //settings.remove(""); // Clear existing data

    for (auto it = m_entries.begin(); it != m_entries.end(); ++it) {
        const QString& name = it.key();
        const TwoFAEntry& entry = it.value();

        settings.beginGroup(name);
        //encryption-decryption
        QString secretToSave;
               if (!m_currentMasterPassword.isEmpty()) {
                   secretToSave = Simple2FA::encryptSecret(entry.secret, m_currentMasterPassword);
               } else {
                   secretToSave = entry.secret; // Save plain text if no password protection
               }
        //
        settings.setValue("secret", secretToSave);
        settings.setValue("issuer", entry.issuer);
        settings.setValue("group", entry.group);
        settings.setValue("comments", entry.comments);

        settings.endGroup();
    }

    settings.endGroup();
}

void TwoFAManager::setupMenus()
{
    QMenuBar* menuBar = new QMenuBar(this);


    QMenu* fileMenu = menuBar->addMenu("File");
    QAction* exportAction = fileMenu->addAction("Export Accounts");
    QAction* importAction = fileMenu->addAction("Import Accounts");

    connect(exportAction, &QAction::triggered, this, &TwoFAManager::exportAccounts);
    connect(importAction, &QAction::triggered, this, &TwoFAManager::importAccounts);
    fileMenu->addSeparator();
    QAction* importQRCodeAction = fileMenu->addAction("Import QR Code");
    connect(importQRCodeAction, &QAction::triggered, this, &TwoFAManager::importQRCode);
    fileMenu->addSeparator();
    QAction* factoryResetAction = fileMenu->addAction("Factory Reset");
    connect(factoryResetAction, &QAction::triggered, this, &TwoFAManager::factoryReset);
    fileMenu->addSeparator();
    QAction* quitAction = fileMenu->addAction("Quit");
    connect(quitAction, &QAction::triggered, [this] {
        QApplication::quit();
    });

    menuBar->addSeparator();

    m_securityManager->setupSecurityMenu(menuBar);
    menuBar->addSeparator();
    QMenu* settingsMenu = menuBar->addMenu("Settings");  // ← NEW SETTINGS MENU



        // Settings menu items
        m_minimizeToTrayAction = new QAction("Minimize to Tray", this);
        m_minimizeToTrayAction->setCheckable(true);

        m_minimizeToTrayAction->setChecked(settings.value("settings/minimizeToTray", false).toBool());
        connect(m_minimizeToTrayAction, &QAction::toggled, this, &TwoFAManager::onToggleMinimizeToTray);

        settingsMenu->addAction(m_minimizeToTrayAction);

        //helpmenu
        QMenu* helpMenu = menuBar->addMenu(tr("Help"));

        QAction* aboutAction = new QAction(tr("About"), this);
        helpMenu->addAction(aboutAction);

        // 2. Connect the About action to show the HelpMenuDialog
        connect(aboutAction, &QAction::triggered, this, [this]() {
            HelpMenuDialog dialog(HelpType::About, this);
            dialog.exec();
        });

        QAction* featuresAction = new QAction(tr("Features"), this);
        helpMenu->addAction(featuresAction);

        // 2. Connect the About action to show the HelpMenuDialog
        connect(featuresAction, &QAction::triggered, this, [this]() {
            HelpMenuDialog dialog(HelpType::Features, this);
            dialog.exec();
        });

        QAction* instructionsAction = new QAction(tr("Instructions"), this);
        helpMenu->addAction(instructionsAction);

        // 2. Connect the About action to show the HelpMenuDialog
        connect(instructionsAction, &QAction::triggered, this, [this]() {
            HelpMenuDialog dialog(HelpType::Instructions, this);
            dialog.exec();
        });


        QAction* whatsnewAction = new QAction(tr("Whats New"), this);
        helpMenu->addAction(whatsnewAction);

        // 2. Connect the About action to show the HelpMenuDialog
        connect(whatsnewAction, &QAction::triggered, this, [this]() {
            HelpMenuDialog dialog(HelpType::WhatsNew, this);
            dialog.exec();
        });

        QAction* supportusAction = new QAction(tr("Support Us"), this);
        helpMenu->addAction(supportusAction);

        // 2. Connect the supportus  action to show the HelpMenuDialog
        connect(supportusAction, &QAction::triggered, this, [this]() {
            DonationDialog dialog(this);
            dialog.exec();
        });

        // ADD SEARCH FIELD FIRST
            QLineEdit* searchBox = new QLineEdit(this);
            searchBox->setToolTip("Search options:\n"
                                 "• [text] - Search account names\n"
                                 "• #g [text] - Search groups\n"
                                 "• #i [text] - Search issuers\n"
                                 "• #c [text] - Search comments\n"
                                 "All searches are case-insensitive");
            searchBox->setPlaceholderText("Search accounts...");
            searchBox->setClearButtonEnabled(true);
            searchBox->setMaximumWidth(200);
            menuBar->setCornerWidget(searchBox, Qt::TopRightCorner);

            connect(searchBox, &QLineEdit::textChanged, this, &TwoFAManager::onSearchTextChanged);

    // Add menu bar to your layout
           layout()->setMenuBar(menuBar);
}

void TwoFAManager::onMasterPasswordVerified(const QString& masterPassword)
{
    // Store the master password for this session (empty if no password required)
    m_currentMasterPassword = masterPassword;
    // Now load data with decryption capability
   // loadData();
    saveData();
    // Update UI
    updateCurrentCode();
}


void TwoFAManager::onEditAccount()
{
    QString currentName = m_accountList->currentItem() ? m_accountList->currentItem()->text() : "";
    if (currentName.isEmpty()) return;

    // GET COPY, NOT REFERENCE - to avoid crash when removing from map
    TwoFAEntry oldEntry = m_entries[currentName];

    QDialog dialog(this);
    dialog.setWindowTitle("Edit 2FA Account");
    dialog.resize(400, 180);

    QFormLayout* layout = new QFormLayout(&dialog);

    QLineEdit* nameEdit = new QLineEdit;
    nameEdit->setText(oldEntry.name);

    QLineEdit* secretEdit = new QLineEdit;
    secretEdit->setText(oldEntry.secret);
    secretEdit->setEchoMode(QLineEdit::Password);

    QLineEdit* issuerEdit = new QLineEdit;
    issuerEdit->setText(oldEntry.issuer);

    //
    QLineEdit* groupEdit = new QLineEdit;
    groupEdit->setText(oldEntry.group);

    QLineEdit* commentsEdit = new QLineEdit;
    commentsEdit->setText(oldEntry.comments);
    //
    layout->addRow("Account Name:", nameEdit);
    layout->addRow("Secret Key:", secretEdit);
    layout->addRow("Issuer:", issuerEdit);
    layout->addRow("Group", groupEdit);
    layout->addRow("Comments", commentsEdit);

    QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    layout->addWidget(buttons);

    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted) {
        QString newName = nameEdit->text().trimmed();
        QString secret = secretEdit->text().trimmed().toUpper().remove(' ');
        QString issuer = issuerEdit->text().trimmed();
        QString group = groupEdit->text().trimmed();
        QString comments = commentsEdit->text().trimmed();

        // Validate
        if (newName.isEmpty()) {
            QMessageBox::warning(this, "Invalid Input", "Please enter an account name.");
            return;
        }

        if (!Simple2FA::isValidSecret(secret)) {
            QMessageBox::warning(this, "Invalid Secret", "Please enter a valid secret key.");
            return;
        }

        // Remove old entry if name changed
        if (newName != currentName) {
            if (m_entries.contains(newName)) {
                QMessageBox::warning(this, "Duplicate Account", "An account with this name already exists.");
                return;
            }
            m_entries.remove(currentName);

            // Remove from QSettings

            settings.beginGroup("TwoFA");
            settings.beginGroup(currentName);
            settings.remove("");
            settings.endGroup();
            settings.endGroup();
        }

        // Create NEW entry (don't reuse old reference)
        TwoFAEntry newEntry;
        newEntry.name = newName;
        newEntry.secret = secret;
        newEntry.issuer = issuer;
        newEntry.group = group;
        newEntry.comments = comments;

        m_entries[newName] = newEntry;

        // Update list
        if (newName != currentName) {
            m_accountList->currentItem()->setText(newName);
        }

        saveData();
        updateCurrentCode();
        //update tooltip immediately
        auto items = m_accountList->findItems(newEntry.name, Qt::MatchExactly);
        if (!items.isEmpty()) {
            items[0]->setToolTip(QString("Account: %1\nSecret: ************\nIssuer: %2\nGroup: %3\nComments: %4")
                                .arg(newEntry.name)
                                .arg(newEntry.issuer.isEmpty() ? "Not specified" : newEntry.issuer)
                                .arg(newEntry.group.isEmpty() ? "Not specified" : newEntry.group)
                                .arg(newEntry.comments.isEmpty() ? "None" : newEntry.comments));
        }
        //
    }
}

// drag and drop
void TwoFAManager::dragEnterEvent(QDragEnterEvent* event)
{
    if (event->mimeData()->hasUrls()) {
        QList<QUrl> urls = event->mimeData()->urls();
        if (!urls.isEmpty()) {
            QString filePath = urls.first().toLocalFile();
            if (filePath.endsWith(".png", Qt::CaseInsensitive) ||
                filePath.endsWith(".jpg", Qt::CaseInsensitive) ||
                filePath.endsWith(".jpeg", Qt::CaseInsensitive)) {
                event->acceptProposedAction();
            }
        }
    }
}

void TwoFAManager::dropEvent(QDropEvent* event)
{
    QList<QUrl> urls = event->mimeData()->urls();
    if (!urls.isEmpty()) {
        QString filePath = urls.first().toLocalFile();
        processDroppedImage(filePath);
    }
    event->acceptProposedAction();
}

void TwoFAManager::processDroppedImage(const QString& filePath)
{
    QString secret, issuer;
    if (extractSecretFromQRCode(filePath, secret, issuer)) {
        // Auto-fill the add account dialog
        showPrefilledAddDialog(secret, issuer);
    } else {
        QMessageBox::warning(this, "QR Code Error", "Could not read QR code from image.");
    }
}

bool TwoFAManager::extractSecretFromQRCode(const QString& imagePath, QString& outSecret, QString& outIssuer)
{
    // Use zbarimg to decode QR code
    QProcess process;
    process.start("zbarimg", {"--quiet", "--raw", imagePath});

    if (!process.waitForFinished(5000)) {
        return false;
    }

    QByteArray output = process.readAllStandardOutput();
    QString qrData = QString::fromUtf8(output).trimmed();

    if (qrData.isEmpty()) {
        return false;
    }

    // Parse otpauth URL: otpauth://totp/Issuer:Account?secret=ABC123&issuer=Issuer
    QUrl url(qrData);
    if (url.scheme() == "otpauth" && url.host() == "totp") {
        QUrlQuery query(url);
        outSecret = query.queryItemValue("secret");
        outIssuer = query.queryItemValue("issuer");

        // If no issuer in query, extract from path: totp/Issuer:Account
        if (outIssuer.isEmpty()) {
            QString path = url.path();
            if (path.startsWith("/")) path = path.mid(1);
            int colonPos = path.indexOf(':');
            if (colonPos > 0) {
                outIssuer = path.left(colonPos);
            }
        }

        return !outSecret.isEmpty();
    }

    return false;
}

void TwoFAManager::showPrefilledAddDialog(const QString& secret, const QString& issuer)
{
    QDialog dialog(this);
    dialog.setWindowTitle("Add 2FA Account from QR Code");
    dialog.resize(400, 180);

    QFormLayout* layout = new QFormLayout(&dialog);

    QLineEdit* nameEdit = new QLineEdit;
    nameEdit->setPlaceholderText("e.g., GitHub, Google, Discord");
    if (!issuer.isEmpty()) {
        nameEdit->setText(issuer);
    }

    QLineEdit* secretEdit = new QLineEdit;
    secretEdit->setText(secret);
    secretEdit->setEchoMode(QLineEdit::Password);

    QLineEdit* issuerEdit = new QLineEdit;
    issuerEdit->setText(issuer);
    issuerEdit->setPlaceholderText("Company name (optional)");

    //
    QLineEdit* groupEdit = new QLineEdit;
    groupEdit->setPlaceholderText("Group name (optional)");
    groupEdit->setToolTip("Organize your accounts in groups like 'Work', 'Personal', etc");

    QLineEdit* commentsEdit = new QLineEdit;
    commentsEdit->setPlaceholderText("Comments (optional)");
    commentsEdit->setToolTip("Add any comments you wish to include");

    //

    layout->addRow("Account Name:", nameEdit);
    layout->addRow("Secret Key:", secretEdit);
    layout->addRow("Issuer:", issuerEdit);
    layout->addRow("Group:", groupEdit);
    layout->addRow("Comments:", commentsEdit);

    QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    layout->addWidget(buttons);

    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted) {
        // Use existing onAddAccount logic with pre-filled values
        QString name = nameEdit->text().trimmed();
        QString finalSecret = secretEdit->text().trimmed().toUpper().remove(' ');
        QString finalIssuer = issuerEdit->text().trimmed();
        QString groupName = groupEdit->text().trimmed();
        QString comments = commentsEdit->text().trimmed();

        // ... validation and saving logic from onAddAccount
        if (name.isEmpty()) {
            QMessageBox::warning(this, "Invalid Input", "Please enter an account name.");
            return;
        }

        if (!Simple2FA::isValidSecret(secret)) {
            QMessageBox::warning(this, "Invalid Secret",
                                 "Please enter a valid secret key. It should contain only letters A-Z and numbers 2-7.");
            return;
        }

        if (m_entries.contains(name)) {
            QMessageBox::warning(this, "Duplicate Account",
                                 "An account with this name already exists.");
            return;
        }

        TwoFAEntry entry;
        entry.name = name;
        entry.secret = secret;
        entry.issuer = issuer;
        entry.group = groupName;
        entry.comments = comments;

        m_entries[name] = entry;
        m_accountList->addItem(name);
        m_accountList->setCurrentRow(m_accountList->count() - 1);

        saveData();
        updateCurrentCode();
    }

}

void TwoFAManager::onToggleMinimizeToTray(bool enabled)
{
    m_minimizeToTray = enabled;
    settings.setValue("settings/minimizeToTray", enabled);
}

void TwoFAManager::setupSystemTray()
{
    if (QSystemTrayIcon::isSystemTrayAvailable()) {
        m_trayIcon = new QSystemTrayIcon(this);
        m_trayIcon->setIcon(QIcon(":/favicon_io/android-chrome-512x512.png"));

        m_trayIcon->setToolTip("Kerveros - 2FA Manager");

        // Create tray menu
        m_trayMenu = new QMenu(this);
        QAction* showAction = m_trayMenu->addAction("Show");
        QAction* exitAction = m_trayMenu->addAction("Quit");

        connect(showAction, &QAction::triggered, this, &TwoFAManager::onShowFromTray);
        connect(exitAction, &QAction::triggered, this, &TwoFAManager::onExitFromTray);

        m_trayIcon->setContextMenu(m_trayMenu);
        connect(m_trayIcon, &QSystemTrayIcon::activated, this, &TwoFAManager::onTrayIconActivated);

        m_trayIcon->show();
    }
}

void TwoFAManager::onTrayIconActivated(QSystemTrayIcon::ActivationReason reason)
{
    if (reason == QSystemTrayIcon::Trigger) {
        onShowFromTray();
    }
}

void TwoFAManager::onShowFromTray()
{
    show();
    raise();
    activateWindow();
}

void TwoFAManager::onExitFromTray()
{
    // Proper cleanup and exit
    if (m_updateTimer) m_updateTimer->stop();
    saveData();
    qApp->quit();
}


void TwoFAManager::closeEvent(QCloseEvent* event)
{
    if (m_minimizeToTray &&  //USE THE MEMBER VARIABLE, NOT SETTING
        QSystemTrayIcon::isSystemTrayAvailable() &&
        m_trayIcon && m_trayIcon->isVisible()) {

        hide();  // Minimize to tray instead of closing
        event->ignore();  // Don't actually close
    } else {
        // Normal close - save and exit
        if (m_updateTimer) m_updateTimer->stop();
        saveData();
        event->accept();
        qApp->quit();
    }
}



void TwoFAManager::factoryReset() {

    // Use QSettings to get the EXACT file path

    QString configFilePath = settings.fileName(); // This gives the full path with .conf

    QMessageBox::StandardButton reply = QMessageBox::question(this,
        "Factory Reset",
        "This will delete ALL your 2FA accounts and settings.\nThis action cannot be undone!\n\nAre you sure?",
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        QFile configFile(configFilePath);
        if (configFile.remove()) {
            // Clear in-memory data
            m_entries.clear();
            m_accountList->clear();
            m_currentMasterPassword.clear();

            // Update UI
            updateCurrentCode();

            QMessageBox::information(this, "Factory Reset Complete",
                QString("All settings and accounts have been reset.\nFile deleted: %1").arg(configFilePath));

            // Close the app
            //close();
            QApplication::quit();
        } else {
            QMessageBox::warning(this, "Reset Failed",
                QString("Could not delete settings file: %1\nPath: %2").arg(configFile.errorString()).arg(configFilePath));
        }
    }
}


void TwoFAManager::exportAccounts()
{
    // Get current config file path
    QString sourcePath = settings.fileName();

    // Create default export path: Documents/Kerveros_backup.conf
    QString exportPath = appDirPath + "/" + QApplication::applicationName() + ".conf.bak";

    // Save current data first to ensure everything is written
    saveData();
    settings.sync();

    // Copy the file
    if (QFile::copy(sourcePath, exportPath)) {
        QMessageBox::information(this, "Export Successful",
            QString("All accounts and settings exported successfully!\n\nFile: %1").arg(exportPath));
    } else {
        QMessageBox::warning(this, "Export Failed",
            "Could not export accounts. The file may be in use or you lack permissions.");
    }
}

void TwoFAManager::importAccounts()
{
    QString importPath = QFileDialog::getOpenFileName(this,
        "Import Accounts",
        appDirPath,
        "Backup Files (*.conf *.bak);;Config Files (*.conf);;All Files (*)");
    if (importPath.isEmpty()) {
        return;
    }

    QMessageBox::StandardButton reply = QMessageBox::question(this,
        "Import Accounts",
        "This will replace ALL your current accounts and settings.\nContinue?",
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);

    if (reply != QMessageBox::Yes) {
        return;
    }

    QString currentPath = settings.fileName();
    QString backupPath = currentPath + ".bak";

    // Create backup
    QFile::copy(currentPath, backupPath);

    // Replace current file with imported file
    QFile::remove(currentPath);
    if (QFile::copy(importPath, currentPath)) {
        // Reload data
        m_entries.clear();
        m_accountList->clear();
        loadData();
        updateCurrentCode();

        // Remove backup
        QFile::remove(backupPath);

        QMessageBox::information(this, "Import Successful", "All accounts imported successfully!");
    } else {
        // Restore backup
        QFile::copy(backupPath, currentPath);
        QMessageBox::warning(this, "Import Failed", "Could not import accounts.");
    }
}

void TwoFAManager::importQRCode()
{
    QString imagePath = QFileDialog::getOpenFileName(this,
        "Import QR Code",
        appDirPath,
        "Image Files (*.png *.jpg *.jpeg);;All Files (*)");

    if (!imagePath.isEmpty()) {
        processDroppedImage(imagePath);  // ← REUSE YOUR EXISTING DRAG/DROP LOGIC!
    }
}
/*
void TwoFAManager::onSearchTextChanged(const QString& searchText)
{
    // Clear current selection
    m_accountList->clearSelection();

    if (searchText.isEmpty()) {
        // Show all items when search is empty
        for (int i = 0; i < m_accountList->count(); ++i) {
            m_accountList->item(i)->setHidden(false);
        }
        return;
    }

    // Filter items based on search
    for (int i = 0; i < m_accountList->count(); ++i) {
        QListWidgetItem* item = m_accountList->item(i);
        QString accountName = item->text();

        // Check if account name contains search text (case-insensitive)
        bool matches = accountName.contains(searchText, Qt::CaseInsensitive);
        item->setHidden(!matches);
    }
}
*/

void TwoFAManager::onSearchTextChanged(const QString& searchText)
{
    m_accountList->clearSelection();

    if (searchText.isEmpty()) {
        // Show all items
        for (int i = 0; i < m_accountList->count(); ++i) {
            m_accountList->item(i)->setHidden(false);
        }
        return;
    }

    // Check for special search prefixes
    bool isGroupSearch = searchText.startsWith("#g ");
    bool isIssuerSearch = searchText.startsWith("#i ");
    bool isCommentsSearch = searchText.startsWith("#c ");

    QString searchTerm;
    if (isGroupSearch || isIssuerSearch || isCommentsSearch) {
        searchTerm = searchText.mid(3).trimmed();
    } else {
        searchTerm = searchText;
    }

    for (int i = 0; i < m_accountList->count(); ++i) {
        QListWidgetItem* item = m_accountList->item(i);
        QString itemText = item->text();
        bool matches = false;

        if (isGroupSearch) {
            // Search in group field
            if (m_entries.contains(itemText)) {
                QString group = m_entries[itemText].group;
                matches = group.contains(searchTerm, Qt::CaseInsensitive);
            }
        } else if (isIssuerSearch) {
            // Search in issuer field
            if (m_entries.contains(itemText)) {
                QString issuer = m_entries[itemText].issuer;
                matches = issuer.contains(searchTerm, Qt::CaseInsensitive);
            }
        } else if (isCommentsSearch) {
            // Search in comments field
            if (m_entries.contains(itemText)) {
                QString comments = m_entries[itemText].comments;
                matches = comments.contains(searchTerm, Qt::CaseInsensitive);
            }
        } else {
            // Search in account name (default)
            matches = itemText.contains(searchTerm, Qt::CaseInsensitive);
        }

        item->setHidden(!matches);
    }
}
