#include "customspinbox.h"
#include <QIntValidator>

// ============= CustomDoubleSpinBox =============

CustomDoubleSpinBox::CustomDoubleSpinBox(QWidget *parent)
    : QWidget(parent)
    , m_value(1.0)
    , m_minimum(0.1)
    , m_maximum(5.0)
    , m_singleStep(0.1)
    , m_decimals(1)
{
    m_layout = new QHBoxLayout(this);
    m_layout->setContentsMargins(0, 0, 0, 0);
    m_layout->setSpacing(6);

    // 创建减号按钮
    m_decrementButton = new QPushButton("◀", this);
    m_decrementButton->setFixedSize(28, 28);
    m_decrementButton->setObjectName("spinBoxButton");

    // 创建输入框
    m_lineEdit = new QLineEdit(this);
    m_lineEdit->setAlignment(Qt::AlignCenter);
    m_lineEdit->setFixedHeight(28);
    m_lineEdit->setMinimumWidth(60);
    QDoubleValidator *validator = new QDoubleValidator(m_minimum, m_maximum, m_decimals, this);
    m_lineEdit->setValidator(validator);

    // 创建加号按钮
    m_incrementButton = new QPushButton("▶", this);
    m_incrementButton->setFixedSize(28, 28);
    m_incrementButton->setObjectName("spinBoxButton");

    m_layout->addWidget(m_decrementButton);
    m_layout->addWidget(m_lineEdit, 1);
    m_layout->addWidget(m_incrementButton);

    connect(m_incrementButton, &QPushButton::clicked, this, &CustomDoubleSpinBox::onIncrementClicked);
    connect(m_decrementButton, &QPushButton::clicked, this, &CustomDoubleSpinBox::onDecrementClicked);
    connect(m_lineEdit, &QLineEdit::textChanged, this, &CustomDoubleSpinBox::onTextChanged);

    applyStyles();
    updateDisplay();
}

void CustomDoubleSpinBox::setValue(double value)
{
    if (value < m_minimum) value = m_minimum;
    if (value > m_maximum) value = m_maximum;

    if (qAbs(m_value - value) > 0.0001) {
        m_value = value;
        updateDisplay();
        emit valueChanged(m_value);
    }
}

void CustomDoubleSpinBox::setMinimum(double min)
{
    m_minimum = min;
    if (m_value < m_minimum) {
        setValue(m_minimum);
    }
}

void CustomDoubleSpinBox::setMaximum(double max)
{
    m_maximum = max;
    if (m_value > m_maximum) {
        setValue(m_maximum);
    }
}

void CustomDoubleSpinBox::setRange(double min, double max)
{
    m_minimum = min;
    m_maximum = max;
    if (m_value < m_minimum) {
        setValue(m_minimum);
    } else if (m_value > m_maximum) {
        setValue(m_maximum);
    }
}

void CustomDoubleSpinBox::setSuffix(const QString& suffix)
{
    m_suffix = suffix;
    updateDisplay();
}

void CustomDoubleSpinBox::setDecimals(int decimals)
{
    m_decimals = decimals;
    // 重新创建validator以更新小数位数
    QDoubleValidator *validator = new QDoubleValidator(m_minimum, m_maximum, decimals, this);
    m_lineEdit->setValidator(validator);
    updateDisplay();
}

void CustomDoubleSpinBox::onIncrementClicked()
{
    setValue(m_value + m_singleStep);
}

void CustomDoubleSpinBox::onDecrementClicked()
{
    setValue(m_value - m_singleStep);
}

void CustomDoubleSpinBox::onTextChanged()
{
    QString text = m_lineEdit->text();
    // 移除后缀
    if (!m_suffix.isEmpty() && text.endsWith(m_suffix)) {
        text.chop(m_suffix.length());
    }

    bool ok;
    double newValue = text.toDouble(&ok);
    if (ok) {
        if (newValue >= m_minimum && newValue <= m_maximum) {
            if (qAbs(m_value - newValue) > 0.0001) {
                m_value = newValue;
                emit valueChanged(m_value);
            }
        }
    }
}

void CustomDoubleSpinBox::updateDisplay()
{
    QString text = QString::number(m_value, 'f', m_decimals) + m_suffix;
    if (m_lineEdit->text() != text) {
        m_lineEdit->blockSignals(true);
        m_lineEdit->setText(text);
        m_lineEdit->blockSignals(false);
    }
}

void CustomDoubleSpinBox::applyStyles()
{
    QString style = R"(
        QPushButton#spinBoxButton {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                                       stop:0 #3498DB, stop:1 #2980B9);
            color: white;
            border: none;
            border-radius: 14px;
            font-size: 11px;
            padding: 0px;
        }

        QPushButton#spinBoxButton:hover {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                                       stop:0 #5DADE2, stop:1 #3498DB);
        }

        QPushButton#spinBoxButton:pressed {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                                       stop:0 #2980B9, stop:1 #21618C);
        }

        QLineEdit {
            background-color: white;
            border: 1px solid #D5DBDB;
            border-radius: 5px;
            padding: 5px 8px;
            color: #2C3E50;
            font-size: 12px;
            font-weight: 500;
        }

        QLineEdit:focus {
            border-color: #3498DB;
            border-width: 2px;
        }
    )";

    this->setStyleSheet(style);
}

// ============= CustomSpinBox =============

CustomSpinBox::CustomSpinBox(QWidget *parent)
    : QWidget(parent)
    , m_value(1)
    , m_minimum(1)
    , m_maximum(1000)
    , m_singleStep(1)
{
    m_layout = new QHBoxLayout(this);
    m_layout->setContentsMargins(0, 0, 0, 0);
    m_layout->setSpacing(6);

    // 创建减号按钮
    m_decrementButton = new QPushButton("◀", this);
    m_decrementButton->setFixedSize(28, 28);
    m_decrementButton->setObjectName("spinBoxButton");

    // 创建输入框
    m_lineEdit = new QLineEdit(this);
    m_lineEdit->setAlignment(Qt::AlignCenter);
    m_lineEdit->setFixedHeight(28);
    m_lineEdit->setMinimumWidth(80);
    QIntValidator *validator = new QIntValidator(m_minimum, m_maximum, this);
    m_lineEdit->setValidator(validator);

    // 创建加号按钮
    m_incrementButton = new QPushButton("▶", this);
    m_incrementButton->setFixedSize(28, 28);
    m_incrementButton->setObjectName("spinBoxButton");

    m_layout->addWidget(m_decrementButton);
    m_layout->addWidget(m_lineEdit, 1);
    m_layout->addWidget(m_incrementButton);

    connect(m_incrementButton, &QPushButton::clicked, this, &CustomSpinBox::onIncrementClicked);
    connect(m_decrementButton, &QPushButton::clicked, this, &CustomSpinBox::onDecrementClicked);
    connect(m_lineEdit, &QLineEdit::textChanged, this, &CustomSpinBox::onTextChanged);

    applyStyles();
    updateDisplay();
}

void CustomSpinBox::setValue(int value)
{
    if (value < m_minimum) value = m_minimum;
    if (value > m_maximum) value = m_maximum;

    if (m_value != value) {
        m_value = value;
        updateDisplay();
        emit valueChanged(m_value);
    }
}

void CustomSpinBox::setMinimum(int min)
{
    m_minimum = min;
    if (m_value < m_minimum) {
        setValue(m_minimum);
    }
}

void CustomSpinBox::setMaximum(int max)
{
    m_maximum = max;
    if (m_value > m_maximum) {
        setValue(m_maximum);
    }
}

void CustomSpinBox::setRange(int min, int max)
{
    m_minimum = min;
    m_maximum = max;
    if (m_value < m_minimum) {
        setValue(m_minimum);
    } else if (m_value > m_maximum) {
        setValue(m_maximum);
    }
}

void CustomSpinBox::setSuffix(const QString& suffix)
{
    m_suffix = suffix;
    updateDisplay();
}

void CustomSpinBox::onIncrementClicked()
{
    setValue(m_value + m_singleStep);
}

void CustomSpinBox::onDecrementClicked()
{
    setValue(m_value - m_singleStep);
}

void CustomSpinBox::onTextChanged()
{
    QString text = m_lineEdit->text();
    // 移除后缀
    if (!m_suffix.isEmpty() && text.endsWith(m_suffix)) {
        text.chop(m_suffix.length());
    }

    bool ok;
    int newValue = text.toInt(&ok);
    if (ok) {
        if (newValue >= m_minimum && newValue <= m_maximum) {
            if (m_value != newValue) {
                m_value = newValue;
                emit valueChanged(m_value);
            }
        }
    }
}

void CustomSpinBox::updateDisplay()
{
    QString text = QString::number(m_value) + m_suffix;
    if (m_lineEdit->text() != text) {
        m_lineEdit->blockSignals(true);
        m_lineEdit->setText(text);
        m_lineEdit->blockSignals(false);
    }
}

void CustomSpinBox::applyStyles()
{
    QString style = R"(
        QPushButton#spinBoxButton {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                                       stop:0 #3498DB, stop:1 #2980B9);
            color: white;
            border: none;
            border-radius: 14px;
            font-size: 11px;
            padding: 0px;
        }

        QPushButton#spinBoxButton:hover {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                                       stop:0 #5DADE2, stop:1 #3498DB);
        }

        QPushButton#spinBoxButton:pressed {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                                       stop:0 #2980B9, stop:1 #21618C);
        }

        QLineEdit {
            background-color: white;
            border: 1px solid #D5DBDB;
            border-radius: 5px;
            padding: 5px 8px;
            color: #2C3E50;
            font-size: 12px;
            font-weight: 500;
        }

        QLineEdit:focus {
            border-color: #3498DB;
            border-width: 2px;
        }
    )";

    this->setStyleSheet(style);
}
