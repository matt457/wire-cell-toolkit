#ifndef WIRECELL_ITENSORSETSINK
#define WIRECELL_ITENSORSETSINK

#include "WireCellIface/ITensorSet.h"
#include "WireCellIface/ISinkNode.h"
#include "WireCellUtil/IComponent.h"

namespace WireCell
{

/** A frame sink is something that generates IFrames.
 */
class ITensorSetSink : public ISinkNode<ITensorSet>
{
public:
    typedef std::shared_ptr<ITensorSetSink> pointer;

    virtual ~ITensorSetSink() {};

    // subclass supply:
    // virtual bool operator()(const ITensorSet::pointer& in);
};

} // namespace WireCell

#endif
