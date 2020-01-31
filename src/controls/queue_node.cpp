#include "behaviortree_cpp_v3/controls/queue_node.h"
#include "behaviortree_cpp_v3/action_node.h"

namespace BT
{


QueueNode::QueueNode(const std::string& name)
    : ControlNode::ControlNode(name, {} )
  , current_child_idx_(0)
  , child_failure_(false)
{
    setRegistrationID("Queue");
}

void QueueNode::halt()
{
    current_child_idx_ = 0;
    ControlNode::halt();
}

NodeStatus QueueNode::tick()
{
    const size_t children_count = children_nodes_.size();
    setStatus(NodeStatus::RUNNING);

    while (current_child_idx_ < children_count)
    {
        TreeNode* current_child_node = children_nodes_[current_child_idx_];
        const NodeStatus child_status = current_child_node->executeTick();

        switch (child_status)
        {
            case NodeStatus::RUNNING:
            {
                return child_status;
            }
            case NodeStatus::FAILURE:
            {
                // Does not reset on failure
                child_failure_ = true;
                current_child_idx_++;
            }
            break;

            case NodeStatus::SUCCESS:
            {
                current_child_idx_++;
            }
            break;

            case NodeStatus::IDLE:
            {
                throw LogicError("A child node must never return IDLE");
            }
        }   // end switch
    }       // end while loop

    // The entire while loop completed. 
    if (current_child_idx_ == children_count)
    {
        haltChildren(0);
        current_child_idx_ = 0;
    }
    return child_failure_ ? NodeStatus::FAILURE : NodeStatus::SUCCESS;
}

}
