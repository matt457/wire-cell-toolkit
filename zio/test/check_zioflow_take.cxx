/// Test tensor -> flow broker -> tensor where the broker is some
/// external service such as "zio flow-file-server".

#include "WireCellZio/TestHelpers.h"
#include "WireCellUtil/Testing.h"
#include "WireCellUtil/Logging.h"

#include "zio/actor.hpp"
#include "zio/logging.hpp"
#include "zio/main.hpp"

using namespace WireCell;
using namespace WireCell::Test;

static zio::message_t msg_term("TERM", 4);

int main()
{
    Log::add_stdout(true, "debug");
    Log::set_level("debug");
    auto log = Log::logger("test");
    log->info("Started WCT logging");
    zio::init_all();
    zio::info("Started ZIO logging");

    std::string mm_node = "zioflow";
    std::string mm_iport = "flow";
    std::string mm_oport = "flow";

    Configuration t_cfg;
    t_cfg["verbose"] = 0;
    t_cfg["nodename"] = "taker";
    t_cfg["portname"] = "takec";
    t_cfg["timeout"] = 5000;
    t_cfg["connects"][0]["nodename"] = mm_node;
    t_cfg["connects"][0]["portname"] = mm_oport;
    t_cfg["attributes"]["stream"] = "check_zioflow";
    
    zio::context_t ctx;
    zio::zactor_t t(ctx, flow_taker, t_cfg);

    zio::message_t msg;
    {
        auto res = t.link().recv(msg, zio::recv_flags::none);
        Assert(res);
        log->info("taker returned");
    }

    return 0;
}
      
