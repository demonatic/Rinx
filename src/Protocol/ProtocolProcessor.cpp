#include "Protocol/ProtocolProcessor.h"

namespace Rinx {

RxProtoProcessor::~RxProtoProcessor()
{

}

RxConnection *RxProtoProcessor::get_connection()
{
    assert(_conn_belongs!=nullptr);
    return _conn_belongs;
}

} //namespace Rinx
