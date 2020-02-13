#include "Protocol/ProtocolProcessor.h"

namespace Rinx {

RxProtoProcessor::~RxProtoProcessor()
{

}

RxConnection *RxProtoProcessor::get_connection()
{
    return _conn_belongs;
}

} //namespace Rinx
