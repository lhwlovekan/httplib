#pragma once
#include <sw/redis++/redis++.h>

#include <iostream>
#include <map>
#include <vector>

#include "Json.h"
using namespace std;
using namespace sw::redis;

class MyRedis {
public:
    // 数据库是否存在异常
    bool IsRun();
    // 数据库根据json文件内容初始化
    bool InitConnect(Json);
    // 数据库中是否存在此文件
    bool FileExists(string);
    // 数据库中查询大文件对应的小文件索引列表
    bool SelectMergeFile(string, vector<string> &);
    // 数据库中查询小文件内容
    bool SelectSmallFile(string, map<string, string> &);
    // 数据库中删除小文件内容
    bool DeleteSmallFile(string);
    // 数据库中删除大文件对应的小文件索引
    bool DeleteMergeFile(string, string);
    // 数据库中修改&插入小文件内容
    bool InsertSmallFile(string, map<string, string>);
    // 数据库中插入大文件索引
    bool InsertMergeFile(string, string);
    // 对需要对数据库进行修改的map进行校验，检验其是否符合要求
    bool CheckProperties(map<string, string>);

private:
    // 数据库连接
    Redis redis = Redis("tcp://127.0.0.0:6379");
};
