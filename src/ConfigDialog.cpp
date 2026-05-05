#include "ConfigDialog.h"

#include "ConfigModel.h"
#include "NativeStyle.h"
#include "PluginContext.h"

#include <QtCore/QTimer>
#include <QtGui/QCloseEvent>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QRadioButton>
#include <QtWidgets/QSlider>
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
    // See comment in OverlayWidget — override TS3's dark-Fusion defaults.
    applyNativeWindowsLook(this);

    setWindowTitle(QStringLiteral("Speaker View — Settings"));
    setMinimumWidth(380);

    auto* root = new QVBoxLayout(this);

    // Fade slider
    auto* fadeRow = new QHBoxLayout();
    fadeRow->addWidget(new QLabel(QStringLiteral("Fade-out after talking:"), this));
    m_fade = new QSlider(Qt::Horizontal, this);
    m_fade->setRange(0, 100);  // tenths of a second, 0.0–10.0 s
    fadeRow->addWidget(m_fade, 1);
    m_fadeValue = new QLabel(this);
    m_fadeValue->setMinimumWidth(50);
    m_fadeValue->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    fadeRow->addWidget(m_fadeValue);
    root->addLayout(fadeRow);

    // Display mode
    auto* modeBox = new QGroupBox(QStringLiteral("Display mode"), this);
    auto* modeLayout = new QVBoxLayout(modeBox);
    m_radioAll    = new QRadioButton(QStringLiteral("All speakers in my channel"), modeBox);
    m_radioLatest = new QRadioButton(QStringLiteral("Only the latest speaker"),   modeBox);
    modeLayout->addWidget(m_radioAll);
    modeLayout->addWidget(m_radioLatest);
    root->addWidget(modeBox);

    // Toggles
    m_chkAlwaysOnTop   = new QCheckBox(QStringLiteral("Always on top"), this);
    m_chkRememberFrame = new QCheckBox(QStringLiteral("Remember position and size between sessions"), this);
    m_chkClickThrough  = new QCheckBox(QStringLiteral("Click-through (mouse events pass through)"), this);
    m_chkShowChannel   = new QCheckBox(QStringLiteral("Show channel name per entry"), this);
    m_chkShowSelf      = new QCheckBox(QStringLiteral("Also show my own voice"), this);
    root->addWidget(m_chkAlwaysOnTop);
    root->addWidget(m_chkRememberFrame);
    root->addWidget(m_chkClickThrough);
    root->addWidget(m_chkShowChannel);
    root->addWidget(m_chkShowSelf);

    root->addStretch();

    auto* redistHint = new QLabel(
        QStringLiteral(
            "<span style='color:palette(mid);'>Requires the Microsoft Visual C++ "
            "2015–2022 Redistributable (x64) — "
            "<a href='https://aka.ms/vs/17/release/vc_redist.x64.exe'>download</a>."
            "</span>"),
        this);
    redistHint->setTextFormat(Qt::RichText);
    redistHint->setOpenExternalLinks(true);
    redistHint->setWordWrap(true);
    root->addWidget(redistHint);

    connect(m_fade, &QSlider::valueChanged, this, &ConfigDialog::onFadeSliderChanged);
    connect(m_radioAll,    &QRadioButton::toggled, this, &ConfigDialog::onModeAllToggled);
    connect(m_radioLatest, &QRadioButton::toggled, this, &ConfigDialog::onModeLatestToggled);
    for (QCheckBox* c : {m_chkAlwaysOnTop, m_chkRememberFrame, m_chkClickThrough,
                         m_chkShowChannel, m_chkShowSelf}) {
        connect(c, &QCheckBox::toggled, this, &ConfigDialog::onToggleChanged);
    }

    loadFromModel();
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
}
