#include "behaviortree_cpp_v3/bt_factory.h"
#include <Eigen/Geometry>

using namespace BT;

/* SE3 State Space */

struct SE3StateSpace 
{ 
    Eigen::Vector3d position;
    Eigen::Quaterniond orientation;
};

namespace BT
{
template <> inline SE3StateSpace convertFromString(StringView str)
{
    // real numbers separated by semicolons
    auto parts = splitString(str, ';');
    if (parts.size() != 7)
    {
        throw RuntimeError("invalid input)");
    }
    else
    {
        SE3StateSpace output;
        output.position =       {   convertFromString<double>(parts[0]),
                                    convertFromString<double>(parts[1]),
                                    convertFromString<double>(parts[2])
                                };
        output.orientation =    {   convertFromString<double>(parts[3]),
                                    convertFromString<double>(parts[4]),
                                    convertFromString<double>(parts[5]),
                                    convertFromString<double>(parts[6])
                                };
        return output;
    }
}
} // end namespace BT


class CalculateGoal: public SyncActionNode
{
public:
    CalculateGoal(const std::string& name, const NodeConfiguration& config):
        SyncActionNode(name,config)
    {}

    NodeStatus tick() override
    {
        SE3StateSpace mygoal; 
        mygoal.position = {1.1, 2.3, 1.0};
        mygoal.orientation = {1, 0, 0, 0};
        setOutput("goal", mygoal);
        return NodeStatus::SUCCESS;
    }
    static PortsList providedPorts()
    {
        return { OutputPort<SE3StateSpace>("goal") };
    }
};


class PrintTarget: public SyncActionNode
{
public:
    PrintTarget(const std::string& name, const NodeConfiguration& config):
        SyncActionNode(name,config)
    {}

    NodeStatus tick() override
    {
        auto res = getInput<SE3StateSpace>("target");
        if( !res )
        {
            throw RuntimeError("error reading port [target]:", res.error() );
        }
        SE3StateSpace goal = res.value();
        printf("Target Position3D: [ %.1f, %.1f, %.1f ]\n", 
            goal.position.x(), goal.position.y(), goal.position.z());
        printf("Target Orientation: [ %.2f, %.2f, %.2f, %.2f]\n", 
            goal.orientation.w(), goal.orientation.x(), goal.orientation.y(),
            goal.orientation.z());
        return NodeStatus::SUCCESS;
    }

    static PortsList providedPorts()
    {
        // Optionally, a port can have a human readable description
        const char*  description = "Simply print the target on console...";
        return { InputPort<SE3StateSpace>("target", description) };
    }
};

//----------------------------------------------------------------

/** The tree is a Sequence of 4 actions

*  1) Store a value of SE3StateSpace in the entry "GoalPosition"
*     using the action CalculateGoal.
*
*  2) Call PrintTarget. The input "target" will be read from the Blackboard
*     entry "GoalPosition".
*
*  3) Use the built-in action SetBlackboard to write the key "OtherGoal".
*     A conversion from string to Position2D will be done under the hood.
*
*  4) Call PrintTarget. The input "goal" will be read from the Blackboard
*     entry "OtherGoal".
*/

// clang-format off
static const char* xml_text = R"(

 <root main_tree_to_execute = "MainTree" >
     <BehaviorTree ID="MainTree">
        <Sequence name="root">
            <CalculateGoal   goal="{GoalPosition}" />
            <PrintTarget     target="{GoalPosition}" />
            <SetBlackboard   output_key="OtherGoal" value="0;1;2;1;0.71;0;0.71" />
            <PrintTarget     target="{OtherGoal}" />
        </Sequence>
     </BehaviorTree>
 </root>
 )";

// clang-format on

int main()
{
    using namespace BT;

    BehaviorTreeFactory factory;
    factory.registerNodeType<CalculateGoal>("CalculateGoal");
    factory.registerNodeType<PrintTarget>("PrintTarget");

    auto tree = factory.createTreeFromText(xml_text);
    tree.root_node->executeTick();

/* Expected output:
 *
 * Target Position3D: [ 1.1, 2.3, 1.0 ] 
   Target Orientation: [1.00, 0.00, 0.00, 0.00] 
   Target Position3D: [ 0.0, 1.0, 2.0 ] 
   Target Orientation: [1.00, 0.71, 0.00, 0.71] 
*/
    return 0;
}

