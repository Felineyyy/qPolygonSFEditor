#ifndef SF_VALUE_INPUT_DIALOG_HEADER
#define SF_VALUE_INPUT_DIALOG_HEADER

#include <QDialog>

namespace Ui {
class SFValueInputDialog;
}

//! SF数值输入对话框
class SFValueInputDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SFValueInputDialog(const QString& sfName, 
                               int selectedPointsCount,
                               double minValue, 
                               double maxValue,
                               QWidget* parent = nullptr);
    ~SFValueInputDialog();

    //! 获取输入的新值
    double getNewValue() const;

private slots:
    void onValueChanged(double value);

private:
    Ui::SFValueInputDialog* ui;
    double m_minValue;
    double m_maxValue;
};

#endif // SF_VALUE_INPUT_DIALOG_HEADER