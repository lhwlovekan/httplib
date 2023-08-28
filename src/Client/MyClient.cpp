#include "MyClient.h"

void MyClient::InitParams(string name, string address, int port) {
    ClientName = name;
    cli = make_shared<Client>(address, port);
    // 保持连接
    cli->set_keep_alive(true);
    // 客户端设置连接和读写超时
    cli->set_connection_timeout(0, 300000);
    cli->set_read_timeout(5, 0);
    cli->set_write_timeout(5, 0);
}

void MyClient::TestGet() {
    if (auto result = cli->Get("/hi", {{"NAME", ClientName}})) {
        PrintColorLine("服务器返回:" + result->get_header_value("RETURN"),
                       result->get_header_value("ACTION") == "SUCCESS" ? FontColor::LGreen : FontColor::LRed);
    } else {
        auto err = result.error();
        PrintColorLine("HTTP连接异常:" + to_string(err), FontColor::LRed);
    }
}

void MyClient::FixedUpload(string src, string url, bool is_check) {
    string prefix = "/post";
    // 本地文件生成MD5
    MD5_CTX md5_local;
    MD5_Init(&md5_local);
    unsigned char Localresult[MD5_DIGEST_LENGTH];
    fstream file(src, ios::in | ios::binary);
    if (!file.is_open()) {
        PrintColorLine("客户端打开本地文件失败!", FontColor::LRed);
        return;
    }
    if (opendir(src.c_str()) != nullptr) {
        PrintColorLine("不能以文件形式上传目录!", FontColor::LRed);
        return;
    }
    // 读出文件长度
    file.seekg(0, file.end);
    size_t data_length = (size_t)file.tellg();
    // 读指针回到文件起点处
    file.seekg(0, file.beg);
    if (auto result = cli->Post(
            prefix + url, {{"NAME", ClientName}}, data_length,
            [&](size_t offset, size_t length, DataSink& sink) {
                string buffer(length, '\0');
                file.read(&buffer[0], length);
                // 一边上传文件一边更新MD5
                if (is_check) {
                    if (MD5_Update(&md5_local, buffer.c_str(), length) == 0) {
                        PrintColorLine("客户端在读取文件并更新MD5码的过程中出现错误!", FontColor::LRed);
                        return false;
                    }
                }
                sink.write(&buffer[0], length);
                return true;
            },
            "application/octet-stream")) {
        PrintColorLine("服务器返回:" + result->get_header_value("RETURN"),
                       result->get_header_value("ACTION") == "SUCCESS" ? FontColor::LGreen : FontColor::LRed);
    } else {
        auto err = result.error();
        PrintColorLine("HTTP连接异常:" + to_string(err), FontColor::LRed);
    }
    // 若用户定义了需要校验，则在上传完毕后立刻下载该文件并比较MD5码
    if (is_check) {
        MD5_Final(Localresult, &md5_local);
        CheckDocument(src, url, Localresult);
    }
}

void MyClient::ChunkedUpload(string src, string url, bool is_check) {
    string prefix = "/post";
    // 本地文件生成MD5
    MD5_CTX md5_local;
    MD5_Init(&md5_local);
    unsigned char Localresult[MD5_DIGEST_LENGTH];
    fstream file(src, ios::in | ios::binary);
    if (!file.is_open()) {
        PrintColorLine("客户端打开本地文件失败!", FontColor::LRed);
        return;
    }
    if (opendir(src.c_str()) != nullptr) {
        PrintColorLine("不能以文件形式上传目录!", FontColor::LRed);
        return;
    }
    string buffer(BUFFER_SIZE, '\0');
    if (auto result = cli->Post(
            prefix + url, {{"NAME", ClientName}},
            [&](size_t offset, DataSink& sink) {
                if (!file.eof()) {
                    file.read(&buffer[0], BUFFER_SIZE);
                    if (is_check) {
                        if (MD5_Update(&md5_local, buffer.c_str(), file.gcount()) == 0) {
                            PrintColorLine("客户端在读取文件并更新MD5码的过程中出现错误!", FontColor::LRed);
                            return false;
                        }
                    }
                    sink.write(&buffer[0], file.gcount());
                } else {
                    sink.done();
                }
                return true;
            },
            "application/octet-stream")) {
        PrintColorLine("服务器返回:" + result->get_header_value("RETURN"),
                       result->get_header_value("ACTION") == "SUCCESS" ? FontColor::LGreen : FontColor::LRed);
    } else {
        auto err = result.error();
        PrintColorLine("HTTP连接异常:" + to_string(err), FontColor::LRed);
    }
    if (is_check) {
        MD5_Final(Localresult, &md5_local);
        CheckDocument(src, url, Localresult);
    }
}

void MyClient::DownloadContent(string src, string url) {
    string prefix = "/get";
    fstream file(src, ios::out | ios::binary);
    if (!file.is_open()) {
        PrintColorLine("客户端创建本地文件失败!", FontColor::LRed);
        return;
    }
    if (auto result = cli->Get(
            prefix + url, {{"NAME", ClientName}},
            [&](const char* data, size_t data_length) {
                file.write(data, data_length);
                return true;
            },
            [&](size_t len, size_t total) {
                PrintColor("[" + to_string(len) + "字节/" + to_string(total) + "字节]", FontColor::LCyan);
                PrintColor("[" + string((int)(len * 50 / total), '#') + string(50 - (int)(len * 50 / total), '-') + "]", FontColor::LBlue);
                PrintColor("[" + to_string((int)(len * 100 / total)) + "%]", FontColor::LGreen);
                cout << (len == total ? "\r\n" : "\r");
                return true;
            })) {
        PrintColorLine("服务器返回:" + result->get_header_value("RETURN"),
                       result->get_header_value("ACTION") == "SUCCESS" ? FontColor::LGreen : FontColor::LRed);
        if (result->get_header_value("ACTION") == "FAILURE") {
            // 若传输失败，则将本地创建的文件删除
            remove(src.c_str());
        }
    } else {
        remove(src.c_str());
        auto err = result.error();
        PrintColorLine("HTTP连接异常:" + to_string(err), FontColor::LRed);
    }
}

// 递归上传文件夹
// src以'/'结尾，而url以文件夹名称结尾
void MyClient::UploadDir(string src, string url, string option, bool is_check) {
    // 先检验本地是否存在该文件夹
    if (opendir(src.c_str()) == nullptr) {
        PrintColorLine("客户端打开本地文件夹失败!", FontColor::LRed);
        return;
    }
    // 创建新文件夹，同名文件夹时返回错误
    if (MakeNewDir(url) == false) {
        return;
    }
    DIR* LocalDir;
    LocalDir = opendir(src.c_str());
    dirent* LocalDirent;
    struct stat StatBuffer;
    while ((LocalDirent = readdir(LocalDir)) != nullptr) {
        if (strcmp(LocalDirent->d_name, ".") == 0 || strcmp(LocalDirent->d_name, "..") == 0) {
            continue;
        }
        memset(&StatBuffer, 0, sizeof(StatBuffer));
        char FilePath[1000];
        strcpy(FilePath, src.c_str());
        strcat(FilePath, LocalDirent->d_name);
        lstat(FilePath, &StatBuffer);
        // 判断是文件夹还是文件
        if (S_ISDIR(StatBuffer.st_mode)) {
            UploadDir(string(FilePath) + '/', url + '/' + string(LocalDirent->d_name), option, is_check);
        } else {
            if (option == "chunked") {
                ChunkedUpload(string(FilePath), url + '/' + string(LocalDirent->d_name), is_check);
            } else if (option == "fixed") {
                FixedUpload(string(FilePath), url + '/' + string(LocalDirent->d_name), is_check);
            }
        }
    }
    closedir(LocalDir);
}

// 递归下载文件夹
// src和url都以'/'结尾
void MyClient::DownloadDir(string src, string url) {
    string prefix = "/dir";
    // 向服务器申请获得对应文件夹的目录列表
    if (auto result = cli->Get(prefix + url, {{"NAME", ClientName}})) {
        if (result->get_header_value("ACTION") == "FAILURE") {
            PrintColorLine("服务器返回:" + result->get_header_value("RETURN"), FontColor::LRed);
            return;
        }
        // 在本地创建该文件夹
        if (mkdir(src.c_str(), 0777) == -1) {
            PrintColorLine("客户端创建本地文件夹失败!", FontColor::LRed);
            return;
        }
        json Dir_info = json::parse(result->body);
        for (auto file : Dir_info) {
            // 如果名称中含有'/'，则为文件夹
            if (file["文件名称"].get<string>().find_first_of('/') != string::npos) {
                DownloadDir(src + file["文件名称"].get<string>(), url + file["文件名称"].get<string>());
            } else {
                DownloadContent(src + file["文件名称"].get<string>(), url + file["文件名称"].get<string>());
            }
        }
    } else {
        auto err = result.error();
        PrintColorLine("HTTP连接异常:" + to_string(err), FontColor::LRed);
    }
}

// 递归删除文件夹
// url以'/'结尾
void MyClient::DeleteDir(string url) {
    string prefix = "/dir";
    // 向服务器申请获得对应文件夹的目录列表
    if (auto result = cli->Get(prefix + url, {{"NAME", ClientName}})) {
        if (result->get_header_value("ACTION") == "FAILURE") {
            PrintColorLine("服务器返回:" + result->get_header_value("RETURN"), FontColor::LRed);
            return;
        }
        json Dir_info = json::parse(result->body);
        for (auto file : Dir_info) {
            // 如果名称中含有'/'，则为文件夹
            if (file["文件名称"].get<string>().find_first_of('/') != string::npos) {
                DeleteDir(url + file["文件名称"].get<string>());
            } else {
                DeleteFile(url + file["文件名称"].get<string>());
            }
        }
        // 最后将空文件夹删除
        DeleteFile(url.substr(0, url.size() - 1));
    } else {
        auto err = result.error();
        PrintColorLine("HTTP连接异常:" + to_string(err), FontColor::LRed);
    }
}

void MyClient::DeleteFile(string url) {
    string prefix = "/delete";
    if (auto result = cli->Delete(prefix + url, {{"NAME", ClientName}})) {
        PrintColorLine("服务器返回:" + result->get_header_value("RETURN"),
                       result->get_header_value("ACTION") == "SUCCESS" ? FontColor::LGreen : FontColor::LRed);
    } else {
        auto err = result.error();
        PrintColorLine("HTTP连接异常:" + to_string(err), FontColor::LRed);
    }
}

bool MyClient::MakeNewDir(string url) {
    string prefix = "/mkdir";
    if (auto result = cli->Get(prefix + url, {{"NAME", ClientName}})) {
        PrintColorLine("服务器返回:" + result->get_header_value("RETURN"),
                       result->get_header_value("ACTION") == "SUCCESS" ? FontColor::LGreen : FontColor::LRed);
        return result->get_header_value("ACTION") == "SUCCESS" ? true : false;
    } else {
        auto err = result.error();
        PrintColorLine("HTTP连接异常:" + to_string(err), FontColor::LRed);
        return false;
    }
}

bool MyClient::DealURLCommand(string& url, string command) {
    deque<string> url_temp;
    vector<string> result;
    // 绝对路径和相对路径
    if (command[0] == '/') {
        StrSplit(command, '/', result);
    } else {
        StrSplit(url + command, '/', result);
    }
    for (int i = 0; i < result.size(); i++) {
        if (result[i] == ".") {
            continue;
        } else if (result[i] == ".." && !url_temp.empty()) {
            url_temp.pop_back();
        } else if (result[i] == ".." && url_temp.empty()) {
            PrintColorLine("根目录无法跳转到上级目录!", FontColor::LRed);
            return false;
        } else {
            url_temp.push_back(result[i]);
        }
    }
    string url_final = "/";
    while (!url_temp.empty()) {
        url_final += url_temp.front() + "/";
        url_temp.pop_front();
    }
    url = url_final;
    return true;
}

void MyClient::ChangeRemoteURL(string command) {
    string prefix = "/url";
    string url = RemoteURL;
    if (!DealURLCommand(url, command)) {
        return;
    }
    if (auto result = cli->Get(prefix + url, {{"NAME", ClientName}})) {
        PrintColorLine("服务器返回:" + result->get_header_value("RETURN"),
                       result->get_header_value("ACTION") == "SUCCESS" ? FontColor::LGreen : FontColor::LRed);
        if (result->get_header_value("ACTION") == "SUCCESS") {
            // 如果服务器返回目录合法，则进行更改
            RemoteURL = url;
        }
    } else {
        auto err = result.error();
        PrintColorLine("HTTP连接异常:" + to_string(err), FontColor::LRed);
    }
}

void MyClient::ChangeLocalURL(string command) {
    string url = LocalURL;
    if (!DealURLCommand(url, command)) {
        return;
    }
    DIR* LocalDir;
    LocalDir = opendir(url.c_str());
    if (LocalDir == nullptr) {
        PrintColorLine("本地目录更改失败!", FontColor::LRed);
    } else {
        PrintColorLine("本地目录更改成功!", FontColor::LGreen);
        LocalURL = url;
    }
}

void MyClient::GetRemoteDir(string url) {
    string prefix = "/dir";
    cout<<prefix+url<<endl;
    if (auto result = cli->Get(prefix + url, {{"NAME", ClientName}})) {
        PrintColorLine("服务器返回:" + result->get_header_value("RETURN"),
                       result->get_header_value("ACTION") == "SUCCESS" ? FontColor::LGreen : FontColor::LRed);
        if (result->get_header_value("ACTION") == "FAILURE") {
            return;
        }
        json Dir_info = json::parse(result->body);
        stringstream output;
        output << left << setw(30) << "ModifiedTime" << left << setw(30) << "FileSize" << left << setw(30) << "FileName";
        PrintColorLine(output.str(), FontColor::LPurple, FontStyle::Bold);
        output.str("");
        output << left << setw(30) << "----------" << left << setw(30) << "----------" << left << setw(30) << "----------";
        PrintColorLine(output.str(), FontColor::LPurple);
        string FileName, FileSize, ModifiedTime;
        for (auto file : Dir_info) {
            output.str("");
            FileName = file["文件名称"].get<string>();
            FileSize = file["文件大小"].get<string>();
            ModifiedTime = file["修改时间"].get<string>();
            output << left << setw(30) << ModifiedTime << left << setw(30) << FileSize << left << setw(30) << FileName;
            PrintColorLine(output.str(), FontColor::LPurple);
        }
    } else {
        auto err = result.error();
        PrintColorLine("HTTP连接异常:" + to_string(err), FontColor::LRed);
    }
}

void MyClient::GetLocalDir(string src) {
    DIR* LocalDir;
    LocalDir = opendir(src.c_str());
    if (LocalDir == nullptr) {
        PrintColorLine("目录不存在!", FontColor::LRed);
        return;
    }
    dirent* LocalDirent;
    struct stat StatBuffer;
    stringstream output;
    output << left << setw(30) << "ModifiedTime" << left << setw(30) << "FileSize" << left << setw(30) << "FileName";
    PrintColorLine(output.str(), FontColor::LPurple, FontStyle::Bold);
    output.str("");
    output << left << setw(30) << "----------" << left << setw(30) << "----------" << left << setw(30) << "----------";
    PrintColorLine(output.str(), FontColor::LPurple);
    while ((LocalDirent = readdir(LocalDir)) != nullptr) {
        if (strcmp(LocalDirent->d_name, ".") == 0 || strcmp(LocalDirent->d_name, "..") == 0) {
            continue;
        }
        memset(&StatBuffer, 0, sizeof(StatBuffer));
        char FilePath[1000];
        strcpy(FilePath, src.c_str());
        strcat(FilePath, LocalDirent->d_name);
        lstat(FilePath, &StatBuffer);
        // 文件的各项属性
        string FileName = string(LocalDirent->d_name);
        string FileSize = to_string(StatBuffer.st_size) + "byte";
        char buffer[100];
        strftime(buffer, 100, "%Y-%m-%d %H:%M:%S", localtime(&StatBuffer.st_mtime));
        string ModifiedTime(buffer);
        // 判断是文件夹还是文件
        if (S_ISDIR(StatBuffer.st_mode)) {
            FileName += "/";
        }
        output.str("");
        output << left << setw(30) << ModifiedTime << left << setw(30) << FileSize << left << setw(30) << FileName;
        PrintColorLine(output.str(), FontColor::LPurple);
    }
    closedir(LocalDir);
}

void MyClient::CheckDocument(string src, string url, unsigned char* Localresult) {
    string prefix = "/get";
    // 服务器传输的流生成MD5
    MD5_CTX md5_server;
    MD5_Init(&md5_server);
    unsigned char Serverresult[MD5_DIGEST_LENGTH];
    if (auto result = cli->Get(prefix + url, {{"NAME", ClientName}}, [&](const char* data, size_t data_length) {
            if (MD5_Update(&md5_server, data, data_length) == 0) {
                PrintColorLine("服务器传输流在更新MD5码的过程中出现错误!", FontColor::LRed);
                return false;
            }
            return true;
        })) {
        PrintColorLine("服务器返回:" + result->get_header_value("RETURN"),
                       result->get_header_value("ACTION") == "SUCCESS" ? FontColor::LGreen : FontColor::LRed);
        // 生成服务器传输流的MD5校验码
        MD5_Final(Serverresult, &md5_server);
        if (result->get_header_value("ACTION") == "SUCCESS") {
            stringstream localstream, serverstream;
            for (int i = 0; i < MD5_DIGEST_LENGTH; i++) {
                localstream << hex << setw(2) << setfill('0') << static_cast<int>(Localresult[i]);
                serverstream << hex << setw(2) << setfill('0') << static_cast<int>(Serverresult[i]);
            }
            PrintColorLine("文件名称:" + src, FontColor::LPurple);
            PrintColorLine("本地MD5校验码:" + localstream.str(), FontColor::LPurple);
            PrintColorLine("服务器传输流MD5校验码:" + serverstream.str(), FontColor::LPurple);
            if (localstream.str() != serverstream.str()) {
                PrintColorLine("MD5校验码不一致,上传的文件出现损坏!", FontColor::LRed);
            } else {
                PrintColorLine("校验完成,上传的文件未损坏!", FontColor::LCyan);
            }
        }
    } else {
        auto err = result.error();
        PrintColorLine("HTTP连接异常:" + to_string(err), FontColor::LRed);
    }
}

void MyClient::StopServer(string password) {
    if (auto result = cli->Get("/close", {{"NAME", ClientName}, {"PASSWORD", password}})) {
        PrintColorLine("服务器返回:" + result->get_header_value("RETURN"),
                       result->get_header_value("ACTION") == "SUCCESS" ? FontColor::LGreen : FontColor::LRed);
    } else {
        auto err = result.error();
        PrintColorLine("HTTP连接异常:" + to_string(err), FontColor::LRed);
    }
}
