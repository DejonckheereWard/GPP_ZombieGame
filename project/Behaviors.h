#pragma once
#include "stdafx.h"
#include "BehaviorTree.h"

namespace BT_Actions
{
	BehaviorState Test(Blackboard* pBlackboard)
	{
		std::cout << "YOU ARE IN THE TEST ACTION" << std::endl;
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