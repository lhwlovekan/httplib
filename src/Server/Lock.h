#pragma once
#include <sys/file.h>
#include <unistd.h>

#include <iostream>
using namespace std;

// 文件锁初始化类型
enum class FlockType { RDFLCK = 0, WRFLCK };

// 文件锁
class FileLock {
public:
    FileLock(string path, FlockType flocktype) {
        if (static_cast<int>(flocktype) == 0) {
            file_desc = open(path.c_str(), O_RDONLY, 0777);
        } else if (static_cast<int>(flocktype) == 1) {
            file_desc = open(path.c_str(), O_WRONLY | O_CREAT, 0777);
        }
    }
    int SET_WRFLCK() {
        int ret = flock(file_desc, LOCK_EX | LOCK_NB);
        // 文件创建失败返回-2，加锁失败返回-1
        return file_desc == -1 ? -2 : ret;
    }
    int SET_RDFLCK() {
        int ret = flock(file_desc, LOCK_SH | LOCK_NB);
        return file_desc == -1 ? -2 : ret;
    }
    int UNFLCK() {
        int ret = flock(file_desc, LOCK_UN | LOCK_NB);
        // 为避免意外，将关闭文件描述符
        close(file_desc);
        return file_desc == -1 ? -2 : ret;
    }
    // 返回文件描述符
    int GetFileDesc() { return file_desc; }

private:
    int file_desc;
};
