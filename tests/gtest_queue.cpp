#include <gtest/gtest.h>
#include "action_test_node.h"
#include "condition_test_node.h"
#include "behaviortree_cpp_v3/behavior_tree.h"

using BT::NodeStatus;
using std::chrono::milliseconds;

struct SimpleQueueTest : testing::Test
{
    BT::QueueNode root;
    BT::AsyncActionTest action;
    BT::ConditionTestNode condition;

    SimpleQueueTest() :
      root("root_queue")
      , action("action", milliseconds(10))
      , condition("condition")
    {
        root.addChild(&condition);
        root.addChild(&action);
    }
    ~SimpleQueueTest()
    {
        haltAllActions(&root);
    }
};

struct QueueTripleActionTest : testing::Test
{
    BT::QueueNode root;
    BT::ConditionTestNode condition;
    BT::AsyncActionTest action_1;
    BT::SyncActionTest action_2;
    BT::AsyncActionTest action_3;

    QueueTripleActionTest()
      : root("root_queue")
      , condition("condition")
      , action_1("action_1", milliseconds(100))
      , action_2("action_2")
      , action_3("action_3", milliseconds(100))
    {
        root.addChild(&condition);
        root.addChild(&action_1);
        root.addChild(&action_2);
        root.addChild(&action_3);
    }
    ~QueueTripleActionTest()
    {
        haltAllActions(&root);
    }
};

struct ComplexQueue2ActionsTest : testing::Test
{
    BT::QueueNode root;
    BT::AsyncActionTest action_1;
    BT::AsyncActionTest action_2;
    BT::QueueNode queue_1;
    BT::QueueNode queue_2;

    BT::ConditionTestNode condition_1;
    BT::ConditionTestNode condition_2;

    ComplexQueue2ActionsTest()
      : root("root_queue")
      , action_1("action_1", milliseconds(100))
      , action_2("action_2", milliseconds(100))
      , queue_1("queue_1")
      , queue_2("queue_2")
      , condition_1("condition_1")
      , condition_2("condition_2")
    {
        root.addChild(&queue_1);
        {
            queue_1.addChild(&condition_1);
            queue_1.addChild(&action_1);
        }
        root.addChild(&queue_2);
        {
            queue_2.addChild(&condition_2);
            queue_2.addChild(&action_2);
        }
    }
    ~ComplexQueue2ActionsTest()
    {
        haltAllActions(&root);
    }
};

/****************TESTS START HERE***************************/
TEST_F(SimpleQueueTest, ConditionTrue)
{
    std::cout << "Ticking the root node !" << std::endl << std::endl;
    // Ticking the root node
    BT::NodeStatus state = root.executeTick();

    ASSERT_EQ(NodeStatus::RUNNING, action.status());
    ASSERT_EQ(NodeStatus::RUNNING, state);
}

TEST_F(SimpleQueueTest, ConditionTurnToFalse)
{
    using namespace BT;
    using namespace std::chrono;

    condition.setBoolean(false);
    BT::NodeStatus state = root.executeTick();

    ASSERT_EQ(NodeStatus::RUNNING, state);
    ASSERT_EQ(NodeStatus::FAILURE, condition.status());
    ASSERT_EQ(NodeStatus::RUNNING, action.status());

    std::this_thread::sleep_for(milliseconds(30));

    ASSERT_EQ(NodeStatus::SUCCESS, action.status());
    state = root.executeTick();
    ASSERT_EQ(NodeStatus::FAILURE, state);
    
    // Condition is called only once
    ASSERT_EQ(condition.tickCount(), 1);
    // action is called only once
    ASSERT_EQ(action.tickCount(), 1);
}

TEST_F(QueueTripleActionTest, TripleAction)
{
    using namespace BT;
    using namespace std::chrono;
    const auto timeout = system_clock::now() + milliseconds(650);

    action_1.setTime( milliseconds(300) );
    action_3.setTime( milliseconds(300) );
    // the sequence is supposed to finish in (300 ms * 2) = 600 ms

    // first tick
    NodeStatus state = root.executeTick();

    ASSERT_EQ(NodeStatus::RUNNING, state);
    ASSERT_EQ(NodeStatus::RUNNING, action_1.status());
    ASSERT_EQ(NodeStatus::IDLE, action_2.status());
    ASSERT_EQ(NodeStatus::IDLE, action_3.status());

    // continue until succesful
    while (state != NodeStatus::SUCCESS && system_clock::now() < timeout)
    {
        std::this_thread::sleep_for(milliseconds(10));
        state = root.executeTick();
    }

    ASSERT_EQ(NodeStatus::SUCCESS, state);

    // Condition is called only once
    ASSERT_EQ(condition.tickCount(), 1);
    // all the actions are called only once
    ASSERT_EQ(action_1.tickCount(), 1);
    ASSERT_EQ(action_2.tickCount(), 1);
    ASSERT_EQ(action_3.tickCount(), 1);

    ASSERT_EQ(NodeStatus::IDLE, action_1.status());
    ASSERT_EQ(NodeStatus::IDLE, action_2.status());
    ASSERT_EQ(NodeStatus::IDLE, action_3.status());
    ASSERT_TRUE(system_clock::now() < timeout);   // no timeout should occur
}

TEST_F(ComplexQueue2ActionsTest, ConditionsTrue)
{
    BT::NodeStatus state = root.executeTick();

    state = root.executeTick();

    ASSERT_EQ(NodeStatus::RUNNING, state);
    ASSERT_EQ(NodeStatus::RUNNING, queue_1.status());
    ASSERT_EQ(NodeStatus::SUCCESS, condition_1.status());
    ASSERT_EQ(NodeStatus::RUNNING, action_1.status());
    ASSERT_EQ(NodeStatus::IDLE, queue_2.status());
    ASSERT_EQ(NodeStatus::IDLE, condition_2.status());
    ASSERT_EQ(NodeStatus::IDLE, action_2.status());

    std::this_thread::sleep_for(milliseconds(300));
    state = root.executeTick();

    ASSERT_EQ(NodeStatus::RUNNING, state);
    ASSERT_EQ(NodeStatus::SUCCESS, queue_1.status());
    ASSERT_EQ(NodeStatus::IDLE, condition_1.status());
    ASSERT_EQ(NodeStatus::IDLE, action_1.status());
    ASSERT_EQ(NodeStatus::RUNNING, queue_2.status());
    ASSERT_EQ(NodeStatus::SUCCESS, condition_2.status());
    ASSERT_EQ(NodeStatus::RUNNING, action_2.status());

    state = root.executeTick();
}

TEST_F(ComplexQueue2ActionsTest, ComplexSequenceConditions1ToFalse)
{
    condition_1.setBoolean(false);

    BT::NodeStatus state = root.executeTick();

    ASSERT_EQ(NodeStatus::RUNNING, state);
    ASSERT_EQ(NodeStatus::RUNNING, queue_1.status());
    ASSERT_EQ(NodeStatus::FAILURE, condition_1.status());
    ASSERT_EQ(NodeStatus::RUNNING, action_1.status());
    ASSERT_EQ(NodeStatus::IDLE, queue_2.status());
    ASSERT_EQ(NodeStatus::IDLE, condition_2.status());
    ASSERT_EQ(NodeStatus::IDLE, action_2.status());

    std::this_thread::sleep_for(milliseconds(300));
    state = root.executeTick();

    ASSERT_EQ(NodeStatus::RUNNING, state);
    ASSERT_EQ(NodeStatus::FAILURE, queue_1.status());
    ASSERT_EQ(NodeStatus::IDLE, condition_1.status());
    ASSERT_EQ(NodeStatus::IDLE, action_1.status());
    ASSERT_EQ(NodeStatus::RUNNING, queue_2.status());
    ASSERT_EQ(NodeStatus::SUCCESS, condition_2.status());
    ASSERT_EQ(NodeStatus::RUNNING, action_2.status());

    std::this_thread::sleep_for(milliseconds(300));
    state = root.executeTick();
    ASSERT_EQ(NodeStatus::FAILURE, state);
    ASSERT_EQ(NodeStatus::IDLE, condition_1.status());
    ASSERT_EQ(NodeStatus::IDLE, condition_2.status());
    ASSERT_EQ(NodeStatus::IDLE, action_1.status());
    ASSERT_EQ(NodeStatus::IDLE, action_2.status());
}

TEST_F(ComplexQueue2ActionsTest, ComplexSequenceConditions2ToFalse)
{
    condition_2.setBoolean(false);

    BT::NodeStatus state = root.executeTick();

    ASSERT_EQ(NodeStatus::RUNNING, state);
    ASSERT_EQ(NodeStatus::RUNNING, queue_1.status());
    ASSERT_EQ(NodeStatus::SUCCESS, condition_1.status());
    ASSERT_EQ(NodeStatus::RUNNING, action_1.status());
    ASSERT_EQ(NodeStatus::IDLE, queue_2.status());
    ASSERT_EQ(NodeStatus::IDLE, condition_2.status());
    ASSERT_EQ(NodeStatus::IDLE, action_2.status());

    std::this_thread::sleep_for(milliseconds(300));
    state = root.executeTick();

    ASSERT_EQ(NodeStatus::RUNNING, state);
    ASSERT_EQ(NodeStatus::SUCCESS, queue_1.status());
    ASSERT_EQ(NodeStatus::IDLE, condition_1.status());
    ASSERT_EQ(NodeStatus::IDLE, action_1.status());
    ASSERT_EQ(NodeStatus::RUNNING, queue_2.status());
    ASSERT_EQ(NodeStatus::FAILURE, condition_2.status());
    ASSERT_EQ(NodeStatus::RUNNING, action_2.status());

    std::this_thread::sleep_for(milliseconds(300));
    state = root.executeTick();
    ASSERT_EQ(NodeStatus::FAILURE, state);
    ASSERT_EQ(NodeStatus::IDLE, condition_1.status());
    ASSERT_EQ(NodeStatus::IDLE, condition_2.status());
    ASSERT_EQ(NodeStatus::IDLE, action_1.status());
    ASSERT_EQ(NodeStatus::IDLE, action_2.status());
}