#include "WireCellZio/TensorSetSink.h"
#include "WireCellZio/FlowConfigurable.h"
#include "WireCellZio/TensUtil.h"
#include "WireCellUtil/NamedFactory.h"
#include "WireCellUtil/Logging.h"

WIRECELL_FACTORY(ZioTensorSetSink, WireCell::Zio::TensorSetSink,
                 WireCell::ITensorSetSink, WireCell::IConfigurable)

using namespace WireCell;

Zio::TensorSetSink::TensorSetSink()
    : FlowConfigurable("extract"),
      m_had_eos(false)
{
}

Zio::TensorSetSink::~TensorSetSink() {}

void Zio::TensorSetSink::post_configure()
{
    if (!pre_flow()) {
        throw std::runtime_error("Failed to set up flow.  Is server alive?");
    }
}

bool Zio::TensorSetSink::operator()(const ITensorSet::pointer &in)
{
    auto log = Log::logger("wctzio");

    pre_flow();
    if (!m_flow) {
        log->debug("[TensorSetSink {}:{}] no flow",
                 m_node.nick(), m_portname);                
        return false;
    }

    if (!in) {  // eos
        log->debug("[TensorSetSink {}:{}] got EOS",
                 m_node.nick(), m_portname);                
        if (m_had_eos) {
            finalize();
            return false;
        }
        m_had_eos = true;
        log->debug("[TensorSetSink {}:{}] send empty message as EOS",
                 m_node.nick(), m_portname);                
        zio::Message eos("FLOW"); // send empty DAT as WCT EOS
        bool noto;
        try {
            noto = m_flow->put(eos);
        }
        catch (zio::flow::end_of_transmission) {
            log->debug("[TensorSetSink {}:{}] receive EOT on flow put EOS",
                     m_node.nick(), m_portname);

            m_flow->eotack();
            m_flow = nullptr;
            return false;
        }
        if (!noto) { // timeout
            log->warn("[TensorSetSink {}:{}] timeout on flow put EOS",
                    m_node.nick(), m_portname);
            finalize();
            return false;
        }
        log->debug("[TensorSetSink {}:{}] sent EOS",
                 m_node.nick(), m_portname);                

        return true;
    }

    m_had_eos = false;
    zio::Message msg = Zio::pack(in);

    log->debug("[TensorSetSink {}:{}] putting DAT",
             m_node.nick(), m_portname);

    bool noto;
    try {
        noto = m_flow->put(msg);
    }
    catch (zio::flow::end_of_transmission) {
        log->debug("[TensorSetSink {}:{}] got EOT on flow put DAT",
                 m_node.nick(), m_portname);
        m_flow->eotack();
        m_flow = nullptr;
        return false;
    }
    if (!noto) { // timeout
        log->warn("[TensorSetSink {}:{}] timeout on flow put",
                m_node.nick(), m_portname);
        finalize();
        return false;
    }
    log->debug("[TensorSetSink {}:{}] sent tensor as DAT",
             m_node.nick(), m_portname);                

    return true;
}

