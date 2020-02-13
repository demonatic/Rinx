#ifndef PROTOCOLECHO_H
#define PROTOCOLECHO_H

#include "Network/Connection.h"
#include "Protocol/ProtocolProcessorFactory.h"

namespace Rinx {
class RxProtoEchoProcessor:public RxProtoProcessor
{
public:
    RxProtoEchoProcessor(RxConnection *conn):RxProtoProcessor(conn){

    }

    virtual bool process_read_data(RxConnection *conn,RxChainBuffer &input_buf) override{
        std::cout<<"process_read_data"<<std::endl;
        std::swap(input_buf,conn->get_output_buf());
        conn->send();
        return true;
    }
    virtual bool handle_write_prepared(RxConnection *conn,RxChainBuffer &output_buf) override{
        std::cout<<"handle_write_prepared"<<std::endl;
        conn->send();
        return true;
    }
};

class RxProtocolEchoFactory:public RxProtocolFactory
{
public:
    RxProtocolEchoFactory()=default;
    virtual std::unique_ptr<RxProtoProcessor> new_proto_processor(RxConnection *conn) override{
        return std::make_unique<RxProtoEchoProcessor>(conn);
    }
};

} //namespace Rinx

#endif // PROTOCOLECHO_H

