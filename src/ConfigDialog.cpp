#include "ConfigDialog.h"

#include "ConfigModel.h"
#include "HttpControlServer.h"
#include "PluginContext.h"
#include "Theme.h"

#include <QtCore/QTimer>
#include <QtGui/QCloseEvent>
#include <QtGui/QFontDatabase>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPlainTextEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QRadioButton>
#include <QtWidgets/QSlider>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QVBoxLayout>

void ConfigDialog::presentModal(QWidget* parent) {
    auto* existing = PluginContext::instance().configDialog();
    if (existing) {
        existing->raise();
        existing->activateWindow();
        return;
    }
    auto* dlg = new ConfigDialog(parent);
    PluginContext::instance().setConfigDialog(dlg);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->show();
    dlg->raise();
    dlg->activateWindow();
}

ConfigDialog::ConfigDialog(QWidget* parent) : QDialog(parent) {
    setWindowTitle(QStringLiteral("Speaker View — Settings"));
    setMinimumWidth(460);
    setMinimumHeight(460);

    auto* root = new QVBoxLayout(this);
    m_tabs = new QTabWidget(this);
    root->addWidget(m_tabs);

    auto* general = new QWidget(m_tabs);
    buildGeneralTab(general);
    m_tabs->addTab(general, QStringLiteral("General"));

    auto* sd = new QWidget(m_tabs);
    buildStreamDeckTab(sd);
    m_tabs->addTab(sd, QStringLiteral("Stream Deck / HTTP"));

    // Apply the dark palette / stylesheet *after* the widget tree is built
    // so every descendant inherits it. No-op on light-mode systems.
    sv_applyThemeToDialog(this);

    loadFromModel();
}

void ConfigDialog::buildGeneralTab(QWidget* tab) {
    auto* root = new QVBoxLayout(tab);

    auto* fadeRow = new QHBoxLayout();
    fadeRow->addWidget(new QLabel(QStringLiteral("Fade-out after talking:"), tab));
    m_fade = new QSlider(Qt::Horizontal, tab);
    m_fade->setRange(0, 100);  // tenths of a second, 0.0–10.0 s
    fadeRow->addWidget(m_fade, 1);
    m_fadeValue = new QLabel(tab);
    m_fadeValue->setMinimumWidth(50);
    m_fadeValue->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    fadeRow->addWidget(m_fadeValue);
    root->addLayout(fadeRow);

    auto* modeBox = new QGroupBox(QStringLiteral("Display mode"), tab);
    auto* modeLayout = new QVBoxLayout(modeBox);
    m_radioAll    = new QRadioButton(QStringLiteral("All speakers in my channel"), modeBox);
    m_radioLatest = new QRadioButton(QStringLiteral("Only the latest speaker"),   modeBox);
    modeLayout->addWidget(m_radioAll);
    modeLayout->addWidget(m_radioLatest);
    root->addWidget(modeBox);

    m_chkAlwaysOnTop   = new QCheckBox(QStringLiteral("Always on top"), tab);
    m_chkRememberFrame = new QCheckBox(QStringLiteral("Remember position and size between sessions"), tab);
    m_chkClickThrough  = new QCheckBox(QStringLiteral("Click-through (mouse events pass through)"), tab);
    m_chkShowChannel   = new QCheckBox(QStringLiteral("Show channel name per entry"), tab);
    m_chkShowSelf      = new QCheckBox(QStringLiteral("Also show my own voice"), tab);
    m_chkShowTrayIcon  = new QCheckBox(QStringLiteral("Show tray icon (Toggle / Quit TeamSpeak)"), tab);
    for (QCheckBox* c : {m_chkAlwaysOnTop, m_chkRememberFrame, m_chkClickThrough,
                         m_chkShowChannel, m_chkShowSelf, m_chkShowTrayIcon}) {
        root->addWidget(c);
    }

    root->addStretch();

    connect(m_fade, &QSlider::valueChanged, this, &ConfigDialog::onFadeSliderChanged);
    connect(m_radioAll,    &QRadioButton::toggled, this, &ConfigDialog::onModeAllToggled);
    connect(m_radioLatest, &QRadioButton::toggled, this, &ConfigDialog::onModeLatestToggled);
    for (QCheckBox* c : {m_chkAlwaysOnTop, m_chkRememberFrame, m_chkClickThrough,
                         m_chkShowChannel, m_chkShowSelf, m_chkShowTrayIcon}) {
        connect(c, &QCheckBox::toggled, this, &ConfigDialog::onToggleChanged);
    }
}

void ConfigDialog::buildStreamDeckTab(QWidget* tab) {
    auto* root = new QVBoxLayout(tab);

    auto* heading = new QLabel(
        QStringLiteral("Stream Deck / external control (HTTP, localhost only)"),
        tab);
    QFont f = heading->font();
    f.setBold(true);
    heading->setFont(f);
    root->addWidget(heading);

    m_chkHttpEnabled = new QCheckBox(QStringLiteral("Enable HTTP control server"), tab);
    root->addWidget(m_chkHttpEnabled);

    auto* portRow = new QHBoxLayout();
    portRow->addWidget(new QLabel(QStringLiteral("Port:"), tab));
    m_httpPort = new QSpinBox(tab);
    m_httpPort->setRange(1, 65535);
    // Locale-aware QSpinBox renders 30000 as "30.000" on de_DE; that
    // confuses users who read it as a decimal number. Drop the grouping
    // separator so the displayed value matches what curl / Stream Deck
    // expect verbatim.
    m_httpPort->setGroupSeparatorShown(false);
    portRow->addWidget(m_httpPort);
    m_httpApply = new QPushButton(QStringLiteral("Apply"), tab);
    portRow->addWidget(m_httpApply);
    portRow->addStretch();
    root->addLayout(portRow);

    m_httpStatus = new QLabel(tab);
    m_httpStatus->setStyleSheet(QStringLiteral("color: #888;"));
    root->addWidget(m_httpStatus);

    auto* docHeading = new QLabel(QStringLiteral("Endpoints"), tab);
    QFont fd = docHeading->font();
    fd.setBold(true);
    docHeading->setFont(fd);
    root->addWidget(docHeading);

    m_httpEndpoints = new QPlainTextEdit(tab);
    m_httpEndpoints->setReadOnly(true);
    m_httpEndpoints->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    root->addWidget(m_httpEndpoints, 1);

    connect(m_chkHttpEnabled, &QCheckBox::toggled,
            this, &ConfigDialog::onHttpEnabledChanged);
    connect(m_httpPort, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &ConfigDialog::onHttpPortEdited);
    connect(m_httpApply, &QPushButton::clicked,
            this, &ConfigDialog::onHttpApplyClicked);
}

void ConfigDialog::closeEvent(QCloseEvent* event) {
    PluginContext::instance().setConfigDialog(nullptr);
    QDialog::closeEvent(event);
}

void ConfigDialog::loadFromModel() {
    ConfigModel* cfg = PluginContext::instance().config();
    if (!cfg) return;
    QSignalBlocker b0(m_fade);
    QSignalBlocker b1(m_radioAll);
    QSignalBlocker b2(m_radioLatest);
    QSignalBlocker b3(m_chkHttpEnabled);
    QSignalBlocker b4(m_httpPort);

    m_fade->setValue(static_cast<int>(cfg->fadeOutSeconds() * 10.0));
    m_fadeValue->setText(QString::number(cfg->fadeOutSeconds(), 'f', 1) + QStringLiteral(" s"));

    m_radioAll->setChecked(cfg->displayMode() == DisplayMode::AllInChannel);
    m_radioLatest->setChecked(cfg->displayMode() == DisplayMode::LatestOnly);

    auto set = [](QCheckBox* c, bool v) { QSignalBlocker b(c); c->setChecked(v); };
    set(m_chkAlwaysOnTop,   cfg->alwaysOnTop());
    set(m_chkRememberFrame, cfg->rememberFrame());
    set(m_chkClickThrough,  cfg->clickThrough());
    set(m_chkShowChannel,   cfg->showChannel());
    set(m_chkShowSelf,      cfg->showSelf());
    set(m_chkShowTrayIcon,  cfg->showTrayIcon());

    m_chkHttpEnabled->setChecked(cfg->httpControlEnabled());
    m_httpPort->setValue(cfg->httpControlPort());

    refreshHttpStatusAndDocs();
}

void ConfigDialog::onFadeSliderChanged(int value) {
    double seconds = value / 10.0;
    m_fadeValue->setText(QString::number(seconds, 'f', 1) + QStringLiteral(" s"));
    if (auto* cfg = PluginContext::instance().config()) cfg->setFadeOutSeconds(seconds);
}

void ConfigDialog::onModeAllToggled(bool on) {
    if (!on) return;
    if (auto* cfg = PluginContext::instance().config()) cfg->setDisplayMode(DisplayMode::AllInChannel);
}

void ConfigDialog::onModeLatestToggled(bool on) {
    if (!on) return;
    if (auto* cfg = PluginContext::instance().config()) cfg->setDisplayMode(DisplayMode::LatestOnly);
}

void ConfigDialog::onToggleChanged() {
    ConfigModel* cfg = PluginContext::instance().config();
    if (!cfg) return;
    QCheckBox* s = qobject_cast<QCheckBox*>(sender());
    if (!s) return;
    bool on = s->isChecked();
    if      (s == m_chkAlwaysOnTop)   cfg->setAlwaysOnTop(on);
    else if (s == m_chkRememberFrame) cfg->setRememberFrame(on);
    else if (s == m_chkClickThrough)  cfg->setClickThrough(on);
    else if (s == m_chkShowChannel)   cfg->setShowChannel(on);
    else if (s == m_chkShowSelf)      cfg->setShowSelf(on);
    else if (s == m_chkShowTrayIcon)  cfg->setShowTrayIcon(on);
}

QString ConfigDialog::endpointsTextWithBase(const QString& base) const {
    return QStringLiteral(
        "GET %1/health\n"
        "    Liveness probe -> \"speakerview-control ok\"\n\n"
        "Microphone (input mute):\n"
        "  GET %1/mic/state\n"
        "  GET %1/mic/toggle\n\n"
        "Speaker (output mute):\n"
        "  GET %1/speaker/state\n"
        "  GET %1/speaker/toggle\n\n"
        "Away:\n"
        "  GET %1/away/state\n"
        "  GET %1/away/toggle\n\n"
        "Silent mode (mic + speaker + away in lockstep):\n"
        "  GET %1/silent/state\n"
        "  GET %1/silent/toggle\n\n"
        "Quit TeamSpeak (hard, no graceful disconnect):\n"
        "  GET %1/quit\n\n"
        "Responses are JSON like:\n"
        "  {\"muted\":true,\"connected\":true}\n"
        "  {\"muted\":null,\"connected\":false}\n\n"
        "Stream Deck setup:\n"
        "  Action: System -> Open\n"
        "  URL:    %1/mic/toggle (etc.)\n"
        "  Tick \"Open in background\" to suppress browser focus.\n"
    ).arg(base);
}

void ConfigDialog::refreshHttpStatusAndDocs() {
    ConfigModel* cfg = PluginContext::instance().config();
    if (!cfg) return;
    HttpControlServer* srv = PluginContext::instance().http();

    bool desiredEnabled = m_chkHttpEnabled->isChecked();
    int  fieldPort      = m_httpPort->value();
    bool pending = (desiredEnabled != cfg->httpControlEnabled())
                || (fieldPort      != cfg->httpControlPort());

    QString status;
    if (pending) {
        status = QStringLiteral("changes pending — click Apply to restart the server");
    } else if (!cfg->httpControlEnabled()) {
        status = QStringLiteral("disabled");
    } else if (srv && srv->running()) {
        status = QStringLiteral("running on port %1").arg(srv->port());
    } else {
        status = QStringLiteral("not running (port in use?)");
    }
    m_httpStatus->setText(status);

    QString base = QStringLiteral("http://127.0.0.1:%1").arg(cfg->httpControlPort());
    m_httpEndpoints->setPlainText(endpointsTextWithBase(base));
    m_httpPort->setEnabled(desiredEnabled);
}

void ConfigDialog::onHttpEnabledChanged() {
    // Don't push to the model immediately — leave pending until Apply,
    // mirroring the Mac UX so the user can change port + enabled together.
    refreshHttpStatusAndDocs();
}

void ConfigDialog::onHttpPortEdited() {
    refreshHttpStatusAndDocs();
}

void ConfigDialog::onHttpApplyClicked() {
    ConfigModel* cfg = PluginContext::instance().config();
    if (!cfg) return;
    int p = m_httpPort->value();
    if (p < 1 || p > 65535) {
        m_httpPort->setValue(cfg->httpControlPort());
        refreshHttpStatusAndDocs();
        return;
    }
    bool desiredEnabled = m_chkHttpEnabled->isChecked();
    if (cfg->httpControlEnabled() != desiredEnabled) cfg->setHttpControlEnabled(desiredEnabled);
    if (cfg->httpControlPort()    != p)              cfg->setHttpControlPort(p);
    refreshHttpStatusAndDocs();
}
