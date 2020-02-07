#include "behaviortree_cpp_v3/bt_factory.h"

using namespace BT;

namespace FindToyBT
{

inline void SleepMS(int ms)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

class RetrieveToyRoom : public BT::SyncActionNode
{
    public:
        RetrieveToyRoom(const std::string& name, 
            const BT::NodeConfiguration& config);
        
        BT::NodeStatus tick() override;

        static BT::PortsList providedPorts()
        {
            return { BT::OutputPort<std::string>("retrieve_room"), 
                BT::InputPort<std::string>("toy_found") };
        }
    
    private:
        std::queue<std::string> toy_rooms_;
        bool first_tick = true;

        void print(std::queue<std::string> q);
};

class GotoRoom : public BT::SyncActionNode
{
    public:
        GotoRoom(const std::string& name, 
            const BT::NodeConfiguration& config);
        
        BT::NodeStatus tick() override;

        static BT::PortsList providedPorts()
        {
            return { BT::InputPort<std::string>("room") };
        }
};

class EnterRoom : public BT::SyncActionNode
{
    public:
        EnterRoom(const std::string& name);
        
        BT::NodeStatus tick() override;
};

class InspectRoomToy : public BT::SyncActionNode
{
    public:
        InspectRoomToy(const std::string& name, 
            const BT::NodeConfiguration& config);
        
        BT::NodeStatus tick() override;

        static BT::PortsList providedPorts()
        {
            return { BT::OutputPort<std::string>("toy_pose"), 
                BT::InputPort<std::string>("toy_type"),
                BT::OutputPort<std::string>("found"),
                BT::InputPort<std::string>("room") };
        }
};

BT::NodeStatus ToyInSight();

void RegisterNodes(BT::BehaviorTreeFactory& factory);

} // namespace FindToyBT