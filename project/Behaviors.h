#pragma once
#include "stdafx.h"
#include "EBehaviorTree.h"
#include "EBlackboard.h"
#include <Exam_HelperStructs.h>

// Interfaces
#define BB_EXAM_INTERFACE_PTR "pExamInterface"  // Interface to request extra data from entities and more

// INPUT DATA
#define BB_HOUSES_PTR "pHouses"
#define BB_ITEMS_PTR "pItems"  // Not Extended
#define BB_ENEMIES_PTR "pEnemies"
#define BB_PURGEZONES_PTR "pPurgeZones"

#define BB_AGENT_INFO_PTR "pAgentInfo"
#define BB_WORLD_INFO_PTR "pWorldInfo"


// OUTPUT VARIABLES
#define BB_STEERING_TARGET "targetPos"
//#define BB_LOOK_DIRECTION "lookDirection"
//#define BB_SCAN_AREA "scanArea"  // True if we want to rotate to scan this frame (for the scan action)  // Auto resets to false after frame
#define BB_CAN_RUN "canRun"


namespace BT_Utils
{
	Elite::Vector2 FleeFromTarget(Elite::Vector2 agentPos, Elite::Vector2 target)
	{
		// Inverts direction of the target
		// Returns the target where agent needs to go to to flee

		const Elite::Vector2 invDirection{ agentPos - target };
		const Elite::Vector2 fleeTarget{ agentPos + invDirection };
		return fleeTarget;
	}
}

using namespace BT_Utils;
namespace BT_Actions
{
	BehaviorState Test(Blackboard* pBlackboard)
	{
		std::cout << "YOU ARE IN THE TEST ACTION\n";
		return BehaviorState::Success;
	}

	BehaviorState FleeFromPurgeZones(Blackboard* pBlackboard)
	{
		std::cout << "FLEEING FROM PURGE ZONE\n";

		const float radiusMargin{ 5.0f };  // Extra marging around flee zones


		AgentInfo* pAgentInfo;
		if(!pBlackboard->GetData(BB_AGENT_INFO_PTR, pAgentInfo) || pAgentInfo == nullptr)
			return BehaviorState::Failure;

		std::vector<PurgeZoneInfoExtended>* pPurgeZoneInfoVec;
		if(!pBlackboard->GetData(BB_PURGEZONES_PTR, pPurgeZoneInfoVec) || pPurgeZoneInfoVec == nullptr)
			return BehaviorState::Failure;

		if(pPurgeZoneInfoVec->size() == 0)
			return BehaviorState::Failure;

		// If multiple purge zones need to be fled from this will take average position of those combined
		// Not perfect (could still run into close ones before realising its in multiple), but its a nice extra
		Elite::Vector2 averageFleeTarget{};
		int cntFleeTargets{};
		for(auto& purgeZoneInfo : *pPurgeZoneInfoVec)
		{
			const float distanceToCenterSqr{ (pAgentInfo->Position - purgeZoneInfo.Center).MagnitudeSquared() };
			if(distanceToCenterSqr < Elite::Square(purgeZoneInfo.Radius + radiusMargin))
			{
				// Player is within radius of this purgezone
				averageFleeTarget += purgeZoneInfo.Center;
				cntFleeTargets++;
			}
		}
		averageFleeTarget /= float(cntFleeTargets);

		const Elite::Vector2 target{ FleeFromTarget(pAgentInfo->Position, averageFleeTarget) };

		// Set target in the blackboard
		if(!pBlackboard->ChangeData(BB_STEERING_TARGET, target))
			return BehaviorState::Failure;

		if(!pBlackboard->ChangeData(BB_CAN_RUN, true))
			return BehaviorState::Failure;

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


	bool IsInPurgeZone(Blackboard* pBlackboard)
	{
		const float radiusMargin{ 5.0f };  // Extra marging around flee zones
		AgentInfo* pAgentInfo;
		if(!pBlackboard->GetData(BB_AGENT_INFO_PTR, pAgentInfo) || pAgentInfo == nullptr)
			return false;

		std::vector<PurgeZoneInfoExtended>* pPurgeZoneInfoVec;
		if(!pBlackboard->GetData(BB_PURGEZONES_PTR, pPurgeZoneInfoVec) || pPurgeZoneInfoVec == nullptr)
			return false;

		if(pPurgeZoneInfoVec->size() == 0)
			return false;

		for(auto& purgeZoneInfo : *pPurgeZoneInfoVec)
		{
			const float distanceToCenterSqr{ (pAgentInfo->Position - purgeZoneInfo.Center).MagnitudeSquared() };
			if(distanceToCenterSqr < Elite::Square(purgeZoneInfo.Radius + radiusMargin))
				return true;
		}

		return false;
	}
}