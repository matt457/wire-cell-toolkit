#include "WireCellZio/TensUtil.h"
#include "WireCellAux/SimpleTensorSet.h"
#include "WireCellAux/SimpleTensor.h"
#include "WireCellUtil/Logging.h"
#include "zio/tens.hpp"
#include <sstream>

using namespace WireCell;

void Zio::zio_to_wct(const zio::json& zioj, Json::Value& wctj)
{
    std::stringstream ss(zioj.dump());
    ss >> wctj;
}
void Zio::wct_to_zio(const Json::Value& wctj, zio::json& zioj)
{
    std::stringstream ss;
    ss << wctj;
    zioj = zio::json::parse(ss.str());
}


zio::Message Zio::pack(const ITensorSet::pointer & itens)
{
    Configuration label;
    auto md = itens->metadata();
    if (!md.isNull()) {
        label[zio::tens::form]["metadata"] = md;
    }

    zio::Message msg(zio::tens::form);
    Json::FastWriter jwriter;
    msg.set_label(jwriter.write(label));
    
    for(auto ten : *(itens->tensors())) {
        zio::json md;
        wct_to_zio(ten->metadata(), md);

        zio::tens::append(msg,
                          zio::message_t(ten->data(), ten->size()),
                          ten->shape(),
                          ten->element_size(),
                          zio::tens::type_name(ten->element_type()), md);
    }
    
    return msg;
}

template<typename TYPE>
void fill_tensor(ITensor::vector* itv, const std::vector<size_t>& shape,
                 const zio::message_t& one, const zio::json& md)
{
    Aux::SimpleTensor<TYPE>* st = new Aux::SimpleTensor<TYPE>(shape);
    auto& store = st->store();
    memcpy(store.data(), one.data(), one.size());
    if (! md.is_null()) {
        Zio::zio_to_wct(md, st->metadata());
    }
    
    itv->push_back(ITensor::pointer(st));
}



ITensorSet::pointer Zio::unpack(const zio::Message& zmsg)
{
    ITensor::vector* itv = new ITensor::vector;

    auto log = Log::logger("tens");

    auto label = zmsg.label_object();

    for(auto jten : label[zio::tens::form]["tensors"]) {

        // log->debug("jten: {}", jten.dump());

        auto md = jten["metadata"];
        std::vector<size_t> shape = jten["shape"].get<std::vector<size_t>>();

        int ind = jten["part"].get<int>();
        const zio::message_t& one = zio::tens::at(zmsg, ind);

        int word = jten["word"].get<int>();
        std::string dtype = jten["dtype"].get<std::string>();
        if (dtype == "f") {
            if (word == 4) {
                fill_tensor<float>(itv, shape, one, md);
            }
            else if (word == 8) {
                fill_tensor<double>(itv, shape, one, md);
            }
            else{
                log->error("unknown floating point size: {}", word);
            }
        }
        else if (dtype == "i") {
            if (word == 1) {
                fill_tensor<int8_t>(itv, shape, one, md);
            }
            else if (word == 2) {
                fill_tensor<int16_t>(itv, shape, one, md);
            }
            else if (word == 4) {
                fill_tensor<int32_t>(itv, shape, one, md);
            }
            else if (word == 8) {
                fill_tensor<int64_t>(itv, shape, one, md);
            }
            else {
                log->error("unknown integer size: {}", word);
            }
        }
        else if (dtype == "u") {
            if (word == 1) {
                fill_tensor<uint8_t>(itv, shape, one, md);
            }
            else if (word == 2) {
                fill_tensor<uint16_t>(itv, shape, one, md);
            }
            else if (word == 4) {
                fill_tensor<uint32_t>(itv, shape, one, md);
            }
            else if (word == 8) {
                fill_tensor<uint64_t>(itv, shape, one, md);
            }
            else {
                log->error("unknown unsigned integer size: {}", word);
            }
        }
        else {
            log->error("unknown dtype: {}, size: {}", dtype, word);
        }
    }

    // fixme: this violates layers.  ITensorSet has no seqno.  It's
    // ident needs to be stored as TENS metadata in pack() and then
    // removed here.
    int seqno = zmsg.seqno();   
    Configuration md;
    Json::Reader reader;
    reader.parse(label[zio::tens::form]["metadata"].dump(), md);

    return std::make_shared<Aux::SimpleTensorSet>(seqno, md, ITensor::shared_vector(itv));
}


