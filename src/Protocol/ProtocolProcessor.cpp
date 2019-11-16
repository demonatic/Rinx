#include "ProtocolProcessor.h"


RxProtoProcessor::~RxProtoProcessor()
{

}

RxConnection *RxProtoProcessor::conn_belongs()
{
    return _conn_belongs;
}
