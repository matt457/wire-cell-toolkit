#include "WireCellZio/TestHelpers.h"
#include "WireCellZio/TensorSetSink.h"
#include "WireCellZio/TensorSetSource.h"
#include "WireCellIface/ITensorSet.h"
#include "WireCellZio/TensUtil.h"
#include "zio/tens.hpp"

#include "zio/tens.hpp"
#include "zio/logging.hpp"

using namespace WireCell;

static zio::message_t msg_ready("READY", 5);
static zio::message_t msg_done("DONE", 4);

/// Make a dummy tensor set.
template<typename TYPE>
ITensorSet::pointer make_tensor_set(zio::json md = zio::json{})
{
    TYPE tensor[2][3][4] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23};
    const TYPE* tensor1 = (TYPE*) tensor;
    std::vector<size_t> shape={2,3,4};

    zio::Message msg1(zio::tens::form);

    // Add an initial, unrelated message part just to make sure tens
    // respects the rules that tens parts aren't all parts.
    msg1.add(zio::message_t((char*)nullptr,0));
    assert(msg1.payload().size() == 1);
    zio::tens::append(msg1, tensor1, shape);
    assert(msg1.payload().size() == 2);

    auto lo = msg1.label_object();
    lo["TENS"]["metadata"] = md;
    lo["TENS"]["tensors"][0]["metadata"]["extra"] = "hello world";
    msg1.set_label_object(lo);

    return Zio::unpack(msg1);
}

void Test::flow_middleman(zio::socket_t& link, Configuration cfg)
{
    const std::string nodename = get<std::string>(cfg,"nodename");
    const std::string oportname = get<std::string>(cfg,"oportname");
    const std::string iportname = get<std::string>(cfg,"iportname");
    const int cred = get<int>(cfg, "credit");
    zio::timeout_t timeout(get<int>(cfg, "timeout"));

    link.send(msg_ready, zio::send_flags::none); // ready

    zio::Node node(nodename);
    node.set_verbose(false);
    auto iport = node.port("takes", ZMQ_SERVER);
    auto oport = node.port("gives", ZMQ_SERVER);
    // server/client can't to inproc, "@" is "abstract namespace" and Linux-only
    iport->bind();
    oport->bind();
    zio::Flow iflow(iport, zio::flow::direction_e::inject, cred, timeout);
    zio::Flow oflow(oport, zio::flow::direction_e::extract, cred, timeout);
    node.online();

    zio::debug("[middleman]: online");

    zio::poller_t<> poller;
    poller.add(iport->socket(), zio::event_flags::pollin);
    poller.add(oport->socket(), zio::event_flags::pollin);
    poller.add(link, zio::event_flags::pollin);

    bool noto; // no timeout

    zio::debug("[middleman]: bot handshake with {}", iport->name()); 
    noto = iflow.bot();    // let EOT exception past
    if (!noto) {
        zio::debug("[middleman]: bot handshake timeout with {}", iport->name());
        throw(std::runtime_error("unexpected bot handshake timeout with " + iport->name()));
    }

    zio::debug("[middleman]: bot handshake with {}", oport->name()); 
    noto = oflow.bot();    // let EOT exception past
    if (!noto) {
        zio::debug("[middleman]: bot handshake timeout with {}", oport->name());
        throw(std::runtime_error("unexpected bot handshake timeout with " + oport->name()));
    }

    iflow.pay();                // flushes pay to giver client
    oflow.pay();                // income pay from taker client, if any

    // a short max wait on flow ports.  This makes the loop into a
    // slow spin.  Efectively ping-ponging between iport/oport.  A
    // more proper way is to put both in a common poll.
    const auto wait = std::chrono::milliseconds{100};

    zio::debug("[middleman]: starting loop with timeout {}", wait.count()); 

    std::vector<zio::poller_event<>> events(3);

    bool keep_going = true;
    while (keep_going) {

        int nhits = poller.wait_all(events, wait);

        for (int ihit=0; ihit < nhits; ++ihit) {
            if (events[ihit].socket == link) {
                zio::info("[middleman]: link hit");
                iflow.eot();
                oflow.eot();
                keep_going = false;
                break;
            }

            if (events[ihit].socket == oport->socket()) {

                zio::Message msg;
                bool noto = oflow.recv(msg); // doesn't throw on EOT
                assert(noto);                // poll assures
                zio::debug("[middleman]: oport hit {}", msg.label());
                
                const zio::flow::Label lab(msg);
                if (lab.msgtype() == zio::flow::msgtype_e::eot) {
                    oflow.eotack(msg);
                    iflow.eot(msg);
                    keep_going = false;
                    break;
                }
                // otherwise, this must be pay, which should be
                // processed correctly by recv().
                assert(lab.msgtype() == zio::flow::msgtype_e::pay);

            }

            if (events[ihit].socket == iport->socket()) {

                zio::Message msg;
                bool noto = iflow.recv(msg); // doesn't throw on EOT
                assert(noto);                // poll assures
                zio::debug("[middleman]: iport hit {}", msg.label());
                
                const zio::flow::Label lab(msg);
                if (lab.msgtype() == zio::flow::msgtype_e::eot) {
                    zio::debug("[middleman]: EOT from {}", iport->name());
                    iflow.eotack(msg);
                    oflow.eot(msg);
                    keep_going = false;
                    break;
                }

                if (lab.msgtype() == zio::flow::msgtype_e::dat) {
                    bool noto;
                    try {
                        noto = oflow.put(msg);
                    }
                    catch (zio::flow::end_of_transmission) {
                        zio::debug("[middleman]: EOT from PUT with {}", iport->name());
                        oflow.eotack();
                        iflow.eot();
                        keep_going = false;
                        break;
                    }
                    if (!noto) {
                        zio::debug("[middleman]: timeout from PUT with {}", iport->name());
                        iflow.eot();
                        oflow.eot();
                        keep_going = false;
                        break;
                    }
                    oflow.pay(); // flush pay
                }
            }
        }
    }
    link.send(msg_done, zio::send_flags::none);    
    zio::info("[middleman]: going offline");
    node.offline();
}

void Test::flow_giver(zio::socket_t& link, Configuration ucfg)
{
    link.send(msg_ready, zio::send_flags::none);

    Zio::TensorSetSink sink;
    auto cfg = sink.default_configuration();
    WireCell::update(cfg, ucfg);
    // cfg["verbose"] = 0;
    // cfg["nodename"] = "giver";
    // cfg["portname"] = "givec";
    // cfg["timeout"] = 5000;
    // cfg["connects"][0]["nodename"] = "middleman";
    // cfg["connects"][0]["portname"] = "takes";
    sink.configure(cfg);

    const std::string prefix = fmt::format("[{} {}]", 
                                           get<std::string>(cfg,"nodename"),
                                           get<std::string>(cfg,"portname"));

    for (int ind=0; ind<10; ++ind) {
        zio::info("[giver]: pretending work");
        zio::sleep_ms(zio::time_unit_t{100});
        auto ts = make_tensor_set<int>();
        assert(ts);
        bool ok = sink(ts);
        if (!ok) {
            zio::info("{} failed to give", prefix);
            break;
        }
        zio::info("[giver]: sunk ITensorSet");

        zio::message_t msg;     // check for term
        auto res = link.recv(msg, zio::recv_flags::dontwait);
        if (res) {
            zio::info("{} link hit", prefix);
            break;
        }
    }
    zio::info("{} send EOS", prefix);
    sink(nullptr);
    zio::info("{} send EOT", prefix);
    sink(nullptr);

    zio::info("{} sending actor done message", prefix);
    link.send(msg_done, zio::send_flags::none);    
}

void Test::flow_taker(zio::socket_t& link, Configuration ucfg)
{
    link.send(msg_ready, zio::send_flags::none);

    Zio::TensorSetSource src;
    auto cfg = src.default_configuration();
    WireCell::update(cfg, ucfg);
    // cfg["verbose"] = 0;
    // cfg["nodename"] = "taker";
    // cfg["portname"] = "takec";
    // cfg["timeout"] = 5000;
    // cfg["connects"][0]["nodename"] = "middleman";
    // cfg["connects"][0]["portname"] = "gives";
    src.configure(cfg);

    const std::string prefix = fmt::format("[{} {}]", 
                                           get<std::string>(cfg,"nodename"),
                                           get<std::string>(cfg,"portname"));
    while (true) {
        ITensorSet::pointer ts;
        bool ok = src(ts);
        if (!ok) {
            zio::info("{} failed to take", prefix);
            break;
        }
        if (!ts) {
            zio::info("{} EOS", prefix);
            break;
        }
        zio::info("{} sourced ITensorSet", prefix);

        zio::message_t msg;     // check for term
        auto res = link.recv(msg, zio::recv_flags::dontwait);
        if (res) {
            zio::info("{} link hit", prefix);
            break;
        }
    }

    zio::info("{} sending actor done message", prefix);
    link.send(msg_done, zio::send_flags::none);    
}
