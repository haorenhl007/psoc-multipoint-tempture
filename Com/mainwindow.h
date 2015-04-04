#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class QSerialPort;

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    void setLineEdit(int nodeID, QString text);

public slots:
    void serialReceiver();
    void openCloseClick(bool type);

private slots:
    void on_pushButton_clicked();

    void on_pushButton_2_clicked();

    void on_pushButton_3_clicked();

    void on_pushButton_4_clicked();

    void on_comboBox_5_currentIndexChanged(const QString &arg1);


private:
    Ui::MainWindow *ui;
    QSerialPort *serial;
    QString buffer;
};

#endif // MAINWINDOW_H
