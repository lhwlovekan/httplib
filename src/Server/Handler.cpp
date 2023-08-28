#include "Handler.h"

json Handler::CreateLog(const Request &req, string method, string err) {
    json Log_info, header_info;
    time_t now = time(0);
    char buffer[100];
    strftime(buffer, 100, "%Y-%m-%d %H:%M:%S", localtime(&now));
    Log_info["服务器时间"] = string(buffer);
    Log_info["客户端名称"] = req.get_header_value("NAME");
    Log_info["客户端行为"] = "向服务器申请" + method;
    for (auto header : req.headers) {
        header_info[header.first] = header.second;
    }
    Log_info["客户端报头"] = header_info;
    Log_info["服务器异常"] = err;
    return Log_info;
}

void Handler::WriteLog(json server_log) {
    struct stat StatBuffer;
    if (stat("./server_log.json", &StatBuffer) != 0) {
        fstream create_file("./server_log.json", ios::out);
        if (!create_file.is_open()) {
            cout << "日志文件创建失败!" << endl;
            return;
        }
        create_file.close();
    }
    fstream file("./server_log.json", ios::out | ios::in | ios::ate | ios::binary);
    if (!file.is_open()) {
        cout << "日志文件打开失败!" << endl;
        return;
    }
    if ((size_t)file.tellg() == 0) {
        // 合法的json格式需要加上'[]'
        file << "[]";
    }
    file.seekg(-1, file.end);
    if ((size_t)file.tellg() == 1) {
        file << server_log.dump(4) << "]";
    } else {
        file << "," << server_log.dump(4) << "]";
    }
}

string Handler::FirstMatch(string first_match) {
    int index = first_match.find_first_of('/', 1);
    first_match = first_match.substr(0, index);
    return first_match;
}

void SpecialHandler::Method(const Request &req, Response &res) {
    string ServerPath = path + req.matches[1].str() + req.matches[2].str();
    string err = "";
    switch (static_cast<int>(MethodMap[FirstMatch(req.matches[0].str())])) {
    case static_cast<int>(MethodType::TestServer): {
        Test(req, res, err);
        WriteLog(CreateLog(req, "进行连接测试", err));
        break;
    }
    case static_cast<int>(MethodType::CloseServer): {
        cout << req.matches[0] << endl;
        CloseServer(req, res, err);
        WriteLog(CreateLog(req, "关闭服务器", err));
        break;
    }
    }
}

void SpecialHandler::Test(const Request &req, Response &res, string &err) { ACTION_RETURN(res, err, ActionType::ConnectSuccess); }

void SpecialHandler::CloseServer(const Request &req, Response &res, string &err) {
    if (password == req.get_header_value("PASSWORD")) {
        ACTION_RETURN(res, err, ActionType::CloseSuccess);
        svr.stop();
    } else {
        ACTION_RETURN(res, err, ActionType::CloseError);
    }
}

void FormatHandler::Method(const Request &req, Response &res) {
    string ServerPath = path + req.matches[1].str() + req.matches[2].str();
    string err = "";
    switch (static_cast<int>(MethodMap[FirstMatch(req.matches[0].str())])) {
    case static_cast<int>(MethodType::SendClientContent): {
        SendClientContent(ServerPath, res, myredis, err);
        WriteLog(CreateLog(req, "下载文件" + req.matches[1].str() + req.matches[2].str(), err));
        break;
    }
    }
}

void FormatHandler::MethodWithContentReader(const Request &req, Response &res, const ContentReader &content_reader) {
    string ServerPath = path + req.matches[1].str() + req.matches[2].str();
    string err = "";
    switch (static_cast<int>(MethodMap[FirstMatch(req.matches[0].str())])) {
    case static_cast<int>(MethodType::SaveClientContent): {
        SaveClientContent(ServerPath, res, content_reader, myredis, err);
        WriteLog(CreateLog(req, "上传文件" + req.matches[1].str() + req.matches[2].str(), err));
        break;
    }
    }
}

void FormatHandler::SaveClientContent(string path, Response &res, const ContentReader &content_reader, MyRedis &myredis, string &err) {
    // 上传文件时，如果用户提供错误路径，不能马上退出，需要等待用户写完，否则会破坏管道
    bool success = true;
    // 在打开文件流前就加锁
    FileLock file_lock(path, FlockType::WRFLCK);
    fstream file;
    if (file_lock.SET_WRFLCK() == -1) {
        ACTION_RETURN(res, err, ActionType::WriteConflict);
        success = false;
    } else {
        file.open(path, ios::out | ios::binary);
        if (!file.is_open()) {
            ACTION_RETURN(res, err, ActionType::OpenFileError);
            success = false;
        }
        // 判断数据库功能是否异常
        if (!myredis.IsRun()) {
            ACTION_RETURN(res, err, ActionType::DataBaseError);
            success = false;
        }
        // 数据库存在同名文件，则删除数据库中的相关文件
        if (myredis.FileExists(path)) {
            map<string, string> Properties;
            myredis.SelectSmallFile(path, Properties);
            FileLock delete_lock(Properties["MergeFileName"], FlockType::WRFLCK);
            if (delete_lock.SET_WRFLCK() == -1) {
                ACTION_RETURN(res, err, ActionType::WriteConflict);
                success = false;
            } else {
                myredis.DeleteSmallFile(path);
                myredis.DeleteMergeFile(Properties["MergeFileName"], path);
                vector<string> SmallFileList;
                myredis.SelectMergeFile(Properties["MergeFileName"], SmallFileList);
                if (SmallFileList.empty()) {
                    remove(Properties["MergeFileName"].c_str());
                }
                delete_lock.UNFLCK();
            }
        }
    }
    bool is_smallfile = true;
    string content;
    content_reader([&file, &content, &is_smallfile, success](const char *data, size_t data_length) {
        if (success) {
            content.append(data, data_length);
            if (content.size() <= SMALL_BIG_LENGTH && is_smallfile) {
                return true;
            } else {
                is_smallfile = false;
                file.write(&content[0], content.size());
                content.clear();
            }
        }
        return true;
    });
    // 文件操作完成就解锁
    file_lock.UNFLCK();
    if (success && !is_smallfile) {
        ACTION_RETURN(res, err, ActionType::UploadSuccess);
    } else {
        remove(path.c_str());
    }
    // 若为小文件，进入到小文件合并操作
    if (success && is_smallfile) {
        MergeSmallFile(path, content, res, myredis, err);
    }
}

void FormatHandler::SendClientContent(string path, Response &res, MyRedis &myredis, string &err) {
    if (!myredis.IsRun()) {
        ACTION_RETURN(res, err, ActionType::DataBaseError);
        return;
    }
    // 若路径在数据库中存在，则跳转至发送小文件的接口
    if (myredis.FileExists(path)) {
        SendSmallFile(path, res, myredis, err);
        return;
    }
    // 获取文件大小，根据设定的区分值来决定以定长还是变长发送
    // 获取大小时加读锁
    FileLock file_lock(path, FlockType::RDFLCK);
    if (file_lock.SET_RDFLCK() == -1) {
        ACTION_RETURN(res, err, ActionType::ReadConflict);
        return;
    }
    fstream file(path, ios::in | ios::binary);
    if (!file.is_open()) {
        ACTION_RETURN(res, err, ActionType::OpenFileError);
        file_lock.UNFLCK();
        return;
    }
    if (opendir(path.c_str()) != nullptr) {
        ACTION_RETURN(res, err, ActionType::DirectoryConflict);
        file_lock.UNFLCK();
        return;
    }
    file.seekg(0, file.end);
    size_t data_length = (size_t)file.tellg();
    if (data_length <= FIXED_CHUNKED_LENGTH) {
        SendFixedContent(path, data_length, res, myredis, err);
    } else {
        SendChunkedContent(path, res, myredis, err);
    }
    file_lock.UNFLCK();
}

void FormatHandler::SendFixedContent(string path, int data_length, Response &res, MyRedis &myredis, string &err) {
    // 加读锁
    FileLock file_lock(path, FlockType::RDFLCK);
    if (file_lock.SET_RDFLCK() == -1) {
        ACTION_RETURN(res, err, ActionType::ReadConflict);
        return;
    }
    res.set_content_provider(
        data_length, "application/octet-stream",
        [path](size_t offset, size_t length, DataSink &sink) mutable {
            fstream file(path, ios::binary | ios::in);
            if (!file.is_open()) {
                return false;
            }
            file.seekg(offset, file.beg);
            string buffer(length, '\0');
            file.read(&buffer[0], length);
            sink.write(&buffer[0], length);
            return true;
        },
        [file_lock](bool success) mutable { file_lock.UNFLCK(); });
    ACTION_RETURN(res, err, ActionType::DownloadSuccess);
}

void FormatHandler::SendChunkedContent(string path, Response &res, MyRedis &myredis, string &err) {
    FileLock file_lock(path, FlockType::RDFLCK);
    if (file_lock.SET_RDFLCK() == -1) {
        ACTION_RETURN(res, err, ActionType::ReadConflict);
        return;
    }
    res.set_chunked_content_provider(
        "application/octet-stream",
        [path](size_t offset, DataSink &sink) mutable {
            fstream file(path, ios::binary | ios::in);
            if (!file.is_open()) {
                return false;
            }
            file.seekg(offset, file.beg);
            string buffer(BUFFER_SIZE, '\0');
            file.read(&buffer[0], BUFFER_SIZE);
            if (file.gcount() != 0) {
                sink.write(&buffer[0], file.gcount());
            } else {
                sink.done_with_trailer({{"RETURN", "文件下载成功!"}, {"ACTION", "SUCCESS"}});
            }
            return true;
        },
        [file_lock](bool success) mutable { file_lock.UNFLCK(); });
}

void FormatHandler::SendSmallFile(string path, Response &res, MyRedis &myredis, string &err) {
    map<string, string> Properties;
    myredis.SelectSmallFile(path, Properties);
    size_t offset = stoull(Properties["Offset"]);
    size_t data_length = stoull(Properties["Size"]);
    string FileName = Properties["MergeFileName"];
    FileLock file_lock(FileName, FlockType::RDFLCK);
    if (file_lock.SET_RDFLCK() == -1) {
        ACTION_RETURN(res, err, ActionType::ReadConflict);
        return;
    }
    fstream file(FileName, ios::in | ios::binary);
    if (!file.is_open()) {
        ACTION_RETURN(res, err, ActionType::OpenFileError);
        file_lock.UNFLCK();
        return;
    }
    // 读指针移到偏移量位置
    file.seekg(offset, file.beg);
    // 指定要读取的字符串长度为数据库中存储的大小
    string buffer(data_length, '\0');
    file.read(&buffer[0], data_length);
    // 解锁
    file_lock.UNFLCK();
    res.set_content(buffer.c_str(), buffer.size(), "application/octet-stream");
    ACTION_RETURN(res, err, ActionType::DownloadSmallFileSuccess);
}

void FormatHandler::MergeSmallFile(string path, string &content, Response &res, MyRedis &myredis, string &err) {
    // 大文件路径，找到最后一个/的位置，截取后添加大文件的名称
    string MergePath = path.substr(0, path.rfind('/') + 1) + MERGE_FILE_NAME;
    // 小文件路径
    string SmallPath = path;
    // 若原本存在与小文件同名的大文件，则删除大文件
    struct stat StatBuffer;
    if (stat(SmallPath.c_str(), &StatBuffer) == 0) {
        FileLock delete_lock(path, FlockType::WRFLCK);
        if (delete_lock.SET_WRFLCK() == -1) {
            ACTION_RETURN(res, err, ActionType::WriteConflict);
            return;
        }
        remove(SmallPath.c_str());
        delete_lock.UNFLCK();
    }
    FileLock file_lock(MergePath, FlockType::WRFLCK);
    if (file_lock.SET_WRFLCK() == -1) {
        ACTION_RETURN(res, err, ActionType::WriteConflict);
        return;
    }
    // 以追加形式打开，写指针指向文件末尾
    fstream file(MergePath, ios::out | ios::app | ios::binary);
    if (!file.is_open()) {
        ACTION_RETURN(res, err, ActionType::OpenFileError);
        file_lock.UNFLCK();
        return;
    }
    // 小文件偏移量
    size_t offset = (size_t)file.tellg();
    // 小文件大小
    size_t size = content.size();
    file << content;
    // 解锁
    file_lock.UNFLCK();
    // 小文件的修改时间设定为本地时间
    time_t now = time(0);
    char buffer[100];
    strftime(buffer, 100, "%Y-%m-%d %H:%M:%S", localtime(&now));
    string ModifiedTime(buffer);
    // 数据库记录小文件名称（绝对地址），对应大文件名称（绝对地址），偏移量、大小和修改时间
    map<string, string> Properties = {
        {"MergeFileName", MergePath}, {"Offset", to_string(offset)}, {"Size", to_string(size)}, {"ModifiedTime", ModifiedTime}};
    if (!myredis.IsRun()) {
        ACTION_RETURN(res, err, ActionType::DataBaseError);
        return;
    }
    myredis.InsertMergeFile(MergePath, SmallPath);
    myredis.InsertSmallFile(SmallPath, Properties);
    ACTION_RETURN(res, err, ActionType::UploadSmallFileSuccess);
}

void DeleteHandler::Method(const Request &req, Response &res) {
    string ServerPath = path + req.matches[1].str() + req.matches[2].str();
    string err = "";
    switch (static_cast<int>(MethodMap[FirstMatch(req.matches[0].str())])) {
    case static_cast<int>(MethodType::DeleteFile): {
        DeleteFile(ServerPath, res, myredis, err);
        WriteLog(CreateLog(req, "删除文件" + req.matches[1].str() + req.matches[2].str(), err));
        break;
    }
    }
}

void DeleteHandler::DeleteFile(string path, Response &res, MyRedis &myredis, string &err) {
    // 判断数据库功能是否异常
    if (!myredis.IsRun()) {
        ACTION_RETURN(res, err, ActionType::DataBaseError);
        return;
    }
    // 如果是已经合并的小文件，则需要把数据库信息删除
    if (myredis.FileExists(path)) {
        // 先查询小文件对应的大文件名，大文件索引和小文件内容都要删除
        map<string, string> Properties;
        myredis.SelectSmallFile(path, Properties);
        // 给大文件加锁
        FileLock file_lock(Properties["MergeFileName"], FlockType::WRFLCK);
        if (file_lock.SET_WRFLCK() == -1) {
            ACTION_RETURN(res, err, ActionType::WriteConflict);
            return;
        }
        myredis.DeleteSmallFile(path);
        myredis.DeleteMergeFile(Properties["MergeFileName"], path);
        // 如果大文件对应的小文件列表为空，则将大文件删除
        vector<string> SmallFileList;
        myredis.SelectMergeFile(Properties["MergeFileName"], SmallFileList);
        if (SmallFileList.empty()) {
            remove(Properties["MergeFileName"].c_str());
        }
        ACTION_RETURN(res, err, ActionType::DeleteSuccess);
        file_lock.UNFLCK();
        return;
    }
    // 判断文件是否存在
    struct stat StatBuffer;
    if (stat(path.c_str(), &StatBuffer) != 0) {
        ACTION_RETURN(res, err, ActionType::FileExistsError);
        return;
    }
    // 给普通文件加锁
    FileLock file_lock(path, FlockType::WRFLCK);
    if (file_lock.SET_WRFLCK() == -1) {
        ACTION_RETURN(res, err, ActionType::WriteConflict);
        return;
    }
    // 使用remove函数删除文件
    if (remove(path.c_str()) == 0) {
        ACTION_RETURN(res, err, ActionType::DeleteSuccess);
    } else {
        if (opendir(path.c_str()) != nullptr) {
            ACTION_RETURN(res, err, ActionType::DeleteDirectoryError);
        } else {
            ACTION_RETURN(res, err, ActionType::DeleteFileError);
        }
    }
    file_lock.UNFLCK();
}

void DirHandler::Method(const Request &req, Response &res) {
    string ServerPath = path + req.matches[1].str();
    string err = "";
    switch (static_cast<int>(MethodMap[FirstMatch(req.matches[0].str())])) {
    case static_cast<int>(MethodType::SendDirList): {
        SendDirList(ServerPath, res, myredis, err);
        WriteLog(CreateLog(req, "获取" + req.matches[1].str() + "目录下的文件列表", err));
        break;
    }
    case static_cast<int>(MethodType::ChangeCilentURL): {
        ChangeClientURL(ServerPath, res, err);
        WriteLog(CreateLog(req, "改变当前远程目录为" + req.matches[1].str(), err));
        break;
    }
    case static_cast<int>(MethodType::MakeNewDir): {
        ServerPath += req.matches[2].str();
        MakeNewDir(ServerPath, res, err);
        WriteLog(CreateLog(req, "在" + req.matches[1].str() + "目录下创建文件夹" + req.matches[2].str(), err));
        break;
    }
    }
}

json DirHandler::CreateDirList(string path, MyRedis &myredis) {
    json Dir_info;
    DIR *LocalDir;
    LocalDir = opendir(path.c_str());
    dirent *LocalDirent;
    struct stat StatBuffer;
    while ((LocalDirent = readdir(LocalDir)) != nullptr) {
        json dir_info;
        // 跳过两个特殊索引以及用来存储小文件的特殊大文件，已经合并的小文件需要访问数据库单独处理
        if (strcmp(LocalDirent->d_name, ".") == 0 || strcmp(LocalDirent->d_name, "..") == 0 ||
            strcmp(LocalDirent->d_name, MERGE_FILE_NAME.c_str()) == 0) {
            continue;
        }
        // 清空StatBuffer
        memset(&StatBuffer, 0, sizeof(StatBuffer));
        // 得到文件的绝对路径，单纯以名称访问会得到错误结果
        char FilePath[1000];
        strcpy(FilePath, path.c_str());
        strcat(FilePath, LocalDirent->d_name);
        lstat(FilePath, &StatBuffer);
        // 文件的各项属性
        string FileName = string(LocalDirent->d_name);
        string FileSize = to_string(StatBuffer.st_size) + "byte";
        char buffer[100];
        strftime(buffer, 100, "%Y-%m-%d %H:%M:%S", localtime(&StatBuffer.st_mtime));
        string ModifiedTime(buffer);
        dir_info["文件大小"] = FileSize;
        dir_info["修改时间"] = ModifiedTime;
        // 判断是文件夹还是文件
        if (S_ISDIR(StatBuffer.st_mode)) {
            FileName += '/';
        }
        dir_info["文件名称"] = FileName;
        Dir_info.emplace_back(dir_info);
    }
    closedir(LocalDir);
    // 若目录下存在大文件，则将大文件中的小文件列表显示出来
    vector<string> SmallFileList;
    myredis.SelectMergeFile(path + MERGE_FILE_NAME, SmallFileList);
    for (string SmallFileName : SmallFileList) {
        json dir_info;
        map<string, string> Properties;
        myredis.SelectSmallFile(SmallFileName, Properties);
        int LastIndex = SmallFileName.find_last_of('/') + 1;
        string FileName = SmallFileName.substr(LastIndex, SmallFileName.size() - LastIndex);
        dir_info["文件名称"] = FileName;
        dir_info["文件大小"] = Properties["Size"] + "byte";
        dir_info["修改时间"] = Properties["ModifiedTime"];
        Dir_info.emplace_back(dir_info);
    }
    return Dir_info;
}

void DirHandler::SendDirList(string path, Response &res, MyRedis &myredis, string &err) {
    if (!myredis.IsRun()) {
        ACTION_RETURN(res, err, ActionType::DataBaseError);
        return;
    }
    if (opendir(path.c_str()) == nullptr) {
        ACTION_RETURN(res, err, ActionType::DirectoryListError);
        return;
    }
    json Dir_info = CreateDirList(path, myredis);
    stringstream body;
    body << Dir_info;
    res.set_content(body.str(), "application/json");
    ACTION_RETURN(res, err, ActionType::DirectoryListSuccess);
}

void DirHandler::ChangeClientURL(string path, Response &res, string &err) {
    if (opendir(path.c_str()) == nullptr) {
        ACTION_RETURN(res, err, ActionType::ChangeURLError);
        return;
    }
    ACTION_RETURN(res, err, ActionType::ChangeURLSuccess);
}

void DirHandler::MakeNewDir(string path, Response &res, string &err) {
    if (mkdir(path.c_str(), 0777) == -1) {
        ACTION_RETURN(res, err, ActionType::MakeDirError);
    } else {
        ACTION_RETURN(res, err, ActionType::MakeDirSuccess);
    }
}
