#ifndef QUEUENODE_H
#define QUEUENODE_H

#include "behaviortree_cpp_v3/control_node.h"

namespace BT
{
/**
 * @brief The QueueNode is used to tick children in an ordered sequence.
 * If any child returns RUNNING, previous children will NOT be ticked again.
 *
 * - If all the children return SUCCESS, this node returns SUCCESS.
 *
 * - If a child returns RUNNING, this node returns RUNNING.
 *   Loop is NOT restarted, the same running child will be ticked again.
 *
 * - If a child returns FAILURE, the loop continues, but QueueNode will 
 *  return FAILURE at the end.
 *
 */
class QueueNode : public ControlNode
{
  public:
    QueueNode(const std::string& name);

    virtual ~QueueNode() override = default;

    virtual void halt() override;

  private:
    size_t current_child_idx_;
    bool child_failure_;
    std::vector<unsigned int> visited_list;
    std::vector<unsigned int> success_list;

    virtual BT::NodeStatus tick() override;
};

}

#endif // QUEUENODE_H
