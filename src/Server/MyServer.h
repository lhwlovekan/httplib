#pragma once
#include <pwd.h>

#include <iostream>

#include "../include/httplib.h"
#include "HandlerFactory.h"
#include "Json.h"
using namespace std;
using namespace httplib;

class MyServer {
public:
    // 默认用户是root
    MyServer() {
        // 用户默认是此时电脑的用户
        passwd *pwd = getpwuid(getuid());
        runUser = pwd->pw_name;
    }
    // 用于将解析后的Json输入进行初始化
    bool SetParams(Json);
    // 服务器开始运行
    void run();
    // 用于返回MyServer的属性
    string GetrunUser() { return runUser; }
    string Getaddress() { return address; }
    int Getport() { return port; }
    int GetconcurrencyLimit() { return concurrencyLimit; }
    int GetkeepaliveLimit() { return keepaliveLimit; }
    int GetthreadpoolCount() { return threadpoolCount; }
    string Getpassword() { return password; }
    map<string, json> GetparamsList() { return paramsList; }

private:
    Server svr;
    string runUser;
    string address;
    int port;
    int concurrencyLimit;
    int keepaliveLimit;
    int threadpoolCount;
    string password;
    map<string, json> paramsList;
};
