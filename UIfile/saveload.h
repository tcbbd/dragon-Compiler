#ifndef SAVELOAD_H
#define SAVELOAD_H
#include<QWidget>
#include<QTextStream>
#include<QtGui>
#include<QMessageBox>
#include<QFileDialog>
#include<qstring.h>
class Saveload:public QWidget
{
private:
    QString code;//该数组用于记录步数
public:
    Saveload(QWidget *parent=0);
    QString getCode();
    void setCode(QString input);

    bool isSaved; //为true时标志文件已经保存，为false时标志文件尚未保存
    QString curFile; //保存当前文件的文件名

    void do_file_SaveOrNot(); //修改过的文件是否保存
    void do_file_Save(); //保存文件
    void do_file_SaveAs(); //文件另存为
    bool saveFile(const QString& fileName); //存储文件
    void do_file_Open(); //打开文件
    bool do_file_Load(const QString& fileName); //读取文件
};

#endif // SAVELOAD_H
