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
    // Sanity check (map without rooms)
    if (toy_rooms_.empty())
    {
        return BT::NodeStatus::FAILURE;
    }
    
    Optional<std::string> msg = getInput<std::string>("toy_found");
    if (!msg && first_tick)
    {
        first_tick = false;
    }

    else if ((!msg && !first_tick) || (msg.value() != toy_rooms_.front()))
    {
        auto last_room = toy_rooms_.front();    
        toy_rooms_.push(last_room);
        toy_rooms_.pop();
    }
    
    else if (msg.value() == toy_rooms_.front())
    {
        toy_rooms_.pop();
        
        if (toy_rooms_.empty())
        {
            return BT::NodeStatus::FAILURE;
        }
    }
    print(toy_rooms_);
    auto room = toy_rooms_.front();
    setOutput("retrieve_room", room);
    return BT::NodeStatus::SUCCESS;
}

void RetrieveToyRoom::print(std::queue<std::string> q)
{
    std::cout << "Available rooms: ";
    while (!q.empty())
    {
        std::cout << q.front() << ", ";
        q.pop();
    }
    std::cout << std::endl;
}

//----------------------------------------------------
// GotoRoom
GotoRoom::GotoRoom(const std::string& name, 
    const BT::NodeConfiguration& config) :  BT::SyncActionNode(name, config)
{}
        
BT::NodeStatus GotoRoom::tick()
{
    SleepMS(10);
    Optional<std::string> msg = getInput<std::string>("room");
    if (msg)
    {
        std::cout << "Robot arrives at " << msg.value() << "\n";
        if (msg.value() == "hall")
        {
            std::cout << "A person is blocking the Hall, Goto fails." << "\n";
            return BT::NodeStatus::FAILURE;    
        }
    }
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

BT::NodeStatus InspectRoomToy::tick()
{
    SleepMS(500);
    Optional<std::string> msg = getInput<std::string>("room");
    if (msg)
    {
        std::cout << "Robot inspects " << msg.value() << "\n";
        if ("kitchen" == msg.value())
        {
            std::cout << "Toy found in " << msg.value() << "\n";
            setOutput("toy_pose", "1;2");
            return BT::NodeStatus::SUCCESS;
        }
        setOutput("found", msg.value());
        return BT::NodeStatus::FAILURE;
    }
    else
    {
        std::cout << "Something goes odd in InspectRoom\n";
        return BT::NodeStatus::FAILURE;
    }
    
}

//----------------------------------------------------
// ToyInSight
BT::NodeStatus ToyInSight()
{
    return BT::NodeStatus::SUCCESS;
}

} //namespace FindToyBT

void FindToyBT::RegisterNodes(BehaviorTreeFactory& factory)
{
    factory.registerNodeType<FindToyBT::RetrieveToyRoom>("RetrieveToyRoom");
    factory.registerNodeType<FindToyBT::GotoRoom>("GotoRoom");
    factory.registerNodeType<FindToyBT::EnterRoom>("EnterRoom");
    factory.registerNodeType<FindToyBT::InspectRoomToy>("InspectRoomToy");
    factory.registerSimpleCondition("ToyInSight", std::bind(ToyInSight));
}

// This function must be implemented in the .cpp file to create
// a plugin that can be loaded at run-time
BT_REGISTER_NODES(factory)
{
    FindToyBT::RegisterNodes(factory);
}
