#include "MyServer.h"

bool MyServer::SetParams(Json config) {
    // 如果为root用户，则切换，否则不变
    if (runUser == "root") {
        runUser = config.GetServerJson().runUser;
        passwd *pw = getpwnam(runUser.c_str());
        if (pw == nullptr) {
            cout << "无法找到当前用户:" << runUser << endl;
            return false;
        }
        // 切换到指定用户的身份
        if (setuid(pw->pw_uid) != 0) {
            cout << "无法切换到当前用户:" << runUser << endl;
            return false;
        }
    }
    address = config.GetServerJson().address;
    port = config.GetServerJson().port;
    concurrencyLimit = config.GetServerJson().concurrencyLimit;
    keepaliveLimit = config.GetServerJson().keepaliveLimit;
    password = config.GetServerJson().password;
    // 指定keep-alive的次数
    svr.set_keep_alive_max_count(keepaliveLimit);
    // 指定线程池的线程个数
    svr.new_task_queue = [this] { return new ThreadPool(concurrencyLimit); };
    // 服务器读取处理器的参数配置
    paramsList = config.GetServerJson().paramsList;
    // 服务器设置连接和读写超时
    svr.set_read_timeout(5, 0);
    svr.set_write_timeout(5, 0);
    svr.set_idle_interval(0, 100000);
    return true;
}

// 关闭:/close
// 测试请求:/hi
// 上传:/post
// 下载:/get
// 删除:/delete
// 创建新文件夹:/mkdir
// 读取指定目录下的文件内容:/dir
// 改变客户端当前目录:/url
void MyServer::run() {
    cout << "服务器开始运行!" << endl;
    cout << "正在监听:" << address << ":" << to_string(port) << endl;
    // 数据库初始化
    Json config;
    config.DBJsonParse("../../config/db_config.json");
    MyRedis myredis;
    if (!myredis.InitConnect(config)) {
        cout << "数据库初始化失败!" << endl;
        return;
    }
    // 创建处理器
    HandlerFactory handlerFactory(svr, password, myredis, paramsList);
    AllHandler allhandler;
    handlerFactory.CreateHandler(allhandler);
    // 注册
    svr.Get("/close", allhandler.specialhandler->Method_);
    svr.Get("/hi", allhandler.specialhandler->Method_);
    svr.Post(R"(/post(\/(?:[^/]+\/)*)([^/]+))", allhandler.formathandler->MethodWithContentReader_);
    svr.Get(R"(/get(\/(?:[^/]+\/)*)([^/]+))", allhandler.formathandler->Method_);
    svr.Delete(R"(/delete(\/(?:[^/]+\/)*)([^/]+))", allhandler.deletehandler->Method_);
    svr.Get(R"(/mkdir(\/(?:[^/]+\/)*)([^/]+))", allhandler.dirhandler->Method_);
    svr.Get(R"(/dir(\/(?:[^/]+\/)*))", allhandler.dirhandler->Method_);
    svr.Get(R"(/url(\/(?:[^/]+\/)*))", allhandler.dirhandler->Method_);
    // 监听
    svr.listen(address, port);
}
