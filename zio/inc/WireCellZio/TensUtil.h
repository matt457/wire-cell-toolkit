#include "WireCellIface/ITensorSet.h"
#include "zio/message.hpp"

namespace WireCell {
    namespace Zio {

        /// Pack the ITensorSet into a ZIO Message
        zio::Message pack(const ITensorSet::pointer & itens);

        /// Unpack ZIO Message to ITensorSet
        ITensorSet::pointer unpack(const zio::Message& zmsg);


        /// Convert from zio to wct json objects.
        void zio_to_wct(const zio::json& zioj, Json::Value& wctj);

        /// Convert from wct to zio json objects.
        void wct_to_zio(const Json::Value& wctj, zio::json& zioj);


    }
}
