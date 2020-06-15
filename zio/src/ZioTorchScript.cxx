#include "WireCellZio/ZioTorchScript.h"
#include "WireCellUtil/NamedFactory.h"
#include "WireCellUtil/String.h"

#include "WireCellAux/SimpleTensorSet.h"
#include "WireCellAux/SimpleTensor.h"
#include "WireCellIface/ITensorSet.h"
#include "WireCellUtil/Exceptions.h"

#include "WireCellZio/TensUtil.h"

#include "zio/domo/client.hpp"
#include "zio/tens.hpp"

#include <thread>

WIRECELL_FACTORY(ZioTorchScript, WireCell::Pytorch::ZioTorchScript,
                 WireCell::ITensorSetFilter, WireCell::IConfigurable)

using namespace WireCell;

Pytorch::ZioTorchScript::ZioTorchScript() : l(Log::logger("pytorch")) {}

Configuration Pytorch::ZioTorchScript::default_configuration() const
{
    Configuration cfg;

    // zio
    cfg["address"] = "tcp://localhost:5555";
    cfg["service"] = "torch:dnnroi";

    // TorchScript model
    cfg["model"] = "model.ts";
    cfg["gpu"] = true;

    // if failed, wait this time and try again
    cfg["wait_time"] = 500; // ms

    // for debug
    cfg["nloop"] = 10;

    return cfg;
}

void Pytorch::ZioTorchScript::configure(const WireCell::Configuration &cfg)
{
    m_cfg = cfg;
}

namespace
{
std::string dump(const zio::Message &msg) {
    std::stringstream ss;
    ss << "zio.Message: ";
    ss << "ZIO" << msg.level() << msg.form() << msg.label();
    ss << " + [0x" << msg.origin() << "," << msg.granule() << "," << msg.seqno() << "]";
    ss << " + [" << msg.payload().str() << "]";
    return ss.str();
}
std::string dump(const ITensorSet::pointer &itens) {
    std::stringstream ss;
    ss << "ITensorSet: ";
    Json::FastWriter jwriter;
    ss << itens->ident() << ", " << jwriter.write(itens->metadata());
    for (auto iten : *itens->tensors()) {
        ss << "shape: [";
        for(auto l : iten->shape()) {
            ss << l << " ";
        }
        ss << "]\n";
    }
    return ss.str();
}
} // namespace

bool Pytorch::ZioTorchScript::operator()(const ITensorSet::pointer& in, ITensorSet::pointer& out)
{
    l->debug("ZioTorchScript::forward");

    if (!in)
    {
        out = nullptr;
        return true;
    }

    int wait_time = m_cfg["wait_time"].asInt();
    int thread_wait_time = 0;

    zmq::context_t ctx;
    zmq::socket_t sock(ctx, ZMQ_CLIENT);
    zio::domo::Client m_client(sock, get<std::string>(m_cfg, "address", "tcp://localhost:5555"));

    // for(int iloop=0; iloop<m_cfg["nloop"].asInt();++iloop) {
    bool success = false;
    while (!success)
    {
        try
        {
            auto msg = Zio::pack(in);
            zmq::multipart_t mmsg(msg.toparts());
            m_client.send(get<std::string>(m_cfg, "service", "torch:dnnroi"), mmsg);
            mmsg.clear();
            m_client.recv(mmsg);
            msg.fromparts(mmsg);
            out = Zio::unpack(msg);
            
            success = true;
        }
        catch (...)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(wait_time));
            thread_wait_time += wait_time;
        }
    }
    // }

    l->debug("thread_wait_time: {} sec", thread_wait_time / 1000.);

    return true;
}
