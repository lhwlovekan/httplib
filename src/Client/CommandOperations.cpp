#include "CommandOperations.h"

// 将字符串转化成字面值整数进行匹配
constexpr size_t HASH_STRING_PIECE(const char *string_piece, size_t hashNum = 0) {
    return *string_piece ? HASH_STRING_PIECE(string_piece + 1, (hashNum * 131) + *string_piece) : hashNum;
}

// 重载操作符
constexpr size_t operator"" _HASH(const char *string_pice, size_t) { return HASH_STRING_PIECE(string_pice); }

// 返回最后不带有'/'的路径
bool CommandOperations::DealURLCommand(string dealurl, string targeturl, pair<string, string> &result) {
    vector<string> dealtemp;
    StrSplit(dealurl, '/', dealtemp);
    deque<string> dealfinal;
    // 源路径需要解析
    for (int i = 0; i < dealtemp.size(); i++) {
        if (dealtemp[i] == ".") {
            continue;
        } else if (dealtemp[i] == ".." && !dealfinal.empty()) {
            dealfinal.pop_back();
        } else if (dealtemp[i] == ".." && dealfinal.empty()) {
            PrintColorLine("根目录无法跳转到上级目录!", FontColor::LRed);
            return false;
        } else {
            dealfinal.push_back(dealtemp[i]);
        }
    }
    // 目标路径需要加名
    targeturl += dealfinal.back();
    result.first = "/";
    while (!dealfinal.empty()) {
        result.first += dealfinal.front() + "/";
        dealfinal.pop_front();
    }
    // 将最后的'/'去掉
    result.first.pop_back();
    result.second = targeturl;
    return true;
}

void CommandOperations::MainHelp() {
    PrintColorLine("|创建客户端| create [客户端名称] [ip地址] [端口号]", FontColor::LPurple);
    PrintColorLine("|切换客户端| user [客户端名称]", FontColor::LPurple);
    PrintColorLine("|删除客户端| delete [客户端名称]", FontColor::LPurple);
    PrintColorLine("|客户端列表| userlist", FontColor::LPurple);
    PrintColorLine("|退出该程序| quit", FontColor::LPurple);
}

void CommandOperations::MainRun() {
    while (true) {
        string cmd_header = AddColorStyle("lhwhttp", FontColor::LCyan, FontStyle::Bold);
        cmd_header += " > ";
        cmd_header += AddColorStyle("输入help打开帮助菜单", FontStyle::Italic);
        cmd_header += " :";
        string main_command;
        // 不跳过空格
        main_command = readline(cmd_header.c_str());
        if (main_command != "") {
            add_history(main_command.c_str());
        }
        SwitchMain(main_command);
    }
}

void CommandOperations::CreateClient(string ClientName, string address, int port) {
    if (ClientList.find(ClientName) != ClientList.end()) {
        PrintColorLine("该客户端已创建!", FontColor::LRed);
        return;
    }
    // 命令行指向新创建的客户端
    mycli = make_shared<MyClient>();
    mycli->InitParams(ClientName, address, port);
    ClientList[ClientName] = mycli;
    // 客户端运行
    system("clear");
    ClientRun();
}

bool CommandOperations::SwitchClient(string ClientName) {
    if (ClientList.empty()) {
        PrintColorLine("不存在已创建的客户端!", FontColor::LRed);
        return false;
    }
    if (ClientList.find(ClientName) == ClientList.end()) {
        PrintColorLine("不存在此客户端!", FontColor::LRed);
        return false;
    }
    mycli = ClientList[ClientName];
    return true;
}

void CommandOperations::DeleteClient(string ClientName) {
    if (ClientList.empty()) {
        PrintColorLine("不存在已创建的客户端!", FontColor::LRed);
        return;
    }
    if (ClientList.find(ClientName) == ClientList.end()) {
        PrintColorLine("不存在此客户端!", FontColor::LRed);
        return;
    }
    // 从列表中删除客户端信息
    ClientList.erase(ClientName);
    PrintColorLine("删除客户端[" + ClientName + "]成功!", FontColor::LGreen);
}

void CommandOperations::ShowClientList() {
    if (ClientList.empty()) {
        PrintColorLine("不存在已创建的客户端!", FontColor::LRed);
        return;
    }
    for (auto it = ClientList.begin(); it != ClientList.end(); it++) {
        PrintColorLine("[" + it->first + "@" + it->second->Getaddress() + ":" + to_string(it->second->Getport()) + "]", FontColor::LPurple);
    }
}

void CommandOperations::SwitchMain(string main_command) {
    vector<string> result;
    StrSplit(main_command, ' ', result);
    if (result.empty()) {
        PrintColorLine("指令不能为空!", FontColor::LRed);
        return;
    }
    switch (HASH_STRING_PIECE(result[0].c_str())) {
    case "help"_HASH: {
        MainHelp();
        break;
    }
    case "create"_HASH: {
        if (result.size() < 4) {
            PrintColorLine("客户端名称/ip地址/端口号不能为空!", FontColor::LRed);
            break;
        }
        if (result[3].find_first_not_of("0123456789") != string::npos || result[3].size() > 5) {
            PrintColorLine("端口号格式错误!", FontColor::LRed);
            break;
        }
        CreateClient(result[1], result[2], stoi(result[3]));
        break;
    }
    case "user"_HASH: {
        if (result.size() < 2) {
            PrintColorLine("客户端名称不能为空!", FontColor::LRed);
            break;
        };
        // 切换成功，则调用客户端运行函数
        if (SwitchClient(result[1])) {
            system("clear");
            ClientRun();
        }
        break;
    }
    case "delete"_HASH: {
        if (result.size() < 2) {
            PrintColorLine("客户端名称不能为空!", FontColor::LRed);
            break;
        }
        DeleteClient(result[1]);
        break;
    }
    case "userlist"_HASH: {
        ShowClientList();
        break;
    }
    case "quit"_HASH: {
        exit(1);
        break;
    }
    default: {
        PrintColorLine("输入的操作为非法操作!", FontColor::LRed);
        break;
    }
    }
}

void CommandOperations::ClientRun() {
    string localurl, remoteurl;
    while (true) {
        localurl = mycli->GetLocalURL();
        remoteurl = mycli->GetRemoteURL();
        localurl.resize(localurl.size() == 1 ? 1 : localurl.size() - 1);
        remoteurl.resize(remoteurl.size() == 1 ? 1 : remoteurl.size() - 1);
        string cmd_header = AddColorStyle(mycli->GetClientName() + " ", FontColor::LCyan, FontStyle::Bold);
        cmd_header += AddColorStyle(localurl, FontColor::LBlue, FontStyle::Default);
        cmd_header += ":";
        cmd_header += AddColorStyle(remoteurl, FontColor::LPurple, FontStyle::Default);
        cmd_header += " > ";
        cmd_header += AddColorStyle("输入help打开帮助菜单", FontStyle::Italic);
        cmd_header += " :";
        string client_command = readline(cmd_header.c_str());
        // 将非空指令加入终端历史记录
        if (client_command != "") {
            add_history(client_command.c_str());
        }
        // 返回主页会退出客户端的运行
        if (client_command == "quit") {
            system("clear");
            break;
        }
        SwitchCommands(client_command);
    }
}

void CommandOperations::ClientHelp() {
    PrintColorLine("|测试请求| test", FontColor::LPurple);
    PrintColorLine("|切换用户| user [用户名称]", FontColor::LPurple);
    PrintColorLine("|上传文件| put [-f/fixed] [-r/recursion] [-c/check] [本地文件/文件夹名]", FontColor::LPurple);
    PrintColorLine("|下载文件| get [-f/fixed] [-r/recursion] [远程文件/文件夹名]", FontColor::LPurple);
    PrintColorLine("|删除文件| delete [-r/recursion] [远程文件/文件夹名]", FontColor::LPurple);
    PrintColorLine("|显示文件| ls [-l/local]", FontColor::LPurple);
    PrintColorLine("|创文件夹| mkdir [文件夹名]", FontColor::LPurple);
    PrintColorLine("|改变目录| cd [-l/local] |回到上级目录-../| |进入下级目录-[文件夹名]| |跳转其他目录-[绝对路径]|", FontColor::LPurple);
    PrintColorLine("|关服务器| close [权限密码]", FontColor::LPurple);
    PrintColorLine("|返回主页| quit", FontColor::LPurple);
}

void CommandOperations::SwitchCommands(string client_command) {
    vector<string> result;
    StrSplit(client_command, ' ', result);
    if (result.empty()) {
        PrintColorLine("指令不能为空!", FontColor::LRed);
        return;
    }
    // 当前客户端对应的远程目录
    string remoteurl = mycli->GetRemoteURL();
    // 当前客户端对应的本地目录
    string localurl = mycli->GetLocalURL();
    switch (HASH_STRING_PIECE(result[0].c_str())) {
    case "help"_HASH: {
        ClientHelp();
        break;
    }
    case "test"_HASH: {
        mycli->TestGet();
        break;
    }
    case "user"_HASH: {
        if (result.size() < 2) {
            PrintColorLine("用户名不能为空!", FontColor::LRed);
            break;
        }
        SwitchClient(result[1]);
        break;
    }
    case "put"_HASH: {
        if (result.size() < 2) {
            PrintColorLine("本地文件/文件夹名不能为空!", FontColor::LRed);
            break;
        }
        bool is_illegal = false, is_fixed = false, is_recursion = false, is_check = false;
        pair<string, string> LRurl;
        if (result[result.size() - 1][0] == '/') {
            is_illegal = !DealURLCommand(result[result.size() - 1], remoteurl, LRurl);
        } else {
            is_illegal = !DealURLCommand(localurl + result[result.size() - 1], remoteurl, LRurl);
        }
        for (int i = 1; i < result.size() - 1; i++) {
            if (result[i] == "-r" || result[i] == "-recursion") {
                is_recursion = true;
            } else if (result[i] == "-f" || result[i] == "-fixed") {
                is_fixed = true;
            } else if (result[i] == "-c" || result[i] == "-check") {
                is_check = true;
            } else {
                PrintColorLine("上传文件配置错误!", FontColor::LRed);
                is_illegal = true;
            }
        }
        if (is_illegal) {
            break;
        }
        if (is_fixed && is_recursion) {
            mycli->UploadDir(LRurl.first + '/', LRurl.second, "fixed", is_check);
        } else if (is_recursion) {
            mycli->UploadDir(LRurl.first + '/', LRurl.second, "chunked", is_check);
        } else if (is_fixed) {
            mycli->FixedUpload(LRurl.first, LRurl.second, is_check);
        } else {
            mycli->ChunkedUpload(LRurl.first, LRurl.second, is_check);
        }
        break;
    }
    case "get"_HASH: {
        if (result.size() < 2) {
            PrintColorLine("远程文件/文件夹名不能为空!", FontColor::LRed);
            break;
        }
        bool is_illegal = false, is_fixed = false, is_recursion = false;
        pair<string, string> LRurl;
        if (result[result.size() - 1][0] == '/') {
            is_illegal = !DealURLCommand(result[result.size() - 1], localurl, LRurl);
        } else {
            is_illegal = !DealURLCommand(remoteurl + result[result.size() - 1], localurl, LRurl);
        }
        for (int i = 1; i < result.size() - 1; i++) {
            if (result[i] == "-r" || result[i] == "-recursion") {
                is_recursion = true;
            } else if (result[i] == "-f" || result[i] == "-fixed") {
                is_fixed = true;
            } else {
                PrintColorLine("下载文件配置错误!", FontColor::LRed);
                is_illegal = true;
            }
        }
        if (is_illegal) {
            break;
        }
        if (is_fixed && is_recursion) {
            mycli->DownloadDir(LRurl.second + '/', LRurl.first + '/', "fixed");
        } else if (is_recursion) {
            mycli->DownloadDir(LRurl.second + '/', LRurl.first + '/', "chunked");
        } else if (is_fixed) {
            mycli->FixedDownload(LRurl.second, LRurl.first);
        } else {
            mycli->ChunkedDownload(LRurl.second, LRurl.first);
        }
        break;
    }
    case "delete"_HASH: {
        if (result.size() < 2) {
            PrintColorLine("远程文件/文件夹名不能为空!", FontColor::LRed);
            break;
        }
        bool is_illegal = false, is_recursion = false;
        pair<string, string> LRurl;
        if (result[result.size() - 1][0] == '/') {
            is_illegal = !DealURLCommand(result[result.size() - 1], localurl, LRurl);
        } else {
            is_illegal = !DealURLCommand(remoteurl + result[result.size() - 1], localurl, LRurl);
        }
        for (int i = 1; i < result.size() - 1; i++) {
            if (result[i] == "-r" || result[i] == "-recursion") {
                is_recursion = true;
            } else {
                PrintColorLine("删除文件配置错误!", FontColor::LRed);
                is_illegal = true;
            }
        }
        if (is_illegal) {
            break;
        }
        if (is_recursion) {
            mycli->DeleteDir(LRurl.first + '/');
        } else {
            mycli->DeleteFile(LRurl.first);
        }
        break;
    }
    case "ls"_HASH: {
        bool is_illegal = false, is_local = false;
        for (int i = 1; i < result.size(); i++) {
            if (result[i] == "-l" || result[i] == "-local") {
                is_local = true;
            } else {
                PrintColorLine("显示目录配置错误!", FontColor::LRed);
                is_illegal = true;
            }
        }
        if (is_illegal) {
            break;
        }
        if (is_local) {
            mycli->GetLocalDir(localurl);
        } else {
            mycli->GetRemoteDir(remoteurl);
        }
        break;
    }
    case "mkdir"_HASH: {
        if (result.size() < 2) {
            PrintColorLine("文件夹名不能为空!", FontColor::LRed);
            break;
        }
        bool is_illegal = false;
        pair<string, string> LRurl;
        if (result[result.size() - 1][0] == '/') {
            is_illegal = !DealURLCommand(result[result.size() - 1], localurl, LRurl);
        } else {
            is_illegal = !DealURLCommand(remoteurl + result[result.size() - 1], localurl, LRurl);
        }
        if (is_illegal) {
            break;
        }
        mycli->MakeNewDir(LRurl.first);
        break;
    }
    case "cd"_HASH: {
        if (result.size() < 2) {
            PrintColorLine("路径名不能为空!", FontColor::LRed);
            break;
        }
        bool is_illegal = false, is_local = false;
        for (int i = 1; i < result.size() - 1; i++) {
            if (result[i] == "-l" || result[i] == "-local") {
                is_local = true;
            } else {
                PrintColorLine("更改目录配置错误!", FontColor::LRed);
                is_illegal = true;
            }
        }
        if (is_illegal) {
            break;
        }
        if (is_local) {
            mycli->ChangeLocalURL(result[result.size() - 1]);
        } else {
            mycli->ChangeRemoteURL(result[result.size() - 1]);
        }
        break;
    }
    case "close"_HASH: {
        if (result.size() < 2) {
            PrintColorLine("密码不能为空!", FontColor::LRed);
            break;
        }
        mycli->StopServer(result[1]);
        break;
    }
    default: {
        PrintColorLine("输入的指令为非法操作!", FontColor::LRed);
        break;
    }
    }
}
