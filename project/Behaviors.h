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

// INVENTORY SLOTS
#define BB_SHOTGUN_INV_SLOT 0
#define BB_PISTOL_INV_SLOT 1
#define BB_MEDKIT_INV_SLOT 2
#define BB_FOOD_INV_SLOT 3

#define BB_UNUSED_INV_SLOT 4

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

	BehaviorState GoToClosestLootableHouse(Blackboard* pBlackboard)
	{

		// Get the agent
		AgentInfo* pAgentInfo;
		if(!pBlackboard->GetData(BB_AGENT_INFO_PTR, pAgentInfo) || pAgentInfo == nullptr)
			return BehaviorState::Failure;

		// Get the interface
		std::vector<HouseInfoExtended>* pHouseVec;
		if(!pBlackboard->GetData(BB_HOUSES_PTR, pHouseVec) || pHouseVec == nullptr)
			return BehaviorState::Failure;

		float closestDistance{ FLT_MAX };
		HouseInfoExtended closestHouse{};
		for(const HouseInfoExtended& house : *pHouseVec)
		{
			const float distance{ house.Center.Distance(pAgentInfo->Position) };
			if(house.Looted == false && distance < closestDistance)
			{
				closestHouse = house;
				closestDistance = distance;
			}
		}

		if(closestDistance == FLT_MAX)
		{
			// No lootable houses found
			return BehaviorState::Failure;
		}

		// Set target in the blackboard
		if(!pBlackboard->ChangeData(BB_STEERING_TARGET, closestHouse.Center))
			return BehaviorState::Failure;
		return BehaviorState::Success;
	}

	BehaviorState UseMedkit(Blackboard* pBlackboard)
	{
		std::cout << "USING MEDKIT\n";

		IExamInterface* pExamInterface;
		if(!pBlackboard->GetData(BB_EXAM_INTERFACE_PTR, pExamInterface) || pExamInterface == nullptr)
			return BehaviorState::Failure;

		if(!pExamInterface->Inventory_UseItem(BB_MEDKIT_INV_SLOT))
			return BehaviorState::Failure;

		return BehaviorState::Success;
	}

	BehaviorState UseFood(Blackboard* pBlackboard)
	{
		std::cout << "USING FOOD\n";

		IExamInterface* pExamInterface;
		if(!pBlackboard->GetData(BB_EXAM_INTERFACE_PTR, pExamInterface) || pExamInterface == nullptr)
			return BehaviorState::Failure;

		if(!pExamInterface->Inventory_UseItem(BB_FOOD_INV_SLOT))
			return BehaviorState::Failure;

		return BehaviorState::Success;
	}

	BehaviorState UsePistol(Blackboard* pBlackboard)
	{
		std::cout << "USING PISTOL\n";

		IExamInterface* pExamInterface;
		if(!pBlackboard->GetData(BB_EXAM_INTERFACE_PTR, pExamInterface) || pExamInterface == nullptr)
			return BehaviorState::Failure;

		if(!pExamInterface->Inventory_UseItem(BB_PISTOL_INV_SLOT))
			return BehaviorState::Failure;

		return BehaviorState::Success;
	}

	BehaviorState UseShotgun(Blackboard* pBlackboard)
	{
		std::cout << "USING SHOTGUN\n";

		IExamInterface* pExamInterface;
		if(!pBlackboard->GetData(BB_EXAM_INTERFACE_PTR, pExamInterface) || pExamInterface == nullptr)
			return BehaviorState::Failure;

		if(!pExamInterface->Inventory_UseItem(BB_SHOTGUN_INV_SLOT))
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

	bool LowHealth(Blackboard* pBlackboard)
	{
		// Get the agent
		AgentInfo* pAgentInfo;
		if(!pBlackboard->GetData(BB_AGENT_INFO_PTR, pAgentInfo) || pAgentInfo == nullptr)
			return false;

		const float healthThreshold{ 3.0f };
		if(pAgentInfo->Health < healthThreshold)
			return true;
		return false;
	}

	bool SlightlyDamaged(Blackboard* pBlackboard)
	{
		// Get the agent
		AgentInfo* pAgentInfo;
		if(!pBlackboard->GetData(BB_AGENT_INFO_PTR, pAgentInfo) || pAgentInfo == nullptr)
			return false;

		const float healthThreshold{ 7.0f };
		if(pAgentInfo->Health < healthThreshold)
			return true;
		return false;
	}

	bool LowEnergy(Blackboard* pBlackboard)
	{
		// Get the agent
		AgentInfo* pAgentInfo;
		if(!pBlackboard->GetData(BB_AGENT_INFO_PTR, pAgentInfo) || pAgentInfo == nullptr)
			return false;

		const float foodThreshold{ 3.0f };
		if(pAgentInfo->Energy < foodThreshold)
			return true;
		return false;
	}

	bool SlightlyUsedEnergy(Blackboard* pBlackboard)
	{
		// Get the agent
		AgentInfo* pAgentInfo;
		if(!pBlackboard->GetData(BB_AGENT_INFO_PTR, pAgentInfo) || pAgentInfo == nullptr)
			return false;

		const float foodThreshold{ 7.0f };
		if(pAgentInfo->Energy < foodThreshold)
			return true;
		return false;
	}

	bool HasMedkit(Blackboard* pBlackboard)
	{
		// Get the interface
		IExamInterface* pInterface;
		if(!pBlackboard->GetData(BB_EXAM_INTERFACE_PTR, pInterface) || pInterface == nullptr)
			return false;

		ItemInfo itemInfo;
		if(!pInterface->Inventory_GetItem(BB_MEDKIT_INV_SLOT, itemInfo))
			return false;
		return true;
	}

	bool HasShotgun(Blackboard* pBlackboard)
	{
		// Get the interface
		IExamInterface* pInterface;
		if(!pBlackboard->GetData(BB_EXAM_INTERFACE_PTR, pInterface) || pInterface == nullptr)
			return false;

		ItemInfo itemInfo;
		if(!pInterface->Inventory_GetItem(BB_SHOTGUN_INV_SLOT, itemInfo))
			return false;
		return true;
	}

	bool HasShotgunAmmo(Blackboard* pBlackboard)
	{
		// Get the interface
		IExamInterface* pInterface;
		if(!pBlackboard->GetData(BB_EXAM_INTERFACE_PTR, pInterface) || pInterface == nullptr)
			return false;

		ItemInfo itemInfo;
		if(!pInterface->Inventory_GetItem(BB_SHOTGUN_INV_SLOT, itemInfo))
			return false;


		int shotgunAmmo{ pInterface->Weapon_GetAmmo(itemInfo) };
		if(shotgunAmmo > 0)
			return true;

		return false;
	}

	bool LowShotgunAmmo(Blackboard* pBlackboard)
	{
		// Get the interface
		IExamInterface* pInterface;
		if(!pBlackboard->GetData(BB_EXAM_INTERFACE_PTR, pInterface) || pInterface == nullptr)
			return false;

		ItemInfo itemInfo;
		if(!pInterface->Inventory_GetItem(BB_SHOTGUN_INV_SLOT, itemInfo))
			return false;

		int shotgunAmmo{ pInterface->Weapon_GetAmmo(itemInfo) };
		if(shotgunAmmo < 2)
			return true;

		return false;
	}

	bool HasPistol(Blackboard* pBlackboard)
	{
		// Get the interface
		IExamInterface* pInterface;
		if(!pBlackboard->GetData(BB_EXAM_INTERFACE_PTR, pInterface) || pInterface == nullptr)
			return false;

		ItemInfo itemInfo;
		if(!pInterface->Inventory_GetItem(BB_PISTOL_INV_SLOT, itemInfo))
			return false;
		return true;
	}

	bool HasPistolAmmo(Blackboard* pBlackboard)
	{
		// Get the interface
		IExamInterface* pInterface;
		if(!pBlackboard->GetData(BB_EXAM_INTERFACE_PTR, pInterface) || pInterface == nullptr)
			return false;

		ItemInfo itemInfo;
		if(!pInterface->Inventory_GetItem(BB_PISTOL_INV_SLOT, itemInfo))
			return false;

		int pistolAmmo{ pInterface->Weapon_GetAmmo(itemInfo) };
		if(pistolAmmo > 0)
			return true;

		return false;
	}

	bool LowPistolAmmo(Blackboard* pBlackboard)
	{
		// Get the interface
		IExamInterface* pInterface;
		if(!pBlackboard->GetData(BB_EXAM_INTERFACE_PTR, pInterface) || pInterface == nullptr)
			return false;

		ItemInfo itemInfo;
		if(!pInterface->Inventory_GetItem(BB_PISTOL_INV_SLOT, itemInfo))
			return false;

		int shotgunAmmo{ pInterface->Weapon_GetAmmo(itemInfo) };
		if(shotgunAmmo < 4)
			return true;

		return false;
	}

	bool HasFood(Blackboard* pBlackboard)
	{
		// Get the interface
		IExamInterface* pInterface;
		if(!pBlackboard->GetData(BB_EXAM_INTERFACE_PTR, pInterface) || pInterface == nullptr)
			return false;

		ItemInfo itemInfo;
		if(!pInterface->Inventory_GetItem(BB_FOOD_INV_SLOT, itemInfo))
			return false;
		return true;
	}


	bool EnemyNearby(Blackboard* pBlackboard)
	{
		const float nearbyRadius{ 20.0f };  // Radius which is considered to be NEARBY

		// Get the agent
		AgentInfo* pAgentInfo;
		if(!pBlackboard->GetData(BB_AGENT_INFO_PTR, pAgentInfo) || pAgentInfo == nullptr)
			return false;

		// Get the interface
		std::vector<EnemyInfoExtended>* pEnemiesVec;
		if(!pBlackboard->GetData(BB_ENEMIES_PTR, pEnemiesVec) || pEnemiesVec == nullptr)
			return false;


		// Check if there are enemies nearby
		for(const EnemyInfoExtended& enemy : *pEnemiesVec)
		{
			if(enemy.Location.Distance(pAgentInfo->Position) < nearbyRadius)
				return true;
		}

		return false;
	}

	bool PistolNearby(Blackboard* pBlackboard)
	{
		const float nearbyRadius{ 20.0f };  // Radius which is considered to be NEARBY

		// Get the agent
		AgentInfo* pAgentInfo;
		if(!pBlackboard->GetData(BB_AGENT_INFO_PTR, pAgentInfo) || pAgentInfo == nullptr)
			return false;

		// Get the interface
		std::vector<ItemInfo>* pItemsVec;
		if(!pBlackboard->GetData(BB_ITEMS_PTR, pItemsVec) || pItemsVec == nullptr)
			return false;

		for(const ItemInfo& item : *pItemsVec)
		{
			if(item.Type == eItemType::PISTOL && item.Location.Distance(pAgentInfo->Position) < nearbyRadius)
				return true;
		}
		return false;
	}

	bool ShotgunNearby(Blackboard* pBlackboard)
	{
		const float nearbyRadius{ 20.0f };  // Radius which is considered to be NEARBY

		// Get the agent
		AgentInfo* pAgentInfo;
		if(!pBlackboard->GetData(BB_AGENT_INFO_PTR, pAgentInfo) || pAgentInfo == nullptr)
			return false;

		// Get the interface
		std::vector<ItemInfo>* pItemsVec;
		if(!pBlackboard->GetData(BB_ITEMS_PTR, pItemsVec) || pItemsVec == nullptr)
			return false;

		for(const ItemInfo& item : *pItemsVec)
		{
			if(item.Type == eItemType::SHOTGUN && item.Location.Distance(pAgentInfo->Position) < nearbyRadius)
				return true;
		}
		return false;
	}

	bool FoodNearby(Blackboard* pBlackboard)
	{
		const float nearbyRadius{ 20.0f };  // Radius which is considered to be NEARBY

		// Get the agent
		AgentInfo* pAgentInfo;
		if(!pBlackboard->GetData(BB_AGENT_INFO_PTR, pAgentInfo) || pAgentInfo == nullptr)
			return false;

		// Get the interface
		std::vector<ItemInfo>* pItemsVec;
		if(!pBlackboard->GetData(BB_ITEMS_PTR, pItemsVec) || pItemsVec == nullptr)
			return false;

		for(const ItemInfo& item : *pItemsVec)
		{
			if(item.Type == eItemType::FOOD && item.Location.Distance(pAgentInfo->Position) < nearbyRadius)
				return true;
		}
		return false;
	}

	bool MedkitNearby(Blackboard* pBlackboard)
	{
		const float nearbyRadius{ 20.0f };  // Radius which is considered to be NEARBY

		// Get the agent
		AgentInfo* pAgentInfo;
		if(!pBlackboard->GetData(BB_AGENT_INFO_PTR, pAgentInfo) || pAgentInfo == nullptr)
			return false;

		// Get the interface
		std::vector<ItemInfo>* pItemsVec;
		if(!pBlackboard->GetData(BB_ITEMS_PTR, pItemsVec) || pItemsVec == nullptr)
			return false;

		for(const ItemInfo& item : *pItemsVec)
		{
			if(item.Type == eItemType::MEDKIT && item.Location.Distance(pAgentInfo->Position) < nearbyRadius)
				return true;
		}
		return false;
	}

	bool LootableHouseNearby(Blackboard* pBlackboard)
	{
		const float nearbyRadius{ 50.0f };

		// Get the agent
		AgentInfo* pAgentInfo;
		if(!pBlackboard->GetData(BB_AGENT_INFO_PTR, pAgentInfo) || pAgentInfo == nullptr)
			return false;

		// Get the interface
		std::vector<HouseInfoExtended>* pHouseVec;
		if(!pBlackboard->GetData(BB_HOUSES_PTR, pHouseVec) || pHouseVec == nullptr)
			return false;

		for(const HouseInfoExtended& house : *pHouseVec)
		{
			if(house.Looted == false && house.Center.Distance(pAgentInfo->Position) < nearbyRadius)
				return true;
		}
		return false;
	}



}