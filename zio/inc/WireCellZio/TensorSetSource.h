/**
 * zio::Message -> WireCell::ITensorSet
 */

#ifndef WIRECELLZIO_ZIOTENSORSETSOURCE
#define WIRECELLZIO_ZIOTENSORSETSOURCE

#include "WireCellZio/FlowConfigurable.h"
#include "WireCellIface/ITensorSetSource.h"

namespace WireCell
{
namespace Zio
{

class TensorSetSource : public ITensorSetSource, public FlowConfigurable
{
public:
    TensorSetSource();
    virtual ~TensorSetSource();

    virtual bool operator()(ITensorSet::pointer &out);

private:
    bool m_had_eos;
    ITensorSet::vector m_tensors; // current set of depos
};
} // namespace Zio
} // namespace WireCell
#endif // WIRECELLZIO_ZIOTENSORSETSOURCE
