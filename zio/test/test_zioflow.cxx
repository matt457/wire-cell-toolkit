/// Test tensor -> flow -> tensor 

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

    Configuration mm_cfg;
    mm_cfg["nodename"] = "middleman";
    mm_cfg["oportname"] = "gives";
    mm_cfg["iportname"] = "takes";
    mm_cfg["credit"] = 10;
    mm_cfg["timeout"] = 50000;

    Configuration g_cfg;
    g_cfg["verbose"] = 0;
    g_cfg["nodename"] = "giver";
    g_cfg["portname"] = "givec";
    g_cfg["timeout"] = 5000;
    g_cfg["connects"][0]["nodename"] = mm_cfg["nodename"];
    g_cfg["connects"][0]["portname"] = mm_cfg["iportname"];

    Configuration t_cfg;
    t_cfg["verbose"] = 0;
    t_cfg["nodename"] = "taker";
    t_cfg["portname"] = "takec";
    t_cfg["timeout"] = 5000;
    t_cfg["connects"][0]["nodename"] = mm_cfg["nodename"];
    t_cfg["connects"][0]["portname"] = mm_cfg["oportname"];


    zio::context_t ctx;
    zio::zactor_t mm(ctx, flow_middleman, mm_cfg);
    zio::zactor_t g(ctx, flow_giver, g_cfg);
    zio::zactor_t t(ctx, flow_taker, t_cfg);

    zio::message_t msg;
    {
        auto res = g.link().recv(msg, zio::recv_flags::none);
        Assert(res);
        log->info("giver returned");
    }
    {
        auto res = t.link().recv(msg, zio::recv_flags::none);
        Assert(res);
        log->info("taker returned");
    }
    {
        log->info("shutdown middleman");
        auto sres = mm.link().send(msg_term, zio::send_flags::none);
        Assert(sres);
        log->info("wait for middleman");
        auto rres = mm.link().recv(msg_term, zio::recv_flags::none);
        Assert(rres);
        log->info("middleman returned");
    }

    return 0;
}
      
