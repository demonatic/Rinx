#include "Server/Server.h"

int main(){
    RxSignalManager::disable_current_thread_signal();
    nanolog::initialize(nanolog::GuaranteedLogger(),"/tmp/","rinx_log",20);
    RxServer server(10000);
    server.start("0.0.0.0",8080);

    return 0;
}
