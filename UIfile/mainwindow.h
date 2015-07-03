#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QApplication>
#include<QtWidgets>
#include"saveload.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    Ui::MainWindow *getUI();
    void setCode(QString);
    QString getCode();
    void CreateProcess(QString processname, QString infile, QString outfile);
    void CreateProcess2(QString processname, QString infile,QString file, QString outfile);
    bool checkCode();

private:
    Ui::MainWindow *ui;
    QString code;//用于记录代码
    QString outcode;
    Saveload* save;//用于文件存储和读取
    QString infile;
    QString outfile;


private slots:
    void on_actionSave_triggered();
    void on_actionLoad_triggered();
    //void on_actionRead_triggered();

    void on_pushButton_1_clicked();
    void on_pushButton_2_clicked();
    void on_pushButton_3_clicked();
    void on_pushButton_4_clicked();
};

#endif // MAINWINDOW_H
