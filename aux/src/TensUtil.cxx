#include "WireCellAux/TensUtil.h"
using namespace WireCell;

ITensor::pointer Aux::get_tens(const ITensorSet::pointer& in, std::string tag, std::string type)
{
    auto tens = in->tensors();
    size_t ntens = tens->size();

    // not the fastest search but for small numbers, it's maybe okay.
    for (size_t ind=0; ind<ntens; ++ind) {
        auto ten = tens->at(ind);
        const auto& md = ten->metadata();
        if (md["tag"] == tag and md["type"] == type) {
            return ten;
        }
    }
    return nullptr;
}

