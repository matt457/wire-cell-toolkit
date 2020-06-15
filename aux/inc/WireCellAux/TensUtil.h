#include "WireCellIface/ITensor.h"
#include "WireCellIface/ITensorSet.h"

#include <string>

namespace WireCell {
    namespace Aux {

        // Return a pointer to an ITensor in set by matching metadata attributes tag and type.
        ITensor::pointer get_tens(const ITensorSet::pointer& in, std::string tag, std::string type);
    }
}
    
