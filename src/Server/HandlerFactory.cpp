#include "HandlerFactory.h"

// 将字符串转化成字面值整数进行匹配
constexpr size_t HASH_STRING_PIECE(const char *string_piece, size_t hashNum = 0) {
    return *string_piece ? HASH_STRING_PIECE(string_piece + 1, (hashNum * 131) + *string_piece) : hashNum;
}

// 重载操作符
constexpr size_t operator"" _HASH(const char *string_pice, size_t) { return HASH_STRING_PIECE(string_pice); }

// 关闭:/stop
// 测试请求:/hi
// 上传:/post
// 定长下载:/fixed
// 变长下载:/chunked
// 删除:/delete
// 创建新文件夹:/mkdir
// 读取指定目录下的文件内容:/dir
// 改变客户端当前目录:/url
// 合并小文件:/small
AllHandler HandlerFactory::CreateHandler(string type) {
    AllHandler allhander;
    switch (HASH_STRING_PIECE(type.c_str())) {
    case "/stop"_HASH: {
        allhander.specialhandler = make_unique<SpecialHandler>(paramsList["/data"], password, svr, MethodType::CloseServer, myredis);
        break;
    }
    case "/hi"_HASH: {
        allhander.specialhandler = make_unique<SpecialHandler>(paramsList["/data"], password, svr, MethodType::TestServer, myredis);
        break;
    }
    case "/post"_HASH: {
        allhander.formathandler = make_unique<FormatHandler>(paramsList["/data"], MethodType::SaveClientContent, myredis);
        break;
    }
    case "/fixed"_HASH: {
        allhander.formathandler = make_unique<FormatHandler>(paramsList["/data"], MethodType::SendFixedContent, myredis);
        break;
    }
    case "/chunked"_HASH: {
        allhander.formathandler = make_unique<FormatHandler>(paramsList["/data"], MethodType::SendChunkedContent, myredis);
        break;
    }
    case "/small"_HASH: {
        allhander.formathandler = make_unique<FormatHandler>(paramsList["/data"], MethodType::MergeSmallFile, myredis);
        break;
    }
    case "/delete"_HASH: {
        allhander.deletehandler = make_unique<DeleteHandler>(paramsList["/data"], MethodType::DeleteFile, myredis);
        break;
    }
    case "/dir"_HASH: {
        allhander.dirhandler = make_unique<DirHandler>(paramsList["/data"], MethodType::SendDirList, myredis);
        break;
    }
    case "/url"_HASH: {
        allhander.dirhandler = make_unique<DirHandler>(paramsList["/data"], MethodType::ChangeCilentURL, myredis);
        break;
    }
    case "/mkdir"_HASH: {
        allhander.dirhandler = make_unique<DirHandler>(paramsList["/data"], MethodType::MakeNewDir, myredis);
        break;
    }
    default: {
        cout << "处理器创建失败!" << endl;
        break;
    }
    }
    return allhander;
}
