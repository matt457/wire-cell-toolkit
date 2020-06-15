#ifndef WIRECELL_ITENSORSET
#define WIRECELL_ITENSORSET

#include "WireCellIface/ITensor.h"
#include "WireCellUtil/Configuration.h"

namespace WireCell {

    class ITensorSet : public IData<ITensorSet> {
    public:

        virtual ~ITensorSet() {}

        /// Return some identifier number that is unique to this set.
        virtual int ident() const = 0;

        /// Optional metadata associated with the set of tensors
        virtual Configuration metadata() const {
            return Configuration();
        }

        /// Return the tensors in this set.
        virtual ITensor::shared_vector tensors() const = 0;

    };
}

#endif
