#ifndef WIRECELL_AUX_TAGGEDFRAMETENSORSET
#define WIRECELL_AUX_TAGGEDFRAMETENSORSET

#include "WireCellIface/IFrameTensorSet.h"
#include "WireCellIface/IConfigurable.h"

namespace WireCell {
    namespace Aux {

        /*! Produce a tensors and metadata from a frame with tags.
         *
         * The resulting waveform tensors are dense and thus represent
         * an information loss in the case of a sparse frame.
         *
         * The inverse converter is @ref TaggedTensorSetFrame.
         *
         * The converter configuration consists of an object
         * containing an attribute "tensors" that holds an array of
         * "tensor config objects", each of which has the attributes:
         *
         * - tag :: the frame trace tag supplying the traces, optional, default='""'.
         *
         * - pad :: number to pad sparse tensors, optional, default=0.0.
         *
         * Each element of the "tensors" configuration array, if the
         * "tag" is found, results in multiple tensors on output.  If
         * "tag" is not found, no tensors are output.
         *
         * Each "tag" produces the following set of tensor types:
         *
         * - waveform :: 2D float in channel (each row) X tick (each column)
         * - channels :: 1D int, gives channel IDs for each row
         * - summary :: 1D double, optional, gives a per-channel value
         *
         * Waveform tensors are in row-major (C-style) order with a
         * row holding one waveform from one channel.  Sparse frames
         * result in dense, padded tensors (initialized with "pad"
         * value to which input is summed).  Any overlapping traces
         * are summed.
         *
         * The resulting ITensorSet also provides a metadata object
         * with these metadata attributes:
         *
         * - time :: float, the frame's reference time  (in WCT's units)
         * - tick :: float, the sample period (in WCT's units)
         * - tags :: array of string, the tags saved
         *
         * Each tensor of a tag is saved with the following  metadata:
         *
         * - tag :: the corresponding tag
         * - type :: a string indicate frame tensor type ("waveform", "channels", "summary")
         *
         * Additional metadata depending on type:
         *
         * - tbin :: ("waveform" type) the time bin of the first column of the tensor
         * - pad :: the configured pad value
         * 
         */
        class TaggedFrameTensorSet
            : public WireCell::IConfigurable
            , public WireCell::IFrameTensorSet {
        public:
            TaggedFrameTensorSet();
            virtual ~TaggedFrameTensorSet();

            virtual bool operator()(const input_pointer& in, output_pointer& out);
            
            virtual WireCell::Configuration default_configuration() const;
            virtual void configure(const WireCell::Configuration& config);
        private:
            WireCell::Configuration m_cfg;
        };
    }
}

#endif


