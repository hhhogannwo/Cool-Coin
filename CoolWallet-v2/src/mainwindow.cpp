#include <QMap>
#include <QUrl>
#include <QDesktopServices>
#include <QSpinBox>
#include <QListWidget>
#include <QInputDialog>
#include <QTextBrowser>
#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QRegularExpression>
#include <QSet>
#include <QTimer>
#include <QThread>
#include <QTextCursor>
#include <QProgressBar>
#include <QTextEdit>
#include <QHBoxLayout>
#include <QCoreApplication>
#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLocale>
#include <QProcess>
#include <QTimer>
#include <QFormLayout>
#include <QDoubleSpinBox>
#include <QVBoxLayout>
#include <QPushButton>
#include <QMessageBox>
#include <QLineEdit>
#include <QDialog>
#include <QClipboard>
#include <QApplication>
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QRegularExpression>


namespace {

const QString CLI_PATH =
    "bin/coolcoin-cli";

const QString DATA_DIR =
    "-datadir=data";

const QString CONF_PATH =
    "-conf=data/coolcoin.conf";

QString runCoolCoinCli(const QStringList& command, bool useWallet = false)
{
    QProcess process;
    QStringList arguments;

    arguments << DATA_DIR;
    arguments << CONF_PATH;

    if (useWallet) {
        arguments << "-rpcwallet=coolwallet";
    }

    arguments << command;

    process.start(CLI_PATH, arguments);

    if (!process.waitForStarted(3000)) {
        return {};
    }

    if (!process.waitForFinished(15000)) {
        process.kill();
        process.waitForFinished();
        return {};
    }

    if (process.exitStatus() != QProcess::NormalExit ||
        process.exitCode() != 0) {
        return {};
    }

    return QString::fromUtf8(
        process.readAllStandardOutput()
    ).trimmed();
}

void setLabelText(QWidget* parent,
                  const QString& objectName,
                  const QString& text)
{
    QLabel* label = parent->findChild<QLabel*>(objectName);

    if (label) {
        label->setText(text);
    }
}

QString formatCoinAmount(double amount)
{
    return QString::number(amount, 'f', 8) + " COOL";
}

QString transactionHtml(const QJsonArray& transactions)
{
    if (transactions.isEmpty()) {
        return "<span style='color:#aaaaaa;'>No transactions found.</span>";
    }

    QString html;

    // Display newest transaction first.
    for (int i = transactions.size() - 1; i >= 0; --i) {
        const QJsonObject tx = transactions.at(i).toObject();

        const QString category = tx.value("category").toString();
        const double amount = tx.value("amount").toDouble();
        const qint64 timestamp =
            static_cast<qint64>(tx.value("time").toDouble());
        const int confirmations =
            tx.value("confirmations").toInt();

        QString icon;
        QString description;
        QString amountColor;

        if (category == "generate" ||
            category == "immature" ||
            category == "orphan") {
            icon = "⛏";
            description = "Mined reward";
            amountColor = "#00ff66";
        } else if (category == "send") {
            icon = "↑";
            description = "Sent";
            amountColor = "#ff5555";
        } else {
            icon = "↓";
            description = "Received";
            amountColor = "#00ff66";
        }

        const QString date =
            QDateTime::fromSecsSinceEpoch(timestamp)
                .toString("MM/dd/yyyy hh:mm");

        const QString amountText =
            QString("%1%2 COOL")
                .arg(amount >= 0 ? "+" : "")
                .arg(QString::number(amount, 'f', 8));

        html += QString(
            "<div style='margin-bottom:14px;'>"
            "<span style='font-size:24px;color:#00ff66;'>%1</span>"
            "&nbsp;&nbsp;"
            "<span style='color:white;font-weight:bold;'>%2</span>"
            "<br>"
            "<span style='color:#aaaaaa;'>%3</span>"
            "&nbsp;&nbsp;"
            "<span style='color:%4;font-weight:bold;'>%5</span>"
            "<br>"
            "<span style='color:#777777;'>Confirmations: %6</span>"
            "</div>"
        )
        .arg(icon)
        .arg(description)
        .arg(date)
        .arg(amountColor)
        .arg(amountText)
        .arg(confirmations);
    }

    return html;
}

} // namespace

static void ensureFallbackFee()
{
    const QString dataDir = "data";
    const QString configPath = dataDir + "/coolcoin.conf";
    const QString fallbackLine = "fallbackfee=0.00001000";

    QDir().mkpath(dataDir);

    QFile file(configPath);

    QString contents;

    if (file.exists()) {
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            return;
        }

        QTextStream input(&file);
        contents = input.readAll();
        file.close();

        if (contents.contains(QRegularExpression(
                R"((^|\n)\s*fallbackfee\s*=)",
                QRegularExpression::MultilineOption))) {
            return;
        }
    }

    if (!file.open(QIODevice::Append | QIODevice::Text)) {
        return;
    }

    QTextStream output(&file);

    if (!contents.isEmpty() && !contents.endsWith('\n')) {
        output << '\n';
    }

    output << fallbackLine << '\n';
    file.close();
}

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent),
      ui(new Ui::MainWindow)
{
    ensureFallbackFee();

    ui->setupUi(this);

    // COOLWALLET_LIVE_LOCAL_HASHRATE_BEGIN
    //
    // Read live mining speed from CKPool first.
    // Fall back to adding the newest rate from every minerd thread.

    auto *coolLocalHashrateTimer = new QTimer(this);
    coolLocalHashrateTimer->setInterval(1000);

    connect(
        coolLocalHashrateTimer,
        &QTimer::timeout,
        this,
        [this]()
        {
            const QString appDir =
                QCoreApplication::applicationDirPath();

            const QString parentDir =
                QFileInfo(appDir).absoluteDir().absolutePath();

            const QString grandParentDir =
                QFileInfo(parentDir).absoluteDir().absolutePath();

            QStringList searchRoots = {
                appDir,
                appDir + "/logs",
                appDir + "/mining-runtime",
                parentDir,
                parentDir + "/logs",
                parentDir + "/mining-runtime",
                grandParentDir + "/logs",
                grandParentDir + "/mining-runtime"
            };

            QString newestCkpoolLog;
            QString newestMinerdLog;

            qint64 newestCkpoolModified = 0;
            qint64 newestMinerdModified = 0;

            for (const QString &root : searchRoots)
            {
                if (!QFileInfo::exists(root))
                    continue;

                QDirIterator iterator(
                    root,
                    QStringList() << "*.log",
                    QDir::Files,
                    QDirIterator::Subdirectories
                );

                while (iterator.hasNext())
                {
                    const QString path = iterator.next();
                    const QFileInfo info(path);
                    const QString name =
                        info.fileName().toLower();

                    const qint64 modified =
                        info.lastModified().toMSecsSinceEpoch();

                    if (
                        name.contains("ckpool") &&
                        modified > newestCkpoolModified
                    )
                    {
                        newestCkpoolModified = modified;
                        newestCkpoolLog = path;
                    }

                    if (
                        name.contains("minerd") &&
                        modified > newestMinerdModified
                    )
                    {
                        newestMinerdModified = modified;
                        newestMinerdLog = path;
                    }
                }
            }

            auto readTail =
                [](const QString &path, qint64 bytes) -> QString
            {
                if (path.isEmpty())
                    return {};

                QFile file(path);

                if (!file.open(QIODevice::ReadOnly))
                    return {};

                if (file.size() > bytes)
                    file.seek(file.size() - bytes);

                return QString::fromUtf8(file.readAll());
            };

            double totalHashesPerSecond = 0.0;
            QString displayText = "0 H/s";

            /*
             * CKPool example:
             *
             * 67.5MH/s  0.0 SPS  1 users  1 workers  5 shares
             */
            const QString ckpoolText =
                readTail(newestCkpoolLog, 1024 * 1024);

            QRegularExpression ckpoolRegex(
                R"(([0-9]+(?:\.[0-9]+)?)\s*([kKmMgGtT]?H/s)\s+[0-9.]+\s+SPS\s+\d+\s+users?\s+\d+\s+workers?)"
            );

            auto ckpoolMatches =
                ckpoolRegex.globalMatch(ckpoolText);

            double latestPoolValue = 0.0;
            QString latestPoolUnit;

            while (ckpoolMatches.hasNext())
            {
                const QRegularExpressionMatch match =
                    ckpoolMatches.next();

                latestPoolValue =
                    match.captured(1).toDouble();

                latestPoolUnit =
                    match.captured(2).toUpper();
            }

            if (latestPoolValue > 0.0)
            {
                double multiplier = 1.0;

                if (latestPoolUnit == "KH/S")
                    multiplier = 1000.0;
                else if (latestPoolUnit == "MH/S")
                    multiplier = 1000000.0;
                else if (latestPoolUnit == "GH/S")
                    multiplier = 1000000000.0;
                else if (latestPoolUnit == "TH/S")
                    multiplier = 1000000000000.0;

                totalHashesPerSecond =
                    latestPoolValue * multiplier;

                displayText =
                    QString::number(latestPoolValue, 'f', 2)
                    + " "
                    + latestPoolUnit;
            }

            /*
             * minerd fallback:
             *
             * thread 20: 134217696 hashes, 2662 khash/s
             */
            if (totalHashesPerSecond <= 0.0)
            {
                const QString minerdText =
                    readTail(newestMinerdLog, 2 * 1024 * 1024);

                QRegularExpression minerdRegex(
                    R"(thread\s+(\d+):\s+\d+\s+hashes,\s+([0-9]+(?:\.[0-9]+)?)\s+khash/s)",
                    QRegularExpression::CaseInsensitiveOption
                );

                QMap<int, double> threadRates;

                auto minerdMatches =
                    minerdRegex.globalMatch(minerdText);

                while (minerdMatches.hasNext())
                {
                    const QRegularExpressionMatch match =
                        minerdMatches.next();

                    const int threadNumber =
                        match.captured(1).toInt();

                    const double khash =
                        match.captured(2).toDouble();

                    threadRates[threadNumber] = khash;
                }

                double totalKhash = 0.0;

                for (
                    auto it = threadRates.constBegin();
                    it != threadRates.constEnd();
                    ++it
                )
                {
                    totalKhash += it.value();
                }

                if (totalKhash > 0.0)
                {
                    totalHashesPerSecond =
                        totalKhash * 1000.0;

                    if (totalKhash >= 1000000.0)
                    {
                        displayText =
                            QString::number(
                                totalKhash / 1000000.0,
                                'f',
                                3
                            )
                            + " GH/s";
                    }
                    else if (totalKhash >= 1000.0)
                    {
                        displayText =
                            QString::number(
                                totalKhash / 1000.0,
                                'f',
                                3
                            )
                            + " MH/s";
                    }
                    else
                    {
                        displayText =
                            QString::number(
                                totalKhash,
                                'f',
                                3
                            )
                            + " KH/s";
                    }
                }
            }

            ui->localHashrateValue->setText(displayText);
        }
    );

    coolLocalHashrateTimer->start();


    // COOLWALLET_LIVE_LOCAL_HASHRATE_END


    // LIVE_DASHBOARD_PATCH_BEGIN
    //
    // Updates the Mining Status and Connected Peers cards directly from:
    //   1. CKPool/minerd logs
    //   2. Established TCP connections on Cool Coin P2P port 6464
    //
    // This intentionally does not depend on RPC authentication.

    auto *liveDashboardTimer = new QTimer(this);
    liveDashboardTimer->setInterval(1000);

    connect(liveDashboardTimer, &QTimer::timeout, this, [this]() {
        const QString appDir = QCoreApplication::applicationDirPath();
        const QString parentDir =
            QFileInfo(appDir).absoluteDir().absolutePath();

        /*
         * Locate current mining logs.
         *
         * Running from the source build normally uses:
         *   build/mining-runtime/...
         *
         * Running from the portable release normally uses:
         *   logs/...
         */
        QStringList searchRoots = {
            appDir + "/logs",
            appDir + "/mining-runtime",
            parentDir + "/logs",
            parentDir + "/mining-runtime"
        };

        QString newestCkpoolLog;
        QString newestMinerdLog;
        qint64 newestCkpoolTime = 0;
        qint64 newestMinerdTime = 0;

        for (const QString &root : searchRoots) {
            QDir rootDir(root);
            if (!rootDir.exists()) {
                continue;
            }

            QDirIterator it(
                root,
                QStringList() << "*.log",
                QDir::Files,
                QDirIterator::Subdirectories
            );

            while (it.hasNext()) {
                const QString path = it.next();
                const QFileInfo info(path);
                const QString lowerName = info.fileName().toLower();
                const qint64 modified =
                    info.lastModified().toMSecsSinceEpoch();

                if (lowerName.contains("ckpool") &&
                    modified > newestCkpoolTime) {
                    newestCkpoolTime = modified;
                    newestCkpoolLog = path;
                }

                if (lowerName.contains("minerd") &&
                    modified > newestMinerdTime) {
                    newestMinerdTime = modified;
                    newestMinerdLog = path;
                }
            }
        }

        auto readLogTail = [](const QString &path,
                              qint64 maximumBytes) -> QString {
            if (path.isEmpty()) {
                return {};
            }

            QFile file(path);
            if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                return {};
            }

            const qint64 size = file.size();
            if (size > maximumBytes) {
                file.seek(size - maximumBytes);
            }

            return QString::fromUtf8(file.readAll());
        };

        double localHashrateHps = 0.0;
        QString displayedHashrate = "0 H/s";

        /*
         * Prefer CKPool's total worker speed.
         *
         * Example:
         *   67.5MH/s  0.0 SPS  1 users  1 workers  5 shares
         */
        const QString ckpoolText =
            readLogTail(newestCkpoolLog, 1024 * 1024);

        if (!ckpoolText.isEmpty()) {
            QRegularExpression poolRateRegex(
                R"(([0-9]+(?:\.[0-9]+)?)\s*([kKmMgGtT]?H/s)\s+[0-9.]+\s+SPS\s+\d+\s+users?\s+\d+\s+workers?)"
            );

            auto matches = poolRateRegex.globalMatch(ckpoolText);

            double latestValue = 0.0;
            QString latestUnit;

            while (matches.hasNext()) {
                const auto match = matches.next();
                latestValue = match.captured(1).toDouble();
                latestUnit = match.captured(2);
            }

            if (latestValue > 0.0) {
                const QString unit = latestUnit.toUpper();

                if (unit == "KH/S") {
                    localHashrateHps = latestValue * 1000.0;
                } else if (unit == "MH/S") {
                    localHashrateHps = latestValue * 1000000.0;
                } else if (unit == "GH/S") {
                    localHashrateHps = latestValue * 1000000000.0;
                } else if (unit == "TH/S") {
                    localHashrateHps = latestValue * 1000000000000.0;
                } else {
                    localHashrateHps = latestValue;
                }

                displayedHashrate =
                    QString::number(latestValue, 'f', 2) + " " +
                    latestUnit.toUpper();
            }
        }

        /*
         * Fallback: add together the newest reported speed from every
         * minerd thread.
         *
         * Example:
         *   thread 20: 134217696 hashes, 2662 khash/s
         */
        if (localHashrateHps <= 0.0) {
            const QString minerdText =
                readLogTail(newestMinerdLog, 2 * 1024 * 1024);

            QRegularExpression threadRegex(
                R"(thread\s+(\d+):\s+\d+\s+hashes,\s+([0-9]+(?:\.[0-9]+)?)\s+khash/s)",
                QRegularExpression::CaseInsensitiveOption
            );

            QMap<int, double> latestThreadRates;
            auto matches = threadRegex.globalMatch(minerdText);

            while (matches.hasNext()) {
                const auto match = matches.next();
                const int threadNumber = match.captured(1).toInt();
                const double khash = match.captured(2).toDouble();
                latestThreadRates[threadNumber] = khash;
            }

            double totalKhash = 0.0;
            for (auto it = latestThreadRates.constBegin();
                 it != latestThreadRates.constEnd();
                 ++it) {
                totalKhash += it.value();
            }

            if (totalKhash > 0.0) {
                localHashrateHps = totalKhash * 1000.0;

                if (totalKhash >= 1000.0) {
                    displayedHashrate =
                        QString::number(totalKhash / 1000.0, 'f', 2) +
                        " MH/s";
                } else {
                    displayedHashrate =
                        QString::number(totalKhash, 'f', 2) +
                        " KH/s";
                }
            }
        }

        ui->localHashrateValue->setText(displayedHashrate);

        /*
         * Show a genuinely active status only while a hashrate is present.
         */
        if (localHashrateHps > 0.0) {
            ui->miningStateValue->setText(QString::fromUtf8("⚒ MINING"));
            ui->miningStateValue->setStyleSheet(
                "color:#00ff88; font-weight:700;"
            );
        } else {
            ui->miningStateValue->setText(QString::fromUtf8("⚒ READY"));
            ui->miningStateValue->setStyleSheet(
                "color:#00ff88; font-weight:700;"
            );
        }

        /*
         * Read established Cool Coin P2P connections directly from Linux.
         * This keeps the peer card working even when rpcuser/rpcpassword
         * do not match.
         */
        QProcess ssProcess;
        ssProcess.start(
            "ss",
            QStringList() << "-Htn" << "state" << "established"
        );

        QStringList peerAddresses;

        if (ssProcess.waitForFinished(1500)) {
            const QString output =
                QString::fromUtf8(ssProcess.readAllStandardOutput());

            const QStringList lines =
                output.split('\n', Qt::SkipEmptyParts);

            QSet<QString> uniquePeers;

            for (const QString &line : lines) {
                const QString simplified = line.simplified();

                if (!simplified.contains(":6464")) {
                    continue;
                }

                const QStringList fields =
                    simplified.split(' ', Qt::SkipEmptyParts);

                if (fields.size() < 4) {
                    continue;
                }

                QString localAddress;
                QString remoteAddress;

                /*
                 * ss output is normally:
                 * Recv-Q Send-Q LocalAddress PeerAddress
                 */
                if (fields.size() >= 4) {
                    localAddress = fields.at(fields.size() - 2);
                    remoteAddress = fields.at(fields.size() - 1);
                }

                QString peer = remoteAddress;

                /*
                 * If the remote side is the local Cool Coin port, the
                 * connection is inbound; display the opposite endpoint.
                 */
                if (remoteAddress.endsWith(":6464") &&
                    !localAddress.endsWith(":6464")) {
                    peer = localAddress;
                }

                peer.remove('[');
                peer.remove(']');

                if (peer.isEmpty() ||
                    peer.startsWith("127.0.0.1:") ||
                    peer.startsWith("0.0.0.0:") ||
                    peer.startsWith("::1:")) {
                    continue;
                }

                uniquePeers.insert(peer);
            }

            peerAddresses = uniquePeers.values();
            peerAddresses.sort();
        }

        if (peerAddresses.isEmpty()) {
            ui->connectedPeersValue->setText("No peers");
        } else {
            QStringList displayedPeers;

            const int maximumDisplayedPeers = 6;

            for (int index = 0;
                 index < peerAddresses.size() &&
                 index < maximumDisplayedPeers;
                 ++index) {
                displayedPeers
                    << QString::fromUtf8("● ") + peerAddresses.at(index);
            }

            if (peerAddresses.size() > maximumDisplayedPeers) {
                displayedPeers
                    << QString("+%1 more")
                           .arg(peerAddresses.size() -
                                maximumDisplayedPeers);
            }

            ui->connectedPeersValue->setText(
                displayedPeers.join('\n')
            );
        }
    });

    liveDashboardTimer->start();


    // LIVE_DASHBOARD_PATCH_END

    setWindowTitle("Cool Wallet");

    QPushButton* sendButton =
        findChild<QPushButton*>("sendButton");

    if (sendButton) {
        connect(
            sendButton,
            &QPushButton::clicked,
            this,
            [this]() {
                QDialog dialog(this);
                dialog.setWindowTitle("Send COOL");
                dialog.setMinimumWidth(540);

                QVBoxLayout* mainLayout =
                    new QVBoxLayout(&dialog);

                QLabel* title =
                    new QLabel("Send COOL", &dialog);

                title->setStyleSheet(
                    "color:#00ff66;"
                    "font-size:22pt;"
                    "font-weight:bold;"
                    "margin-bottom:12px;"
                );

                QFormLayout* formLayout =
                    new QFormLayout();

                QLineEdit* addressEdit =
                    new QLineEdit(&dialog);

                addressEdit->setPlaceholderText(
                    "Enter CoolCoin address"
                );

                QDoubleSpinBox* amountEdit =
                    new QDoubleSpinBox(&dialog);

                amountEdit->setDecimals(8);
                amountEdit->setMinimum(0.00000001);
                amountEdit->setMaximum(2100000000.0);
                amountEdit->setSingleStep(1.0);
                amountEdit->setSuffix(" COOL");

                QLineEdit* memoEdit =
                    new QLineEdit(&dialog);

                memoEdit->setPlaceholderText(
                    "Optional memo"
                );

                formLayout->addRow(
                    "Recipient address:",
                    addressEdit
                );

                formLayout->addRow(
                    "Amount:",
                    amountEdit
                );

                formLayout->addRow(
                    "Memo:",
                    memoEdit
                );

                QLabel* status =
                    new QLabel(&dialog);

                status->setWordWrap(true);

                QPushButton* confirmButton =
                    new QPushButton(
                        "Send COOL",
                        &dialog
                    );

                QPushButton* cancelButton =
                    new QPushButton(
                        "Cancel",
                        &dialog
                    );

                mainLayout->addWidget(title);
                mainLayout->addLayout(formLayout);
                mainLayout->addWidget(status);
                mainLayout->addWidget(confirmButton);
                mainLayout->addWidget(cancelButton);

                connect(
                    cancelButton,
                    &QPushButton::clicked,
                    &dialog,
                    &QDialog::reject
                );

                connect(
                    confirmButton,
                    &QPushButton::clicked,
                    &dialog,
                    [this,
                     &dialog,
                     addressEdit,
                     amountEdit,
                     memoEdit,
                     status,
                     confirmButton]() {
                        const QString address =
                            addressEdit->text().trimmed();

                        const double amount =
                            amountEdit->value();

                        if (address.isEmpty()) {
                            status->setText(
                                "Enter a recipient address."
                            );

                            status->setStyleSheet(
                                "color:#ff5555;"
                                "font-weight:bold;"
                            );

                            return;
                        }

                        const QString validationJson =
                            runCoolCoinCli(
                                {
                                    "validateaddress",
                                    address
                                }
                            );

                        if (validationJson.isEmpty()) {
                            status->setText(
                                "Node offline."
                            );

                            status->setStyleSheet(
                                "color:#ff5555;"
                                "font-weight:bold;"
                            );

                            return;
                        }

                        const QJsonObject validation =
                            QJsonDocument::fromJson(
                                validationJson.toUtf8()
                            ).object();

                        if (!validation.value("isvalid").toBool()) {
                            status->setText(
                                "Invalid CoolCoin address."
                            );

                            status->setStyleSheet(
                                "color:#ff5555;"
                                "font-weight:bold;"
                            );

                            return;
                        }

                        const QString amountText =
                            QString::number(
                                amount,
                                'f',
                                8
                            );

                        const QString confirmation =
                            QString(
                                "Send %1 COOL to:\n\n%2\n\n"
                                "This transaction cannot be reversed."
                            )
                            .arg(amountText)
                            .arg(address);

                        const QMessageBox::StandardButton answer =
                            QMessageBox::question(
                                this,
                                "Confirm COOL Transaction",
                                confirmation,
                                QMessageBox::Yes |
                                QMessageBox::No,
                                QMessageBox::No
                            );

                        if (answer != QMessageBox::Yes) {
                            return;
                        }

                        confirmButton->setEnabled(false);
                        status->setText("Sending transaction...");
                        status->setStyleSheet(
                            "color:#ffaa00;"
                            "font-weight:bold;"
                        );

                        QStringList command{
                            "sendtoaddress",
                            address,
                            amountText
                        };

                        if (!memoEdit->text().trimmed().isEmpty()) {
                            command
                                << memoEdit->text().trimmed()
                                << memoEdit->text().trimmed();
                        }

                        const QString transactionId =
                            runCoolCoinCli(
                                command,
                                true
                            );

                        confirmButton->setEnabled(true);

                        if (transactionId.isEmpty()) {
                            status->setText(
                                "Transaction failed. Check the "
                                "wallet balance, wallet lock status, "
                                "address, and node connection."
                            );

                            status->setStyleSheet(
                                "color:#ff5555;"
                                "font-weight:bold;"
                            );

                            return;
                        }

                        QMessageBox::information(
                            this,
                            "COOL Sent",
                            QString(
                                "Transaction sent successfully.\n\n"
                                "Transaction ID:\n%1"
                            ).arg(transactionId)
                        );

                        dialog.accept();
                    }
                );

                dialog.exec();
            }
        );
    }

    QPushButton* receiveButton =
        findChild<QPushButton*>("receiveButton");

    if (receiveButton) {
        connect(
            receiveButton,
            &QPushButton::clicked,
            this,
            [this]() {
                const QString address =
                    runCoolCoinCli({"getnewaddress"}, true);

                if (address.isEmpty()) {
                    QMessageBox::warning(
                        this,
                        "Receive COOL",
                        "Wallet offline or not loaded."
                    );
                    return;
                }

                QDialog dialog(this);
                dialog.setWindowTitle("Receive COOL");
                dialog.setMinimumWidth(520);

                QVBoxLayout* layout =
                    new QVBoxLayout(&dialog);

                QLabel* title =
                    new QLabel("Receive COOL", &dialog);

                title->setStyleSheet(
                    "color:#00ff66;"
                    "font-size:22pt;"
                    "font-weight:bold;"
                );

                QLineEdit* addressEdit =
                    new QLineEdit(address, &dialog);

                addressEdit->setReadOnly(true);

                QPushButton* copyButton =
                    new QPushButton("Copy Address", &dialog);

                QPushButton* newAddressButton =
                    new QPushButton("Generate New Address", &dialog);

                QPushButton* closeButton =
                    new QPushButton("Close", &dialog);

                QLabel* status =
                    new QLabel("Address ready.", &dialog);

                layout->addWidget(title);
                layout->addWidget(addressEdit);
                layout->addWidget(copyButton);
                layout->addWidget(newAddressButton);
                layout->addWidget(status);
                layout->addWidget(closeButton);

                connect(
                    copyButton,
                    &QPushButton::clicked,
                    &dialog,
                    [addressEdit, status]() {
                        QApplication::clipboard()->setText(
                            addressEdit->text()
                        );
                        status->setText("Address copied.");
                    }
                );

                connect(
                    newAddressButton,
                    &QPushButton::clicked,
                    &dialog,
                    [addressEdit, status]() {
                        const QString newAddress =
                            runCoolCoinCli({"getnewaddress"}, true);

                        if (newAddress.isEmpty()) {
                            status->setText(
                                "Could not generate address."
                            );
                        } else {
                            addressEdit->setText(newAddress);
                            status->setText(
                                "New address generated."
                            );
                        }
                    }
                );

                connect(
                    closeButton,
                    &QPushButton::clicked,
                    &dialog,
                    &QDialog::accept
                );

                dialog.exec();
            }
        );
    }


    QPushButton* overviewButton =
        findChild<QPushButton*>("overviewButton");

    if (overviewButton) {
        connect(
            overviewButton,
            &QPushButton::clicked,
            this,
            [this]() {
                activateWindow();
                raise();
                QMessageBox::information(
                    this,
                    "Overview",
                    "The Cool Wallet overview is already displayed."
                );
            }
        );
    }

    QPushButton* sendNavButton =
        findChild<QPushButton*>("sendNavButton");

    if (sendNavButton && sendButton) {
        connect(
            sendNavButton,
            &QPushButton::clicked,
            sendButton,
            &QPushButton::click
        );
    }

    QPushButton* receiveNavButton =
        findChild<QPushButton*>("receiveNavButton");

    if (receiveNavButton && receiveButton) {
        connect(
            receiveNavButton,
            &QPushButton::clicked,
            receiveButton,
            &QPushButton::click
        );
    }

    QPushButton* miningButton =
        findChild<QPushButton*>("miningButton");

    if (miningButton) {
        connect(
            miningButton,
            &QPushButton::clicked,
            this,
            [this]() {
                const QString runtimeDirectory =
                    QCoreApplication::applicationDirPath();

                const QString startScript =
                    runtimeDirectory +
                    "/scripts/start-mining.sh";

                const QString stopScript =
                    runtimeDirectory +
                    "/scripts/stop-mining.sh";

                const QString minerLog =
                    runtimeDirectory +
                    "/logs/minerd.log";

                const QString ckpoolLog =
                    runtimeDirectory +
                    "/logs/ckpool-console.log";

                const QString addressFile =
                    runtimeDirectory +
                    "/runtime/mining-address";

                const QString minerPidFile =
                    runtimeDirectory +
                    "/runtime/minerd.pid";

                const QString ckpoolPidFile =
                    runtimeDirectory +
                    "/runtime/ckpool.pid";

                QDialog dialog(this);
                dialog.setWindowTitle("Cool Coin Mining");
                dialog.resize(820, 680);

                dialog.setStyleSheet(
                    "QDialog {"
                    " background:#080b0a;"
                    " color:#eeeeee;"
                    "}"
                    "QLabel {"
                    " color:#eeeeee;"
                    "}"
                    "QLineEdit {"
                    " background:#101512;"
                    " color:#00ff66;"
                    " border:2px solid #00aa44;"
                    " border-radius:6px;"
                    " padding:9px;"
                    " font-family:monospace;"
                    " font-size:11pt;"
                    "}"
                    "QPushButton {"
                    " background:#152019;"
                    " color:#ffffff;"
                    " border:2px solid #00aa44;"
                    " border-radius:7px;"
                    " padding:10px;"
                    " font-weight:bold;"
                    "}"
                    "QPushButton:hover {"
                    " background:#1e3525;"
                    " border-color:#00ff66;"
                    "}"
                );

                QVBoxLayout* layout =
                    new QVBoxLayout(&dialog);

                layout->setSpacing(10);
                layout->setContentsMargins(18, 18, 18, 18);

                QLabel* title =
                    new QLabel(
                        "⛏ COOL COIN CPU MINING",
                        &dialog
                    );

                title->setAlignment(Qt::AlignCenter);
                title->setStyleSheet(
                    "color:#00ff66;"
                    "font-size:22pt;"
                    "font-weight:bold;"
                    "padding:8px;"
                );

                QLabel* movingMiner =
                    new QLabel(
                        "⛏  . . . . . . . . . . . .  COOL",
                        &dialog
                    );

                movingMiner->setAlignment(Qt::AlignCenter);
                movingMiner->setStyleSheet(
                    "color:#00ff66;"
                    "font-size:18pt;"
                    "font-weight:bold;"
                    "background:#101512;"
                    "border:2px solid #007733;"
                    "border-radius:8px;"
                    "padding:12px;"
                );

                QProgressBar* miningProgress =
                    new QProgressBar(&dialog);

                miningProgress->setRange(0, 0);
                miningProgress->setTextVisible(true);
                miningProgress->setFormat(
                    "Searching for a Cool Coin block..."
                );

                miningProgress->setMinimumHeight(32);
                miningProgress->setStyleSheet(
                    "QProgressBar {"
                    " border:2px solid #00aa44;"
                    " border-radius:7px;"
                    " background:#101512;"
                    " color:white;"
                    " text-align:center;"
                    " font-weight:bold;"
                    "}"
                    "QProgressBar::chunk {"
                    " background:#00aa44;"
                    " width:18px;"
                    " margin:1px;"
                    "}"
                );

                QLabel* statusLabel =
                    new QLabel(
                        "● CHECKING MINING STATUS",
                        &dialog
                    );

                statusLabel->setAlignment(Qt::AlignCenter);
                statusLabel->setStyleSheet(
                    "color:#ffaa00;"
                    "font-size:14pt;"
                    "font-weight:bold;"
                    "padding:8px;"
                );

                QLabel* hashRateLabel =
                    new QLabel(
                        "CPU Hashrate: 0.00 MH/s",
                        &dialog
                    );

                QLabel* elapsedLabel =
                    new QLabel(
                        "Mining Time: 00:00:00",
                        &dialog
                    );

                QLabel* workerLabel =
                    new QLabel(
                        "Workers: 0 / 0",
                        &dialog
                    );

                QLabel* blockFoundLabel =
                    new QLabel(
                        "BLOCK STATUS: SEARCHING...",
                        &dialog
                    );

                blockFoundLabel->setAlignment(Qt::AlignCenter);
                blockFoundLabel->setMinimumHeight(48);
                blockFoundLabel->setStyleSheet(
                    "background:#101512;"
                    "color:#44ccff;"
                    "border:2px solid #007799;"
                    "border-radius:8px;"
                    "font-size:14pt;"
                    "font-weight:bold;"
                    "padding:8px;"
                );

                hashRateLabel->setAlignment(Qt::AlignCenter);
                elapsedLabel->setAlignment(Qt::AlignCenter);
                workerLabel->setAlignment(Qt::AlignCenter);

                const QString statisticStyle =
                    "background:#101512;"
                    "border:1px solid #006622;"
                    "border-radius:6px;"
                    "padding:8px;"
                    "font-size:12pt;"
                    "font-weight:bold;";

                hashRateLabel->setStyleSheet(statisticStyle);
                elapsedLabel->setStyleSheet(statisticStyle);
                workerLabel->setStyleSheet(statisticStyle);

                QHBoxLayout* statisticsLayout =
                    new QHBoxLayout();

                statisticsLayout->addWidget(hashRateLabel);
                statisticsLayout->addWidget(elapsedLabel);
                statisticsLayout->addWidget(workerLabel);

                QLabel* addressTitle =
                    new QLabel(
                        "Mining Reward Address:",
                        &dialog
                    );

                addressTitle->setStyleSheet(
                    "font-size:12pt;"
                    "font-weight:bold;"
                );

                QLineEdit* addressEdit =
                    new QLineEdit(&dialog);

                addressEdit->setReadOnly(true);
                addressEdit->setPlaceholderText(
                    "Generate or start mining to create an address"
                );

                QPushButton* generateAddressButton =
                    new QPushButton(
                        "＋ GENERATE NEW ADDRESS",
                        &dialog
                    );

                QPushButton* copyAddressButton =
                    new QPushButton(
                        "COPY ADDRESS",
                        &dialog
                    );

                QHBoxLayout* addressButtons =
                    new QHBoxLayout();

                addressButtons->addWidget(
                    generateAddressButton
                );

                addressButtons->addWidget(
                    copyAddressButton
                );

                QPushButton* startButton =
                    new QPushButton(
                        "▶ START MINING",
                        &dialog
                    );

                QPushButton* stopButton =
                    new QPushButton(
                        "■ STOP MINING",
                        &dialog
                    );

                startButton->setMinimumHeight(48);
                stopButton->setMinimumHeight(48);

                startButton->setStyleSheet(
                    "QPushButton {"
                    " background:#006622;"
                    " color:white;"
                    " border:2px solid #00ff66;"
                    " border-radius:8px;"
                    " font-size:14pt;"
                    " font-weight:bold;"
                    " padding:10px;"
                    "}"
                    "QPushButton:hover {"
                    " background:#008833;"
                    "}"
                );

                stopButton->setStyleSheet(
                    "QPushButton {"
                    " background:#770000;"
                    " color:white;"
                    " border:2px solid #ff5555;"
                    " border-radius:8px;"
                    " font-size:14pt;"
                    " font-weight:bold;"
                    " padding:10px;"
                    "}"
                    "QPushButton:hover {"
                    " background:#990000;"
                    "}"
                );

                QHBoxLayout* miningButtons =
                    new QHBoxLayout();

                miningButtons->addWidget(startButton);
                miningButtons->addWidget(stopButton);

                QTextEdit* output =
                    new QTextEdit(&dialog);

                output->setReadOnly(true);
                output->setMinimumHeight(220);
                output->setStyleSheet(
                    "QTextEdit {"
                    " background:#020403;"
                    " color:#eeeeee;"
                    " border:2px solid #00aa44;"
                    " border-radius:9px;"
                    " font-family:monospace;"
                    " font-size:11pt;"
                    " padding:8px;"
                    " selection-background-color:#006622;"
                    "}"
                    "QScrollBar:vertical {"
                    " background:#07100b;"
                    " width:14px;"
                    " margin:2px;"
                    "}"
                    "QScrollBar::handle:vertical {"
                    " background:#00aa44;"
                    " border-radius:6px;"
                    " min-height:30px;"
                    "}"
                );

                QPushButton* closeButton =
                    new QPushButton(
                        "Close Mining Window",
                        &dialog
                    );

                layout->addWidget(title);
                layout->addWidget(movingMiner);
                layout->addWidget(miningProgress);
                layout->addWidget(statusLabel);
                layout->addLayout(statisticsLayout);
                layout->addWidget(blockFoundLabel);
                layout->addWidget(addressTitle);
                layout->addWidget(addressEdit);
                layout->addLayout(addressButtons);
                layout->addLayout(miningButtons);
                layout->addWidget(output);
                layout->addWidget(closeButton);

                QTimer* displayTimer =
                    new QTimer(&dialog);

                displayTimer->setInterval(500);

                QDateTime* miningStartTime =
                    new QDateTime();

                int* animationFrame =
                    new int(0);

                int* miningDisplayTick =
                    new int(0);

                int* previousMinedTransactionCount =
                    new int(-1);

                QString* previousBestBlockHash =
                    new QString();

                auto processIsRunning =
                    [](const QString& pidFileName) {
                        QFile pidFile(pidFileName);

                        if (!pidFile.open(
                                QIODevice::ReadOnly |
                                QIODevice::Text
                            )) {
                            return false;
                        }

                        const QByteArray pid =
                            pidFile.readAll().trimmed();

                        pidFile.close();

                        if (pid.isEmpty()) {
                            return false;
                        }

                        QProcess process;

                        process.start(
                            "/bin/kill",
                            QStringList()
                                << "-0"
                                << QString::fromUtf8(pid)
                        );

                        process.waitForFinished(1500);

                        return process.exitCode() == 0;
                    };

                auto loadAddress =
                    [addressEdit, addressFile]() {
                        QFile file(addressFile);

                        if (
                            file.open(
                                QIODevice::ReadOnly |
                                QIODevice::Text
                            )
                        ) {
                            const QString address =
                                QString::fromUtf8(
                                    file.readAll()
                                ).trimmed();

                            if (!address.isEmpty()) {
                                addressEdit->setText(address);
                            }
                        }
                    };

                auto updateMiningDisplay =
                    [
                        statusLabel,
                        hashRateLabel,
                        elapsedLabel,
                        workerLabel,
                        movingMiner,
                        miningProgress,
                        output,
                        addressEdit,
                        minerLog,
                        addressFile,
                        minerPidFile,
                        ckpoolPidFile,
                        ckpoolLog,
                        miningStartTime,
                        animationFrame,
                        miningDisplayTick,
                        previousMinedTransactionCount,
                        previousBestBlockHash,
                        blockFoundLabel,
                        processIsRunning,
                        loadAddress,
                        this
                    ]() {
                        const bool minerRunning =
                            processIsRunning(minerPidFile);

                        const bool ckpoolRunning =
                            processIsRunning(ckpoolPidFile);

                        loadAddress();

                        if (minerRunning && ckpoolRunning) {
                            statusLabel->setText(
                                "● MINING IS ACTIVE"
                            );

                            statusLabel->setStyleSheet(
                                "color:#00ff66;"
                                "font-size:14pt;"
                                "font-weight:bold;"
                                "padding:8px;"
                            );

                            miningProgress->setRange(0, 0);
                            miningProgress->setFormat(
                                "Searching for a Cool Coin block..."
                            );

                            if (!miningStartTime->isValid()) {
                                *miningStartTime =
                                    QDateTime::currentDateTime();
                            }
                        } else {
                            statusLabel->setText(
                                "● MINING IS STOPPED"
                            );

                            statusLabel->setStyleSheet(
                                "color:#ff5555;"
                                "font-size:14pt;"
                                "font-weight:bold;"
                                "padding:8px;"
                            );

                            miningProgress->setRange(0, 100);
                            miningProgress->setValue(0);
                            miningProgress->setFormat(
                                "Mining stopped"
                            );

                            *miningStartTime = QDateTime();
                        }

                        static const QStringList frames{
                            "⛏  COOL ░░░░░░░░░░░░░░░░",
                            "  ⛏ COOL ▓░░░░░░░░░░░░░░",
                            "    ⛏ COOL ▓▓░░░░░░░░░░░░",
                            "      ⛏ COOL ▓▓▓░░░░░░░░░░░",
                            "        ⛏ COOL ▓▓▓▓░░░░░░░░░░",
                            "          ⛏ COOL ▓▓▓▓▓░░░░░░░░░",
                            "            ⛏ COOL ▓▓▓▓▓▓░░░░░░░░",
                            "              ⛏ COOL ▓▓▓▓▓▓▓░░░░░░░",
                            "                ⛏ COOL ▓▓▓▓▓▓▓▓░░░░░░",
                            "                  ⛏ COOL ▓▓▓▓▓▓▓▓▓░░░░░"
                        };

                        if (minerRunning) {
                            movingMiner->setText(
                                frames.at(
                                    *animationFrame %
                                    frames.size()
                                )
                            );

                            *animationFrame =
                                (*animationFrame + 1) %
                                frames.size();
                        } else {
                            movingMiner->setText(
                                "⛏  READY TO MINE COOL"
                            );
                        }

                        if (
                            minerRunning &&
                            miningStartTime->isValid()
                        ) {
                            const qint64 seconds =
                                miningStartTime->secsTo(
                                    QDateTime::currentDateTime()
                                );

                            const qint64 hours =
                                seconds / 3600;

                            const qint64 minutes =
                                (seconds % 3600) / 60;

                            const qint64 remainingSeconds =
                                seconds % 60;

                            elapsedLabel->setText(
                                QString(
                                    "Mining Time: %1:%2:%3"
                                )
                                .arg(hours, 2, 10, QChar('0'))
                                .arg(minutes, 2, 10, QChar('0'))
                                .arg(
                                    remainingSeconds,
                                    2,
                                    10,
                                    QChar('0')
                                )
                            );
                        } else {
                            elapsedLabel->setText(
                                "Mining Time: 00:00:00"
                            );
                        }

                        QFile logFile(minerLog);

                        if (
                            !logFile.open(
                                QIODevice::ReadOnly |
                                QIODevice::Text
                            )
                        ) {
                            output->setPlainText(
                                "Waiting for miner output..."
                            );

                            hashRateLabel->setText(
                                "CPU Hashrate: 0.00 MH/s"
                            );

                            workerLabel->setText(
                                "Workers: 0"
                            );

                            return;
                        }

                        QString log =
                            QString::fromUtf8(
                                logFile.readAll()
                            );

                        logFile.close();

                        if (log.length() > 40000) {
                            log = log.right(40000);
                        }

                        output->setPlainText(log);

                        QTextCursor cursor =
                            output->textCursor();

                        cursor.movePosition(
                            QTextCursor::End
                        );

                        output->setTextCursor(cursor);
                        output->ensureCursorVisible();

                        /*
                         * Read the newest reported speed for every CPU thread.
                         * Reading backward prevents old speeds from being added
                         * together with current speeds.
                         */
                        const QStringList speedLines =
                            log.split(
                                '\n',
                                Qt::SkipEmptyParts
                            );

                        QRegularExpression speedExpression(
                            R"(thread\s+(\d+):\s+\d+\s+hashes,\s+([0-9.]+)\s+khash/s)",
                            QRegularExpression::CaseInsensitiveOption
                        );

                        QMap<int, double> currentThreadRates;

                        for (
                            int lineNumber =
                                speedLines.size() - 1;
                            lineNumber >= 0;
                            --lineNumber
                        ) {
                            const QRegularExpressionMatch match =
                                speedExpression.match(
                                    speedLines.at(lineNumber)
                                );

                            if (!match.hasMatch()) {
                                continue;
                            }

                            const int threadNumber =
                                match.captured(1).toInt();

                            if (
                                currentThreadRates.contains(
                                    threadNumber
                                )
                            ) {
                                continue;
                            }

                            currentThreadRates.insert(
                                threadNumber,
                                match.captured(2).toDouble()
                            );

                            /*
                             * This machine currently uses 32 threads.
                             * Stop as soon as every current thread is found.
                             */
                            if (
                                currentThreadRates.size() >=
                                QThread::idealThreadCount()
                            ) {
                                break;
                            }
                        }

                        double totalKhashPerSecond = 0.0;

                        for (
                            auto rateIterator =
                                currentThreadRates.constBegin();
                            rateIterator !=
                                currentThreadRates.constEnd();
                            ++rateIterator
                        ) {
                            totalKhashPerSecond +=
                                rateIterator.value();
                        }

                        const double totalMhashPerSecond =
                            totalKhashPerSecond / 1000.0;

                        hashRateLabel->setText(
                            QString(
                                "LIVE HASHRATE: %1 MH/s"
                            ).arg(
                                totalMhashPerSecond,
                                0,
                                'f',
                                2
                            )
                        );

                        hashRateLabel->setStyleSheet(
                            totalMhashPerSecond > 0.0
                                ? "background:#101512;"
                                  "color:#00ff66;"
                                  "border:2px solid #00aa44;"
                                  "border-radius:6px;"
                                  "padding:8px;"
                                  "font-size:15pt;"
                                  "font-weight:bold;"
                                : "background:#101512;"
                                  "color:#ff5555;"
                                  "border:2px solid #992222;"
                                  "border-radius:6px;"
                                  "padding:8px;"
                                  "font-size:15pt;"
                                  "font-weight:bold;"
                        );

                        const int expectedWorkers =
                            qMax(
                                1,
                                QThread::idealThreadCount()
                            );

                        workerLabel->setText(
                            QString(
                                "Workers: %1 / %2"
                            )
                            .arg(currentThreadRates.size())
                            .arg(expectedWorkers)
                        );

                        /*
                         * Check CKPool output for an explicit solved-block event.
                         */
                        bool ckpoolReportsBlock = false;
                        QString ckpoolBlockLine;

                        QFile ckpoolFile(ckpoolLog);

                        if (
                            ckpoolFile.open(
                                QIODevice::ReadOnly |
                                QIODevice::Text
                            )
                        ) {
                            QString ckpoolText =
                                QString::fromUtf8(
                                    ckpoolFile.readAll()
                                );

                            ckpoolFile.close();

                            const QStringList ckpoolLines =
                                ckpoolText.split(
                                    '\n',
                                    Qt::SkipEmptyParts
                                );

                            for (
                                int lineNumber =
                                    ckpoolLines.size() - 1;
                                lineNumber >= 0;
                                --lineNumber
                            ) {
                                const QString candidate =
                                    ckpoolLines.at(
                                        lineNumber
                                    ).trimmed();

                                if (
                                    candidate.contains(
                                        "block solved",
                                        Qt::CaseInsensitive
                                    ) ||
                                    candidate.contains(
                                        "solved block",
                                        Qt::CaseInsensitive
                                    ) ||
                                    candidate.contains(
                                        "block found",
                                        Qt::CaseInsensitive
                                    ) ||
                                    candidate.contains(
                                        "found block",
                                        Qt::CaseInsensitive
                                    ) ||
                                    candidate.contains(
                                        "block accepted",
                                        Qt::CaseInsensitive
                                    ) ||
                                    candidate.contains(
                                        "submitblock",
                                        Qt::CaseInsensitive
                                    )
                                ) {
                                    ckpoolReportsBlock = true;
                                    ckpoolBlockLine = candidate;
                                    break;
                                }
                            }
                        }

                        /*
                         * RPC checking runs once every five display updates.
                         * It detects generated or immature mining rewards in
                         * the actual coolwallet wallet.
                         */
                        ++(*miningDisplayTick);

                        if (*miningDisplayTick >= 5) {
                            *miningDisplayTick = 0;

                            const QString bestBlockHash =
                                runCoolCoinCli(
                                    {
                                        "getbestblockhash"
                                    },
                                    false
                                ).trimmed();

                            if (
                                !bestBlockHash.isEmpty() &&
                                previousBestBlockHash->isEmpty()
                            ) {
                                *previousBestBlockHash =
                                    bestBlockHash;
                            } else if (
                                !bestBlockHash.isEmpty() &&
                                bestBlockHash !=
                                    *previousBestBlockHash
                            ) {
                                *previousBestBlockHash =
                                    bestBlockHash;

                                blockFoundLabel->setText(
                                    "NEW COOL COIN BLOCK RECEIVED — "
                                    "MINING CONTINUES"
                                );

                                blockFoundLabel->setStyleSheet(
                                    "background:#10201a;"
                                    "color:#44ccff;"
                                    "border:2px solid #44ccff;"
                                    "border-radius:8px;"
                                    "font-size:14pt;"
                                    "font-weight:bold;"
                                    "padding:8px;"
                                );
                            }

                            const QString transactions =
                                runCoolCoinCli(
                                    {
                                        "listtransactions",
                                        "*",
                                        "500",
                                        "0",
                                        "true"
                                    },
                                    true
                                );

                            QRegularExpression minedCategoryExpression(
                                R"COOL("category"\s*:\s*"(generate|immature)")COOL",
                                QRegularExpression::CaseInsensitiveOption
                            );

                            int minedTransactionCount = 0;

                            QRegularExpressionMatchIterator minedIterator =
                                minedCategoryExpression.globalMatch(
                                    transactions
                                );

                            while (minedIterator.hasNext()) {
                                minedIterator.next();
                                ++minedTransactionCount;
                            }

                            if (
                                *previousMinedTransactionCount < 0
                            ) {
                                *previousMinedTransactionCount =
                                    minedTransactionCount;
                            } else if (
                                minedTransactionCount >
                                *previousMinedTransactionCount
                            ) {
                                *previousMinedTransactionCount =
                                    minedTransactionCount;

                                blockFoundLabel->setText(
                                    "★ YAY! COOLWALLET FOUND A BLOCK! ★"
                                );

                                blockFoundLabel->setStyleSheet(
                                    "background:#3a3000;"
                                    "color:#ffdd33;"
                                    "border:3px solid #ffcc00;"
                                    "border-radius:8px;"
                                    "font-size:17pt;"
                                    "font-weight:bold;"
                                    "padding:10px;"
                                );

                                QApplication::beep();

                                QMessageBox::information(
                                    this,
                                    "YAY! Block Found!",
                                    "CoolWallet found a Cool Coin block!\n\n"
                                    "The mining reward is now showing as "
                                    "immature until it reaches maturity."
                                );
                            }
                        }

                        if (ckpoolReportsBlock) {
                            blockFoundLabel->setText(
                                "★ YAY! CKPOOL REPORTS A BLOCK! ★"
                            );

                            blockFoundLabel->setToolTip(
                                ckpoolBlockLine
                            );

                            blockFoundLabel->setStyleSheet(
                                "background:#3a3000;"
                                "color:#ffdd33;"
                                "border:3px solid #ffcc00;"
                                "border-radius:8px;"
                                "font-size:17pt;"
                                "font-weight:bold;"
                                "padding:10px;"
                            );
                        } else if (
                            totalMhashPerSecond > 0.0 &&
                            !blockFoundLabel->text().contains(
                                "YAY",
                                Qt::CaseInsensitive
                            )
                        ) {
                            blockFoundLabel->setText(
                                "BLOCK STATUS: ACTIVELY SEARCHING"
                            );

                            blockFoundLabel->setStyleSheet(
                                "background:#101512;"
                                "color:#44ccff;"
                                "border:2px solid #007799;"
                                "border-radius:8px;"
                                "font-size:14pt;"
                                "font-weight:bold;"
                                "padding:8px;"
                            );
                        }
                    };

                connect(
                    generateAddressButton,
                    &QPushButton::clicked,
                    &dialog,
                    [
                        this,
                        addressEdit,
                        addressFile
                    ]() {
                        const QString newAddress =
                            runCoolCoinCli(
                                {
                                    "getnewaddress",
                                    "CoolWallet Mining"
                                },
                                true
                            ).trimmed();

                        if (newAddress.isEmpty()) {
                            QMessageBox::warning(
                                this,
                                "Address Error",
                                "Could not generate a new address.\n\n"
                                "Make sure coolwallet is loaded and "
                                "the Cool Coin node is online."
                            );

                            return;
                        }

                        addressEdit->setText(newAddress);

                        QFile file(addressFile);

                        if (
                            file.open(
                                QIODevice::WriteOnly |
                                QIODevice::Truncate |
                                QIODevice::Text
                            )
                        ) {
                            file.write(
                                newAddress.toUtf8()
                            );

                            file.write("\n");
                            file.close();
                        }

                        QMessageBox::information(
                            this,
                            "New Mining Address",
                            QString(
                                "A new Cool Coin address was created:\n\n%1"
                            ).arg(newAddress)
                        );
                    }
                );

                connect(
                    copyAddressButton,
                    &QPushButton::clicked,
                    &dialog,
                    [this, addressEdit]() {
                        const QString address =
                            addressEdit->text().trimmed();

                        if (address.isEmpty()) {
                            QMessageBox::warning(
                                this,
                                "Copy Address",
                                "Generate an address first."
                            );

                            return;
                        }

                        QApplication::clipboard()->setText(
                            address
                        );

                        QMessageBox::information(
                            this,
                            "Address Copied",
                            "The Cool Coin address was copied."
                        );
                    }
                );

                connect(
                    startButton,
                    &QPushButton::clicked,
                    &dialog,
                    [
                        this,
                        startScript,
                        miningStartTime,
                        updateMiningDisplay
                    ]() {
                        QProcess process;

                        process.setProcessChannelMode(
                            QProcess::MergedChannels
                        );

                        process.start(
                            "/bin/bash",
                            QStringList() << startScript
                        );

                        if (!process.waitForStarted(5000)) {
                            QMessageBox::critical(
                                this,
                                "Mining Error",
                                "The mining script could not start."
                            );

                            return;
                        }

                        process.waitForFinished(40000);

                        const QString result =
                            QString::fromUtf8(
                                process.readAll()
                            ).trimmed();

                        if (
                            process.exitStatus() !=
                                QProcess::NormalExit ||
                            process.exitCode() != 0
                        ) {
                            QMessageBox::critical(
                                this,
                                "Mining Failed",
                                result.isEmpty()
                                    ? "Mining failed to start."
                                    : result
                            );

                            updateMiningDisplay();
                            return;
                        }

                        *miningStartTime =
                            QDateTime::currentDateTime();

                        updateMiningDisplay();

                        QMessageBox::information(
                            this,
                            "Mining Started",
                            "CKPool and CPUminer are now mining.\n\n"
                            "The mining display will update every second."
                        );
                    }
                );

                connect(
                    stopButton,
                    &QPushButton::clicked,
                    &dialog,
                    [
                        this,
                        stopScript,
                        miningStartTime,
                        updateMiningDisplay
                    ]() {
                        QProcess process;

                        process.setProcessChannelMode(
                            QProcess::MergedChannels
                        );

                        process.start(
                            "/bin/bash",
                            QStringList() << stopScript
                        );

                        process.waitForFinished(20000);

                        *miningStartTime = QDateTime();

                        updateMiningDisplay();

                        QMessageBox::information(
                            this,
                            "Mining Stopped",
                            "CKPool and CPUminer have stopped."
                        );
                    }
                );

                connect(
                    displayTimer,
                    &QTimer::timeout,
                    &dialog,
                    updateMiningDisplay
                );

                connect(
                    closeButton,
                    &QPushButton::clicked,
                    &dialog,
                    &QDialog::accept
                );

                connect(
                    &dialog,
                    &QDialog::finished,
                    &dialog,
                    [
                        displayTimer,
                        miningStartTime,
                        animationFrame,
                        miningDisplayTick,
                        previousMinedTransactionCount,
                        previousBestBlockHash
                    ]() {
                        displayTimer->stop();
                        delete miningStartTime;
                        delete animationFrame;
                        delete miningDisplayTick;
                        delete previousMinedTransactionCount;
                        delete previousBestBlockHash;
                    }
                );

                loadAddress();
                updateMiningDisplay();
                displayTimer->start();

                dialog.exec();
            }
        );
    }

    QPushButton* transactionHistoryButton =
        findChild<QPushButton*>("transactionHistoryButton");

    if (transactionHistoryButton) {
        connect(
            transactionHistoryButton,
            &QPushButton::clicked,
            this,
            [this]() {
                QDialog dialog(this);

                dialog.setWindowTitle(
                    "CoolWallet Transaction History"
                );

                dialog.setModal(true);
                dialog.resize(760, 620);

                dialog.setStyleSheet(
                    "QDialog {"
                    "  background-color:#07111d;"
                    "  color:#f0f7ff;"
                    "}"
                    "QLabel {"
                    "  color:#f0f7ff;"
                    "}"
                    "QTextBrowser {"
                    "  background-color:#0a1725;"
                    "  color:#f0f7ff;"
                    "  border:2px solid #18d6ff;"
                    "  border-radius:10px;"
                    "  padding:12px;"
                    "  selection-background-color:#18d6ff;"
                    "  selection-color:#07111d;"
                    "}"
                    "QPushButton {"
                    "  min-width:120px;"
                    "  min-height:36px;"
                    "  padding:5px 16px;"
                    "  color:#f0f7ff;"
                    "  background-color:#10263a;"
                    "  border:2px solid #18d6ff;"
                    "  border-radius:8px;"
                    "  font-weight:700;"
                    "}"
                    "QPushButton:hover {"
                    "  background-color:#173854;"
                    "}"
                    "QPushButton:pressed {"
                    "  background-color:#18d6ff;"
                    "  color:#07111d;"
                    "}"
                );

                QVBoxLayout* mainLayout =
                    new QVBoxLayout(&dialog);

                mainLayout->setContentsMargins(
                    18,
                    18,
                    18,
                    18
                );

                mainLayout->setSpacing(12);

                QLabel* titleLabel =
                    new QLabel(
                        "☷  TRANSACTION HISTORY",
                        &dialog
                    );

                titleLabel->setStyleSheet(
                    "color:#18d6ff;"
                    "font-size:18px;"
                    "font-weight:800;"
                    "padding:4px;"
                );

                QLabel* subtitleLabel =
                    new QLabel(
                        "Recent Cool Coin wallet activity",
                        &dialog
                    );

                subtitleLabel->setStyleSheet(
                    "color:#a8bdd0;"
                    "font-size:12px;"
                    "padding-bottom:4px;"
                );

                QTextBrowser* historyBrowser =
                    new QTextBrowser(&dialog);

                historyBrowser->setOpenExternalLinks(false);
                historyBrowser->setReadOnly(true);

                historyBrowser->setTextInteractionFlags(
                    Qt::TextSelectableByMouse |
                    Qt::LinksAccessibleByMouse
                );

                QPushButton* refreshButton =
                    new QPushButton(
                        "↻  Refresh",
                        &dialog
                    );

                QPushButton* closeButton =
                    new QPushButton(
                        "Close",
                        &dialog
                    );

                QHBoxLayout* buttonLayout =
                    new QHBoxLayout();

                buttonLayout->addWidget(refreshButton);
                buttonLayout->addStretch();
                buttonLayout->addWidget(closeButton);

                mainLayout->addWidget(titleLabel);
                mainLayout->addWidget(subtitleLabel);
                mainLayout->addWidget(historyBrowser, 1);
                mainLayout->addLayout(buttonLayout);

                const auto loadTransactionHistory =
                    [this, historyBrowser]() {
                        historyBrowser->setHtml(
                            "<div style='"
                            "color:#18d6ff;"
                            "font-size:14px;"
                            "padding:20px;"
                            "text-align:center;"
                            "'>"
                            "Loading Cool Coin transactions..."
                            "</div>"
                        );

                        QApplication::processEvents();

                        const QString transactionsJson =
                            runCoolCoinCli(
                                {
                                    "listtransactions",
                                    "*",
                                    "100",
                                    "0",
                                    "true"
                                },
                                true
                            ).trimmed();

                        if (transactionsJson.isEmpty()) {
                            historyBrowser->setHtml(
                                "<div style='"
                                "color:#ff7777;"
                                "font-size:14px;"
                                "padding:24px;"
                                "text-align:center;"
                                "'>"
                                "<b>Transaction history unavailable</b>"
                                "<br><br>"
                                "The wallet may be offline, locked, "
                                "or not loaded."
                                "</div>"
                            );

                            return;
                        }

                        QJsonParseError parseError;

                        const QJsonDocument transactionDocument =
                            QJsonDocument::fromJson(
                                transactionsJson.toUtf8(),
                                &parseError
                            );

                        if (
                            parseError.error !=
                                QJsonParseError::NoError ||
                            !transactionDocument.isArray()
                        ) {
                            historyBrowser->setHtml(
                                QString(
                                    "<div style='"
                                    "color:#ff7777;"
                                    "font-size:14px;"
                                    "padding:24px;"
                                    "'>"
                                    "<b>Could not read transaction data.</b>"
                                    "<br><br>"
                                    "%1"
                                    "</div>"
                                ).arg(
                                    transactionsJson
                                        .left(600)
                                        .toHtmlEscaped()
                                )
                            );

                            return;
                        }

                        const QJsonArray transactions =
                            transactionDocument.array();

                        QString historyHtml =
                            "<html>"
                            "<body style='"
                            "background-color:#0a1725;"
                            "color:#f0f7ff;"
                            "font-family:sans-serif;"
                            "margin:4px;"
                            "'>";

                        historyHtml +=
                            QString(
                                "<div style='"
                                "color:#a8bdd0;"
                                "font-size:12px;"
                                "margin-bottom:16px;"
                                "'>"
                                "Showing %1 recent transaction%2"
                                "</div>"
                            )
                                .arg(transactions.size())
                                .arg(
                                    transactions.size() == 1
                                        ? ""
                                        : "s"
                                );

                        historyHtml +=
                            transactionHtml(transactions);

                        historyHtml +=
                            "</body>"
                            "</html>";

                        historyBrowser->setHtml(historyHtml);
                    };

                connect(
                    refreshButton,
                    &QPushButton::clicked,
                    &dialog,
                    loadTransactionHistory
                );

                connect(
                    closeButton,
                    &QPushButton::clicked,
                    &dialog,
                    &QDialog::accept
                );

                loadTransactionHistory();

                dialog.exec();
            }
        );
    }

    QPushButton* addressBookButton =
        findChild<QPushButton*>("addressBookButton");

    if (addressBookButton) {
        disconnect(
            addressBookButton,
            nullptr,
            this,
            nullptr
        );

        connect(
            addressBookButton,
            &QPushButton::clicked,
            this,
            [this]() {
                QDialog dialog(this);

                dialog.setWindowTitle(
                    "CoolWallet Address Book"
                );

                dialog.resize(820, 560);
                dialog.setModal(true);

                dialog.setStyleSheet(
                    "QDialog {"
                    " background:#07111d;"
                    " color:white;"
                    "}"
                    "QLabel {"
                    " color:white;"
                    "}"
                    "QListWidget {"
                    " background:#0a1725;"
                    " color:white;"
                    " border:2px solid #18d6ff;"
                    " border-radius:8px;"
                    " padding:8px;"
                    " font-size:13px;"
                    "}"
                    "QListWidget::item {"
                    " padding:10px;"
                    " border-bottom:1px solid #20384d;"
                    "}"
                    "QListWidget::item:selected {"
                    " background:#174d68;"
                    " color:white;"
                    "}"
                    "QPushButton {"
                    " min-height:36px;"
                    " padding:5px 14px;"
                    " background:#10263a;"
                    " color:white;"
                    " border:2px solid #18d6ff;"
                    " border-radius:8px;"
                    " font-weight:bold;"
                    "}"
                    "QPushButton:hover {"
                    " background:#173854;"
                    "}"
                );

                QVBoxLayout* layout =
                    new QVBoxLayout(&dialog);

                QLabel* title =
                    new QLabel(
                        "▣  COOLWALLET ADDRESS BOOK",
                        &dialog
                    );

                title->setStyleSheet(
                    "color:#18d6ff;"
                    "font-size:20px;"
                    "font-weight:bold;"
                );

                QLabel* status =
                    new QLabel(
                        "Loading wallet addresses...",
                        &dialog
                    );

                status->setStyleSheet(
                    "color:#a8bdd0;"
                    "font-size:12px;"
                );

                QListWidget* list =
                    new QListWidget(&dialog);

                QPushButton* createButton =
                    new QPushButton(
                        "+  Create Address",
                        &dialog
                    );

                QPushButton* copyButton =
                    new QPushButton(
                        "Copy Address",
                        &dialog
                    );

                QPushButton* refreshButton =
                    new QPushButton(
                        "Refresh",
                        &dialog
                    );

                QPushButton* closeButton =
                    new QPushButton(
                        "Close",
                        &dialog
                    );

                QHBoxLayout* buttons =
                    new QHBoxLayout();

                buttons->addWidget(createButton);
                buttons->addWidget(copyButton);
                buttons->addWidget(refreshButton);
                buttons->addStretch();
                buttons->addWidget(closeButton);

                layout->addWidget(title);
                layout->addWidget(status);
                layout->addWidget(list, 1);
                layout->addLayout(buttons);

                const auto refreshAddresses =
                    [this, list, status]() {
                        list->clear();

                        status->setText(
                            "Loading wallet addresses..."
                        );

                        QApplication::processEvents();

                        const QString json =
                            runCoolCoinCli(
                                {
                                    "listreceivedbyaddress",
                                    "0",
                                    "true",
                                    "true"
                                },
                                true
                            ).trimmed();

                        if (json.isEmpty()) {
                            status->setStyleSheet(
                                "color:#ff6666;"
                                "font-size:12px;"
                            );

                            status->setText(
                                "Wallet is offline or not loaded."
                            );

                            return;
                        }

                        QJsonParseError error;

                        const QJsonDocument document =
                            QJsonDocument::fromJson(
                                json.toUtf8(),
                                &error
                            );

                        if (
                            error.error !=
                                QJsonParseError::NoError ||
                            !document.isArray()
                        ) {
                            status->setStyleSheet(
                                "color:#ff6666;"
                                "font-size:12px;"
                            );

                            status->setText(
                                "Could not read address data."
                            );

                            return;
                        }

                        const QJsonArray array =
                            document.array();

                        for (
                            const QJsonValue& value :
                            array
                        ) {
                            const QJsonObject object =
                                value.toObject();

                            QString label =
                                object
                                    .value("label")
                                    .toString()
                                    .trimmed();

                            const QString address =
                                object
                                    .value("address")
                                    .toString()
                                    .trimmed();

                            const double amount =
                                object
                                    .value("amount")
                                    .toDouble();

                            if (address.isEmpty()) {
                                continue;
                            }

                            if (label.isEmpty()) {
                                label = "Unlabeled";
                            }

                            QListWidgetItem* item =
                                new QListWidgetItem(
                                    QString(
                                        "%1\n%2\nReceived: %3 COOL"
                                    )
                                        .arg(label)
                                        .arg(address)
                                        .arg(
                                            QString::number(
                                                amount,
                                                'f',
                                                8
                                            )
                                        ),
                                    list
                                );

                            item->setData(
                                Qt::UserRole,
                                address
                            );

                            item->setToolTip(
                                "Double-click to copy address"
                            );
                        }

                        status->setStyleSheet(
                            "color:#00ff66;"
                            "font-size:12px;"
                        );

                        status->setText(
                            QString(
                                "%1 address%2 loaded."
                            )
                                .arg(list->count())
                                .arg(
                                    list->count() == 1
                                        ? ""
                                        : "es"
                                )
                        );

                        if (list->count() > 0) {
                            list->setCurrentRow(0);
                        }
                    };

                connect(
                    refreshButton,
                    &QPushButton::clicked,
                    &dialog,
                    refreshAddresses
                );

                connect(
                    closeButton,
                    &QPushButton::clicked,
                    &dialog,
                    &QDialog::accept
                );

                const auto copySelectedAddress =
                    [list, status]() {
                        QListWidgetItem* item =
                            list->currentItem();

                        if (!item) {
                            status->setStyleSheet(
                                "color:#ffcc55;"
                                "font-size:12px;"
                            );

                            status->setText(
                                "Select an address first."
                            );

                            return;
                        }

                        const QString address =
                            item
                                ->data(Qt::UserRole)
                                .toString();

                        QApplication
                            ::clipboard()
                            ->setText(address);

                        status->setStyleSheet(
                            "color:#00ff66;"
                            "font-size:12px;"
                            "font-weight:bold;"
                        );

                        status->setText(
                            "Address copied to clipboard."
                        );
                    };

                connect(
                    copyButton,
                    &QPushButton::clicked,
                    &dialog,
                    copySelectedAddress
                );

                connect(
                    list,
                    &QListWidget::itemDoubleClicked,
                    &dialog,
                    [
                        copySelectedAddress
                    ](QListWidgetItem*) {
                        copySelectedAddress();
                    }
                );

                connect(
                    createButton,
                    &QPushButton::clicked,
                    &dialog,
                    [
                        this,
                        &dialog,
                        refreshAddresses,
                        status
                    ]() {
                        bool ok = false;

                        QString label =
                            QInputDialog::getText(
                                &dialog,
                                "Create Cool Coin Address",
                                "Address label:",
                                QLineEdit::Normal,
                                "CoolWallet",
                                &ok
                            ).trimmed();

                        if (!ok) {
                            return;
                        }

                        if (label.isEmpty()) {
                            label = "CoolWallet";
                        }

                        QString newAddress =
                            runCoolCoinCli(
                                {
                                    "getnewaddress",
                                    label
                                },
                                true
                            ).trimmed();

                        newAddress.remove('"');

                        if (newAddress.isEmpty()) {
                            status->setStyleSheet(
                                "color:#ff6666;"
                                "font-size:12px;"
                            );

                            status->setText(
                                "Could not create address."
                            );

                            return;
                        }

                        QApplication
                            ::clipboard()
                            ->setText(newAddress);

                        QMessageBox::information(
                            &dialog,
                            "New Cool Coin Address",
                            QString(
                                "New address created:\n\n"
                                "%1\n\n"
                                "It was copied to the clipboard."
                            ).arg(newAddress)
                        );

                        refreshAddresses();
                    }
                );

                refreshAddresses();
                dialog.exec();
            }
        );
    }

    QPushButton* walletSecurityButton =
        findChild<QPushButton*>("walletSecurityButton");

    if (walletSecurityButton) {
        disconnect(
            walletSecurityButton,
            nullptr,
            this,
            nullptr
        );

        connect(
            walletSecurityButton,
            &QPushButton::clicked,
            this,
            [this]() {
                QDialog dialog(this);

                dialog.setWindowTitle(
                    "CoolWallet Security"
                );

                dialog.resize(720, 560);
                dialog.setModal(true);

                dialog.setStyleSheet(
                    "QDialog {"
                    " background-color:#07111d;"
                    " color:#f0f7ff;"
                    "}"
                    "QLabel {"
                    " color:#f0f7ff;"
                    "}"
                    "QSpinBox {"
                    " min-height:34px;"
                    " background-color:#0a1725;"
                    " color:white;"
                    " border:2px solid #18d6ff;"
                    " border-radius:7px;"
                    " padding:3px 8px;"
                    "}"
                    "QPushButton {"
                    " min-height:38px;"
                    " padding:5px 15px;"
                    " color:#f0f7ff;"
                    " background-color:#10263a;"
                    " border:2px solid #18d6ff;"
                    " border-radius:8px;"
                    " font-weight:700;"
                    "}"
                    "QPushButton:hover {"
                    " background-color:#173854;"
                    "}"
                    "QPushButton:pressed {"
                    " background-color:#18d6ff;"
                    " color:#07111d;"
                    "}"
                    "QPushButton:disabled {"
                    " color:#647789;"
                    " background-color:#0b1722;"
                    " border-color:#304454;"
                    "}"
                );

                QVBoxLayout* mainLayout =
                    new QVBoxLayout(&dialog);

                mainLayout->setContentsMargins(
                    20,
                    20,
                    20,
                    20
                );

                mainLayout->setSpacing(14);

                QLabel* titleLabel =
                    new QLabel(
                        "🔐  COOLWALLET SECURITY",
                        &dialog
                    );

                titleLabel->setStyleSheet(
                    "color:#18d6ff;"
                    "font-size:20px;"
                    "font-weight:800;"
                );

                QLabel* descriptionLabel =
                    new QLabel(
                        "Protect the private keys stored in your "
                        "Cool Coin wallet.",
                        &dialog
                    );

                descriptionLabel->setStyleSheet(
                    "color:#a8bdd0;"
                    "font-size:12px;"
                );

                QLabel* securityStateLabel =
                    new QLabel(&dialog);

                securityStateLabel->setAlignment(
                    Qt::AlignCenter
                );

                securityStateLabel->setMinimumHeight(58);

                QLabel* walletNameValue =
                    new QLabel("--", &dialog);

                QLabel* walletFormatValue =
                    new QLabel("--", &dialog);

                QLabel* encryptionValue =
                    new QLabel("--", &dialog);

                QLabel* lockValue =
                    new QLabel("--", &dialog);

                QLabel* privateKeysValue =
                    new QLabel("--", &dialog);

                QLabel* descriptorsValue =
                    new QLabel("--", &dialog);

                QLabel* avoidReuseValue =
                    new QLabel("--", &dialog);

                const QList<QLabel*> informationValues = {
                    walletNameValue,
                    walletFormatValue,
                    encryptionValue,
                    lockValue,
                    privateKeysValue,
                    descriptorsValue,
                    avoidReuseValue
                };

                for (QLabel* valueLabel : informationValues) {
                    valueLabel->setStyleSheet(
                        "color:#18d6ff;"
                        "font-weight:700;"
                    );

                    valueLabel->setTextInteractionFlags(
                        Qt::TextSelectableByMouse
                    );
                }

                QFormLayout* informationLayout =
                    new QFormLayout();

                informationLayout->setHorizontalSpacing(28);
                informationLayout->setVerticalSpacing(9);

                informationLayout->addRow(
                    "Wallet:",
                    walletNameValue
                );

                informationLayout->addRow(
                    "Format:",
                    walletFormatValue
                );

                informationLayout->addRow(
                    "Encryption:",
                    encryptionValue
                );

                informationLayout->addRow(
                    "Current status:",
                    lockValue
                );

                informationLayout->addRow(
                    "Private keys:",
                    privateKeysValue
                );

                informationLayout->addRow(
                    "Descriptor wallet:",
                    descriptorsValue
                );

                informationLayout->addRow(
                    "Avoid address reuse:",
                    avoidReuseValue
                );

                QLabel* unlockDurationLabel =
                    new QLabel(
                        "Unlock duration:",
                        &dialog
                    );

                QSpinBox* unlockSeconds =
                    new QSpinBox(&dialog);

                unlockSeconds->setRange(
                    10,
                    86400
                );

                unlockSeconds->setValue(300);
                unlockSeconds->setSuffix(" seconds");

                QHBoxLayout* durationLayout =
                    new QHBoxLayout();

                durationLayout->addWidget(
                    unlockDurationLabel
                );

                durationLayout->addWidget(
                    unlockSeconds,
                    1
                );

                QPushButton* encryptButton =
                    new QPushButton(
                        "🔑  Encrypt Wallet",
                        &dialog
                    );

                QPushButton* unlockButton =
                    new QPushButton(
                        "🔓  Unlock Wallet",
                        &dialog
                    );

                QPushButton* lockButton =
                    new QPushButton(
                        "🔒  Lock Wallet",
                        &dialog
                    );

                QPushButton* changePassphraseButton =
                    new QPushButton(
                        "↻  Change Passphrase",
                        &dialog
                    );

                QPushButton* refreshButton =
                    new QPushButton(
                        "Refresh Status",
                        &dialog
                    );

                QPushButton* closeButton =
                    new QPushButton(
                        "Close",
                        &dialog
                    );

                QHBoxLayout* firstButtonRow =
                    new QHBoxLayout();

                firstButtonRow->addWidget(encryptButton);
                firstButtonRow->addWidget(unlockButton);
                firstButtonRow->addWidget(lockButton);

                QHBoxLayout* secondButtonRow =
                    new QHBoxLayout();

                secondButtonRow->addWidget(
                    changePassphraseButton
                );

                secondButtonRow->addStretch();
                secondButtonRow->addWidget(refreshButton);
                secondButtonRow->addWidget(closeButton);

                QLabel* messageLabel =
                    new QLabel(&dialog);

                messageLabel->setWordWrap(true);
                messageLabel->setMinimumHeight(42);

                messageLabel->setStyleSheet(
                    "color:#a8bdd0;"
                    "font-size:12px;"
                    "padding:6px;"
                );

                mainLayout->addWidget(titleLabel);
                mainLayout->addWidget(descriptionLabel);
                mainLayout->addWidget(securityStateLabel);
                mainLayout->addLayout(informationLayout);
                mainLayout->addSpacing(4);
                mainLayout->addLayout(durationLayout);
                mainLayout->addSpacing(4);
                mainLayout->addLayout(firstButtonRow);
                mainLayout->addLayout(secondButtonRow);
                mainLayout->addWidget(messageLabel);
                mainLayout->addStretch();

                const auto refreshSecurityStatus =
                    [
                        this,
                        securityStateLabel,
                        walletNameValue,
                        walletFormatValue,
                        encryptionValue,
                        lockValue,
                        privateKeysValue,
                        descriptorsValue,
                        avoidReuseValue,
                        encryptButton,
                        unlockButton,
                        lockButton,
                        changePassphraseButton,
                        unlockSeconds,
                        messageLabel
                    ]() {
                        messageLabel->setStyleSheet(
                            "color:#a8bdd0;"
                            "font-size:12px;"
                            "padding:6px;"
                        );

                        messageLabel->setText(
                            "Reading wallet security status..."
                        );

                        QApplication::processEvents();

                        const QString walletInfo =
                            runCoolCoinCli(
                                {
                                    "getwalletinfo"
                                },
                                true
                            ).trimmed();

                        if (walletInfo.isEmpty()) {
                            securityStateLabel->setText(
                                "⚠ WALLET OFFLINE"
                            );

                            securityStateLabel->setStyleSheet(
                                "color:#ff6666;"
                                "background:#2b1117;"
                                "border:2px solid #ff5555;"
                                "border-radius:9px;"
                                "font-size:17px;"
                                "font-weight:800;"
                            );

                            walletNameValue->setText("--");
                            walletFormatValue->setText("--");
                            encryptionValue->setText("--");
                            lockValue->setText("--");
                            privateKeysValue->setText("--");
                            descriptorsValue->setText("--");
                            avoidReuseValue->setText("--");

                            encryptButton->setEnabled(false);
                            unlockButton->setEnabled(false);
                            lockButton->setEnabled(false);

                            changePassphraseButton
                                ->setEnabled(false);

                            unlockSeconds->setEnabled(false);

                            messageLabel->setStyleSheet(
                                "color:#ff7777;"
                                "font-size:12px;"
                                "padding:6px;"
                            );

                            messageLabel->setText(
                                "The wallet is offline or the "
                                "coolwallet wallet is not loaded."
                            );

                            return;
                        }

                        QJsonParseError parseError;

                        const QJsonDocument document =
                            QJsonDocument::fromJson(
                                walletInfo.toUtf8(),
                                &parseError
                            );

                        if (
                            parseError.error !=
                                QJsonParseError::NoError ||
                            !document.isObject()
                        ) {
                            securityStateLabel->setText(
                                "⚠ STATUS ERROR"
                            );

                            securityStateLabel->setStyleSheet(
                                "color:#ffcc55;"
                                "background:#29220d;"
                                "border:2px solid #ffcc55;"
                                "border-radius:9px;"
                                "font-size:17px;"
                                "font-weight:800;"
                            );

                            messageLabel->setText(
                                "CoolWallet could not parse the "
                                "wallet security information."
                            );

                            return;
                        }

                        const QJsonObject object =
                            document.object();

                        const bool privateKeysEnabled =
                            object
                                .value("private_keys_enabled")
                                .toBool();

                        /*
                         * Bitcoin-derived wallet RPC only includes
                         * unlocked_until when the wallet is encrypted.
                         */
                        const bool encrypted =
                            object.contains(
                                "unlocked_until"
                            );

                        const qint64 unlockedUntil =
                            object
                                .value("unlocked_until")
                                .toVariant()
                                .toLongLong();

                        const qint64 currentTime =
                            QDateTime
                                ::currentSecsSinceEpoch();

                        const bool unlocked =
                            encrypted &&
                            unlockedUntil > currentTime;

                        walletNameValue->setText(
                            object
                                .value("walletname")
                                .toString()
                        );

                        walletFormatValue->setText(
                            object
                                .value("format")
                                .toString()
                        );

                        encryptionValue->setText(
                            encrypted
                                ? "Encrypted"
                                : "Not encrypted"
                        );

                        if (!encrypted) {
                            lockValue->setText(
                                "No passphrase protection"
                            );
                        } else if (unlocked) {
                            const QDateTime unlockTime =
                                QDateTime
                                    ::fromSecsSinceEpoch(
                                        unlockedUntil
                                    );

                            lockValue->setText(
                                QString(
                                    "Unlocked until %1"
                                ).arg(
                                    unlockTime.toString(
                                        "MM/dd/yyyy hh:mm:ss"
                                    )
                                )
                            );
                        } else {
                            lockValue->setText("Locked");
                        }

                        privateKeysValue->setText(
                            privateKeysEnabled
                                ? "Enabled"
                                : "Disabled"
                        );

                        descriptorsValue->setText(
                            object
                                .value("descriptors")
                                .toBool()
                                ? "Yes"
                                : "No"
                        );

                        avoidReuseValue->setText(
                            object
                                .value("avoid_reuse")
                                .toBool()
                                ? "Enabled"
                                : "Disabled"
                        );

                        if (!encrypted) {
                            securityStateLabel->setText(
                                "⚠ WALLET NOT ENCRYPTED"
                            );

                            securityStateLabel->setStyleSheet(
                                "color:#ffcc55;"
                                "background:#29220d;"
                                "border:2px solid #ffcc55;"
                                "border-radius:9px;"
                                "font-size:17px;"
                                "font-weight:800;"
                            );
                        } else if (unlocked) {
                            securityStateLabel->setText(
                                "🔓 WALLET UNLOCKED"
                            );

                            securityStateLabel->setStyleSheet(
                                "color:#ffcc55;"
                                "background:#29220d;"
                                "border:2px solid #ffcc55;"
                                "border-radius:9px;"
                                "font-size:17px;"
                                "font-weight:800;"
                            );
                        } else {
                            securityStateLabel->setText(
                                "🔒 WALLET ENCRYPTED AND LOCKED"
                            );

                            securityStateLabel->setStyleSheet(
                                "color:#00ff66;"
                                "background:#092317;"
                                "border:2px solid #00cc55;"
                                "border-radius:9px;"
                                "font-size:17px;"
                                "font-weight:800;"
                            );
                        }

                        encryptButton->setEnabled(
                            !encrypted &&
                            privateKeysEnabled
                        );

                        unlockButton->setEnabled(
                            encrypted &&
                            !unlocked
                        );

                        lockButton->setEnabled(
                            encrypted &&
                            unlocked
                        );

                        changePassphraseButton->setEnabled(
                            encrypted
                        );

                        unlockSeconds->setEnabled(
                            encrypted &&
                            !unlocked
                        );

                        messageLabel->setText(
                            "Wallet security status updated."
                        );
                    };

                connect(
                    refreshButton,
                    &QPushButton::clicked,
                    &dialog,
                    refreshSecurityStatus
                );

                connect(
                    closeButton,
                    &QPushButton::clicked,
                    &dialog,
                    &QDialog::accept
                );

                connect(
                    encryptButton,
                    &QPushButton::clicked,
                    &dialog,
                    [
                        this,
                        &dialog,
                        refreshSecurityStatus,
                        messageLabel
                    ]() {
                        bool firstAccepted = false;

                        const QString passphrase =
                            QInputDialog::getText(
                                &dialog,
                                "Encrypt CoolWallet",
                                "Enter a new wallet passphrase:",
                                QLineEdit::Password,
                                QString(),
                                &firstAccepted
                            );

                        if (!firstAccepted) {
                            return;
                        }

                        if (passphrase.length() < 8) {
                            QMessageBox::warning(
                                &dialog,
                                "Passphrase Too Short",
                                "Use a passphrase containing at "
                                "least 8 characters."
                            );

                            return;
                        }

                        bool confirmationAccepted = false;

                        const QString confirmation =
                            QInputDialog::getText(
                                &dialog,
                                "Confirm Wallet Passphrase",
                                "Enter the same passphrase again:",
                                QLineEdit::Password,
                                QString(),
                                &confirmationAccepted
                            );

                        if (!confirmationAccepted) {
                            return;
                        }

                        if (passphrase != confirmation) {
                            QMessageBox::warning(
                                &dialog,
                                "Passphrases Do Not Match",
                                "The two passphrases were different."
                            );

                            return;
                        }

                        const QMessageBox::StandardButton choice =
                            QMessageBox::warning(
                                &dialog,
                                "Encrypt CoolWallet",
                                "Encrypting the wallet will shut "
                                "down the Cool Coin node.\n\n"
                                "Write down the passphrase and keep "
                                "it somewhere safe. A forgotten "
                                "passphrase cannot be recovered.\n\n"
                                "Continue with wallet encryption?",
                                QMessageBox::Yes |
                                QMessageBox::No,
                                QMessageBox::No
                            );

                        if (choice != QMessageBox::Yes) {
                            return;
                        }

                        messageLabel->setText(
                            "Encrypting wallet..."
                        );

                        QApplication::processEvents();

                        const QString result =
                            runCoolCoinCli(
                                {
                                    "encryptwallet",
                                    passphrase
                                },
                                true
                            ).trimmed();

                        QMessageBox::information(
                            &dialog,
                            "Wallet Encryption Requested",
                            result.isEmpty()
                                ? QString(
                                    "The encryption command was "
                                    "submitted.\n\n"
                                    "Close CoolWallet, restart the "
                                    "node, and reopen CoolWallet."
                                )
                                : QString(
                                    "%1\n\n"
                                    "The Cool Coin node normally "
                                    "shuts down after encryption. "
                                    "Restart CoolWallet afterward."
                                ).arg(result)
                        );

                        refreshSecurityStatus();
                    }
                );

                connect(
                    unlockButton,
                    &QPushButton::clicked,
                    &dialog,
                    [
                        this,
                        &dialog,
                        refreshSecurityStatus,
                        unlockSeconds,
                        messageLabel
                    ]() {
                        bool accepted = false;

                        const QString passphrase =
                            QInputDialog::getText(
                                &dialog,
                                "Unlock CoolWallet",
                                "Wallet passphrase:",
                                QLineEdit::Password,
                                QString(),
                                &accepted
                            );

                        if (!accepted) {
                            return;
                        }

                        if (passphrase.isEmpty()) {
                            QMessageBox::warning(
                                &dialog,
                                "Unlock Wallet",
                                "Enter the wallet passphrase."
                            );

                            return;
                        }

                        messageLabel->setText(
                            "Unlocking wallet..."
                        );

                        QApplication::processEvents();

                        runCoolCoinCli(
                            {
                                "walletpassphrase",
                                passphrase,
                                QString::number(
                                    unlockSeconds->value()
                                )
                            },
                            true
                        );

                        /*
                         * Commands returning JSON null may produce no
                         * standard output, so verify success by reading
                         * getwalletinfo again.
                         */
                        refreshSecurityStatus();
                    }
                );

                connect(
                    lockButton,
                    &QPushButton::clicked,
                    &dialog,
                    [
                        this,
                        refreshSecurityStatus,
                        messageLabel
                    ]() {
                        messageLabel->setText(
                            "Locking wallet..."
                        );

                        QApplication::processEvents();

                        runCoolCoinCli(
                            {
                                "walletlock"
                            },
                            true
                        );

                        refreshSecurityStatus();
                    }
                );

                connect(
                    changePassphraseButton,
                    &QPushButton::clicked,
                    &dialog,
                    [
                        this,
                        &dialog,
                        refreshSecurityStatus,
                        messageLabel
                    ]() {
                        bool oldAccepted = false;

                        const QString oldPassphrase =
                            QInputDialog::getText(
                                &dialog,
                                "Change Wallet Passphrase",
                                "Current wallet passphrase:",
                                QLineEdit::Password,
                                QString(),
                                &oldAccepted
                            );

                        if (!oldAccepted) {
                            return;
                        }

                        bool newAccepted = false;

                        const QString newPassphrase =
                            QInputDialog::getText(
                                &dialog,
                                "New Wallet Passphrase",
                                "New wallet passphrase:",
                                QLineEdit::Password,
                                QString(),
                                &newAccepted
                            );

                        if (!newAccepted) {
                            return;
                        }

                        if (newPassphrase.length() < 8) {
                            QMessageBox::warning(
                                &dialog,
                                "Passphrase Too Short",
                                "Use a passphrase containing at "
                                "least 8 characters."
                            );

                            return;
                        }

                        bool confirmationAccepted = false;

                        const QString confirmation =
                            QInputDialog::getText(
                                &dialog,
                                "Confirm New Passphrase",
                                "Enter the new passphrase again:",
                                QLineEdit::Password,
                                QString(),
                                &confirmationAccepted
                            );

                        if (!confirmationAccepted) {
                            return;
                        }

                        if (newPassphrase != confirmation) {
                            QMessageBox::warning(
                                &dialog,
                                "Passphrases Do Not Match",
                                "The new passphrases were different."
                            );

                            return;
                        }

                        messageLabel->setText(
                            "Changing wallet passphrase..."
                        );

                        QApplication::processEvents();

                        runCoolCoinCli(
                            {
                                "walletpassphrasechange",
                                oldPassphrase,
                                newPassphrase
                            },
                            true
                        );

                        refreshSecurityStatus();
                    }
                );

                refreshSecurityStatus();
                dialog.exec();
            }
        );
    }

    QPushButton* settingsButton =
        findChild<QPushButton*>("settingsButton");

    if (settingsButton) {
        disconnect(
            settingsButton,
            nullptr,
            this,
            nullptr
        );

        connect(
            settingsButton,
            &QPushButton::clicked,
            this,
            [this]() {
                QDialog dialog(this);

                dialog.setWindowTitle(
                    "CoolWallet Settings"
                );

                dialog.resize(800, 650);
                dialog.setModal(true);

                dialog.setStyleSheet(
                    "QDialog {"
                    " background-color:#07111d;"
                    " color:#f0f7ff;"
                    "}"
                    "QLabel {"
                    " color:#f0f7ff;"
                    "}"
                    "QTextBrowser {"
                    " background-color:#0a1725;"
                    " color:#f0f7ff;"
                    " border:2px solid #18d6ff;"
                    " border-radius:10px;"
                    " padding:12px;"
                    " selection-background-color:#18d6ff;"
                    " selection-color:#07111d;"
                    "}"
                    "QPushButton {"
                    " min-height:38px;"
                    " padding:5px 14px;"
                    " color:#f0f7ff;"
                    " background-color:#10263a;"
                    " border:2px solid #18d6ff;"
                    " border-radius:8px;"
                    " font-weight:700;"
                    "}"
                    "QPushButton:hover {"
                    " background-color:#173854;"
                    "}"
                    "QPushButton:pressed {"
                    " background-color:#18d6ff;"
                    " color:#07111d;"
                    "}"
                );

                QVBoxLayout* mainLayout =
                    new QVBoxLayout(&dialog);

                mainLayout->setContentsMargins(
                    18,
                    18,
                    18,
                    18
                );

                mainLayout->setSpacing(12);

                QLabel* titleLabel =
                    new QLabel(
                        "⚙  COOLWALLET SETTINGS",
                        &dialog
                    );

                titleLabel->setStyleSheet(
                    "color:#18d6ff;"
                    "font-size:20px;"
                    "font-weight:800;"
                );

                QLabel* subtitleLabel =
                    new QLabel(
                        "Portable wallet, node and network "
                        "configuration",
                        &dialog
                    );

                subtitleLabel->setStyleSheet(
                    "color:#a8bdd0;"
                    "font-size:12px;"
                );

                QLabel* statusLabel =
                    new QLabel(
                        "Reading CoolWallet settings...",
                        &dialog
                    );

                statusLabel->setWordWrap(true);

                statusLabel->setStyleSheet(
                    "color:#a8bdd0;"
                    "font-size:12px;"
                );

                QTextBrowser* settingsBrowser =
                    new QTextBrowser(&dialog);

                settingsBrowser->setReadOnly(true);

                settingsBrowser->setTextInteractionFlags(
                    Qt::TextSelectableByMouse |
                    Qt::LinksAccessibleByMouse
                );

                QPushButton* openDataButton =
                    new QPushButton(
                        "Open Data Folder",
                        &dialog
                    );

                QPushButton* openConfigButton =
                    new QPushButton(
                        "Open Config Folder",
                        &dialog
                    );

                QPushButton* copyDiagnosticsButton =
                    new QPushButton(
                        "Copy Diagnostics",
                        &dialog
                    );

                QPushButton* refreshButton =
                    new QPushButton(
                        "Refresh",
                        &dialog
                    );

                QPushButton* closeButton =
                    new QPushButton(
                        "Close",
                        &dialog
                    );

                QHBoxLayout* firstButtonRow =
                    new QHBoxLayout();

                firstButtonRow->addWidget(openDataButton);
                firstButtonRow->addWidget(openConfigButton);
                firstButtonRow->addWidget(
                    copyDiagnosticsButton
                );

                QHBoxLayout* secondButtonRow =
                    new QHBoxLayout();

                secondButtonRow->addWidget(refreshButton);
                secondButtonRow->addStretch();
                secondButtonRow->addWidget(closeButton);

                mainLayout->addWidget(titleLabel);
                mainLayout->addWidget(subtitleLabel);
                mainLayout->addWidget(statusLabel);
                mainLayout->addWidget(settingsBrowser, 1);
                mainLayout->addLayout(firstButtonRow);
                mainLayout->addLayout(secondButtonRow);

                const QString applicationDirectory =
                    QCoreApplication::applicationDirPath();

                const QString portableDataDirectory =
                    QDir(applicationDirectory)
                        .filePath("data");

                const QString configurationPath =
                    QDir(portableDataDirectory)
                        .filePath("coolcoin.conf");

                QString* diagnosticsText =
                    new QString();

                connect(
                    &dialog,
                    &QDialog::finished,
                    &dialog,
                    [diagnosticsText]() {
                        delete diagnosticsText;
                    }
                );

                const auto refreshSettings =
                    [
                        this,
                        settingsBrowser,
                        statusLabel,
                        diagnosticsText,
                        applicationDirectory,
                        portableDataDirectory,
                        configurationPath
                    ]() {
                        statusLabel->setStyleSheet(
                            "color:#a8bdd0;"
                            "font-size:12px;"
                        );

                        statusLabel->setText(
                            "Reading CoolWallet settings..."
                        );

                        QApplication::processEvents();

                        QString rpcPort = "6463";
                        QString networkPort = "6464";
                        QString serverSetting = "Unknown";
                        QString listeningSetting = "Unknown";
                        QString transactionIndex = "Unknown";
                        QStringList configuredPeers;

                        QFile configurationFile(
                            configurationPath
                        );

                        if (
                            configurationFile.open(
                                QIODevice::ReadOnly |
                                QIODevice::Text
                            )
                        ) {
                            QTextStream stream(
                                &configurationFile
                            );

                            while (!stream.atEnd()) {
                                QString line =
                                    stream
                                        .readLine()
                                        .trimmed();

                                if (
                                    line.isEmpty() ||
                                    line.startsWith("#")
                                ) {
                                    continue;
                                }

                                const int separator =
                                    line.indexOf('=');

                                if (separator < 1) {
                                    continue;
                                }

                                const QString key =
                                    line
                                        .left(separator)
                                        .trimmed()
                                        .toLower();

                                const QString value =
                                    line
                                        .mid(separator + 1)
                                        .trimmed();

                                /*
                                 * Never display or copy authentication
                                 * information from coolcoin.conf.
                                 */
                                if (
                                    key == "rpcuser" ||
                                    key == "rpcpassword" ||
                                    key == "rpcauth"
                                ) {
                                    continue;
                                }

                                if (key == "rpcport") {
                                    rpcPort = value;
                                } else if (key == "port") {
                                    networkPort = value;
                                } else if (key == "server") {
                                    serverSetting =
                                        value == "1"
                                            ? "Enabled"
                                            : "Disabled";
                                } else if (key == "listen") {
                                    listeningSetting =
                                        value == "1"
                                            ? "Enabled"
                                            : "Disabled";
                                } else if (key == "txindex") {
                                    transactionIndex =
                                        value == "1"
                                            ? "Enabled"
                                            : "Disabled";
                                } else if (key == "addnode") {
                                    configuredPeers.append(
                                        value
                                    );
                                }
                            }

                            configurationFile.close();
                        }

                        QString walletName = "Not loaded";
                        QString walletFormat = "--";
                        QString blockHeight = "--";
                        QString headerHeight = "--";
                        QString connectionCount = "--";
                        QString nodeVersion = "--";
                        QString initialBlockDownload = "--";

                        const QString walletInfoJson =
                            runCoolCoinCli(
                                {
                                    "getwalletinfo"
                                },
                                true
                            ).trimmed();

                        if (!walletInfoJson.isEmpty()) {
                            const QJsonDocument walletDocument =
                                QJsonDocument::fromJson(
                                    walletInfoJson.toUtf8()
                                );

                            if (walletDocument.isObject()) {
                                const QJsonObject walletObject =
                                    walletDocument.object();

                                walletName =
                                    walletObject
                                        .value("walletname")
                                        .toString();

                                walletFormat =
                                    walletObject
                                        .value("format")
                                        .toString();
                            }
                        }

                        const QString blockchainInfoJson =
                            runCoolCoinCli(
                                {
                                    "getblockchaininfo"
                                }
                            ).trimmed();

                        if (!blockchainInfoJson.isEmpty()) {
                            const QJsonDocument chainDocument =
                                QJsonDocument::fromJson(
                                    blockchainInfoJson.toUtf8()
                                );

                            if (chainDocument.isObject()) {
                                const QJsonObject chainObject =
                                    chainDocument.object();

                                blockHeight =
                                    QString::number(
                                        chainObject
                                            .value("blocks")
                                            .toVariant()
                                            .toLongLong()
                                    );

                                headerHeight =
                                    QString::number(
                                        chainObject
                                            .value("headers")
                                            .toVariant()
                                            .toLongLong()
                                    );

                                initialBlockDownload =
                                    chainObject
                                        .value(
                                            "initialblockdownload"
                                        )
                                        .toBool()
                                        ? "Yes"
                                        : "No";
                            }
                        }

                        const QString networkInfoJson =
                            runCoolCoinCli(
                                {
                                    "getnetworkinfo"
                                }
                            ).trimmed();

                        if (!networkInfoJson.isEmpty()) {
                            const QJsonDocument networkDocument =
                                QJsonDocument::fromJson(
                                    networkInfoJson.toUtf8()
                                );

                            if (networkDocument.isObject()) {
                                const QJsonObject networkObject =
                                    networkDocument.object();

                                nodeVersion =
                                    networkObject
                                        .value("subversion")
                                        .toString();

                                connectionCount =
                                    QString::number(
                                        networkObject
                                            .value("connections")
                                            .toInt()
                                    );
                            }
                        }

                        QString peersHtml;

                        if (configuredPeers.isEmpty()) {
                            peersHtml =
                                "<span style='color:#888888;'>"
                                "No addnode entries found"
                                "</span>";
                        } else {
                            for (
                                const QString& peer :
                                configuredPeers
                            ) {
                                peersHtml +=
                                    QString(
                                        "<div style='"
                                        "color:#18d6ff;"
                                        "font-family:monospace;"
                                        "padding:2px 0;"
                                        "'>%1</div>"
                                    ).arg(
                                        peer.toHtmlEscaped()
                                    );
                            }
                        }

                        const QString html =
                            QString(
                                "<html>"
                                "<body style='"
                                "background:#0a1725;"
                                "color:#f0f7ff;"
                                "font-family:sans-serif;"
                                "'>"

                                "<div style='"
                                "color:#18d6ff;"
                                "font-size:15px;"
                                "font-weight:bold;"
                                "margin-bottom:10px;"
                                "'>PORTABLE FILE LOCATIONS</div>"

                                "<table width='100%' "
                                "cellpadding='6'>"
                                "<tr>"
                                "<td width='190' "
                                "style='color:#a8bdd0;'>"
                                "Application folder</td>"
                                "<td style='"
                                "color:white;"
                                "font-family:monospace;"
                                "'>%1</td>"
                                "</tr>"

                                "<tr>"
                                "<td style='color:#a8bdd0;'>"
                                "Data folder</td>"
                                "<td style='"
                                "color:white;"
                                "font-family:monospace;"
                                "'>%2</td>"
                                "</tr>"

                                "<tr>"
                                "<td style='color:#a8bdd0;'>"
                                "Configuration file</td>"
                                "<td style='"
                                "color:white;"
                                "font-family:monospace;"
                                "'>%3</td>"
                                "</tr>"
                                "</table>"

                                "<hr style='"
                                "border:0;"
                                "border-top:1px solid #27445d;"
                                "margin:14px 0;"
                                "'>"

                                "<div style='"
                                "color:#18d6ff;"
                                "font-size:15px;"
                                "font-weight:bold;"
                                "margin-bottom:10px;"
                                "'>WALLET STATUS</div>"

                                "<table width='100%' "
                                "cellpadding='6'>"
                                "<tr>"
                                "<td width='190' "
                                "style='color:#a8bdd0;'>"
                                "Active wallet</td>"
                                "<td style='color:#00ff66;'>"
                                "%4</td>"
                                "</tr>"

                                "<tr>"
                                "<td style='color:#a8bdd0;'>"
                                "Wallet format</td>"
                                "<td>%5</td>"
                                "</tr>"

                                "<tr>"
                                "<td style='color:#a8bdd0;'>"
                                "Block height</td>"
                                "<td>%6</td>"
                                "</tr>"

                                "<tr>"
                                "<td style='color:#a8bdd0;'>"
                                "Header height</td>"
                                "<td>%7</td>"
                                "</tr>"

                                "<tr>"
                                "<td style='color:#a8bdd0;'>"
                                "Initial block download</td>"
                                "<td>%8</td>"
                                "</tr>"

                                "<tr>"
                                "<td style='color:#a8bdd0;'>"
                                "Node connections</td>"
                                "<td>%9</td>"
                                "</tr>"

                                "<tr>"
                                "<td style='color:#a8bdd0;'>"
                                "Node version</td>"
                                "<td>%10</td>"
                                "</tr>"
                                "</table>"

                                "<hr style='"
                                "border:0;"
                                "border-top:1px solid #27445d;"
                                "margin:14px 0;"
                                "'>"

                                "<div style='"
                                "color:#18d6ff;"
                                "font-size:15px;"
                                "font-weight:bold;"
                                "margin-bottom:10px;"
                                "'>NETWORK SETTINGS</div>"

                                "<table width='100%' "
                                "cellpadding='6'>"
                                "<tr>"
                                "<td width='190' "
                                "style='color:#a8bdd0;'>"
                                "RPC port</td>"
                                "<td>%11</td>"
                                "</tr>"

                                "<tr>"
                                "<td style='color:#a8bdd0;'>"
                                "Network port</td>"
                                "<td>%12</td>"
                                "</tr>"

                                "<tr>"
                                "<td style='color:#a8bdd0;'>"
                                "RPC server</td>"
                                "<td>%13</td>"
                                "</tr>"

                                "<tr>"
                                "<td style='color:#a8bdd0;'>"
                                "Listening</td>"
                                "<td>%14</td>"
                                "</tr>"

                                "<tr>"
                                "<td style='color:#a8bdd0;'>"
                                "Transaction index</td>"
                                "<td>%15</td>"
                                "</tr>"
                                "</table>"

                                "<div style='"
                                "color:#a8bdd0;"
                                "margin-top:12px;"
                                "margin-bottom:5px;"
                                "'>Configured peers</div>"

                                "%16"

                                "<div style='"
                                "color:#70889d;"
                                "font-size:11px;"
                                "margin-top:18px;"
                                "'>"
                                "RPC authentication values are "
                                "intentionally hidden."
                                "</div>"

                                "</body>"
                                "</html>"
                            )
                                .arg(
                                    applicationDirectory
                                        .toHtmlEscaped()
                                )
                                .arg(
                                    portableDataDirectory
                                        .toHtmlEscaped()
                                )
                                .arg(
                                    configurationPath
                                        .toHtmlEscaped()
                                )
                                .arg(
                                    walletName.toHtmlEscaped()
                                )
                                .arg(
                                    walletFormat.toHtmlEscaped()
                                )
                                .arg(blockHeight)
                                .arg(headerHeight)
                                .arg(initialBlockDownload)
                                .arg(connectionCount)
                                .arg(
                                    nodeVersion.toHtmlEscaped()
                                )
                                .arg(rpcPort.toHtmlEscaped())
                                .arg(
                                    networkPort.toHtmlEscaped()
                                )
                                .arg(serverSetting)
                                .arg(listeningSetting)
                                .arg(transactionIndex)
                                .arg(peersHtml);

                        settingsBrowser->setHtml(html);

                        *diagnosticsText =
                            QString(
                                "CoolWallet Diagnostics\n"
                                "======================\n"
                                "Application folder: %1\n"
                                "Data folder: %2\n"
                                "Configuration file: %3\n"
                                "Wallet: %4\n"
                                "Wallet format: %5\n"
                                "Block height: %6\n"
                                "Header height: %7\n"
                                "Initial block download: %8\n"
                                "Connections: %9\n"
                                "Node version: %10\n"
                                "RPC port: %11\n"
                                "Network port: %12\n"
                                "RPC server: %13\n"
                                "Listening: %14\n"
                                "Transaction index: %15\n"
                                "Configured peers: %16\n"
                            )
                                .arg(applicationDirectory)
                                .arg(portableDataDirectory)
                                .arg(configurationPath)
                                .arg(walletName)
                                .arg(walletFormat)
                                .arg(blockHeight)
                                .arg(headerHeight)
                                .arg(initialBlockDownload)
                                .arg(connectionCount)
                                .arg(nodeVersion)
                                .arg(rpcPort)
                                .arg(networkPort)
                                .arg(serverSetting)
                                .arg(listeningSetting)
                                .arg(transactionIndex)
                                .arg(
                                    configuredPeers.isEmpty()
                                        ? "None"
                                        : configuredPeers.join(", ")
                                );

                        statusLabel->setStyleSheet(
                            "color:#00ff66;"
                            "font-size:12px;"
                            "font-weight:700;"
                        );

                        statusLabel->setText(
                            "Settings and node status updated."
                        );
                    };

                connect(
                    refreshButton,
                    &QPushButton::clicked,
                    &dialog,
                    refreshSettings
                );

                connect(
                    closeButton,
                    &QPushButton::clicked,
                    &dialog,
                    &QDialog::accept
                );

                connect(
                    openDataButton,
                    &QPushButton::clicked,
                    &dialog,
                    [
                        &dialog,
                        portableDataDirectory
                    ]() {
                        if (
                            !QDir(
                                portableDataDirectory
                            ).exists()
                        ) {
                            QMessageBox::warning(
                                &dialog,
                                "Open Data Folder",
                                QString(
                                    "The data folder does not "
                                    "exist:\n\n%1"
                                ).arg(
                                    portableDataDirectory
                                )
                            );

                            return;
                        }

                        QProcess::startDetached(
                            "xdg-open",
                            {
                                portableDataDirectory
                            }
                        );
                    }
                );

                connect(
                    openConfigButton,
                    &QPushButton::clicked,
                    &dialog,
                    [
                        &dialog,
                        configurationPath
                    ]() {
                        const QFileInfo configInfo(
                            configurationPath
                        );

                        if (!configInfo.exists()) {
                            QMessageBox::warning(
                                &dialog,
                                "Open Config Folder",
                                QString(
                                    "The configuration file does "
                                    "not exist:\n\n%1"
                                ).arg(configurationPath)
                            );

                            return;
                        }

                        QProcess::startDetached(
                            "xdg-open",
                            {
                                configInfo.absolutePath()
                            }
                        );
                    }
                );

                connect(
                    copyDiagnosticsButton,
                    &QPushButton::clicked,
                    &dialog,
                    [
                        diagnosticsText,
                        statusLabel
                    ]() {
                        QApplication
                            ::clipboard()
                            ->setText(
                                *diagnosticsText
                            );

                        statusLabel->setStyleSheet(
                            "color:#00ff66;"
                            "font-size:12px;"
                            "font-weight:700;"
                        );

                        statusLabel->setText(
                            "Safe diagnostics copied. RPC "
                            "credentials were not included."
                        );
                    }
                );

                refreshSettings();
                dialog.exec();
            }
        );
    }

    QPushButton* helpButton =
        findChild<QPushButton*>("helpButton");

    if (helpButton) {
        disconnect(
            helpButton,
            nullptr,
            this,
            nullptr
        );

        connect(
            helpButton,
            &QPushButton::clicked,
            this,
            [this]() {
                QDialog dialog(this);

                dialog.setWindowTitle(
                    "CoolWallet Help"
                );

                dialog.resize(900, 720);
                dialog.setModal(true);

                dialog.setStyleSheet(
                    "QDialog {"
                    " background-color:#07111d;"
                    " color:#f0f7ff;"
                    "}"
                    "QLabel {"
                    " color:#f0f7ff;"
                    "}"
                    "QTextBrowser {"
                    " background-color:#0a1725;"
                    " color:#f0f7ff;"
                    " border:2px solid #18d6ff;"
                    " border-radius:10px;"
                    " padding:14px;"
                    " selection-background-color:#18d6ff;"
                    " selection-color:#07111d;"
                    "}"
                    "QPushButton {"
                    " min-height:38px;"
                    " padding:5px 15px;"
                    " color:#f0f7ff;"
                    " background-color:#10263a;"
                    " border:2px solid #18d6ff;"
                    " border-radius:8px;"
                    " font-weight:700;"
                    "}"
                    "QPushButton:hover {"
                    " background-color:#173854;"
                    "}"
                    "QPushButton:pressed {"
                    " background-color:#18d6ff;"
                    " color:#07111d;"
                    "}"
                );

                QVBoxLayout* mainLayout =
                    new QVBoxLayout(&dialog);

                mainLayout->setContentsMargins(
                    18,
                    18,
                    18,
                    18
                );

                mainLayout->setSpacing(12);

                QLabel* titleLabel =
                    new QLabel(
                        "❓  COOLWALLET HELP CENTER",
                        &dialog
                    );

                titleLabel->setStyleSheet(
                    "color:#18d6ff;"
                    "font-size:21px;"
                    "font-weight:800;"
                );

                QLabel* subtitleLabel =
                    new QLabel(
                        "Learn how to use your Cool Coin wallet, "
                        "node and mining controls.",
                        &dialog
                    );

                subtitleLabel->setStyleSheet(
                    "color:#a8bdd0;"
                    "font-size:12px;"
                );

                QTextBrowser* helpBrowser =
                    new QTextBrowser(&dialog);

                helpBrowser->setOpenExternalLinks(true);

                helpBrowser->setTextInteractionFlags(
                    Qt::TextSelectableByMouse |
                    Qt::LinksAccessibleByMouse
                );

                const QString helpHtml =
                    QString(
                        "<html>"
                        "<body style='"
                        "background:#0a1725;"
                        "color:#f0f7ff;"
                        "font-family:sans-serif;"
                        "font-size:13px;"
                        "line-height:1.4;"
                        "'>"

                        "<div style='"
                        "background:#0d2031;"
                        "border:1px solid #18d6ff;"
                        "border-radius:8px;"
                        "padding:12px;"
                        "margin-bottom:14px;"
                        "'>"
                        "<span style='"
                        "color:#18d6ff;"
                        "font-size:16px;"
                        "font-weight:bold;"
                        "'>Getting Started</span>"
                        "<p>"
                        "CoolWallet automatically starts the Cool Coin "
                        "node, loads the <b>coolwallet</b> wallet and "
                        "connects to configured Cool Chain peers."
                        "</p>"
                        "<p>"
                        "Wait until the wallet shows <b>Connected</b> "
                        "and the block height matches the network before "
                        "sending funds or starting mining."
                        "</p>"
                        "</div>"

                        "<h2 style='color:#18d6ff;'>Dashboard</h2>"
                        "<p>"
                        "The dashboard shows your total balance, "
                        "available balance, immature mining rewards, "
                        "current block height, latest block, network "
                        "hashrate, circulating supply and next halving."
                        "</p>"

                        "<h2 style='color:#18d6ff;'>Send COOL</h2>"
                        "<ol>"
                        "<li>Click <b>Send</b>.</li>"
                        "<li>Enter a valid Cool Coin address.</li>"
                        "<li>Enter the amount of COOL.</li>"
                        "<li>Review the address and amount carefully.</li>"
                        "<li>Confirm the transaction.</li>"
                        "</ol>"
                        "<p style='color:#ffcc55;'>"
                        "Cool Coin transactions cannot be reversed after "
                        "they are broadcast."
                        "</p>"

                        "<h2 style='color:#18d6ff;'>Receive COOL</h2>"
                        "<ol>"
                        "<li>Click <b>Receive</b> or open the Address Book.</li>"
                        "<li>Create a new receiving address.</li>"
                        "<li>Add a useful label.</li>"
                        "<li>Copy the address and send it to the payer.</li>"
                        "</ol>"
                        "<p>"
                        "You may create multiple receiving addresses. "
                        "Using a separate address for each person or "
                        "purpose makes wallet activity easier to track."
                        "</p>"

                        "<h2 style='color:#18d6ff;'>Transaction History</h2>"
                        "<p>"
                        "The Transaction History panel shows received, "
                        "sent and mined transactions along with amounts, "
                        "dates and confirmation status."
                        "</p>"
                        "<p>"
                        "New transactions may show as unconfirmed until "
                        "they are included in a Cool Chain block."
                        "</p>"

                        "<h2 style='color:#18d6ff;'>Address Book</h2>"
                        "<p>"
                        "The Address Book lists wallet receiving "
                        "addresses, labels and received amounts. Use its "
                        "buttons to create, copy or refresh addresses."
                        "</p>"

                        "<h2 style='color:#18d6ff;'>Wallet Security</h2>"
                        "<p>"
                        "Use Wallet Security to encrypt, unlock, lock or "
                        "change the passphrase for the active wallet."
                        "</p>"
                        "<ul>"
                        "<li><b>Encrypt Wallet:</b> protects private keys "
                        "with a passphrase.</li>"
                        "<li><b>Unlock Wallet:</b> temporarily enables "
                        "operations requiring private keys.</li>"
                        "<li><b>Lock Wallet:</b> immediately ends the "
                        "temporary unlock period.</li>"
                        "<li><b>Change Passphrase:</b> replaces the current "
                        "wallet passphrase.</li>"
                        "</ul>"
                        "<p style='color:#ff6666;font-weight:bold;'>"
                        "A forgotten wallet passphrase cannot be recovered."
                        "</p>"

                        "<h2 style='color:#18d6ff;'>Mining</h2>"
                        "<p>"
                        "CoolWallet uses the included ckpool and CPU miner "
                        "to perform SHA-256d solo mining."
                        "</p>"
                        "<p>"
                        "The mining panel displays miner status, current "
                        "hashrate and mining activity. When your miner "
                        "finds a valid block, the block reward is sent to "
                        "the mining address configured for your wallet."
                        "</p>"
                        "<p style='color:#ffcc55;'>"
                        "Mining rewards remain immature until the required "
                        "number of Cool Chain confirmations has passed."
                        "</p>"

                        "<h2 style='color:#18d6ff;'>Settings</h2>"
                        "<p>"
                        "The Settings panel displays the active portable "
                        "folders, configuration path, wallet name, block "
                        "height, connection count, ports and configured peers."
                        "</p>"
                        "<p>"
                        "Copy Diagnostics creates a safe report that omits "
                        "RPC usernames, passwords and authentication values."
                        "</p>"

                        "<h2 style='color:#18d6ff;'>Portable Wallet Files</h2>"
                        "<p>"
                        "CoolWallet stores its portable blockchain, "
                        "configuration and wallets inside the folder "
                        "containing the application."
                        "</p>"
                        "<table width='100%' cellpadding='6' style='"
                        "border-collapse:collapse;"
                        "background:#0d2031;"
                        "'>"
                        "<tr>"
                        "<td style='color:#a8bdd0;'>Blockchain</td>"
                        "<td style='font-family:monospace;'>"
                        "data/blocks and data/chainstate"
                        "</td>"
                        "</tr>"
                        "<tr>"
                        "<td style='color:#a8bdd0;'>Configuration</td>"
                        "<td style='font-family:monospace;'>"
                        "data/coolcoin.conf"
                        "</td>"
                        "</tr>"
                        "<tr>"
                        "<td style='color:#a8bdd0;'>Wallet</td>"
                        "<td style='font-family:monospace;'>"
                        "data/wallets/coolwallet"
                        "</td>"
                        "</tr>"
                        "<tr>"
                        "<td style='color:#a8bdd0;'>Log</td>"
                        "<td style='font-family:monospace;'>"
                        "data/debug.log"
                        "</td>"
                        "</tr>"
                        "</table>"

                        "<h2 style='color:#18d6ff;'>Troubleshooting</h2>"
                        "<p><b>Wallet says Offline:</b></p>"
                        "<ul>"
                        "<li>Verify coolcoind is running.</li>"
                        "<li>Check that RPC port 6463 is listening.</li>"
                        "<li>Open Settings and confirm the data folder.</li>"
                        "<li>Review data/debug.log for errors.</li>"
                        "</ul>"

                        "<p><b>Wallet balance is missing:</b></p>"
                        "<ul>"
                        "<li>Confirm the coolwallet wallet is loaded.</li>"
                        "<li>Wait for blockchain synchronization.</li>"
                        "<li>Refresh the dashboard.</li>"
                        "</ul>"

                        "<p><b>No peers are connected:</b></p>"
                        "<ul>"
                        "<li>Verify network port 6464 is not blocked.</li>"
                        "<li>Confirm addnode entries in coolcoin.conf.</li>"
                        "<li>Check that other Cool Coin nodes are online.</li>"
                        "</ul>"

                        "<p><b>Mining shows no hashrate:</b></p>"
                        "<ul>"
                        "<li>Confirm ckpool is listening on port 3333.</li>"
                        "<li>Confirm minerd is running.</li>"
                        "<li>Check the mining logs for RPC or Stratum errors.</li>"
                        "</ul>"

                        "<hr style='"
                        "border:0;"
                        "border-top:1px solid #27445d;"
                        "margin:18px 0;"
                        "'>"

                        "<div style='text-align:center;'>"
                        "<span style='color:#18d6ff;font-weight:bold;'>"
                        "Cool-Coin.org"
                        "</span><br>"
                        "<span style='color:#70889d;font-size:11px;'>"
                        "Cool vibes and lucky times."
                        "</span>"
                        "</div>"

                        "</body>"
                        "</html>"
                    );

                helpBrowser->setHtml(helpHtml);

                QPushButton* copySupportButton =
                    new QPushButton(
                        "Copy Support Information",
                        &dialog
                    );

                QPushButton* websiteButton =
                    new QPushButton(
                        "Open Cool-Coin.org",
                        &dialog
                    );

                QPushButton* closeButton =
                    new QPushButton(
                        "Close",
                        &dialog
                    );

                QLabel* statusLabel =
                    new QLabel(&dialog);

                statusLabel->setStyleSheet(
                    "color:#00ff66;"
                    "font-size:12px;"
                );

                QHBoxLayout* buttonLayout =
                    new QHBoxLayout();

                buttonLayout->addWidget(copySupportButton);
                buttonLayout->addWidget(websiteButton);
                buttonLayout->addStretch();
                buttonLayout->addWidget(closeButton);

                mainLayout->addWidget(titleLabel);
                mainLayout->addWidget(subtitleLabel);
                mainLayout->addWidget(helpBrowser, 1);
                mainLayout->addWidget(statusLabel);
                mainLayout->addLayout(buttonLayout);

                connect(
                    copySupportButton,
                    &QPushButton::clicked,
                    &dialog,
                    [
                        statusLabel
                    ]() {
                        const QString supportText =
                            QString(
                                "CoolWallet Support Information\n"
                                "==============================\n"
                                "Version: CoolCoin v27.1.0\n"
                                "Website: Cool-Coin.org\n"
                                "RPC port: 6463\n"
                                "Network port: 6464\n"
                                "Stratum port: 3333\n"
                                "Wallet name: coolwallet\n"
                                "Application folder: %1\n"
                            ).arg(
                                QCoreApplication
                                    ::applicationDirPath()
                            );

                        QApplication
                            ::clipboard()
                            ->setText(supportText);

                        statusLabel->setText(
                            "Support information copied."
                        );
                    }
                );

                connect(
                    websiteButton,
                    &QPushButton::clicked,
                    &dialog,
                    []() {
                        QDesktopServices::openUrl(
                            QUrl(
                                "https://Cool-Coin.org"
                            )
                        );
                    }
                );

                connect(
                    closeButton,
                    &QPushButton::clicked,
                    &dialog,
                    &QDialog::accept
                );

                dialog.exec();
            }
        );
    }

    QPushButton* aboutButton =
        findChild<QPushButton*>("aboutButton");

    if (aboutButton) {
        disconnect(
            aboutButton,
            nullptr,
            this,
            nullptr
        );

        connect(
            aboutButton,
            &QPushButton::clicked,
            this,
            [this]() {
                QDialog dialog(this);

                dialog.setWindowTitle(
                    "About CoolWallet"
                );

                dialog.resize(720, 620);
                dialog.setModal(true);

                dialog.setStyleSheet(
                    "QDialog {"
                    " background-color:#07111d;"
                    " color:#f0f7ff;"
                    "}"
                    "QLabel {"
                    " color:#f0f7ff;"
                    "}"
                    "QTextBrowser {"
                    " background-color:#0a1725;"
                    " color:#f0f7ff;"
                    " border:2px solid #18d6ff;"
                    " border-radius:10px;"
                    " padding:14px;"
                    " selection-background-color:#18d6ff;"
                    " selection-color:#07111d;"
                    "}"
                    "QPushButton {"
                    " min-height:38px;"
                    " padding:5px 15px;"
                    " color:#f0f7ff;"
                    " background-color:#10263a;"
                    " border:2px solid #18d6ff;"
                    " border-radius:8px;"
                    " font-weight:700;"
                    "}"
                    "QPushButton:hover {"
                    " background-color:#173854;"
                    "}"
                    "QPushButton:pressed {"
                    " background-color:#18d6ff;"
                    " color:#07111d;"
                    "}"
                );

                QVBoxLayout* mainLayout =
                    new QVBoxLayout(&dialog);

                mainLayout->setContentsMargins(
                    18,
                    18,
                    18,
                    18
                );

                mainLayout->setSpacing(12);

                QLabel* titleLabel =
                    new QLabel(
                        "❄  COOLWALLET",
                        &dialog
                    );

                titleLabel->setAlignment(
                    Qt::AlignCenter
                );

                titleLabel->setStyleSheet(
                    "color:#18d6ff;"
                    "font-size:30px;"
                    "font-weight:900;"
                    "padding:8px;"
                );

                QLabel* versionLabel =
                    new QLabel(
                        "CoolCoin version v27.1.0",
                        &dialog
                    );

                versionLabel->setAlignment(
                    Qt::AlignCenter
                );

                versionLabel->setStyleSheet(
                    "color:#00ff66;"
                    "font-size:15px;"
                    "font-weight:700;"
                );

                QTextBrowser* aboutBrowser =
                    new QTextBrowser(&dialog);

                aboutBrowser->setOpenExternalLinks(true);

                aboutBrowser->setTextInteractionFlags(
                    Qt::TextSelectableByMouse |
                    Qt::LinksAccessibleByMouse
                );

                const QString aboutHtml =
                    QString(
                        "<html>"
                        "<body style='"
                        "background:#0a1725;"
                        "color:#f0f7ff;"
                        "font-family:sans-serif;"
                        "font-size:13px;"
                        "'>"

                        "<div style='"
                        "text-align:center;"
                        "font-size:15px;"
                        "line-height:1.5;"
                        "margin-bottom:16px;"
                        "'>"
                        "Cool Coin is a SHA-256 proof-of-work "
                        "cryptocurrency running on the Cool Chain."
                        "</div>"

                        "<table width='100%' cellpadding='7' style='"
                        "background:#0d2031;"
                        "border-collapse:collapse;"
                        "'>"

                        "<tr>"
                        "<td width='210' style='color:#a8bdd0;'>"
                        "Currency name</td>"
                        "<td style='color:#18d6ff;font-weight:bold;'>"
                        "Cool Coin</td>"
                        "</tr>"

                        "<tr>"
                        "<td style='color:#a8bdd0;'>Currency symbol</td>"
                        "<td style='color:#18d6ff;font-weight:bold;'>"
                        "COOL</td>"
                        "</tr>"

                        "<tr>"
                        "<td style='color:#a8bdd0;'>Blockchain</td>"
                        "<td>Cool Chain</td>"
                        "</tr>"

                        "<tr>"
                        "<td style='color:#a8bdd0;'>Consensus</td>"
                        "<td>SHA-256 Proof of Work</td>"
                        "</tr>"

                        "<tr>"
                        "<td style='color:#a8bdd0;'>Maximum supply</td>"
                        "<td style='color:#00ff66;font-weight:bold;'>"
                        "64,000,000 COOL</td>"
                        "</tr>"

                        "<tr>"
                        "<td style='color:#a8bdd0;'>Initial block reward</td>"
                        "<td>64.00000000 COOL</td>"
                        "</tr>"

                        "<tr>"
                        "<td style='color:#a8bdd0;'>Halving interval</td>"
                        "<td>500,000 blocks</td>"
                        "</tr>"

                        "<tr>"
                        "<td style='color:#a8bdd0;'>RPC port</td>"
                        "<td>6463</td>"
                        "</tr>"

                        "<tr>"
                        "<td style='color:#a8bdd0;'>Network port</td>"
                        "<td>6464</td>"
                        "</tr>"

                        "<tr>"
                        "<td style='color:#a8bdd0;'>Mining protocol</td>"
                        "<td>Stratum on port 3333</td>"
                        "</tr>"

                        "<tr>"
                        "<td style='color:#a8bdd0;'>Wallet name</td>"
                        "<td>coolwallet</td>"
                        "</tr>"

                        "<tr>"
                        "<td style='color:#a8bdd0;'>Platform</td>"
                        "<td>Linux x86_64</td>"
                        "</tr>"

                        "</table>"

                        "<div style='"
                        "background:#092317;"
                        "border:1px solid #00cc55;"
                        "border-radius:8px;"
                        "padding:12px;"
                        "margin-top:16px;"
                        "text-align:center;"
                        "'>"
                        "<span style='"
                        "color:#00ff66;"
                        "font-weight:bold;"
                        "font-size:15px;"
                        "'>"
                        "100% Portable CoolWallet"
                        "</span><br><br>"
                        "Includes the Cool Coin node, command-line client, "
                        "blockchain files, wallet support, ckpool and "
                        "SHA-256d CPU mining tools."
                        "</div>"

                        "<div style='"
                        "text-align:center;"
                        "margin-top:18px;"
                        "'>"
                        "<span style='"
                        "color:#18d6ff;"
                        "font-weight:bold;"
                        "font-size:15px;"
                        "'>"
                        "Cool-Coin.org"
                        "</span>"
                        "</div>"

                        "<div style='"
                        "text-align:center;"
                        "color:#a8bdd0;"
                        "margin-top:14px;"
                        "'>"
                        "Copyright © 2009-2026<br>"
                        "The Cool-Coin developers"
                        "</div>"

                        "<div style='"
                        "text-align:center;"
                        "color:#70889d;"
                        "font-style:italic;"
                        "margin-top:14px;"
                        "'>"
                        "Cool vibes and lucky times."
                        "</div>"

                        "</body>"
                        "</html>"
                    );

                aboutBrowser->setHtml(aboutHtml);

                QLabel* buildLocationLabel =
                    new QLabel(
                        QString(
                            "Running from: %1"
                        ).arg(
                            QCoreApplication
                                ::applicationDirPath()
                        ),
                        &dialog
                    );

                buildLocationLabel->setWordWrap(true);
                buildLocationLabel->setTextInteractionFlags(
                    Qt::TextSelectableByMouse
                );

                buildLocationLabel->setStyleSheet(
                    "color:#70889d;"
                    "font-family:monospace;"
                    "font-size:10px;"
                );

                QPushButton* websiteButton =
                    new QPushButton(
                        "Open Cool-Coin.org",
                        &dialog
                    );

                QPushButton* copyDetailsButton =
                    new QPushButton(
                        "Copy Project Details",
                        &dialog
                    );

                QPushButton* closeButton =
                    new QPushButton(
                        "Close",
                        &dialog
                    );

                QLabel* statusLabel =
                    new QLabel(&dialog);

                statusLabel->setStyleSheet(
                    "color:#00ff66;"
                    "font-size:12px;"
                );

                QHBoxLayout* buttonLayout =
                    new QHBoxLayout();

                buttonLayout->addWidget(websiteButton);
                buttonLayout->addWidget(copyDetailsButton);
                buttonLayout->addStretch();
                buttonLayout->addWidget(closeButton);

                mainLayout->addWidget(titleLabel);
                mainLayout->addWidget(versionLabel);
                mainLayout->addWidget(aboutBrowser, 1);
                mainLayout->addWidget(buildLocationLabel);
                mainLayout->addWidget(statusLabel);
                mainLayout->addLayout(buttonLayout);

                connect(
                    websiteButton,
                    &QPushButton::clicked,
                    &dialog,
                    []() {
                        QDesktopServices::openUrl(
                            QUrl(
                                "https://Cool-Coin.org"
                            )
                        );
                    }
                );

                connect(
                    copyDetailsButton,
                    &QPushButton::clicked,
                    &dialog,
                    [
                        statusLabel
                    ]() {
                        const QString details =
                            QString(
                                "CoolWallet\n"
                                "CoolCoin version v27.1.0\n"
                                "Currency: Cool Coin (COOL)\n"
                                "Blockchain: Cool Chain\n"
                                "Consensus: SHA-256 Proof of Work\n"
                                "Maximum supply: 64,000,000 COOL\n"
                                "Initial block reward: 64.00000000 COOL\n"
                                "Halving interval: 500,000 blocks\n"
                                "RPC port: 6463\n"
                                "Network port: 6464\n"
                                "Stratum port: 3333\n"
                                "Website: Cool-Coin.org\n"
                                "Copyright (C) 2009-2026 "
                                "The Cool-Coin developers\n"
                                "Application folder: %1\n"
                            ).arg(
                                QCoreApplication
                                    ::applicationDirPath()
                            );

                        QApplication
                            ::clipboard()
                            ->setText(details);

                        statusLabel->setText(
                            "Project details copied."
                        );
                    }
                );

                connect(
                    closeButton,
                    &QPushButton::clicked,
                    &dialog,
                    &QDialog::accept
                );

                dialog.exec();
            }
        );
    }

    auto refreshDashboard = [this]() {
        const QString blockchainJson =
            runCoolCoinCli({"getblockchaininfo"});

        if (!blockchainJson.isEmpty()) {
            const QJsonObject object =
                QJsonDocument::fromJson(
                    blockchainJson.toUtf8()
                ).object();

            /*
             * Display the current best block hash beneath LATEST BLOCK.
             * The label shows a readable shortened hash while its tooltip
             * contains the complete 64-character block hash.
             */
            const QString latestBlockHash =
                object.value("bestblockhash").toString().trimmed();

            if (!latestBlockHash.isEmpty()) {
                QString displayedBlockHash = latestBlockHash;

                if (latestBlockHash.length() > 32) {
                    displayedBlockHash =
                        latestBlockHash.left(16) +
                        "..." +
                        latestBlockHash.right(16);
                }

                setLabelText(
                    "latestHashValue",
                    displayedBlockHash
                );

                if (
                    QLabel* latestHashLabel =
                        findChild<QLabel*>("latestHashValue")
                ) {
                    latestHashLabel->setToolTip(latestBlockHash);
                    latestHashLabel->setTextInteractionFlags(
                        Qt::TextSelectableByMouse
                    );
                    latestHashLabel->setAlignment(Qt::AlignCenter);
                    latestHashLabel->setStyleSheet(
                        "color:#44ccff;"
                        "font-family:monospace;"
                        "font-size:11pt;"
                        "font-weight:bold;"
                        "padding:4px;"
                    );
                }
            } else {
                setLabelText(
                    "latestHashValue",
                    "Waiting for block data..."
                );
            }

            /*
             * Read the current best block header so the LATEST BLOCK card
             * displays the real block timestamp and continuously updated age.
             */
            const QString dashboardBestBlockHash =
                object.value("bestblockhash").toString().trimmed();

            if (!dashboardBestBlockHash.isEmpty()) {
                const QString blockHeaderJson =
                    runCoolCoinCli(
                        {
                            "getblockheader",
                            dashboardBestBlockHash,
                            "true"
                        }
                    );

                const QJsonDocument blockHeaderDocument =
                    QJsonDocument::fromJson(
                        blockHeaderJson.toUtf8()
                    );

                if (
                    blockHeaderDocument.isObject() &&
                    blockHeaderDocument.object().contains("time")
                ) {
                    const QJsonObject blockHeaderObject =
                        blockHeaderDocument.object();

                    const qint64 blockTimestamp =
                        blockHeaderObject
                            .value("time")
                            .toVariant()
                            .toLongLong();

                    /*
                     * Display the number of transactions contained in the
                     * current best block. This includes the coinbase reward
                     * transaction.
                     */
                    const qint64 latestBlockTransactionCount =
                        blockHeaderObject
                            .value("nTx")
                            .toVariant()
                            .toLongLong();

                    setLabelText(
                        "latestTxValue",
                        QString::number(
                            latestBlockTransactionCount
                        )
                    );

                    if (
                        QLabel* latestTxLabel =
                            findChild<QLabel*>(
                                "latestTxValue"
                            )
                    ) {
                        latestTxLabel->setAlignment(
                            Qt::AlignLeft |
                            Qt::AlignVCenter
                        );

                        latestTxLabel->setStyleSheet(
                            "color:#18d6ff;"
                            "font-size:11pt;"
                            "font-weight:700;"
                        );

                        latestTxLabel->setToolTip(
                            latestBlockTransactionCount == 1
                                ? "1 transaction in the latest block"
                                : QString(
                                    "%1 transactions in the latest block"
                                ).arg(
                                    latestBlockTransactionCount
                                )
                        );
                    }

                    const QDateTime blockDateTime =
                        QDateTime::fromSecsSinceEpoch(
                            blockTimestamp
                        ).toLocalTime();

                    const qint64 ageSeconds =
                        blockDateTime.secsTo(
                            QDateTime::currentDateTime()
                        );

                    QString ageText;

                    if (ageSeconds < 0) {
                        ageText = "just now";
                    } else if (ageSeconds < 60) {
                        ageText =
                            QString("%1 sec ago")
                                .arg(ageSeconds);
                    } else if (ageSeconds < 3600) {
                        ageText =
                            QString("%1 min ago")
                                .arg(ageSeconds / 60);
                    } else if (ageSeconds < 86400) {
                        const qint64 hours =
                            ageSeconds / 3600;

                        const qint64 minutes =
                            (ageSeconds % 3600) / 60;

                        ageText =
                            QString("%1 hr %2 min ago")
                                .arg(hours)
                                .arg(minutes);
                    } else {
                        const qint64 days =
                            ageSeconds / 86400;

                        const qint64 hours =
                            (ageSeconds % 86400) / 3600;

                        ageText =
                            QString("%1 day%2 %3 hr ago")
                                .arg(days)
                                .arg(days == 1 ? "" : "s")
                                .arg(hours);
                    }

                    const QString displayedBlockTime =
                        QString("%1  (%2)")
                            .arg(
                                blockDateTime.toString(
                                    "MM/dd/yyyy hh:mm:ss AP"
                                )
                            )
                            .arg(ageText);

                    setLabelText(
                        "latestTimeValue",
                        displayedBlockTime
                    );

                    if (
                        QLabel* latestTimeLabel =
                            findChild<QLabel*>(
                                "latestTimeValue"
                            )
                    ) {
                        latestTimeLabel->setToolTip(
                            QString(
                                "Block timestamp: %1\n"
                                "Unix time: %2"
                            )
                                .arg(
                                    blockDateTime.toString(
                                        "dddd, MMMM d, yyyy "
                                        "hh:mm:ss AP"
                                    )
                                )
                                .arg(blockTimestamp)
                        );

                        latestTimeLabel->setAlignment(
                            Qt::AlignLeft |
                            Qt::AlignVCenter
                        );

                        latestTimeLabel->setStyleSheet(
                            "color:#18d6ff;"
                            "font-weight:700;"
                            "font-size:10pt;"
                        );
                    }
                } else {
                    setLabelText(
                        "latestTimeValue",
                        "Waiting for block time..."
                    );
                }
            } else {
                setLabelText(
                    "latestTimeValue",
                    "Waiting for block data..."
                );
            }

            /*
             * Update circulating supply from the live UTXO set.
             *
             * gettxoutsetinfo can be expensive, so it is only requested
             * when the chain reaches a different block height. During normal
             * five-second dashboard refreshes, the previously retrieved value
             * remains displayed.
             */
            const qint64 coolSupplyChainHeight =
                object
                    .value("blocks")
                    .toVariant()
                    .toLongLong();

            static qint64 lastCoolSupplyHeight = -1;
            static QString cachedCoolSupplyText;

            if (
                coolSupplyChainHeight != lastCoolSupplyHeight ||
                cachedCoolSupplyText.isEmpty()
            ) {
                const QString supplyJson =
                    runCoolCoinCli(
                        {
                            "gettxoutsetinfo"
                        }
                    );

                const QJsonDocument supplyDocument =
                    QJsonDocument::fromJson(
                        supplyJson.toUtf8()
                    );

                if (supplyDocument.isObject()) {
                    const QJsonObject supplyObject =
                        supplyDocument.object();

                    if (
                        supplyObject.contains("total_amount") &&
                        supplyObject.value("total_amount").isDouble()
                    ) {
                        const double circulatingSupply =
                            supplyObject
                                .value("total_amount")
                                .toDouble();

                        const QLocale coolNumberLocale(
                            QLocale::English,
                            QLocale::UnitedStates
                        );

                        cachedCoolSupplyText =
                            coolNumberLocale.toString(
                                circulatingSupply,
                                'f',
                                8
                            ) +
                            " COOL";

                        lastCoolSupplyHeight =
                            coolSupplyChainHeight;
                    }
                }
            }

            if (!cachedCoolSupplyText.isEmpty()) {
                setLabelText(
                    "circulatingSupplyValue",
                    cachedCoolSupplyText
                );

                if (
                    QLabel* circulatingSupplyLabel =
                        findChild<QLabel*>(
                            "circulatingSupplyValue"
                        )
                ) {
                    circulatingSupplyLabel->setAlignment(
                        Qt::AlignLeft |
                        Qt::AlignVCenter
                    );

                    circulatingSupplyLabel->setStyleSheet(
                        "color:#18d6ff;"
                        "font-size:13px;"
                        "font-weight:700;"
                    );

                    circulatingSupplyLabel->setToolTip(
                        QString(
                            "Live circulating supply from the "
                            "Cool Chain UTXO set at block %1."
                        ).arg(coolSupplyChainHeight)
                    );

                    circulatingSupplyLabel
                        ->setTextInteractionFlags(
                            Qt::TextSelectableByMouse
                        );
                }
            } else {
                setLabelText(
                    "circulatingSupplyValue",
                    "Loading supply..."
                );
            }

            setLabelText(
                "totalSupplyValue",
                "64,000,000 COOL"
            );

            /*
             * Cool Coin halving schedule:
             *
             * Initial reward: 64 COOL
             * Halving interval: 500,000 blocks
             *
             * This displays how many blocks remain until the next
             * scheduled reward halving.
             */
            const qint64 currentCoolBlockHeight =
                object
                    .value("blocks")
                    .toVariant()
                    .toLongLong();

            constexpr qint64 coolHalvingInterval =
                500000;

            const qint64 nextCoolHalvingHeight =
                (
                    (
                        currentCoolBlockHeight /
                        coolHalvingInterval
                    ) + 1
                ) *
                coolHalvingInterval;

            const qint64 blocksUntilCoolHalving =
                nextCoolHalvingHeight -
                currentCoolBlockHeight;

            const QLocale halvingNumberLocale(
                QLocale::English,
                QLocale::UnitedStates
            );

            const QString nextHalvingDisplay =
                QString("%1 block%2")
                    .arg(
                        halvingNumberLocale.toString(
                            blocksUntilCoolHalving
                        )
                    )
                    .arg(
                        blocksUntilCoolHalving == 1
                            ? ""
                            : "s"
                    );

            setLabelText(
                "nextHalvingValue",
                nextHalvingDisplay
            );

            if (
                QLabel* nextHalvingLabel =
                    findChild<QLabel*>(
                        "nextHalvingValue"
                    )
            ) {
                /*
                 * Cool Coin currently targets ten-minute blocks.
                 * This estimate is informational only because actual
                 * block discovery times vary with network hashrate.
                 */
                constexpr qint64 targetBlockSeconds =
                    10 * 60;

                const qint64 estimatedSeconds =
                    blocksUntilCoolHalving *
                    targetBlockSeconds;

                const double estimatedDays =
                    static_cast<double>(
                        estimatedSeconds
                    ) /
                    86400.0;

                const double estimatedYears =
                    estimatedDays /
                    365.25;

                QString estimatedTimeText;

                if (estimatedYears >= 1.0) {
                    estimatedTimeText =
                        QString(
                            "Approximately %1 years"
                        ).arg(
                            estimatedYears,
                            0,
                            'f',
                            1
                        );
                } else {
                    estimatedTimeText =
                        QString(
                            "Approximately %1 days"
                        ).arg(
                            estimatedDays,
                            0,
                            'f',
                            1
                        );
                }

                nextHalvingLabel->setAlignment(
                    Qt::AlignLeft |
                    Qt::AlignVCenter
                );

                nextHalvingLabel->setStyleSheet(
                    "color:#18d6ff;"
                    "font-size:13px;"
                    "font-weight:700;"
                );

                nextHalvingLabel->setToolTip(
                    QString(
                        "Current block: %1\n"
                        "Next halving block: %2\n"
                        "Blocks remaining: %3\n"
                        "%4 at the ten-minute target spacing."
                    )
                        .arg(
                            halvingNumberLocale.toString(
                                currentCoolBlockHeight
                            )
                        )
                        .arg(
                            halvingNumberLocale.toString(
                                nextCoolHalvingHeight
                            )
                        )
                        .arg(
                            halvingNumberLocale.toString(
                                blocksUntilCoolHalving
                            )
                        )
                        .arg(
                            estimatedTimeText
                        )
                );

                nextHalvingLabel
                    ->setTextInteractionFlags(
                        Qt::TextSelectableByMouse
                    );
            }

            setLabelText(
                "blockHeightValue",
                QString::number(
                    currentCoolBlockHeight
                )
            );

            setLabelText(
                "difficultyValue",
                QString::number(
                    object.value("difficulty").toDouble(),
                    'e',
                    8
                )
            );

            const qint64 blocks =
                object.value("blocks").toVariant().toLongLong();

            const qint64 headers =
                object.value("headers").toVariant().toLongLong();

            const bool initialBlockDownload =
                object.value("initialblockdownload").toBool();

            const double syncPercentage =
                headers > 0
                    ? (static_cast<double>(blocks) /
                       static_cast<double>(headers)) * 100.0
                    : 0.0;

            setLabelText(
                "syncBlocks",
                QString("%1 / %2").arg(blocks).arg(headers)
            );

            setLabelText(
                "syncPercent",
                QString::number(syncPercentage, 'f', 2) + "%"
            );

            setLabelText(
                "syncState",
                initialBlockDownload ? "Synchronizing" : "Synchronized"
            );
        } else {
            setLabelText("blockHeightValue", "Node offline");
            setLabelText("difficultyValue", "Node offline");
            setLabelText("syncBlocks", "0 / 0");
            setLabelText("syncPercent", "0.00%");
            setLabelText("syncState", "Node offline");
        }

        const QString networkJson =
            runCoolCoinCli({"getnetworkinfo"});

        if (!networkJson.isEmpty()) {
            const QJsonObject object =
                QJsonDocument::fromJson(
                    networkJson.toUtf8()
                ).object();

            const int connections =
                object.value("connections").toInt();

            const bool networkActive =
                object.value("networkactive").toBool();

            setLabelText(
                "peersValue",
                QString::number(connections)
            );

            setLabelText(
                "protocolValue",
                QString::number(
                    object.value("protocolversion").toInt()
                )
            );

            QLabel* statusLabel =
                findChild<QLabel*>("networkValue");

            QLabel* peerStatusLabel =
                findChild<QLabel*>("peerNetworkStatusValue");

            QString networkStatusText;

            if (!networkActive) {
                networkStatusText = "Disabled";
            } else if (connections > 0) {
                networkStatusText = "Connected";
            } else {
                networkStatusText = "Running";
            }

            if (statusLabel) {
                statusLabel->setText(networkStatusText);
                statusLabel->setStyleSheet(
                    networkActive
                        ? "color:#00ff66;font-weight:bold;"
                        : "color:#ff5555;font-weight:bold;"
                );
            }

            if (peerStatusLabel) {
                peerStatusLabel->setText(networkStatusText);
                peerStatusLabel->setStyleSheet(
                    networkActive
                        ? "color:#00ff66;font-weight:bold;"
                        : "color:#ff5555;font-weight:bold;"
                );
            }
        } else {
            setLabelText("peersValue", "0");
            setLabelText("protocolValue", "Node offline");

            QLabel* statusLabel =
                findChild<QLabel*>("networkValue");

            QLabel* peerStatusLabel =
                findChild<QLabel*>("peerNetworkStatusValue");

            if (statusLabel) {
                statusLabel->setText("Offline");
                statusLabel->setStyleSheet(
                    "color:#ff5555;font-weight:bold;"
                );
            }

            if (peerStatusLabel) {
                peerStatusLabel->setText("Offline");
                peerStatusLabel->setStyleSheet(
                    "color:#ff5555;font-weight:bold;"
                );
            }
        }


        const QString networkHashrateText =
            runCoolCoinCli({"getnetworkhashps"});

        if (!networkHashrateText.isEmpty()) {
            bool hashOk = false;
            const double hashesPerSecond =
                networkHashrateText.toDouble(&hashOk);

            if (hashOk) {
                QString formattedHashrate;

                if (hashesPerSecond >= 1000000000000.0) {
                    formattedHashrate =
                        QString::number(
                            hashesPerSecond / 1000000000000.0, 'f', 2) + " TH/s";
                } else if (hashesPerSecond >= 1000000000.0) {
                    formattedHashrate =
                        QString::number(
                            hashesPerSecond / 1000000000.0, 'f', 2) + " GH/s";
                } else if (hashesPerSecond >= 1000000.0) {
                    formattedHashrate =
                        QString::number(
                            hashesPerSecond / 1000000.0, 'f', 2) + " MH/s";
                } else if (hashesPerSecond >= 1000.0) {
                    formattedHashrate =
                        QString::number(
                            hashesPerSecond / 1000.0, 'f', 2) + " KH/s";
                } else {
                    formattedHashrate =
                        QString::number(
                            hashesPerSecond, 'f', 2) + " H/s";
                }

                setLabelText(
                    "networkHashrateValue",
                    formattedHashrate
                );
            }
        } else {
            setLabelText(
                "networkHashrateValue",
                "Node offline"
            );
        }

        const QString balancesJson =
            runCoolCoinCli({"getbalances"}, true);

        if (!balancesJson.isEmpty()) {
            const QJsonObject mine =
                QJsonDocument::fromJson(
                    balancesJson.toUtf8()
                ).object().value("mine").toObject();

            setLabelText(
                "availableValue",
                formatCoinAmount(
                    mine.value("trusted").toDouble()
                )
            );

            setLabelText(
                "pendingValue",
                formatCoinAmount(
                    mine.value("untrusted_pending").toDouble()
                )
            );

            setLabelText(
                "immatureValue",
                formatCoinAmount(
                    mine.value("immature").toDouble()
                )
            );

            double total =
                mine.value("trusted").toDouble() +
                mine.value("untrusted_pending").toDouble() +
                mine.value("immature").toDouble();

            setLabelText(
                "totalValue",
                formatCoinAmount(total)
            );

            QLabel* totalLabel =
                findChild<QLabel*>("totalValue");

            if (totalLabel) {
                totalLabel->setStyleSheet(
                    "color:#00ff66;"
                    "font-size:18pt;"
                    "font-weight:bold;"
                );
            }
        } else {
            setLabelText("availableValue", "Wallet offline");
            setLabelText("pendingValue", "Wallet offline");
            setLabelText("immatureValue", "Wallet offline");
        }

        const QString transactionsJson =
            runCoolCoinCli(
                {
                    "listtransactions",
                    "*",
                    "10",
                    "0",
                    "true"
                },
                true
            );

        QLabel* transactionsLabel =
            findChild<QLabel*>("transactionsValue");

        if (transactionsLabel) {
            transactionsLabel->setWordWrap(true);
            transactionsLabel->setTextFormat(Qt::RichText);
            transactionsLabel->setAlignment(
                Qt::AlignTop | Qt::AlignLeft
            );

            if (!transactionsJson.isEmpty()) {
                const QJsonArray transactions =
                    QJsonDocument::fromJson(
                        transactionsJson.toUtf8()
                    ).array();

                transactionsLabel->setText(
                    transactionHtml(transactions)
                );
            } else {
                transactionsLabel->setText(
                    "<span style='color:#ff7777;'>"
                    "Wallet offline or no transaction data."
                    "</span>"
                );
            }
        }
    };

    QTimer* timer = new QTimer(this);

    connect(
        timer,
        &QTimer::timeout,
        refreshDashboard
    );

    timer->start(5000);
    refreshDashboard();
}


void MainWindow::setLabelText(
    const QString &objectName,
    const QString &text)
{
    QLabel *label = findChild<QLabel*>(objectName);

    if (label) {
        label->setText(text);
    }
}

MainWindow::~MainWindow()
{
    delete ui;
}
