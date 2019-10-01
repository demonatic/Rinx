#include "Server/Server.h"

int main(){
    RxSignalManager::disable_current_thread_signal();
    nanolog::initialize(nanolog::GuaranteedLogger(),"/tmp/","rinx_log",20);
    RxServer server(65535);
    server.listen("0.0.0.0",8889);
    return 0;
}
