#include <map>

#include "behaviortree_cpp_v3/bt_factory.h"
#include "behaviortree_cpp_v3/loggers/bt_file_logger.h"
#include "behaviortree_cpp_v3/loggers/bt_minitrace_logger.h"

using namespace BT;

/**
 * In this tutorial, we test coroutines with the GrabBeer problem an validate 
 * it with Sequence and QueueNode.
 *
 */

class MyAsyncAction: public CoroActionNode
{
  public:
    MyAsyncAction(const std::string& name, const int reply = 100):
          CoroActionNode(name, {})
        , reply_(reply)
    {}

    void init(const int reply)
    {
        std::cout << this->name() << " reinitializing reply: " << reply << "\n";
        reply_ = reply;
    }

  private:
    // This is the ideal skeleton/template of an async action:
    //  - A request to a remote service provider.
    //  - A loop where we check if the reply has been received.
    //  - You may call setStatusRunningAndYield() to "pause".
    //  - Code to execute after the reply.
    //  - A simple way to handle halt().

    NodeStatus tick() override

    {
        std::cout << name() <<": Started. Send Request to server." << std::endl;

        auto Now = [](){ return std::chrono::high_resolution_clock::now(); };

        TimePoint initial_time = Now();
        TimePoint time_before_reply = initial_time + 
            std::chrono::milliseconds(reply_);

        int count = 0;
        bool reply_received = false;

        while( !reply_received )
        {
            if( count++ == 0)
            {
                // call this only once
                std::cout << name() <<": Waiting Reply..." << std::endl;
            }
            // pretend that we received a reply
            if( Now() >= time_before_reply )
            {
                reply_received = true;
            }

            if( !reply_received )
            {
                // set status to RUNNING and "pause/sleep"
                // If halt() is called, we will not resume execution (stack destroyed)
                setStatusRunningAndYield();
            }
        }

        // This part of the code is never reached if halt() is invoked,
        // only if reply_received == true;
        std::cout << name() <<": Done. 'Waiting Reply' loop repeated "
                  << count << " times" << std::endl;
        cleanup(false);
        return NodeStatus::SUCCESS;
    }

    // you might want to cleanup differently if it was halted or successful
    void cleanup(bool halted)
    {
        if( halted )
        {
            std::cout << name() <<": cleaning up after a halt()\n";
        }
        else{
            std::cout << name() <<": cleaning up after SUCCESS\n";
        }
    }
    void halt() override
    {
        std::cout << name() <<": Halted." << std::endl;
        cleanup(true);
        // Do not forget to call this at the end.
        CoroActionNode::halt();
        this->setStatus(BT::NodeStatus::FAILURE);
    }

    int reply_;
};

// clang-format off
static const char* xml_text = R"(
<root main_tree_to_execute="BehaviorTree">
    <BehaviorTree ID="BehaviorTree">
        <Sequence>
            <SetBlackboard output_key="slow" value="160" />
            <SetBlackboard output_key="fast" value="10" />
            <Timeout msec="150">
                <OpenFridge name="OpenFridge" ID="OpenFridge"/>
            </Timeout>
            <QueueNode>
                <Timeout msec="150">
                    <GrabBeer name="GrabBeer" ID="GrabBeer"/>
                </Timeout>
                <Timeout msec="150">
                    <CloseFridge name="CloseFridge" ID="CloseFridge"/>
                </Timeout>
            </QueueNode>
        </Sequence>
    </BehaviorTree>
</root>
)";

// clang-format on

int main()
{
    auto Now = [](){ return std::chrono::high_resolution_clock::now(); };

    std::map<BT::NodeStatus, std::string> stat = {
        std::make_pair(BT::NodeStatus::FAILURE, "FAILURE"),
        std::make_pair(BT::NodeStatus::SUCCESS, "SUCCESS"), 
        std::make_pair(BT::NodeStatus::RUNNING, "RUNNING"),
        std::make_pair(BT::NodeStatus::IDLE, "IDLE")};

    NodeBuilder FixedReply = [](const std::string& name, 
        const NodeConfiguration& config)
    {
        return std::make_unique<MyAsyncAction>( name, 100 );
    };

    BehaviorTreeFactory factory;
    factory.registerNodeType<MyAsyncAction>("OpenFridge");
    factory.registerNodeType<MyAsyncAction>("GrabBeer");
    factory.registerNodeType<MyAsyncAction>("CloseFridge");

    auto tree = factory.createTreeFromText(xml_text);

    int slow_reply;
    int fast_reply;
    std::string key;
    for( auto& node: tree.nodes )
    {
        if( auto blackboard = dynamic_cast<SetBlackboard*>(node.get()))
        {
            blackboard->getInput("output_key", key);
            if (key == "slow")
                blackboard->getInput<int>("value", slow_reply);
            else if(key == "fast")
                blackboard->getInput<int>("value", fast_reply);
        }
    }

    FileLogger logger_file(tree, "bt_trace.fbl");
    MinitraceLogger logger_minitrace(tree, "bt_trace.json");

    printTreeRecursively(tree.root_node);

    //----------------------
    // No Timeout
    for( auto& node: tree.nodes )
    {
        if( auto fridge_node = dynamic_cast<MyAsyncAction*>(node.get()))
        {
            if (fridge_node->name() == "OpenFridge")
                fridge_node->init(fast_reply);
            else if (fridge_node->name() == "GrabBeer")
                fridge_node->init(fast_reply);
            else if (fridge_node->name() == "CloseFridge")
                fridge_node->init(fast_reply);
        }
    }
    while( tree.root_node->executeTick() == NodeStatus::RUNNING)
    {
        std::this_thread::sleep_for( std::chrono::milliseconds(10) );
    }
    std::cout << "Sequence returns " << stat[tree.root_node->status()]
        << "\n------------\n";
    //----------------------

    //----------------------
    // Open Fridge Timeout
    for( auto& node: tree.nodes )
    {
        if( auto fridge_node = dynamic_cast<MyAsyncAction*>(node.get()))
        {
            if (fridge_node->name() == "OpenFridge")
                fridge_node->init(slow_reply);
            else if (fridge_node->name() == "GrabBeer")
                fridge_node->init(fast_reply);
            else if (fridge_node->name() == "CloseFridge")
                fridge_node->init(fast_reply);
        }
    }
    while( tree.root_node->executeTick() == NodeStatus::RUNNING)
    {
        std::this_thread::sleep_for( std::chrono::milliseconds(10) );
    }
    std::cout << "Sequence returns " << stat[tree.root_node->status()]
        << "\n------------\n";
    //----------------------

    //----------------------
    // Close Fridge Timeout
    for( auto& node: tree.nodes )
    {
        if( auto fridge_node = dynamic_cast<MyAsyncAction*>(node.get()))
        {
            if (fridge_node->name() == "OpenFridge")
                fridge_node->init(fast_reply);
            else if (fridge_node->name() == "GrabBeer")
                fridge_node->init(fast_reply);
            else if (fridge_node->name() == "CloseFridge")
                fridge_node->init(slow_reply);
        }
    }
    while( tree.root_node->executeTick() == NodeStatus::RUNNING)
    {
        std::this_thread::sleep_for( std::chrono::milliseconds(10) );
    }
    std::cout << "Sequence returns " << stat[tree.root_node->status()]
        << "\n------------\n";
    //----------------------

    //----------------------
    // GrabBeer Timeout
    for( auto& node: tree.nodes )
    {
        if( auto fridge_node = dynamic_cast<MyAsyncAction*>(node.get()))
        {
            if (fridge_node->name() == "OpenFridge")
                fridge_node->init(fast_reply);
            else if (fridge_node->name() == "GrabBeer")
                fridge_node->init(slow_reply);
            else if (fridge_node->name() == "CloseFridge")
                fridge_node->init(fast_reply);
        }
    }
    while( tree.root_node->executeTick() == NodeStatus::RUNNING)
    {
        std::this_thread::sleep_for( std::chrono::milliseconds(10) );
    }
    std::cout << "Sequence returns " << stat[tree.root_node->status()]
        << "\n------------\n";
    //----------------------

    return 0;
}

/* Expected output:

OpenFridge reinitializing reply: 10
GrabBeer reinitializing reply: 10
CloseFridge reinitializing reply: 10
OpenFridge: Started. Send Request to server.
OpenFridge: Waiting Reply...
OpenFridge: Done. 'Waiting Reply' loop repeated 2 times
OpenFridge: cleaning up after SUCCESS
GrabBeer: Started. Send Request to server.
GrabBeer: Waiting Reply...
GrabBeer: Done. 'Waiting Reply' loop repeated 2 times
GrabBeer: cleaning up after SUCCESS
CloseFridge: Started. Send Request to server.
CloseFridge: Waiting Reply...
CloseFridge: Done. 'Waiting Reply' loop repeated 2 times
CloseFridge: cleaning up after SUCCESS
Sequence returns SUCCESS
------------
OpenFridge reinitializing reply: 160
GrabBeer reinitializing reply: 10
CloseFridge reinitializing reply: 10
OpenFridge: Started. Send Request to server.
OpenFridge: Waiting Reply...
OpenFridge: Halted.
OpenFridge: cleaning up after a halt()
Sequence returns FAILURE
------------
OpenFridge reinitializing reply: 10
GrabBeer reinitializing reply: 10
CloseFridge reinitializing reply: 160
OpenFridge: Started. Send Request to server.
OpenFridge: Waiting Reply...
OpenFridge: Done. 'Waiting Reply' loop repeated 2 times
OpenFridge: cleaning up after SUCCESS
GrabBeer: Started. Send Request to server.
GrabBeer: Waiting Reply...
GrabBeer: Done. 'Waiting Reply' loop repeated 2 times
GrabBeer: cleaning up after SUCCESS
CloseFridge: Started. Send Request to server.
CloseFridge: Waiting Reply...
CloseFridge: Halted.
CloseFridge: cleaning up after a halt()
Sequence returns FAILURE
------------
OpenFridge reinitializing reply: 10
GrabBeer reinitializing reply: 160
CloseFridge reinitializing reply: 10
OpenFridge: Started. Send Request to server.
OpenFridge: Waiting Reply...
OpenFridge: Done. 'Waiting Reply' loop repeated 2 times
OpenFridge: cleaning up after SUCCESS
GrabBeer: Started. Send Request to server.
GrabBeer: Waiting Reply...
GrabBeer: Halted.
GrabBeer: cleaning up after a halt()
CloseFridge: Started. Send Request to server.
CloseFridge: Waiting Reply...
CloseFridge: Done. 'Waiting Reply' loop repeated 2 times
CloseFridge: cleaning up after SUCCESS
Sequence returns FAILURE
------------

*/
