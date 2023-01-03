#pragma once
#include "stdafx.h"
#include "EBehaviorTree.h"
#include "EBlackboard.h"
#include ""

namespace BT_Actions
{
	BehaviorState Test(Blackboard* pBlackboard)
	{
		std::cout << "YOU ARE IN THE TEST ACTION" << std::endl;
		return BehaviorState::Success;
	}

	BehaviorState Wander(Blackboard* pBlackboard)
	{
		// Get agent from blackboard
		AgentInfo* pAgent;
		pBlackboard->GetData(BB_AGENT_INFO, pAgent);


		return BehaviorState::Success;
	}

}

namespace BT_Conditions
{
	bool Test(Blackboard* pBlackboard)
	{
		std::cout << "TEST CONDITION CHECKED" << std::endl;
		return true;
	}
}