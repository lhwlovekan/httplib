#pragma once
#include <fstream>
#include <iostream>
#include <map>
#include <string>

#include "../include/nlohmann/json.hpp"
using namespace nlohmann;
using namespace std;

// 服务器json的结构体
struct ServerJson {
    string runUser;
    string address;
    int port;
    int concurrencyLimit;
    int keepaliveLimit;
    string password;
    map<string, json> paramsList;
};

// 数据库json的结构体
struct DBJson {
    string address;
    int port;
};

class Json {
public:
    // 用于进行服务器的Json解析
    void ServerJsonParse(string);
    // 用于进行数据库的Json解析
    void DBJsonParse(string);
    // 返回Json的各项属性
    ServerJson GetServerJson() { return serverjson; }
    DBJson GetDBJson() { return dbjson; }

private:
    ServerJson serverjson;
    DBJson dbjson;
};
