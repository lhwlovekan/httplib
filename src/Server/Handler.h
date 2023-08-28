#pragma once
#include <dirent.h>
#include <sys/types.h>

#include <algorithm>
#include <iostream>
#include <map>
#include <memory>

#include "../include/httplib.h"
#include "Action.h"
#include "Json.h"
#include "Lock.h"
#include "MyRedis.h"
using namespace std;
using namespace httplib;

// 合并后大文件名称
static const string MERGE_FILE_NAME = "LHW_SMALLFILELIST";

// 分块大小
static const size_t BUFFER_SIZE = 1024 * 1024;

// 区分定长和变长的文件大小值
static const size_t FIXED_CHUNKED_LENGTH = 1024 * 1024 * 1024;

// 区分小文件和大文件的文件大小值
static const size_t SMALL_BIG_LENGTH = 1024 * 1024;

enum class MethodType {
    CloseServer = 0,
    TestServer,
    SaveClientContent,
    SendClientContent,
    DeleteFile,
    MakeNewDir,
    SendDirList,
    ChangeCilentURL,
};

class Handler {
public:
    virtual void Method(const Request &, Response &){};
    virtual void MethodWithContentReader(const Request &, Response &, const ContentReader &content_reader){};

protected:
    // 生成日志
    json CreateLog(const Request &, string, string);
    // 打印日志
    void WriteLog(json);
    // 处理首个匹配
    string FirstMatch(string);
    // 字符串与枚举对应
    map<string, MethodType> MethodMap = {{"/close", MethodType::CloseServer},      {"/hi", MethodType::TestServer},
                                         {"/post", MethodType::SaveClientContent}, {"/get", MethodType::SendClientContent},
                                         {"/delete", MethodType::DeleteFile},      {"/mkdir", MethodType::MakeNewDir},
                                         {"/dir", MethodType::SendDirList},        {"/url", MethodType::ChangeCilentURL}};
};

class SpecialHandler : public Handler {
public:
    SpecialHandler(json params, string password, Server &svr, MyRedis &myredis) : myredis(myredis), password(password), svr(svr) {
        url = params["url"].get<string>();
        handlerName = params["handlerName"].get<string>();
        path = params["path"].get<string>();
    }
    void Method(const Request &, Response &) override;
    // 将成员函数嵌套进lambda函数中
    function<void(const Request &, Response &)> Method_ = [&](const Request &req, Response &res) { Method(req, res); };
    function<void(const Request &, Response &, const ContentReader &)> MethodWithContentReader_ =
        [&](const Request &req, Response &res, const ContentReader &content_reader) { MethodWithContentReader(req, res, content_reader); };

protected:
    // 测试请求
    void Test(const Request &req, Response &res, string &);
    // 关闭服务器
    void CloseServer(const Request &, Response &, string &);

private:
    string url;
    string handlerName;
    string path;
    MyRedis &myredis;
    // 服务器传入的权限密码
    string password;
    // 服务器
    Server &svr;
};

class FormatHandler : public Handler {
public:
    FormatHandler(json params, MyRedis &myredis) : myredis(myredis) {
        url = params["url"].get<string>();
        handlerName = params["handlerName"].get<string>();
        path = params["path"].get<string>();
    }
    void Method(const Request &, Response &) override;
    void MethodWithContentReader(const Request &, Response &, const ContentReader &) override;
    function<void(const Request &, Response &)> Method_ = [&](const Request &req, Response &res) { Method(req, res); };
    function<void(const Request &, Response &, const ContentReader &)> MethodWithContentReader_ =
        [&](const Request &req, Response &res, const ContentReader &content_reader) { MethodWithContentReader(req, res, content_reader); };

protected:
    // 保存客户端上传的文件
    void SaveClientContent(string, Response &, const ContentReader &, MyRedis &, string &);
    // 向客户端发送文件
    void SendClientContent(string, Response &, MyRedis &, string &);
    // 以定长格式向客户端发送文件
    void SendFixedContent(string, int, Response &, MyRedis &, string &);
    // 以变长格式向客户端发送文件
    void SendChunkedContent(string, Response &, MyRedis &, string &);
    // 以定长格式向客户端发送小文件
    void SendSmallFile(string, Response &, MyRedis &, string &);
    // 合并客户端发送的小文件
    void MergeSmallFile(string, string &, Response &, MyRedis &, string &);

private:
    string url;
    string handlerName;
    string path;
    MyRedis &myredis;
};

class DeleteHandler : public Handler {
public:
    DeleteHandler(json params, MyRedis &myredis) : myredis(myredis) {
        url = params["url"].get<string>();
        handlerName = params["handlerName"].get<string>();
        path = params["path"].get<string>();
    }
    void Method(const Request &, Response &) override;
    function<void(const Request &, Response &)> Method_ = [&](const Request &req, Response &res) { Method(req, res); };
    function<void(const Request &, Response &, const ContentReader &)> MethodWithContentReader_ =
        [&](const Request &req, Response &res, const ContentReader &content_reader) { MethodWithContentReader(req, res, content_reader); };

protected:
    // 删除客户端指定的文件
    void DeleteFile(string, Response &, MyRedis &, string &);

private:
    string url;
    string handlerName;
    string path;
    MyRedis &myredis;
};

class DirHandler : public Handler {
public:
    DirHandler(json params, MyRedis &myredis) : myredis(myredis) {
        url = params["url"].get<string>();
        handlerName = params["handlerName"].get<string>();
        path = params["path"].get<string>();
    }
    void Method(const Request &, Response &) override;
    function<void(const Request &, Response &)> Method_ = [&](const Request &req, Response &res) { Method(req, res); };
    function<void(const Request &, Response &, const ContentReader &)> MethodWithContentReader_ =
        [&](const Request &req, Response &res, const ContentReader &content_reader) { MethodWithContentReader(req, res, content_reader); };

protected:
    // 以json形式暂存目录
    json CreateDirList(string, MyRedis &);
    // 向客户端发送目录列表
    void SendDirList(string, Response &, MyRedis &, string &);
    // 更改客户端的目录
    void ChangeClientURL(string, Response &, string &);
    // 服务器目录下创建新文件夹
    void MakeNewDir(string, Response &, string &);

private:
    string url;
    string handlerName;
    string path;
    MyRedis &myredis;
};
