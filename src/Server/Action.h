#pragma once
#include <iostream>

#include "../include/httplib.h"
using namespace std;
using namespace httplib;

enum class ActionType {
    // 成功处理
    ConnectSuccess = 0,
    CloseSuccess,
    UploadSuccess,
    DownloadSuccess,
    DeleteSuccess,
    MakeDirSuccess,
    DirectoryListSuccess,
    ChangeURLSuccess,
    UploadSmallFileSuccess,
    DownloadSmallFileSuccess,
    // 错误处理
    WriteConflict,
    ReadConflict,
    DirectoryConflict,
    CloseError,
    FileExistsError,
    CreateFileError,
    OpenFileError,
    DataBaseError,
    DeleteDirectoryError,
    DeleteFileError,
    MakeDirError,
    DirectoryListError,
    ChangeURLError
};

static void ACTION_RETURN(Response &res, string &err, ActionType actiontype) {
    switch (static_cast<int>(actiontype)) {
    case static_cast<int>(ActionType::ConnectSuccess): {
        res.set_header("ACTION", "SUCCESS");
        res.set_header("RETURN", "连接成功!");
        break;
    }
    case static_cast<int>(ActionType::CloseSuccess): {
        res.set_header("ACTION", "SUCCESS");
        res.set_header("RETURN", "密码正确,服务器关闭成功!");
        break;
    }
    case static_cast<int>(ActionType::UploadSuccess): {
        res.set_header("ACTION", "SUCCESS");
        res.set_header("RETURN", "文件上传成功!");
        break;
    }
    case static_cast<int>(ActionType::DownloadSuccess): {
        res.set_header("ACTION", "SUCCESS");
        res.set_header("RETURN", "文件下载成功!");
        break;
    }
    case static_cast<int>(ActionType::DeleteSuccess): {
        res.set_header("ACTION", "SUCCESS");
        res.set_header("RETURN", "文件删除成功!");
        break;
    }
    case static_cast<int>(ActionType::MakeDirSuccess): {
        res.set_header("ACTION", "SUCCESS");
        res.set_header("RETURN", "文件夹创建成功!");
        break;
    }
    case static_cast<int>(ActionType::DirectoryListSuccess): {
        res.set_header("ACTION", "SUCCESS");
        res.set_header("RETURN", "目录获取成功!");
        break;
    }
    case static_cast<int>(ActionType::ChangeURLSuccess): {
        res.set_header("ACTION", "SUCCESS");
        res.set_header("RETURN", "远程目录更改成功!");
        break;
    }
    case static_cast<int>(ActionType::UploadSmallFileSuccess): {
        res.set_header("ACTION", "SUCCESS");
        res.set_header("RETURN", "小文件上传成功!");
        break;
    }
    case static_cast<int>(ActionType::DownloadSmallFileSuccess): {
        res.set_header("ACTION", "SUCCESS");
        res.set_header("RETURN", "小文件下载成功!");
        break;
    }
    case static_cast<int>(ActionType::WriteConflict): {
        err = "存在其他用户对该文件进行读/写操作!";
        res.set_header("ACTION", "FAILURE");
        res.set_header("RETURN", err);
        break;
    }
    case static_cast<int>(ActionType::ReadConflict): {
        err = "存在其他用户对该文件进行写操作!";
        res.set_header("ACTION", "FAILURE");
        res.set_header("RETURN", err);
        break;
    }
    case static_cast<int>(ActionType::DirectoryConflict): {
        err = "不能以文件形式下载目录!";
        res.set_header("ACTION", "FAILURE");
        res.set_header("RETURN", err);
        break;
    }
    case static_cast<int>(ActionType::CloseError): {
        err = "密码错误,没有权限关闭服务器!";
        res.set_header("ACTION", "SUCCESS");
        res.set_header("RETURN", err);
        break;
    }
    case static_cast<int>(ActionType::FileExistsError): {
        err = "文件不存在!";
        res.set_header("ACTION", "FAILURE");
        res.set_header("RETURN", err);
        break;
    }
    case static_cast<int>(ActionType::OpenFileError): {
        err = "在指定目录下打开文件失败!";
        res.set_header("ACTION", "FAILURE");
        res.set_header("RETURN", err);
        break;
    }
    case static_cast<int>(ActionType::DataBaseError): {
        err = "数据库功能异常!";
        res.set_header("ACTION", "FAILURE");
        res.set_header("RETURN", err);
        break;
    }
    case static_cast<int>(ActionType::DeleteDirectoryError): {
        err = "不能以文件形式删除非空目录!";
        res.set_header("ACTION", "FAILURE");
        res.set_header("RETURN", err);
        break;
    }
    case static_cast<int>(ActionType::DeleteFileError): {
        err = "文件删除失败!";
        res.set_header("ACTION", "FAILURE");
        res.set_header("RETURN", err);
        break;
    }
    case static_cast<int>(ActionType::MakeDirError): {
        err = "在指定目录下创建文件夹失败!";
        res.set_header("ACTION", "FAILURE");
        res.set_header("RETURN", err);
        break;
    }
    case static_cast<int>(ActionType::DirectoryListError): {
        err = "目录不存在!";
        res.set_header("ACTION", "FAILURE");
        res.set_header("RETURN", err);
        break;
    }
    case static_cast<int>(ActionType::ChangeURLError): {
        err = "远程目录更改失败!";
        res.set_header("ACTION", "FAILURE");
        res.set_header("RETURN", err);
        break;
    }
    default: {
        err = "回调缺失!";
        res.set_header("ACTION", "FAILURE");
        res.set_header("RETURN", err);
        break;
    }
    }
}
