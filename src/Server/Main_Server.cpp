#include "MyRedis.h"
#include "MyServer.h"

int main() {
    // 解析Json
    Json config;
    config.ServerJsonParse("../../config/config.json");
    // 建立服务器，并将解析后的Json传入，服务器设置参数并运行
    MyServer mysvr;
    if (!mysvr.SetParams(config)) {
        cout << "服务器初始化参数失败!" << endl;
        return 0;
    }
    mysvr.run();
    return 0;
}
