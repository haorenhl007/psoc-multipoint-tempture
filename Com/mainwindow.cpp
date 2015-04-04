#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QSerialPort>
#include <QDebug>


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    serial = new QSerialPort(this);
    serial->setPortName("com4");
    serial->setBaudRate(QSerialPort::Baud115200);
    serial->setDataBits(QSerialPort::Data8);
    serial->setParity(QSerialPort::NoParity);
    serial->setStopBits(QSerialPort::OneStop);
    serial->setFlowControl(QSerialPort::NoFlowControl);
    serial->open(QIODevice::ReadWrite);
    //serial->write("kangqingfei");
    connect(serial,SIGNAL(readyRead()),this,SLOT(serialReceiver()));

}

MainWindow::~MainWindow()
{
    delete ui;
    serial->close();
}

void MainWindow::setLineEdit(int nodeID, QString text)
{
    switch (nodeID) {
    case 0:
    {
        ui->lineEdit->setText(text);
        break;
    }
    case 1:
    {
        ui->lineEdit_2->setText(text);
        break;
    }
    case 2:
    {
        ui->lineEdit_3->setText(text);
        break;
    }
    default:
        break;
    }
}

void MainWindow::serialReceiver()
{
    QString tmp = serial->readAll();
    buffer.append(tmp);
    //qDebug()<<buffer;
    if(tmp.endsWith('e'))
    {
        if(buffer[0]=='s')//以s打头以e结尾
        {
            QString node;
            switch(buffer[2].toLatin1()-48)//获取节点信息
            {
                case 0 :
                {
                    node = "Node 0: ";
                    break;
                }

                case 1 :
                {
                    node = "Node 1: ";
                    break;
                }
                case 2 :
                {
                    node = "Node 2: ";
                    break;
                }
                default :
                {
                    node = "Node 3: ";
                    break;
                }

            }
            if(buffer[1]=='t' && buffer.size()==7)//温度信息
            {
                QString temp;
                temp.append(buffer[3]);
                temp.append(buffer[4]);
                temp.append('.');
                temp.append(buffer[5]);
                setLineEdit(buffer[2].toLatin1()-48,temp);
                ui->textBrowser->append(node + temp + " °C");
                buffer.clear();
            }

            else if(buffer[1]=='h')//温度超警戒上限温度
            {
                setLineEdit(buffer[2].toLatin1()-48,"温度过高");
                ui->textBrowser->append(node + " Too High Tempture");
                buffer.clear();
            }

            else if(buffer[1]=='l')//温度超警戒下限温度
            {
                setLineEdit(buffer[2].toLatin1()-48,"温度过低");
                ui->textBrowser->append(node + " Too Low Tempture");
                buffer.clear();
            }

            else //未识别的命令
            {
                qDebug()<<buffer<<" undefine";
                buffer.clear();
            }

        }
        else if(buffer[0]!='s' || buffer.size()>20)//不以s打头 或者 不以s打头以e结尾或者太大
        {
            qDebug()<<buffer<<" clear error";
            buffer.clear();
        }
    }
    //ui->textBrowser->append(buffer);
}

void MainWindow::openCloseClick(bool type)
{
    char ch[] = "sc00e";
    QString flag;
    //打开
    if(type)
    {
        ch[2] = 'o';
        flag = "start";
    }
    else //关闭
    {
         ch[2] = 'c';
         flag = "stop";
    }

    QString goal;
    goal = ui->comboBox->currentText();
    if(goal == "All")
    {
        ch[3] = 'a';
        ui->lineEdit->setText(flag);
        ui->lineEdit_2->setText(flag);
        ui->lineEdit_3->setText(flag);
        ui->textBrowser->append("all " + flag);
    }
    else if(goal == "0")
    {
        ch[3] = '0';
        ui->lineEdit->setText(flag);
        ui->textBrowser->append("Node 0 " + flag);
    }
    else if(goal == "1")
    {
        ch[3] = '1';
        ui->lineEdit_2->setText(flag);
        ui->textBrowser->append("Node 1 " + flag);
    }
    else if(goal == "2")
    {
        ch[3] = '2';
        ui->lineEdit_3->setText(flag);
        ui->textBrowser->append("Node 2 " + flag);
    }

    serial->write(ch);
    qDebug()<<ch;

}


void MainWindow::on_pushButton_clicked()
{
    openCloseClick(true);
}

void MainWindow::on_pushButton_2_clicked()
{
    openCloseClick(false);
}

void MainWindow::on_pushButton_3_clicked()//设置门限
{
    double gate = ui->doubleSpinBox_2->value();
    QString str = QString::number(gate*10);
    QByteArray ba = str.toLatin1();
    char *ch = ba.data();
    char rdata[10];
    rdata[0] = 's';
    rdata[1] = 's';
    QString goal;
    int config = 0;
    goal = ui->comboBox_2->currentText();
    if(goal == "All")
    {
        rdata[3] = 'a';
        config += 3;
    }
    else if(goal == "0")
    {
        rdata[3] = '0';
        config += 0;
    }
    else if(goal == "1")
    {
        rdata[3] = '1';
        config += 1;
    }
    else if(goal == "2")
    {
        rdata[3] = '2';
        config += 2;
    }
    goal = ui->comboBox_3->currentText();
    if(goal == "上限")
    {
        rdata[2] = 'h';
        config += 0;
    }
    else if(goal == "下限")
    {
        rdata[2] = 'l';
        config += 10;
    }
    for(int i = 0; i < ba.length(); ++i)
    {
        rdata[i+4] = ch[i];
    }
    rdata[ba.length()+4] = 'e';
    rdata[ba.length()+5] = '\0';
    serial->write(rdata);
    switch(config)
    {
        case 0 :
        {
            ui->textBrowser->append("Node 0 High Gate Change to " + QString::number(gate));
            break;
        }
        case 1 :
        {
            ui->textBrowser->append("Node 1 High Gate Change To " + QString::number(gate));
            break;
        }
        case 2 :
        {
            ui->textBrowser->append("Node 2 High Gate Change To " + QString::number(gate));
            break;
        }
        case 3 :
        {
            ui->textBrowser->append("All Node High Gate Change To " + QString::number(gate));
            break;
        }
        case 10 :
        {
            ui->textBrowser->append("Node 0 Low Gate Change To " + QString::number(gate));
            break;
        }
        case 11 :
        {
            ui->textBrowser->append("Node 1 Low Gate Change To " + QString::number(gate));
            break;
        }
        case 12 :
        {
            ui->textBrowser->append("Node 2 Low Gate Change To " + QString::number(gate));
            break;
        }
        case 13 :
        {
            ui->textBrowser->append("All Node Low Gate Change To " + QString::number(gate));
            break;
        }
        default :
        {
            break;
        }


    }
    qDebug()<<rdata;

}

void MainWindow::on_pushButton_4_clicked()//设置周期
{
    double arg1 = ui->doubleSpinBox->value();
    QString str = QString::number(arg1*10);
    QByteArray ba = str.toLatin1();
    char *ch = ba.data();
    char sdata[10];
    sdata[0] = 's';
    sdata[1] = 'p';
    sdata[2] = 'p';
    QString goal;
    goal = ui->comboBox_4->currentText();
    if(goal == "All")
    {
        sdata[3] = 'a';
        ui->textBrowser->append("All Node Period Change To " + QString::number(arg1));
    }
    else if(goal == "0")
    {
        sdata[3] = '0';
        ui->textBrowser->append("Node 0 Period Change To " + QString::number(arg1));
    }
    else if(goal == "1")
    {
        sdata[3] = '1';
        ui->textBrowser->append("Node 1 Period Change To " + QString::number(arg1));
    }
    else if(goal == "2")
    {
        sdata[3] = '2';
        ui->textBrowser->append("Node 2 Period Change To " + QString::number(arg1));
    }
    for(int i = 0; i < ba.length(); ++i)
    {
        sdata[i+4] = ch[i];
    }
    sdata[ba.length()+4] = 'e';
    sdata[ba.length()+5] = '\0';
    serial->write(sdata);
    qDebug()<<sdata;
}

void MainWindow::on_comboBox_5_currentIndexChanged(const QString &arg1)
{
    serial->close();
    serial->setPortName(arg1);
    serial->open(QIODevice::ReadWrite);
}

