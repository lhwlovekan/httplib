#pragma once
#include <readline/history.h>
#include <readline/readline.h>

#include <iostream>
#include <map>

#include "../include/httplib.h"
#include "MyClient.h"
using namespace std;
using namespace httplib;

class CommandOperations {
public:
    // 主页面运行相关操作
    void MainRun();
    // 展示主页面帮助内容
    void MainHelp();
    // 创建客户端
    void CreateClient(string, string, int);
    // 切换客户端
    bool SwitchClient(string);
    // 删除客户端
    void DeleteClient(string);
    // 展示客户端列表
    void ShowClientList();
    // 接收操作并调用主页面函数
    void SwitchMain(string);
    // 客户端运行相关操作
    void ClientRun();
    // 展示客户端帮助内容
    void ClientHelp();
    // 接收操作并调用客户端函数
    void SwitchCommands(string);

protected:
    // 对带'/'的文件名进行处理
    bool DealURLCommand(string, string, pair<string, string>&);

private:
    // 当前客户端
    shared_ptr<MyClient> mycli;
    // 客户端信息列表，用客户端名称进行索引
    map<string, shared_ptr<MyClient>> ClientList;
};
