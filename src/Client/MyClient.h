#pragma once
#include <dirent.h>
#include <openssl/md5.h>

#include <algorithm>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>

#include "../include/httplib.h"
#include "../include/nlohmann/json.hpp"
#include "FontColor.h"
using namespace nlohmann;
using namespace std;
using namespace httplib;

// 分块传输大小
static const size_t BUFFER_SIZE = 1024 * 1024;

class MyClient {
public:
    MyClient() {
        // 初始目录为根目录
        RemoteURL = "/";
        // 本地目录为当前目录
        LocalURL = getcwd(NULL, 0);
        LocalURL += "/";
    }
    // 客户端创建时初始赋值
    void InitParams(string, string, int);
    // 用Get请求进行连接测试
    void TestGet();
    // 以定长传输格式上传、下载文件
    void FixedUpload(string, string, bool);
    void FixedDownload(string, string);
    // 以变长传输格式（chunked格式）上传、下载文件
    void ChunkedUpload(string, string, bool);
    void ChunkedDownload(string, string);
    // 递归上传文件夹
    void UploadDir(string, string, string, bool);
    // 递归下载文件夹
    void DownloadDir(string, string, string);
    // 删除文件
    void DeleteFile(string);
    // 递归删除文件夹
    void DeleteDir(string);
    // 创建新文件夹
    bool MakeNewDir(string);
    // 得到服务器对应目录的文件列表
    void GetRemoteDir(string);
    // 得到本地对应目录的文件列表
    void GetLocalDir(string);
    // 目录更改时根据命令对目录字符串进行操作
    bool DealURLCommand(string &, string);
    // 更改当前对应服务器目录
    void ChangeRemoteURL(string);
    // 更改当前对应客户端本地目录
    void ChangeLocalURL(string);
    // 校验上传的文件
    void CheckDocument(string, string, unsigned char *);
    // 关闭服务器
    void StopServer(string);
    // 返回客户端的属性
    string GetClientName() { return ClientName; }
    string GetLocalURL() { return LocalURL; }
    string GetRemoteURL() { return RemoteURL; }
    string Getaddress() { return cli->host(); }
    int Getport() { return cli->port(); }

private:
    // 客户端名称
    string ClientName;
    // 该客户端对应的本地的url
    string LocalURL;
    // 该客户端对应的服务器的url
    string RemoteURL;
    shared_ptr<Client> cli;
};
