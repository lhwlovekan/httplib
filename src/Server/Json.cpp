#include "Json.h"

void Json::ServerJsonParse(string path) {
    fstream file(path, ios::in);
    if (!file.is_open()) {
        cout << "服务器config文件加载失败!" << endl;
        return;
    }
    json config = json::parse(file);
    if (config.empty()) {
        cout << "服务器Json文件为空/Json文件加载失败!" << endl;
        return;
    }
    file.close();
    serverjson.runUser = config["runUser"].get<string>();
    serverjson.address = config["address"].get<string>();
    serverjson.port = config["port"].get<int>();
    serverjson.concurrencyLimit = config["concurrencyLimit"].get<int>();
    serverjson.keepaliveLimit = config["keepaliveLimit"].get<int>();
    serverjson.password = config["password"].get<string>();
    for (auto it = config["params"].begin(); it != config["params"].end(); it++) {
        serverjson.paramsList[(*it)["url"].get<string>()] = *it;
    }
}

void Json::DBJsonParse(string path) {
    // 数据库初始化
    fstream file(path, ios::in);
    if (!file.is_open()) {
        cout << "数据库的config文件加载失败!" << endl;
        return;
    }
    json config = json::parse(file);
    if (config.empty()) {
        cout << "数据库的Json文件为空/Json文件加载失败!" << endl;
        return;
    }
    file.close();
    dbjson.address = config["address"].get<string>();
    dbjson.port = config["port"].get<int>();
}
