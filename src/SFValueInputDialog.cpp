#include "SFValueInputDialog.h"
#include "ui_SFValueInputDialog.h"

// Qt
#include <QPushButton>

SFValueInputDialog::SFValueInputDialog(const QString& sfName, 
                                     int selectedPointsCount,
                                     double minValue, 
                                     double maxValue,
                                     QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::SFValueInputDialog)
    , m_minValue(minValue)
    , m_maxValue(maxValue)
{
    ui->setupUi(this);
    
    // 设置窗口标题和模态
    setWindowTitle("Modify Scalar Field Values");
    setModal(true);
    
    // 设置信息标签
    ui->infoLabel->setText(QString("<b>Scalar Field:</b> %1<br>"
                                   "<b>Selected Points:</b> %2<br>"
                                   "<b>Current Range:</b> [%3, %4]")
                          .arg(sfName)
                          .arg(selectedPointsCount)
                          .arg(minValue, 0, 'f', 6)
                          .arg(maxValue, 0, 'f', 6));
    
    // 设置数值输入框
    ui->valueSpinBox->setRange(-1e9, 1e9);
    ui->valueSpinBox->setDecimals(6);
    ui->valueSpinBox->setSingleStep(0.1);
    
    // 设置默认值为当前范围的中间值
    double defaultValue = (minValue + maxValue) / 2.0;
    ui->valueSpinBox->setValue(defaultValue);
    
    // 连接信号
    connect(ui->valueSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &SFValueInputDialog::onValueChanged);
    
    // 设置OK按钮快捷键
    QPushButton* okButton = ui->buttonBox->button(QDialogButtonBox::Ok);
    if (okButton) {
        okButton->setDefault(true);
        okButton->setShortcut(QKeySequence(Qt::Key_Return));
    }
    
    // 设置焦点和选择
    ui->valueSpinBox->setFocus();
    ui->valueSpinBox->selectAll();
}

SFValueInputDialog::~SFValueInputDialog()
{
    delete ui;
}

double SFValueInputDialog::getNewValue() const
{
    return ui->valueSpinBox->value();
}

void SFValueInputDialog::onValueChanged(double value)
{
    // 可以留空或者添加简单的验证逻辑
    Q_UNUSED(value);
}