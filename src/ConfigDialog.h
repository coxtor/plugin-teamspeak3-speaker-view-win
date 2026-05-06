#pragma once

#include <QtWidgets/QDialog>

class QSlider;
class QLabel;
class QCheckBox;
class QRadioButton;
class QSpinBox;
class QPushButton;
class QPlainTextEdit;
class QTabWidget;

class ConfigDialog : public QDialog {
    Q_OBJECT
public:
    static void presentModal(QWidget* parent = nullptr);

    explicit ConfigDialog(QWidget* parent = nullptr);

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    void onFadeSliderChanged(int value);
    void onModeAllToggled(bool on);
    void onModeLatestToggled(bool on);
    void onToggleChanged();
    void onHttpEnabledChanged();
    void onHttpPortEdited();
    void onHttpApplyClicked();

private:
    void buildGeneralTab(QWidget* tab);
    void buildStreamDeckTab(QWidget* tab);
    void loadFromModel();
    void refreshHttpStatusAndDocs();
    QString endpointsTextWithBase(const QString& base) const;

    QTabWidget* m_tabs = nullptr;

    // General tab
    QSlider*      m_fade = nullptr;
    QLabel*       m_fadeValue = nullptr;
    QRadioButton* m_radioAll = nullptr;
    QRadioButton* m_radioLatest = nullptr;
    QCheckBox*    m_chkAlwaysOnTop = nullptr;
    QCheckBox*    m_chkRememberFrame = nullptr;
    QCheckBox*    m_chkClickThrough = nullptr;
    QCheckBox*    m_chkShowChannel = nullptr;
    QCheckBox*    m_chkShowSelf = nullptr;
    QCheckBox*    m_chkShowTrayIcon = nullptr;

    // Stream Deck tab
    QCheckBox*      m_chkHttpEnabled = nullptr;
    QSpinBox*       m_httpPort = nullptr;
    QPushButton*    m_httpApply = nullptr;
    QLabel*         m_httpStatus = nullptr;
    QPlainTextEdit* m_httpEndpoints = nullptr;
};
