#include "ProtocolProcessor.h"


RxProtoProcessor::~RxProtoProcessor()
{

}

RxConnection *RxProtoProcessor::get_connection()
{
    return _conn_belongs;
}
