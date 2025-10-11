#ifndef CUSTOMSPINBOX_H
#define CUSTOMSPINBOX_H

#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QHBoxLayout>
#include <QDoubleValidator>

// 自定义SpinBox组件 - 现代化设计
class CustomDoubleSpinBox : public QWidget
{
    Q_OBJECT

public:
    explicit CustomDoubleSpinBox(QWidget *parent = nullptr);

    // 获取和设置值
    double value() const { return m_value; }
    void setValue(double value);

    // 设置范围
    void setMinimum(double min);
    void setMaximum(double max);
    void setRange(double min, double max);

    // 设置步进
    void setSingleStep(double step) { m_singleStep = step; }
    double singleStep() const { return m_singleStep; }

    // 设置后缀
    void setSuffix(const QString& suffix);

    // 设置小数位数
    void setDecimals(int decimals);

signals:
    void valueChanged(double value);

private slots:
    void onIncrementClicked();
    void onDecrementClicked();
    void onTextChanged();

private:
    void updateDisplay();
    void applyStyles();

    QLineEdit *m_lineEdit;
    QPushButton *m_incrementButton;
    QPushButton *m_decrementButton;
    QHBoxLayout *m_layout;

    double m_value;
    double m_minimum;
    double m_maximum;
    double m_singleStep;
    QString m_suffix;
    int m_decimals;
};

// 自定义整数SpinBox
class CustomSpinBox : public QWidget
{
    Q_OBJECT

public:
    explicit CustomSpinBox(QWidget *parent = nullptr);

    // 获取和设置值
    int value() const { return m_value; }
    void setValue(int value);

    // 设置范围
    void setMinimum(int min);
    void setMaximum(int max);
    void setRange(int min, int max);

    // 设置步进
    void setSingleStep(int step) { m_singleStep = step; }
    int singleStep() const { return m_singleStep; }

    // 设置后缀
    void setSuffix(const QString& suffix);

signals:
    void valueChanged(int value);

private slots:
    void onIncrementClicked();
    void onDecrementClicked();
    void onTextChanged();

private:
    void updateDisplay();
    void applyStyles();

    QLineEdit *m_lineEdit;
    QPushButton *m_incrementButton;
    QPushButton *m_decrementButton;
    QHBoxLayout *m_layout;

    int m_value;
    int m_minimum;
    int m_maximum;
    int m_singleStep;
    QString m_suffix;
};

#endif // CUSTOMSPINBOX_H
