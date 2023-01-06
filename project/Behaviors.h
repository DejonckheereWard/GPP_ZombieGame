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

// Extra input
#define BB_CHECKPOINTS_PTR "pCheckpoints"
#define BB_CURRENT_TIME "currentTime"

// OUTPUT VARIABLES
#define BB_STEERING_TARGET "targetPos"
#define BB_ANGULAR_VELOCITY "angularVelocity"
#define BB_CAN_RUN "canRun"

// INVENTORY SLOTS
#define BB_SHOTGUN_INV_SLOT 0
#define BB_PISTOL_INV_SLOT 1
#define BB_MEDKIT_INV_SLOT 2
#define BB_FOOD_INV_SLOT 3
#define BB_UNUSED_INV_SLOT 4  // Unused slot, could be used as extra slot for either of the previous, if need be (medkit might be nice)

#define DEBUG_MESSAGES

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


	void PrintColorAction(const std::string& input)
	{
#ifdef DEBUG_MESSAGES
		std::cout << "\033[1;31m" << input << "\033[0m\n";
#endif // DEBUG_MESSAGES

	}

	void PrintColorCondition(const std::string& input)
	{
#ifdef DEBUG_MESSAGES
		// Print in green color using ansi
		std::cout << "\033[1;32m" << input << "\033[0m\n";
#endif // DEBUG_MESSAGES

	}

}

// *******************
// ----- ACTIONS -----
// *******************

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
		PrintColorAction("FLEEING FROM PURGE ZONE");

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
		PrintColorAction("GOING TO CLOSEST LOOTABLE HOUSE");
		// Get the agent
		AgentInfo* pAgentInfo;
		if(!pBlackboard->GetData(BB_AGENT_INFO_PTR, pAgentInfo) || pAgentInfo == nullptr)
			return BehaviorState::Failure;

		// Get the houses
		std::vector<HouseInfoExtended>* pHouseVec;
		if(!pBlackboard->GetData(BB_HOUSES_PTR, pHouseVec) || pHouseVec == nullptr)
			return BehaviorState::Failure;

		float closestDistance{ FLT_MAX };
		HouseInfoExtended closestHouse;
		for(HouseInfoExtended& house : *pHouseVec)
		{
			const float distance{ house.Center.Distance(pAgentInfo->Position) };
			if(house.Looted == false && distance < closestDistance)
			{
				if(distance < 5.0f)
				{
					house.Looted = true;
				}
				else
				{
					closestHouse = house;
					closestDistance = distance;
				}
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

	BehaviorState GoToNextCheckpoint(Blackboard* pBlackboard)
	{
		PrintColorAction("GOING TO NEXT CHECKPOINT");

		// Get the agent
		AgentInfo* pAgentInfo;
		if(!pBlackboard->GetData(BB_AGENT_INFO_PTR, pAgentInfo) || pAgentInfo == nullptr)
			return BehaviorState::Failure;

		// Get the checkpoints
		std::vector<Checkpoint>* pCheckpointVec;
		if(!pBlackboard->GetData(BB_CHECKPOINTS_PTR, pCheckpointVec) || pCheckpointVec == nullptr)
			return BehaviorState::Failure;

		// Nextcheckpoint is the first that hasnt been visited yet
		for(Checkpoint& checkpoint : *pCheckpointVec)
		{
			if(checkpoint.IsVisited == false)
			{
				if(checkpoint.Location.Distance(pAgentInfo->Position) > 10.0f)
				{
					if(!pBlackboard->ChangeData(BB_STEERING_TARGET, checkpoint.Location))
						return BehaviorState::Failure;

					// Set target in the blackboard
					return BehaviorState::Success;
				}
				else
				{
					checkpoint.IsVisited = true;
				}
			}
		}
		return BehaviorState::Failure;
	}

	BehaviorState GrabClosestPistol(Blackboard* pBlackboard)
	{
		PrintColorAction("GRABBING CLOSEST PISTOL");
		// Get the agent
		AgentInfo* pAgentInfo;
		if(!pBlackboard->GetData(BB_AGENT_INFO_PTR, pAgentInfo) || pAgentInfo == nullptr)
			return BehaviorState::Failure;

		// Get the items
		std::vector<ItemInfo>* pItemVec;
		if(!pBlackboard->GetData(BB_ITEMS_PTR, pItemVec) || pItemVec == nullptr)
			return BehaviorState::Failure;


		float closestDistance{ FLT_MAX };
		ItemInfo closestItem;
		for(ItemInfo& item : *pItemVec)
		{
			const float distance{ item.Location.Distance(pAgentInfo->Position) };
			if(item.Type == eItemType::PISTOL && distance < closestDistance)
			{
				closestItem = item;
				closestDistance = distance;
			}
		}

		if(closestDistance == FLT_MAX)
		{
			// No lootable houses found
			return BehaviorState::Failure;
		}

		if(closestDistance < 3.0f)
		{
			// Pick up item
			// Get the interface
			IExamInterface* pInterface;
			if(!pBlackboard->GetData(BB_EXAM_INTERFACE_PTR, pInterface) || pInterface == nullptr)
				return BehaviorState::Failure;

			EntityInfo itemToGrab{};
			itemToGrab.EntityHash = closestItem.ItemHash;
			ItemInfo grabbedItem{};
			if(!pInterface->Item_Grab(itemToGrab, grabbedItem))
				return BehaviorState::Failure;
			if(!pInterface->Inventory_AddItem(BB_PISTOL_INV_SLOT, grabbedItem))
				return BehaviorState::Failure;

			// Remove item from the list
			pItemVec->erase(std::remove_if(pItemVec->begin(), pItemVec->end(), [closestItem](const ItemInfo& item) { return item.ItemHash == closestItem.ItemHash; }), pItemVec->end());

			return BehaviorState::Success;
		}

		// Set target in the blackboard
		if(!pBlackboard->ChangeData(BB_STEERING_TARGET, closestItem.Location))
			return BehaviorState::Failure;

		return BehaviorState::Success;
	}

	BehaviorState GrabClosestShotgun(Blackboard* pBlackboard)
	{
		PrintColorAction("GRABBING CLOSEST SHOTGUN");
		// Get the agent
		AgentInfo* pAgentInfo;
		if(!pBlackboard->GetData(BB_AGENT_INFO_PTR, pAgentInfo) || pAgentInfo == nullptr)
			return BehaviorState::Failure;

		// Get the items
		std::vector<ItemInfo>* pItemVec;
		if(!pBlackboard->GetData(BB_ITEMS_PTR, pItemVec) || pItemVec == nullptr)
			return BehaviorState::Failure;


		float closestDistance{ FLT_MAX };
		ItemInfo closestItem;
		for(ItemInfo& item : *pItemVec)
		{
			const float distance{ item.Location.Distance(pAgentInfo->Position) };
			if(item.Type == eItemType::SHOTGUN && distance < closestDistance)
			{
				closestItem = item;
				closestDistance = distance;
			}
		}

		if(closestDistance == FLT_MAX)
		{
			// No lootable houses found
			return BehaviorState::Failure;
		}

		if(closestDistance < 3.0f)
		{
			// Pick up item
			// Get the interface
			IExamInterface* pInterface;
			if(!pBlackboard->GetData(BB_EXAM_INTERFACE_PTR, pInterface) || pInterface == nullptr)
				return BehaviorState::Failure;

			EntityInfo itemToGrab{};
			itemToGrab.EntityHash = closestItem.ItemHash;
			ItemInfo grabbedItem{};
			if(!pInterface->Item_Grab(itemToGrab, grabbedItem))
				return BehaviorState::Failure;
			if(!pInterface->Inventory_AddItem(BB_SHOTGUN_INV_SLOT, grabbedItem))
				return BehaviorState::Failure;

			// Remove item from the list
			pItemVec->erase(std::remove_if(pItemVec->begin(), pItemVec->end(), [closestItem](const ItemInfo& item) { return item.ItemHash == closestItem.ItemHash; }), pItemVec->end());


			return BehaviorState::Success;
		}

		// Set target in the blackboard
		if(!pBlackboard->ChangeData(BB_STEERING_TARGET, closestItem.Location))
			return BehaviorState::Failure;

		return BehaviorState::Success;
	}

	BehaviorState GrabClosestFood(Blackboard* pBlackboard)
	{
		PrintColorAction("GRABBING CLOSEST FOOD");
		// Get the agent
		AgentInfo* pAgentInfo;
		if(!pBlackboard->GetData(BB_AGENT_INFO_PTR, pAgentInfo) || pAgentInfo == nullptr)
			return BehaviorState::Failure;

		// Get the items
		std::vector<ItemInfo>* pItemVec;
		if(!pBlackboard->GetData(BB_ITEMS_PTR, pItemVec) || pItemVec == nullptr)
			return BehaviorState::Failure;


		float closestDistance{ FLT_MAX };
		ItemInfo closestItem;
		for(ItemInfo& item : *pItemVec)
		{
			const float distance{ item.Location.Distance(pAgentInfo->Position) };
			if(item.Type == eItemType::FOOD && distance < closestDistance)
			{
				closestItem = item;
				closestDistance = distance;
			}
		}

		if(closestDistance == FLT_MAX)
		{
			// No lootable houses found
			return BehaviorState::Failure;
		}

		if(closestDistance < 3.0f)
		{
			// Pick up item
			// Get the interface
			IExamInterface* pInterface;
			if(!pBlackboard->GetData(BB_EXAM_INTERFACE_PTR, pInterface) || pInterface == nullptr)
				return BehaviorState::Failure;

			EntityInfo itemToGrab{};
			itemToGrab.EntityHash = closestItem.ItemHash;
			ItemInfo grabbedItem{};
			if(!pInterface->Item_Grab(itemToGrab, grabbedItem))
				return BehaviorState::Failure;
			if(!pInterface->Inventory_AddItem(BB_FOOD_INV_SLOT, grabbedItem))
				return BehaviorState::Failure;

			// Remove item from the list
			pItemVec->erase(std::remove_if(pItemVec->begin(), pItemVec->end(), [closestItem](const ItemInfo& item) { return item.ItemHash == closestItem.ItemHash; }), pItemVec->end());


			return BehaviorState::Success;
		}

		// Set target in the blackboard
		if(!pBlackboard->ChangeData(BB_STEERING_TARGET, closestItem.Location))
			return BehaviorState::Failure;

		return BehaviorState::Success;
	}

	BehaviorState GrabClosestMedkit(Blackboard* pBlackboard)
	{
		PrintColorAction("GRABBING CLOSEST MEDKIT");
		// Get the agent
		AgentInfo* pAgentInfo;
		if(!pBlackboard->GetData(BB_AGENT_INFO_PTR, pAgentInfo) || pAgentInfo == nullptr)
			return BehaviorState::Failure;

		// Get the items
		std::vector<ItemInfo>* pItemVec;
		if(!pBlackboard->GetData(BB_ITEMS_PTR, pItemVec) || pItemVec == nullptr)
			return BehaviorState::Failure;


		float closestDistance{ FLT_MAX };
		ItemInfo closestItem;
		for(ItemInfo& item : *pItemVec)
		{
			const float distance{ item.Location.Distance(pAgentInfo->Position) };
			if(item.Type == eItemType::MEDKIT && distance < closestDistance)
			{
				closestItem = item;
				closestDistance = distance;
			}
		}

		if(closestDistance == FLT_MAX)
		{
			// No lootable houses found
			return BehaviorState::Failure;
		}

		if(closestDistance < 3.0f)
		{
			// Pick up item
			// Get the interface
			IExamInterface* pInterface;
			if(!pBlackboard->GetData(BB_EXAM_INTERFACE_PTR, pInterface) || pInterface == nullptr)
				return BehaviorState::Failure;

			EntityInfo itemToGrab{};
			itemToGrab.EntityHash = closestItem.ItemHash;
			ItemInfo grabbedItem{};
			if(!pInterface->Item_Grab(itemToGrab, grabbedItem))
				return BehaviorState::Failure;
			if(!pInterface->Inventory_AddItem(BB_MEDKIT_INV_SLOT, grabbedItem))
				return BehaviorState::Failure;

			// Remove item from the list
			pItemVec->erase(std::remove_if(pItemVec->begin(), pItemVec->end(), [closestItem](const ItemInfo& item) { return item.ItemHash == closestItem.ItemHash; }), pItemVec->end());


			return BehaviorState::Success;
		}

		// Set target in the blackboard
		if(!pBlackboard->ChangeData(BB_STEERING_TARGET, closestItem.Location))
			return BehaviorState::Failure;

		return BehaviorState::Success;
	}

	BehaviorState UseMedkit(Blackboard* pBlackboard)
	{
		PrintColorAction("USING MEDKIT");

		IExamInterface* pExamInterface;
		if(!pBlackboard->GetData(BB_EXAM_INTERFACE_PTR, pExamInterface) || pExamInterface == nullptr)
			return BehaviorState::Failure;

		if(!pExamInterface->Inventory_UseItem(BB_MEDKIT_INV_SLOT))
			return BehaviorState::Failure;

		if(!pExamInterface->Inventory_RemoveItem(BB_MEDKIT_INV_SLOT))
			return BehaviorState::Failure;

		return BehaviorState::Success;
	}

	BehaviorState UseFood(Blackboard* pBlackboard)
	{
		PrintColorAction("USING FOOD");

		IExamInterface* pExamInterface;
		if(!pBlackboard->GetData(BB_EXAM_INTERFACE_PTR, pExamInterface) || pExamInterface == nullptr)
			return BehaviorState::Failure;

		if(!pExamInterface->Inventory_UseItem(BB_FOOD_INV_SLOT))
			return BehaviorState::Failure;

		if(!pExamInterface->Inventory_RemoveItem(BB_FOOD_INV_SLOT))
			return BehaviorState::Failure;

		return BehaviorState::Success;
	}

	BehaviorState UsePistol(Blackboard* pBlackboard)
	{
		PrintColorAction("USING PISTOL");

		IExamInterface* pExamInterface;
		if(!pBlackboard->GetData(BB_EXAM_INTERFACE_PTR, pExamInterface) || pExamInterface == nullptr)
			return BehaviorState::Failure;

		if(!pExamInterface->Inventory_UseItem(BB_PISTOL_INV_SLOT))
			return BehaviorState::Failure;

		// Leftover ammo check
		// If 0 delete
		ItemInfo weapon{};
		if(!pExamInterface->Inventory_GetItem(BB_PISTOL_INV_SLOT, weapon))
		{
			return BehaviorState::Failure;
		}


		if(pExamInterface->Weapon_GetAmmo(weapon) <= 0)
		{
			if(!pExamInterface->Inventory_RemoveItem(BB_PISTOL_INV_SLOT))
				return BehaviorState::Failure;
		}
		return BehaviorState::Success;
	}

	BehaviorState UseShotgun(Blackboard* pBlackboard)
	{
		PrintColorAction("USING SHOTGUN");

		IExamInterface* pExamInterface;
		if(!pBlackboard->GetData(BB_EXAM_INTERFACE_PTR, pExamInterface) || pExamInterface == nullptr)
			return BehaviorState::Failure;

		if(!pExamInterface->Inventory_UseItem(BB_SHOTGUN_INV_SLOT))
			return BehaviorState::Failure;

		// Leftover ammo check
		// If 0 delete
		ItemInfo weapon{};
		if(!pExamInterface->Inventory_GetItem(BB_SHOTGUN_INV_SLOT, weapon))
		{
			return BehaviorState::Failure;
		}

		if(pExamInterface->Weapon_GetAmmo(weapon) <= 0)
		{
			if(!pExamInterface->Inventory_RemoveItem(BB_SHOTGUN_INV_SLOT))
				return BehaviorState::Failure;
		}
		return BehaviorState::Success;
	}

	BehaviorState ScanArea(Blackboard* pBlackboard)
	{
		// Rotate the player so it looks around
		PrintColorAction("SCANNING AREA");

		if(!pBlackboard->ChangeData(BB_ANGULAR_VELOCITY, 1.0f))
			return BehaviorState::Failure;
		return BehaviorState::Success;

	}

	BehaviorState LookAtClosestEnemy(Blackboard* pBlackboard)
	{
		PrintColorAction("ATTEMPTING TO LOOK AT CLOSEST ENEMY");
		// Get the agent
		AgentInfo* pAgentInfo;
		if(!pBlackboard->GetData(BB_AGENT_INFO_PTR, pAgentInfo) || pAgentInfo == nullptr)
			return BehaviorState::Failure;

		// Get the enemies
		std::vector<EnemyInfoExtended>* pEnemiesVec;
		if(!pBlackboard->GetData(BB_ENEMIES_PTR, pEnemiesVec) || pEnemiesVec == nullptr)
			return BehaviorState::Failure;


		// Check if there are enemies nearby
		float closestDistance{ FLT_MAX };
		EnemyInfoExtended closestEnemy{};
		for(const EnemyInfoExtended& enemy : *pEnemiesVec)
		{
			float distance{ Distance(enemy.Location, pAgentInfo->Position) };
			if(distance < closestDistance)
			{
				closestEnemy = enemy;
				closestDistance = distance;
			}
		}
		if(closestDistance == FLT_MAX)
			return BehaviorState::Failure;

		// Change the angular rotation based on where the enemy is
		const Elite::Vector2 vectorToTarget{ closestEnemy.Location - pAgentInfo->Position };
		const float vectorToAngle{ atan2f(vectorToTarget.y, vectorToTarget.x) };

		if(vectorToAngle > pAgentInfo->Orientation)
		{
			if(!pBlackboard->ChangeData(BB_ANGULAR_VELOCITY, 1.0f))
				return BehaviorState::Failure;

			return BehaviorState::Success;
		}
		else
		{
			if(!pBlackboard->ChangeData(BB_ANGULAR_VELOCITY, -1.0f))
				return BehaviorState::Failure;

			return BehaviorState::Success;
		}

	}
}

// ******************
// --- CONDITIONS ---
// ******************
namespace BT_Conditions
{
	bool Test(Blackboard* pBlackboard)
	{
		PrintColorCondition("CHECKING TEST CONDITION");
		return true;
	}


	bool IsInPurgeZone(Blackboard* pBlackboard)
	{
		PrintColorCondition("CHECKING IF IN PURGE ZONE");
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
		PrintColorCondition("CHECKING IF LOW HEALTH");
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
		PrintColorCondition("CHECKING IF SLIGHTLY DAMAGED");
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
		PrintColorCondition("CHECKING IF LOW ENERGY");
		// Get the agent
		AgentInfo* pAgentInfo;
		if(!pBlackboard->GetData(BB_AGENT_INFO_PTR, pAgentInfo) || pAgentInfo == nullptr)
			return false;

		const float foodThreshold{ 2.0f };
		if(pAgentInfo->Energy < foodThreshold)
			return true;
		return false;
	}

	bool SlightlyUsedEnergy(Blackboard* pBlackboard)
	{
		PrintColorCondition("CHECKING IF SLIGHTLY USED ENERGY");
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
		PrintColorCondition("CHECKING IF HAS MEDKIT");
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
		PrintColorCondition("CHECKING IF HAS SHOTGUN");
		// Get the interface
		IExamInterface* pInterface;
		if(!pBlackboard->GetData(BB_EXAM_INTERFACE_PTR, pInterface) || pInterface == nullptr)
			return false;

		ItemInfo itemInfo;
		if(!pInterface->Inventory_GetItem(BB_SHOTGUN_INV_SLOT, itemInfo))
			return false;
		return true;
	}

	bool HasNoShotgun(Blackboard* pBlackboard)
	{
		PrintColorCondition("CHECKING IF HAS NO SHOTGUN");
		// Get the interface
		IExamInterface* pInterface;
		if(!pBlackboard->GetData(BB_EXAM_INTERFACE_PTR, pInterface) || pInterface == nullptr)
			return false;

		ItemInfo itemInfo;
		if(!pInterface->Inventory_GetItem(BB_SHOTGUN_INV_SLOT, itemInfo))
			return true;
		return false;
	}

	bool HasShotgunAmmo(Blackboard* pBlackboard)
	{
		PrintColorCondition("CHECKING IF HAS SHOTGUN AMMO");
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
		PrintColorCondition("CHECKING IF LOW SHOTGUN AMMO");
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
		PrintColorCondition("CHECKING IF HAS PISTOL");
		// Get the interface
		IExamInterface* pInterface;
		if(!pBlackboard->GetData(BB_EXAM_INTERFACE_PTR, pInterface) || pInterface == nullptr)
			return false;

		ItemInfo itemInfo;
		if(!pInterface->Inventory_GetItem(BB_PISTOL_INV_SLOT, itemInfo))
			return false;
		return true;
	}

	bool HasNoPistol(Blackboard* pBlackboard)
	{
		PrintColorCondition("CHECKING IF HAS NO PISTOL");
		// Get the interface
		IExamInterface* pInterface;
		if(!pBlackboard->GetData(BB_EXAM_INTERFACE_PTR, pInterface) || pInterface == nullptr)
			return false;

		ItemInfo itemInfo;
		if(!pInterface->Inventory_GetItem(BB_PISTOL_INV_SLOT, itemInfo))
			return true;
		return false;
	}

	bool HasPistolAmmo(Blackboard* pBlackboard)
	{
		PrintColorCondition("CHECKING IF HAS PISTOL AMMO");
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
		PrintColorCondition("CHECKING IF LOW PISTOL AMMO");
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
		PrintColorCondition("CHECKING IF HAS FOOD");
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
		PrintColorCondition("CHECKING IF ENEMY NEARBY");
		const float nearbyRadius{ 20.0f };  // Radius which is considered to be NEARBY

		// Get the agent
		AgentInfo* pAgentInfo;
		if(!pBlackboard->GetData(BB_AGENT_INFO_PTR, pAgentInfo) || pAgentInfo == nullptr)
			return false;

		// Get the enemies
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
		PrintColorCondition("CHECKING IF PISTOL NEARBY");
		const float nearbyRadius{ 20.0f };  // Radius which is considered to be NEARBY

		// Get the agent
		AgentInfo* pAgentInfo;
		if(!pBlackboard->GetData(BB_AGENT_INFO_PTR, pAgentInfo) || pAgentInfo == nullptr)
			return false;

		// Get the items
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
		PrintColorCondition("CHECKING IF SHOTGUN NEARBY");
		const float nearbyRadius{ 20.0f };  // Radius which is considered to be NEARBY

		// Get the agent
		AgentInfo* pAgentInfo;
		if(!pBlackboard->GetData(BB_AGENT_INFO_PTR, pAgentInfo) || pAgentInfo == nullptr)
			return false;

		// Get the items
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
		PrintColorCondition("CHECKING IF FOOD NEARBY");
		const float nearbyRadius{ 20.0f };  // Radius which is considered to be NEARBY

		// Get the agent
		AgentInfo* pAgentInfo;
		if(!pBlackboard->GetData(BB_AGENT_INFO_PTR, pAgentInfo) || pAgentInfo == nullptr)
			return false;

		// Get the items
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
		PrintColorCondition("CHECKING IF MEDKIT NEARBY");
		const float nearbyRadius{ 20.0f };  // Radius which is considered to be NEARBY

		// Get the agent
		AgentInfo* pAgentInfo;
		if(!pBlackboard->GetData(BB_AGENT_INFO_PTR, pAgentInfo) || pAgentInfo == nullptr)
			return false;

		// Get the items
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
		PrintColorCondition("CHECKING IF LOOTABLE HOUSE NEARBY");
		const float nearbyRadius{ 50.0f };

		// Get the agent
		AgentInfo* pAgentInfo;
		if(!pBlackboard->GetData(BB_AGENT_INFO_PTR, pAgentInfo) || pAgentInfo == nullptr)
			return false;

		// Get the houses
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

	bool IsLookingAtClosestEnemy(Blackboard* pBlackboard)
	{
		PrintColorCondition("CHECKING IF LOOKING AT CLOSEST ENEMY");
		// Get the agent
		AgentInfo* pAgentInfo;
		if(!pBlackboard->GetData(BB_AGENT_INFO_PTR, pAgentInfo) || pAgentInfo == nullptr)
			return false;

		// Get the enemies
		std::vector<EnemyInfoExtended>* pEnemiesVec;
		if(!pBlackboard->GetData(BB_ENEMIES_PTR, pEnemiesVec) || pEnemiesVec == nullptr)
			return false;


		// Check if there are enemies nearby
		float closestDistance{ FLT_MAX };
		EnemyInfoExtended closestEnemy{};
		for(const EnemyInfoExtended& enemy : *pEnemiesVec)
		{
			const float distance{ Distance(enemy.Location, pAgentInfo->Position) };
			if(distance < closestDistance)
			{
				closestEnemy = enemy;
				closestDistance = distance;
			}
		}
		if(closestDistance == FLT_MAX)
			return false;

		// Change the angular rotation based on where the enemy is
		const Elite::Vector2 vectorToTarget{ closestEnemy.Location - pAgentInfo->Position };
		const float vectorToAngle{ atan2f(vectorToTarget.y, vectorToTarget.x) };

		// Check if the agent is looking at the enemy
		const float angleDiff{ abs(vectorToAngle - pAgentInfo->Orientation) };
		if(angleDiff < 0.1f)
			return true;
		return false;
	}

	bool HasAngularVelocity(Blackboard* pBlackboard)
	{
		PrintColorCondition("CHECKING IF PLAYER HAS ANGULAR VELOCITY");
		const float threshold{ 0.1f };

		// Get the agent
		AgentInfo* pAgentInfo;
		if(!pBlackboard->GetData(BB_AGENT_INFO_PTR, pAgentInfo) || pAgentInfo == nullptr)
			return false;

		if(abs(pAgentInfo->AngularVelocity) > threshold)
		{
			return true;
		}
		return false;
	}
}