#include "MyRedis.h"

bool MyRedis::IsRun() {
    try {
        // 使用测试样例来判断是否出现异常
        if (!redis.exists("/merge")) {
            redis.sadd("/merge", "/small");
            map<string, string> Properties = {
                {"MergeFileName", "/merge"}, {"Offset", "0"}, {"Size", "100"}, {"ModifiedTime", "2001-12-1 12:00:00"}};
            redis.hmset("/small", Properties.begin(), Properties.end());
        }
        return true;
    } catch (const ReplyError &err) {
        cout << err.what() << endl;
        return false;
    } catch (const TimeoutError &err) {
        cout << err.what() << endl;
        return false;
    } catch (const ClosedError &err) {
        cout << err.what() << endl;
        return false;
    } catch (const IoError &err) {
        cout << err.what() << endl;
        return false;
    } catch (const Error &err) {
        cout << err.what() << endl;
        return false;
    }
}

bool MyRedis::InitConnect(Json config) {
    try {
        string address, port;
        address = config.GetDBJson().address;
        port = to_string(config.GetDBJson().port);
        Redis new_redis = Redis("tcp://" + address + ":" + port);
        redis = move(new_redis);
        return IsRun();
    } catch (const ReplyError &err) {
        cout << err.what() << endl;
        return false;
    } catch (const TimeoutError &err) {
        cout << err.what() << endl;
        return false;
    } catch (const ClosedError &err) {
        cout << err.what() << endl;
        return false;
    } catch (const IoError &err) {
        cout << err.what() << endl;
        return false;
    } catch (const Error &err) {
        cout << err.what() << endl;
        return false;
    }
}

bool MyRedis::IsExistFile(string FileName) {
    try {
        if (redis.exists(FileName) != 0) {
            return true;
        } else {
            return false;
        }
    } catch (const ReplyError &err) {
        cout << err.what() << endl;
        return false;
    } catch (const TimeoutError &err) {
        cout << err.what() << endl;
        return false;
    } catch (const ClosedError &err) {
        cout << err.what() << endl;
        return false;
    } catch (const IoError &err) {
        cout << err.what() << endl;
        return false;
    } catch (const Error &err) {
        cout << err.what() << endl;
        return false;
    }
}

bool MyRedis::SelectMergeFile(string MergeFileName, vector<string> &SmallFileList) {
    try {
        redis.smembers(MergeFileName, inserter(SmallFileList, SmallFileList.begin()));
        return true;
    } catch (const ReplyError &err) {
        cout << err.what() << endl;
        return false;
    } catch (const TimeoutError &err) {
        cout << err.what() << endl;
        return false;
    } catch (const ClosedError &err) {
        cout << err.what() << endl;
        return false;
    } catch (const IoError &err) {
        cout << err.what() << endl;
        return false;
    } catch (const Error &err) {
        cout << err.what() << endl;
        return false;
    }
}

bool MyRedis::SelectSmallFile(string SmallFileName, map<string, string> &Properties) {
    try {
        redis.hgetall(SmallFileName, inserter(Properties, Properties.begin()));
        return true;
    } catch (const ReplyError &err) {
        cout << err.what() << endl;
        return false;
    } catch (const TimeoutError &err) {
        cout << err.what() << endl;
        return false;
    } catch (const ClosedError &err) {
        cout << err.what() << endl;
        return false;
    } catch (const IoError &err) {
        cout << err.what() << endl;
        return false;
    } catch (const Error &err) {
        cout << err.what() << endl;
        return false;
    }
}

bool MyRedis::DeleteSmallFile(string SmallFileName) {
    try {
        redis.del(SmallFileName);
        return true;
    } catch (const ReplyError &err) {
        cout << err.what() << endl;
        return false;
    } catch (const TimeoutError &err) {
        cout << err.what() << endl;
        return false;
    } catch (const ClosedError &err) {
        cout << err.what() << endl;
        return false;
    } catch (const IoError &err) {
        cout << err.what() << endl;
        return false;
    } catch (const Error &err) {
        cout << err.what() << endl;
        return false;
    }
}

bool MyRedis::DeleteMergeFile(string MergeFileName, string SmallFileName) {
    try {
        redis.srem(MergeFileName, SmallFileName);
        return true;
    } catch (const ReplyError &err) {
        cout << err.what() << endl;
        return false;
    } catch (const TimeoutError &err) {
        cout << err.what() << endl;
        return false;
    } catch (const ClosedError &err) {
        cout << err.what() << endl;
        return false;
    } catch (const IoError &err) {
        cout << err.what() << endl;
        return false;
    } catch (const Error &err) {
        cout << err.what() << endl;
        return false;
    }
}

bool MyRedis::InsertSmallFile(string SmallFileName, map<string, string> Properties) {
    if (CheckProperties(Properties) == false) {
        cout << "传入的文件信息为错误格式!" << endl;
        return false;
    }
    try {
        redis.hmset(SmallFileName, Properties.begin(), Properties.end());
        return true;
    } catch (const ReplyError &err) {
        cout << err.what() << endl;
        return false;
    } catch (const TimeoutError &err) {
        cout << err.what() << endl;
        return false;
    } catch (const ClosedError &err) {
        cout << err.what() << endl;
        return false;
    } catch (const IoError &err) {
        cout << err.what() << endl;
        return false;
    } catch (const Error &err) {
        cout << err.what() << endl;
        return false;
    }
}

bool MyRedis::InsertMergeFile(string MergeFileName, string SmallFileName) {
    try {
        redis.sadd(MergeFileName, SmallFileName);
        return true;
    } catch (const ReplyError &err) {
        cout << err.what() << endl;
        return false;
    } catch (const TimeoutError &err) {
        cout << err.what() << endl;
        return false;
    } catch (const ClosedError &err) {
        cout << err.what() << endl;
        return false;
    } catch (const IoError &err) {
        cout << err.what() << endl;
        return false;
    } catch (const Error &err) {
        cout << err.what() << endl;
        return false;
    }
}

bool MyRedis::CheckProperties(map<string, string> Properties) {
    if (Properties.size() != 4) {
        return false;
    }
    // 判断偏移量和文件大小是否是全数字字符串
    size_t found = Properties["Offset"].find_first_not_of("0123456789");
    if (found != string::npos) {
        cout << "文件偏移量不为全数字字符串!" << endl;
        return false;
    }
    found = Properties["Size"].find_first_not_of("0123456789");
    if (found != string::npos) {
        cout << "文件大小不为全数字字符串!" << endl;
        return false;
    }
    // 判断修改时间是否出现非法字符
    // 时间格式的严格判断实现比较复杂，目前只做简单判断
    found = Properties["ModifiedTime"].find_first_not_of("0123456789-: ");
    if (found != string::npos) {
        cout << "修改时间出现非法字符!" << endl;
        return false;
    }
    return true;
}
