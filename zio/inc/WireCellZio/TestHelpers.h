/** Here we put some code which is mostly useful for more than one test.
 */

#ifndef WIRECELLZIO_TESTHELPERS
#define WIRECELLZIO_TESTHELPERS

#include "WireCellUtil/Configuration.h"
#include "zio/util.hpp"

namespace WireCell {
    namespace Test {

        // An actor function with input and an output flow ports.
        // These ports bind and may be discovered by their node/port
        // names which are given by the cfg.
        void flow_middleman(zio::socket_t& link, Configuration cfg);

        /// An actor function with a flow output port which connects
        /// to a flow input port based on a node/port name pair.  The
        /// config is that of TensorSetSink.
        void flow_giver(zio::socket_t& link, Configuration cfg);
        /// An actor function with a flow input port which connects to
        /// a flow output port based on a node/port name pair.  The
        /// config is that of TensorSetSource.
        void flow_taker(zio::socket_t& link, Configuration cfg);

    }


}

#endif
