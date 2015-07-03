#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QtCore/QProcess>
#include <iostream>
#include <QtCore/QTextStream>
#include <QtCore/QIODevice>


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    code = "";
    outcode="";
    save = new Saveload();
    setWindowTitle("MyLang Complier");
    infile = "";
    outfile = "out.txt";
}

MainWindow::~MainWindow()
{
    delete ui;
}

Ui::MainWindow* MainWindow::getUI()
{
    return ui;
}

QString MainWindow::getCode()
{
    return outcode;
}

void MainWindow::setCode(QString input)
{
    outcode = input;
}

void MainWindow::on_actionSave_triggered()
{
    code = ui->In_plainTextEdit->toPlainText();
    save->setCode(code);
    save->do_file_SaveAs();
}

void MainWindow::on_actionLoad_triggered()
{
    save->do_file_Open();
    setCode(save->getCode());
    ui->Out_plainTextEdit->setPlainText(code); //将文件中的所有内容都写到文本编辑器中
}

void MainWindow::CreateProcess(QString processname,QString infile, QString outfile)
{
    QProcess *process=new QProcess();
    QStringList str;
    str.clear();
    str << infile << outfile;
    process->start(processname,str);
    process->waitForStarted();
    process->waitForFinished();
    QByteArray qb=process->readAll();
    QString str22(qb);
    QTextStream cout(stdout);
    cout<<str22<<endl;
}

void MainWindow::CreateProcess2(QString processname,QString infile, QString file, QString outfile)
{
    QProcess *process=new QProcess();
    QStringList str;
    str.clear();
    str << infile << file << outfile;
    process->start(processname,str);
    process->waitForStarted();
    process->waitForFinished();
    QByteArray qb=process->readAll();
    QString str22(qb);
    QTextStream cout(stdout);
    cout<<str22<<endl;
}

bool MainWindow::checkCode()
{
    QString newcode = "";
    newcode = ui->In_plainTextEdit->toPlainText();
    if (code!= newcode)
        return true;
    else return false;
}


void MainWindow::on_pushButton_1_clicked()//词法分析
{
    outfile = "lex.txt";
    if(checkCode() || !save->isSaved )
    {
        code = ui->In_plainTextEdit->toPlainText();
        save->setCode(code);
        save->do_file_SaveAs();
    }

    infile = save->curFile;
    CreateProcess("./Lex/lex",this->infile, this->outfile);
    save->do_file_Load(outfile);
    setCode(save->getCode());
    ui->Out_plainTextEdit->setPlainText(outcode); //将文件中的所有内容都写到文本编辑器中

}

void MainWindow::on_pushButton_2_clicked()
{
    outfile = "grammar.txt";
    if (checkCode() || !save->isSaved )
    {
        code = ui->In_plainTextEdit->toPlainText();
        save->setCode(code);
        save->do_file_SaveAs();
    }

    infile = save->curFile;
    CreateProcess2("./Semantic/dragon",this->infile, this->outfile, "tmp.txt");
    save->do_file_Load(outfile);
    setCode(save->getCode());
    ui->Out_plainTextEdit->setPlainText(outcode); //将文件中的所有内容都写到文本编辑器中

}

void MainWindow::on_pushButton_3_clicked()
{
    outfile = "asm.txt";
    if(checkCode() || !save->isSaved )
    {
        code = ui->In_plainTextEdit->toPlainText();
        save->setCode(code);
        save->do_file_SaveAs();
    }

    infile = save->curFile;
    CreateProcess2("./Semantic/dragon",this->infile,"grammar.txt",this->outfile);
    save->do_file_Load(outfile);
    setCode(save->getCode());
    ui->Out_plainTextEdit->setPlainText(outcode); //将文件中的所有内容都写到文本编辑器中

}

void MainWindow::on_pushButton_4_clicked()
{
    outfile = "result.txt";
    if(checkCode() || !save->isSaved )
    {
        code = ui->In_plainTextEdit->toPlainText();
        save->setCode(code);
        save->do_file_SaveAs();
    }
    system("lli asm.txt > result.txt");
    save->do_file_Load(outfile);
    setCode(save->getCode());
    ui->Out_plainTextEdit->setPlainText(outcode); //将文件中的所有内容都写到文本编辑器中

}
