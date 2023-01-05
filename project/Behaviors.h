#pragma once
#include "stdafx.h"
#include "EBehaviorTree.h"
#include "EBlackboard.h"
#include <Exam_HelperStructs.h>


// DATA
#define BB_HOUSES_PTR "pHouses"
#define BB_ITEMS_PTR "pItems"  // Not Extended
#define BB_ENEMIES_PTR "pEnemies"
#define BB_PURGEZONES_PTR "pPurgeZones"


// INPUT
#define BB_AGENT_INFO_PTR "pAgentInfo"
#define BB_WORLD_INFO_PTR "pWorldInfo"
#define BB_ENTITIES_IN_FOV_PTR "pEntitiesInFov"
#define BB_HOUSES_IN_FOV_PTR "pHousesInFov"
#define BB_HOUSES_VISITED_PTR "pHousesVisitedQueue"



#define BB_ITEM_INFO_PTRS "pItemInfoPtrs" // Vector of Item Info Ptrs
#define BB_EXAM_INTERFACE_PTR "examInterface"  // Interface to request extra data from entities and more

// OUTPUT
#define BB_TARGET_POS "targetPos"
#define BB_LOOK_DIRECTION "lookDirection"
#define BB_SCAN_AREA "scanArea"  // True if we want to rotate to scan this frame (for the scan action)  // Auto resets to false after frame


namespace BT_Utils
{
	bool FindClosestItem(const std::vector<EntityInfo>& entities, AgentInfo* pAgent, EntityInfo& outInfo)
	{
		float closestDistanceSqr{ FLT_MAX };
		for(const EntityInfo& itemInfo : entities)
		{
			if(itemInfo.Type == eEntityType::ITEM)
			{
				const float sqrDistanceToItem{ (itemInfo.Location - pAgent->Position).MagnitudeSquared() };
				if(sqrDistanceToItem < closestDistanceSqr)
				{
					outInfo = itemInfo;
					closestDistanceSqr = sqrDistanceToItem;
				}
			}
		}

		if(closestDistanceSqr != FLT_MAX)
		{
			return true;
		}
		return false;
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

	BehaviorState RandomWander(Blackboard* pBlackboard)
	{
		std::cout << "SELECTING RANDOM WANDER POINT\n";
		const float wanderRadius{ 15.0f };


		// Get agent from blackboard
		AgentInfo* pAgent;
		if(!pBlackboard->GetData(BB_AGENT_INFO_PTR, pAgent) || pAgent == nullptr)
		{
			return BehaviorState::Failure;
		}

		// Find a random location within a certain radius of the player.
		Elite::Vector2 randomRelativePosition{};
		randomRelativePosition = Elite::randomVector2(wanderRadius);

		Elite::Vector2 targetPos{ pAgent->Position + randomRelativePosition };

		if(!pBlackboard->ChangeData(BB_TARGET_POS, targetPos))
		{
			return BehaviorState::Failure;
		}
		return BehaviorState::Success;
	}

	BehaviorState GoToClosestItem(Blackboard* pBlackboard)
	{
		AgentInfo* pAgent;
		if(!pBlackboard->GetData(BB_AGENT_INFO_PTR, pAgent) || pAgent == nullptr)
		{
			return BehaviorState::Failure;
		}

		std::vector<EntityInfo>* pEntitiesInFov;
		if(!pBlackboard->GetData(BB_ENTITIES_IN_FOV_PTR, pEntitiesInFov))
		{
			return BehaviorState::Failure;
		}



		EntityInfo closestItemInfo{};
		if(FindClosestItem(*pEntitiesInFov, pAgent, closestItemInfo))
		{
			if(!pBlackboard->ChangeData(BB_TARGET_POS, closestItemInfo.Location))
			{
				std::cout << "Failed to change date for: " << BB_TARGET_POS << "\n";
				return BehaviorState::Failure;
			}
			return BehaviorState::Success;
		}
		return BehaviorState::Failure;
	}

	BehaviorState GrabClosestItem(Blackboard* pBlackboard)
	{
		const float pickupRange{ 2.0f };


		// Check if closest item is within grab range
		// Return failure if not

		AgentInfo* pAgent;
		if(!pBlackboard->GetData(BB_AGENT_INFO_PTR, pAgent) || pAgent == nullptr)
		{
			return BehaviorState::Failure;
		}

		std::vector<EntityInfo>* pEntitiesInFov;
		if(!pBlackboard->GetData(BB_ENTITIES_IN_FOV_PTR, pEntitiesInFov))
		{
			return BehaviorState::Failure;
		}

		IExamInterface* pInterface;
		if(!pBlackboard->GetData(BB_EXAM_INTERFACE_PTR, pInterface))
		{
			return BehaviorState::Failure;
		}


		EntityInfo closestItemInfo{};
		if(FindClosestItem(*pEntitiesInFov, pAgent, closestItemInfo))
		{
			if(pAgent->Position.DistanceSquared(closestItemInfo.Location) <= pickupRange)
			{
				// Get item info (needed to grab it)
				ItemInfo itemInfo;
				if(!pInterface->Item_GetInfo(closestItemInfo, itemInfo))
				{
					return BehaviorState::Failure;
				}
				if(!pInterface->Item_Grab({}, itemInfo))
				{
					return BehaviorState::Failure;
				}

				int inventorySlot{};
				while(!pInterface->Inventory_AddItem(inventorySlot, itemInfo))
				{
					++inventorySlot;
				}

			}
		}
		return BehaviorState::Failure;
	}


	BehaviorState VisitHouse(Blackboard* pBlackboard)
	{
		std::vector<HouseInfo>* pHouseInfoVec;

		if(!pBlackboard->GetData(BB_HOUSES_IN_FOV_PTR, pHouseInfoVec))
		{
			std::cout << "Failed to get data from blackboard\n";
			return BehaviorState::Failure;
		}

		// Get center of first house found, add to visited list (i want logic to not revisit the same house over and over)


		if(pHouseInfoVec->size() > 0)
		{
			// Get center of house
			Elite::Vector2 houseCenter{ pHouseInfoVec->at(0).Center };

			// Add house to visited list
			std::vector<Elite::Vector2>* pVisitedHouses;
			if(!pBlackboard->GetData(BB_HOUSES_VISITED_PTR, pVisitedHouses))
			{
				std::cout << "Failed to get data for: " << BB_HOUSES_VISITED_PTR << "\n";
				return BehaviorState::Failure;
			}
			pVisitedHouses->push_back(houseCenter);

			// Set target pos to house center
			if(!pBlackboard->ChangeData(BB_TARGET_POS, houseCenter))
			{
				std::cout << "Failed to change data for: " << BB_TARGET_POS << "\n";
				return BehaviorState::Failure;
			}
			return BehaviorState::Success;
		}
	}

	BehaviorState ScanArea(Blackboard* pBlackboard)
	{
		// Set scan area to true if we want to scan this update.
		if(!pBlackboard->ChangeData(BB_SCAN_AREA, true))
		{
			std::cout << "Failed to change data for: " << BB_SCAN_AREA << "\n";
			return BehaviorState::Failure;
		}
	}

}

namespace BT_Conditions
{
	bool Test(Blackboard* pBlackboard)
	{
		std::cout << "TEST CONDITION CHECKED" << std::endl;
		return true;
	}

	bool ReachedTarget(Blackboard* pBlackboard)
	{
		Elite::Vector2 targetPos;
		AgentInfo* pAgentInfo;
		if(!pBlackboard->GetData(BB_TARGET_POS, targetPos))
		{
			std::cout << "Failed to get data for: " << BB_TARGET_POS << "\n";
			return false;
		}
		if(!pBlackboard->GetData(BB_AGENT_INFO_PTR, pAgentInfo) || pAgentInfo == nullptr)
		{
			std::cout << "Failed to get data for: " << BB_AGENT_INFO_PTR << "\n";
			return false;
		}

		Elite::Vector2 distanceToTarget(targetPos - pAgentInfo->Position);

		if(distanceToTarget.MagnitudeSquared() <= 8.0f)
		{
			return true;
		}
		return false;

	}

	bool ItemInVision(Blackboard* pBlackboard)
	{
		std::vector<EntityInfo>* pEntityInfoVec;

		if(!pBlackboard->GetData(BB_ENTITIES_IN_FOV_PTR, pEntityInfoVec))
		{
			std::cout << "Failed to get data for: " << BB_ENTITIES_IN_FOV_PTR << "\n";
			return false;
		}

		if(pEntityInfoVec->size() > 0)
		{
			for(const EntityInfo& entityInfo : *pEntityInfoVec)
			{
				if(entityInfo.Type == eEntityType::ITEM)
				{
					return true;
				}
			}
		}
		return false;
	}

	bool HouseInFov(Blackboard* pBlackboard)
	{
		std::vector<HouseInfo>* pHouseInfoVec;

		if(!pBlackboard->GetData(BB_HOUSES_IN_FOV_PTR, pHouseInfoVec))
		{
			std::cout << "Failed to get data for: " << BB_HOUSES_IN_FOV_PTR << "\n";
			return false;
		}

		if(pHouseInfoVec->size() > 0)
		{
			return true;
		}
		return false;
	}

	bool IsHousePosVisited(Blackboard* pBlackboard)
	{
		// Get the visited house positions
		//std::vector<Elite::Vector2>* pVisitedHouses;
	}
}