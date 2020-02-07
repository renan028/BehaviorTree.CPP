#include "find_toy_bt_nodes.h"

namespace FindToyBT
{
//----------------------------------------------------
// RetrieveToyRoom
RetrieveToyRoom::RetrieveToyRoom(const std::string& name, 
    const BT::NodeConfiguration& config) : BT::SyncActionNode(name, config)
{
    toy_rooms_.push("bedroom");
    toy_rooms_.push("hall");
    toy_rooms_.push("kitchen");
}

BT::NodeStatus RetrieveToyRoom::tick()
{
    SleepMS(100);
    Optional<std::string> msg = getInput<std::string>("toy_found");
    if (!msg)
    {
        throw BT::RuntimeError("missing required input [message]: ", 
                                msg.error() );
    }
    
    if (msg.value() == "false")
        toy_rooms_.pop();
    else if (msg.value() == "true")
        return BT::NodeStatus::SUCCESS;
    else // put it back to end
    {
        auto last_room = toy_rooms_.front();    
        toy_rooms_.push(last_room);
        toy_rooms_.pop();
    }

    if (toy_rooms_.empty())
        return BT::NodeStatus::FAILURE;

    auto room = toy_rooms_.front();

    setOutput("room", room);
    return BT::NodeStatus::SUCCESS;
}

//----------------------------------------------------
// GotoRoom
GotoRoom::GotoRoom(const std::string& name, 
    const BT::NodeConfiguration& config) :  BT::SyncActionNode(name, config)
{}
        
BT::NodeStatus GotoRoom::tick()
{
    SleepMS(500);
    return BT::NodeStatus::SUCCESS;
}

//----------------------------------------------------
// EnterRoom
EnterRoom::EnterRoom(const std::string& name) : 
    BT::SyncActionNode(name, {})
{}

BT::NodeStatus EnterRoom::tick()
{
    SleepMS(500);
    return BT::NodeStatus::SUCCESS;
}

//----------------------------------------------------
// InspectRoomToy
InspectRoomToy::InspectRoomToy(const std::string& name, 
        const BT::NodeConfiguration& config) : 
    BT::SyncActionNode(name, config)
{}

void InspectRoomToy::init(const std::string& room_with_toy)
{
    rooms_with_toy_ = room_with_toy;
}

BT::NodeStatus InspectRoomToy::tick()
{
    SleepMS(500);
    Optional<std::string> msg = getInput<std::string>("toy_found");
    if (!msg)
    {
        throw BT::RuntimeError("missing required input [message]: ", 
                                   msg.error() );
    }
        
    if (msg.value() == rooms_with_toy_)
    {
        setOutput("toy_pose", "1;2");
        return BT::NodeStatus::SUCCESS;
    }
    else 
    {
        setOutput("toy_pose", "-1;-1");
        return BT::NodeStatus::FAILURE;
    }
}

//----------------------------------------------------
// ToyFound
ToyFound::ToyFound(const std::string& name, 
        const BT::NodeConfiguration& config) : 
    BT::ConditionNode(name, config)
{}

BT::NodeStatus ToyFound::tick()
{
    SleepMS(200);
    setOutput("toy_found", "false");
    return BT::NodeStatus::FAILURE;
}

//----------------------------------------------------
// ToyInSight
BT::NodeStatus ToyInSight()
{
    return BT::NodeStatus::FAILURE;
}

} //namespace FindToyBT

void FindToyBT::RegisterNodes(BehaviorTreeFactory& factory)
{
    factory.registerNodeType<FindToyBT::RetrieveToyRoom>("RetrieveToyRoom");
    factory.registerNodeType<FindToyBT::GotoRoom>("GotoRoom");
    factory.registerNodeType<FindToyBT::EnterRoom>("EnterRoom");
    factory.registerNodeType<FindToyBT::InspectRoomToy>("InspectRoomToy");
    factory.registerNodeType<FindToyBT::ToyFound>("ToyFound");
    factory.registerSimpleCondition("ToyInSight", std::bind(ToyInSight));
}

// This function must be implemented in the .cpp file to create
// a plugin that can be loaded at run-time
BT_REGISTER_NODES(factory)
{
    FindToyBT::RegisterNodes(factory);
}
