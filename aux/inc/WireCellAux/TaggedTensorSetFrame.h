#ifndef WIRECELL_AUX_TAGGEDTENSORSETFRAME
#define WIRECELL_AUX_TAGGEDTENSORSETFRAME

#include "WireCellIface/IConfigurable.h"
#include "WireCellIface/ITensorSetFrame.h"

#include <string>
#include <unordered_set>

namespace WireCell {
    namespace Aux {

        /*! Produce a frame with tagged traces from a tensor set. 
         *
         * This is essentially the inverse of a @ref TaggedFrameTensorSet.  
         * It expects tensors and metadata as specified by that converter.
         *
         * It takes a similar configuration as its cousin where the
         * tag is used to locate the tensor to convert to a set of
         * tagged traces.
         *
         * Note, the set of traces are simply the contanation of all
         * waveform tensors.  No deduplication is done.
         */
        class TaggedTensorSetFrame
            : public WireCell::IConfigurable
            , public WireCell::ITensorSetFrame {
        public:
            TaggedTensorSetFrame();
            virtual ~TaggedTensorSetFrame();

            virtual bool operator()(const input_pointer& in, output_pointer& out);
            
            virtual WireCell::Configuration default_configuration() const;
            virtual void configure(const WireCell::Configuration& config);

        private:
            std::unordered_set<std::string> m_tags;
        };
        
    }
}

#endif
