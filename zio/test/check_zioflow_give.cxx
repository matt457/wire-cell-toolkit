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

    Configuration g_cfg;
    g_cfg["verbose"] = 0;
    g_cfg["nodename"] = "giver";
    g_cfg["portname"] = "givec";
    g_cfg["timeout"] = 5000;
    g_cfg["connects"][0]["nodename"] = mm_node;
    g_cfg["connects"][0]["portname"] = mm_iport;
    g_cfg["attributes"]["stream"] = "check_zioflow";

    zio::context_t ctx;
    zio::zactor_t g(ctx, flow_giver, g_cfg);

    zio::message_t msg;
    {
        auto res = g.link().recv(msg, zio::recv_flags::none);
        Assert(res);
        log->info("giver returned");
    }

    return 0;
}
      
