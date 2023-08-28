#include "HandlerFactory.h"

void HandlerFactory::CreateHandler(AllHandler &allhandler) {
    allhandler.specialhandler = make_unique<SpecialHandler>(paramsList["/data"], password, svr, myredis);
    allhandler.formathandler = make_unique<FormatHandler>(paramsList["/data"], myredis);
    allhandler.deletehandler = make_unique<DeleteHandler>(paramsList["/data"], myredis);
    allhandler.dirhandler = make_unique<DirHandler>(paramsList["/data"], myredis);
}
