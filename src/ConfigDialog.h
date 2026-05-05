#pragma once

#include <QtWidgets/QDialog>

class QSlider;
class QLabel;
class QCheckBox;
class QRadioButton;

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

private:
    void loadFromModel();

    QSlider*      m_fade = nullptr;
    QLabel*       m_fadeValue = nullptr;
    QRadioButton* m_radioAll = nullptr;
    QRadioButton* m_radioLatest = nullptr;
    QCheckBox*    m_chkAlwaysOnTop = nullptr;
    QCheckBox*    m_chkRememberFrame = nullptr;
    QCheckBox*    m_chkClickThrough = nullptr;
    QCheckBox*    m_chkShowChannel = nullptr;
    QCheckBox*    m_chkShowSelf = nullptr;
};
