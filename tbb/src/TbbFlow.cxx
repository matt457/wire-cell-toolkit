#include "WireCellTbb/TbbFlow.h"

#include "WireCellUtil/NamedFactory.h"
#include "WireCellUtil/String.h"
#include "WireCellUtil/Persist.h"

#include <string>
#include <iostream>

WIRECELL_FACTORY(TbbFlow, WireCellTbb::TbbFlow,
                 WireCell::IApplication, WireCell::IConfigurable)

using namespace WireCell;
using namespace WireCellTbb;

TbbFlow::TbbFlow()
{
}

TbbFlow::~TbbFlow()
{
}

Configuration TbbFlow::default_configuration() const
{
    std::string json = R"(
{
"dfp": "TbbDataFlowGraph",
"graph":[]
}
)";
    return Persist::loads(json);
}

void TbbFlow::configure(const Configuration& cfg)
{
    std::string type, name, desc = get<std::string>(cfg, "dfp","TbbDataFlowGraph");
    std::tie(type,name) = String::parse_pair(desc);
    m_dfp = Factory::lookup<IDataFlowGraph>(type, name);

    m_dfpgraph.configure(cfg["graph"]);
    
}

void TbbFlow::execute()
{
    if (!m_dfp) {
	std::cerr << "TbbFlow: not configured\n";
	return;
    }

    std::cerr << "TbbFlow::Execute\n";

    for (auto thc : m_dfpgraph.connections()) {
	auto tail_tn = get<0>(thc);
	auto head_tn = get<1>(thc);
	auto conn = get<2>(thc);

	std::cerr << "TbbFlow: Connect: "
		  <<  tail_tn.type << ":" << tail_tn.name
		  << " ( " << conn.tail << " -> " << conn.head << " ) "
		  << head_tn.type << ":" << head_tn.name << "\n";

	INode::pointer tail_node = WireCell::Factory::lookup<INode>(tail_tn.type, tail_tn.name);
	INode::pointer head_node = WireCell::Factory::lookup<INode>(head_tn.type, head_tn.name);

	m_dfp->connect(tail_node, head_node, conn.tail, conn.head);
    }


    m_dfp->run();
}

