#include <QApplication>
#include "mainwindow.h"

/**
 * @brief 程序入口函数
 * 
 * 该函数负责：
 * 1. 初始化Qt应用程序
 * 2. 创建并显示主窗口
 * 3. 启动事件循环
 * 
 * @param argc 命令行参数数量
 * @param argv 命令行参数数组
 * @return 程序退出码
 */
int main(int argc, char *argv[])
{
    // 创建Qt应用程序对象
    QApplication app(argc, argv);
    
    // 创建主窗口
    MainWindow window;
    
    // 显示主窗口
    window.show();
    
    // 启动事件循环，等待用户操作
    return app.exec();
} 