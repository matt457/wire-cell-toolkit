#include "WireCellUtil/Exceptions.h"
#include "WireCellUtil/Logging.h"
#include "WireCellZio/FlowConfigurable.h"

#include <memory>

using namespace WireCell;

Zio::FlowConfigurable::FlowConfigurable(const std::string& direction,
                                        const std::string& nodename)
    : m_direction(direction)
    , m_node(nodename, (zio::origin_t)this)
{
}

Zio::FlowConfigurable::~FlowConfigurable()
{
    m_node.offline();
}

WireCell::Configuration Zio::FlowConfigurable::default_configuration() const
{
    Configuration cfg;
    // zmq
    cfg["timeout"] = 1000;      // ms
    cfg["binds"] = Json::arrayValue;
    cfg["connects"] = Json::arrayValue;
    cfg["stype"] = m_stype;
    // zyre
    cfg["nodename"] = m_node.nick();
    cfg["headers"] = Json::objectValue;
    // zio
    cfg["portname"] = m_portname;
    // msg
    cfg["origin"] = (Json::UInt64)m_node.origin();
    cfg["level"] = (int)m_level;
    // flow
    cfg["credit"] = 10;
    // zdb
    cfg["attributes"] = Json::objectValue;

    user_default_configuration(cfg);
    return cfg;
}


void Zio::FlowConfigurable::configure(const WireCell::Configuration& cfg)
{
    auto log = Log::logger("wctzio");
    const std::string nick = get<std::string>(cfg, "nodename", m_node.nick());

    bool ok = user_configure(cfg);
    if (!ok) {
        log->critical("node {}: configuration failed", nick);
        THROW(ValueError() << errmsg{"configuration failed for " + nick});
    }

    zio::timeout_t timeout(get<int>(cfg, "timeout", 1000));
    int cred = get<int>(cfg, "credit", 10);

    m_node.set_nick(nick);
    m_node.set_origin(get(cfg, "origin",  (Json::UInt64)m_node.origin()));
    m_portname = get<std::string>(cfg, "portname", m_portname);
    m_level = (zio::level::MessageLevel)get<int>(cfg, "level", (int)m_level);
    m_stype = get<int>(cfg, "stype", m_stype);
    if (! (m_stype == ZMQ_CLIENT or m_stype == ZMQ_SERVER)) {
        log->error("node {}: may only use CLIENT or SERVER stype, got {}",
                 nick, m_stype);
        THROW(RuntimeError() << errmsg{"unsupported socket type"});
    }

    int verbose = 0;
    if (! cfg["verbose"].empty()) {
        verbose = cfg["verbose"].asInt();
        if (verbose) {
            log->debug("node {}: verbose", nick);
            zsys_init();
            m_node.set_verbose(verbose);
        }
    }

    auto port = m_node.port(m_portname, m_stype);

    auto binds = cfg["binds"];
    auto connects = cfg["connects"];
    if (binds.empty() and connects.empty()) {
        log->info("node {}: binding {} to ephemeral", nick, m_portname);
        port->bind();       // default: ephemeral bind
    }
    else {
        for (auto bind : binds) {
            log->debug("node {}: bind: {}", nick, bind);
            if (bind.empty()) {
                log->info("node {}: bind to ephemeral", nick);
                port->bind();
                continue;
            }
            if (bind.isString()) {
                log->info("node {}: bind to {}", nick, bind);
                port->bind(bind.asString());
                continue;
            }
            if (bind.isObject()) {
                log->info("node {}: bind to {}", nick, bind);
                port->bind(bind["host"].asString(), bind["tcpportnum"].asInt());
                continue;
            }
            log->error("node {}: unknown bind type {}", nick, bind);
            THROW(RuntimeError() << errmsg{"unknown bind type"});
        }
        for (auto conn : connects) {
            log->debug("node {}: connect: {}", nick, conn);
            if (conn.isString()) {
                port->connect(conn.asString());
                continue;
            }
            if (conn.isObject()) {
                port->connect(conn["nodename"].asString(), conn["portname"].asString());
                continue;
            }
            log->error("node {}: unknown connect type {}", nick, conn);
            THROW(RuntimeError() << errmsg{"unknown connect type"});
        }
    }

    for (auto key : cfg["headers"].getMemberNames()) {
        auto val = cfg["headers"][key];
        m_headers[key] = val.asString();
    }

    zio::flow::direction_e dir;
    if (m_direction == "inject") { dir = zio::flow::direction_e::inject; }
    else if (m_direction == "extract") { dir = zio::flow::direction_e::extract; }
    else {
        THROW(RuntimeError() << errmsg{"unknown flow direction: " + m_direction});
    }

    m_flow = std::make_unique<zio::Flow>(port, dir, cred, timeout);
    if (!m_flow) {
        THROW(RuntimeError() << errmsg{"failed to make flow"});
    }


    log->debug("{}: going online with headers:", nick);
    for (const auto& hh : m_headers) {
        log->debug("\t{} = {}", hh.first, hh.second);
    }
    m_node.online(m_headers);

    ok = user_online();
    if (!ok) {
        log->critical("node {}: failed to go online", nick);
        THROW(ValueError() << errmsg{"online failed"});
    }
    post_configure();

    zio::json fobj;
    for (auto key : cfg["attributes"].getMemberNames()) {
        auto val = cfg["attributes"][key];
        fobj[key] = val.asString();
    }
    zio::Message msg;
    msg.set_label_object(fobj);
    bool noto = m_flow->bot(msg);
    if (!noto) {
        THROW(RuntimeError() << errmsg{"timeout in BOT " + m_portname});
    }        
}



bool Zio::FlowConfigurable::pre_flow()
{
    return true;
}

void Zio::FlowConfigurable::finalize()
{
    const std::string nick = m_node.nick();
    if (m_flow) {
        m_flow->eot();
        m_flow = nullptr;
    }
    m_node.offline();
}
