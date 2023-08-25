#pragma once
#include "Handler.h"

struct AllHandler {
    unique_ptr<SpecialHandler> specialhandler = nullptr;
    unique_ptr<FormatHandler> formathandler = nullptr;
    unique_ptr<DeleteHandler> deletehandler = nullptr;
    unique_ptr<DirHandler> dirhandler = nullptr;
};

class HandlerFactory {
public:
    HandlerFactory(Server &svr, string password, MyRedis &myredis, map<string, json> &paramsList)
        : svr(svr), password(password), myredis(myredis), paramsList(paramsList) {}
    AllHandler CreateHandler(string);

private:
    Server &svr;
    string password;
    MyRedis &myredis;
    map<string, json> &paramsList;
};
