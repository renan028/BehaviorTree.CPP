#include "behaviortree_cpp_v3/bt_factory.h"
#include "find_toy_bt_nodes.h"
#include "behaviortree_cpp_v3/loggers/bt_file_logger.h"

using namespace BT;

int main(int argc, char** argv)
{
    if( argc != 2)
    {
        std::cout <<" missing name of the XML file to open" << std::endl;
        return 1;
    }

    BehaviorTreeFactory factory;
    FindToyBT::RegisterNodes(factory);


    auto tree = factory.createTreeFromFile(argv[1]);

    for( auto& node: tree.nodes )
    {
        if( auto inspect = dynamic_cast<FindToyBT::InspectRoomToy*>(node.get()))
        {
            inspect->init("beedrom");
        }
    }

    FileLogger logger_file(tree, "bt_trace_t11.fbl");

    while( tree.root_node->executeTick() == NodeStatus::RUNNING)
    {
        std::this_thread::sleep_for( std::chrono::milliseconds(10) );
    }

    return 0;
}
