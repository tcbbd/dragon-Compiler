
#include"saveload.h"
#include"mainwindow.h"
Saveload::Saveload(QWidget *parent)
{
    code = "";
    isSaved = false; //初始化文件为未保存过状态
    curFile = "未命名.cpl"; //初始化文件名为“未命名.cpl”
}
QString Saveload::getCode()
{
    return code;
}

void Saveload::setCode(QString input)
{
    code = input;
}

void Saveload::do_file_SaveOrNot() //弹出是否保存文件对话框
{
    if(isSaved)
        return;
    QMessageBox box; box.setWindowTitle("Warning");
    box.setIcon(QMessageBox::Warning);
    box.setText(curFile + " 尚未保存，是否保存？");
    box.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    if(box.exec() == QMessageBox::Yes) //如果选择保存文件，则执行保存操作
        do_file_Save();
}
void Saveload::do_file_Save() //保存文件
{
    if(isSaved){ //如果文件已经被保存过，直接保存文件
        saveFile(curFile);
    }
    else{
        do_file_SaveAs(); //如果文件是第一次保存，那么调用另存为
    }
}
void Saveload::do_file_SaveAs() //文件另存为
{
    QString fileName = QFileDialog::getSaveFileName(this,"另存为",curFile); //获得文件名
    if(!fileName.isEmpty()) //如果文件名不为空，则保存文件内容
    {
        saveFile(fileName);
    }
}
bool Saveload::saveFile(const QString& fileName) //保存文件内容，因为可能保存失败，所以具有返回值，来表明是否保存成功
{
    QFile file(fileName);
    QFileInfo temDir(fileName);
    QString filesuffix = temDir.suffix();//扩展名
    if(filesuffix!="cpl"){
        QMessageBox::warning(this,"保存文件",tr("无法保存这种格式的文件 %1：\n %2").arg(fileName).arg(file.errorString()));
        return false;
    }
    if(!file.open(QFile::WriteOnly | QFile::Text)) //以只写方式打开文件，如果打开失败则弹出提示框并返回
    {
        QMessageBox::warning(this,"保存文件", tr("无法保存文件 %1:\n %2").arg(fileName) .arg(file.errorString()));
        return false;
    } //%1,%2表示后面的两个arg参数的值
    QTextStream out(&file); //新建流对象，指向选定的文件
    //out << ui->textEdit->toPlainText(); //将文本编辑器里的内容以纯文本的形式输出到流对象中
    out << code;

    isSaved = true;
    curFile = QFileInfo(fileName).canonicalFilePath(); //获得文件的标准路径
    //setWindowTitle(curFile); //将窗口名称改为现在窗口的路径
    return true;
}
void Saveload::do_file_Open()//打开文件
{
    do_file_SaveOrNot();//是否需要保存现有文件
    QString fileName = QFileDialog::getOpenFileName(this,0,".cpl","*.cpl"); //获得要打开的文件的名字
    if(!fileName.isEmpty())//如果文件名不为空
    {
        QFile file(fileName);
        QFileInfo temDir(fileName);
        QString filesuffix = temDir.suffix();//扩展名
        if(filesuffix!="cpl"){
            QMessageBox::warning(this,"读取文件",tr("无法读取该形式的文件 %1:\n%2.").arg(fileName).arg(file.errorString()));
            return;
        }
        do_file_Load(fileName);
    }
    //ui->textEdit->setVisible(true);//文本编辑器可见
}
bool Saveload::do_file_Load(const QString& fileName) //读取文件
{
    QFile file(fileName);
    QFileInfo temDir(fileName);
    if(!file.open(QFile::ReadOnly | QFile::Text))
    {
        QMessageBox::warning(this,"读取文件",tr("无法读取文件 %1:\n%2.").arg(fileName).arg(file.errorString()));
        return false; //如果打开文件失败，弹出对话框，并返回
    }
    QTextStream in(&file);
    //ui->textEdit->setText(in.readAll()); //将文件中的所有内容都写到文本编辑器中
    code = in.readAll();
    //QString temp=in.readAll();
    //QStringList texts = temp.split("\n");
    return true;
}

