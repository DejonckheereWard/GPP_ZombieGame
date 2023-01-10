#include "stdafx.h"
#include "Plugin.h"
#include "IExamInterface.h"
#include "EBehaviorTree.h"
#include "EBlackboard.h"
#include "Behaviors.h"

Plugin::Plugin():
	m_pBehaviorTree{},
	m_pBlackboard{}
{
}

Plugin::~Plugin()
{
}

//Called only once, during initialization
void Plugin::Initialize(IBaseInterface* pInterface, PluginInfo& info)
{
	//Retrieving the interface
	//This interface gives you access to certain actions the AI_Framework can perform for you
	m_pInterface = static_cast<IExamInterface*>(pInterface);

	//Bit information about the plugin
	//Please fill this in!!
	info.BotName = "MinionExam";
	info.Student_FirstName = "Ward";
	info.Student_LastName = "Dejonckheere";
	info.Student_Class = "2DAE07";


	InitCheckpoints();
	m_pBlackboard = CreateBlackboard();
	m_pBehaviorTree = CreateBehaviortree(m_pBlackboard);
}

//Called only once
void Plugin::DllInit()
{
	//Called when the plugin is loaded
}

//Called only once
void Plugin::DllShutdown()
{
	//Called wheb the plugin gets unloaded
}

//Called only once, during initialization
void Plugin::InitGameDebugParams(GameDebugParams& params)
{
	params.AutoFollowCam = true; //Automatically follow the AI? (Default = true)
	params.RenderUI = true; //Render the IMGUI Panel? (Default = true)
	params.SpawnEnemies = true; //Do you want to spawn enemies? (Default = true)
	params.EnemyCount = 20; //How many enemies? (Default = 20)
	params.GodMode = false; //GodMode > You can't die, can be useful to inspect certain behaviors (Default = false)
	params.LevelFile = "GameLevel.gppl";
	params.AutoGrabClosestItem = false; //A call to Item_Grab(...) returns the closest item that can be grabbed. (EntityInfo argument is ignored)
	params.StartingDifficultyStage = 1;
	params.InfiniteStamina = false;
	params.SpawnDebugPistol = false;
	params.SpawnDebugShotgun = false;
	params.SpawnPurgeZonesOnMiddleClick = true;
	params.SpawnZombieOnRightClick = true;
	params.PrintDebugMessages = true;
	params.ShowDebugItemNames = true;
	params.Seed = 63;
}

//Only Active in DEBUG Mode
//(=Use only for Debug Purposes)
void Plugin::Update(float dt)
{
	m_EnableBreakpoint = false;
	//Demo Event Code
	//In the end your AI should be able to walk around without external input
	if(m_pInterface->Input_IsMouseButtonUp(Elite::InputMouseButton::eLeft))
	{
		//Update target based on input
		Elite::MouseData mouseData = m_pInterface->Input_GetMouseData(Elite::InputType::eMouseButton, Elite::InputMouseButton::eLeft);
		const Elite::Vector2 pos = Elite::Vector2(static_cast<float>(mouseData.X), static_cast<float>(mouseData.Y));
		m_Target = m_pInterface->Debug_ConvertScreenToWorld(pos);
	}
	else if(m_pInterface->Input_IsKeyboardKeyDown(Elite::eScancode_Space))
	{
		m_CanRun = true;
	}
	else if(m_pInterface->Input_IsKeyboardKeyDown(Elite::eScancode_Return))
	{
		m_EnableBreakpoint = true;
	}
	else if(m_pInterface->Input_IsKeyboardKeyDown(Elite::eScancode_Left))
	{
		m_AngSpeed -= Elite::ToRadians(10);
	}
	else if(m_pInterface->Input_IsKeyboardKeyDown(Elite::eScancode_Right))
	{
		m_AngSpeed += Elite::ToRadians(10);
	}
	else if(m_pInterface->Input_IsKeyboardKeyDown(Elite::eScancode_G))
	{
		m_GrabItem = true;
	}
	else if(m_pInterface->Input_IsKeyboardKeyDown(Elite::eScancode_U))
	{
		m_UseItem = true;
	}
	else if(m_pInterface->Input_IsKeyboardKeyDown(Elite::eScancode_R))
	{
		m_RemoveItem = true;
	}
	else if(m_pInterface->Input_IsKeyboardKeyUp(Elite::eScancode_Space))
	{
		m_CanRun = false;
	}
	else if(m_pInterface->Input_IsKeyboardKeyDown(Elite::eScancode_Delete))
	{
		m_pInterface->RequestShutdown();
	}
	else if(m_pInterface->Input_IsKeyboardKeyDown(Elite::eScancode_KP_Minus))
	{
		if(m_InventorySlot > 0)
			--m_InventorySlot;
	}
	else if(m_pInterface->Input_IsKeyboardKeyDown(Elite::eScancode_KP_Plus))
	{
		if(m_InventorySlot < 4)
			++m_InventorySlot;
	}
	else if(m_pInterface->Input_IsKeyboardKeyDown(Elite::eScancode_Q))
	{
		ItemInfo info = {};
		m_pInterface->Inventory_GetItem(m_InventorySlot, info);
		std::cout << (int)info.Type << std::endl;
	}
}

//Update
//This function calculates the new SteeringOutput, called once per frame
SteeringPlugin_Output Plugin::UpdateSteering(float dt)
{
	// This is basically the main update function (the other update is debug only)
	m_CurrentTime += dt;  // Keep track of total time since program started (used for timestamping events)

	//std::cout << "-------------- UPDATING STEERING --------------\n";
	//std::cout << "------------- CURRENT TIME: " << m_CurrentTime << " ------------\n";

	//std::cout << "AGENT ORIENTATION: " << m_pInterface->Agent_GetInfo().Orientation << std::endl;

	// Reset target before update
	ResetOutputVariables(m_pBlackboard);
	CheckForNewHouses();
	CheckForNewEntities();
	UpdateEntities();  // Updates and cleans up the storage of entities where needed
	m_pBlackboard->ChangeData(BB_AGENT_INFO_PTR, &m_pInterface->Agent_GetInfo());
	m_pBlackboard->ChangeData(BB_CURRENT_TIME, m_CurrentTime);
	m_pBehaviorTree->Update(dt);

	UpdateOutputVariables(m_pBlackboard);  // Copies output variables into the member variables as needed

	if(m_EnableBreakpoint)
		std::cout << "BREAKPOINT HERE\n";

	Elite::Vector2 behaviorTarget{};
	m_pBlackboard->GetData(BB_STEERING_TARGET, behaviorTarget);

	SteeringPlugin_Output steering = SteeringPlugin_Output();

	//Use the Interface (IAssignmentInterface) to 'interface' with the AI_Framework
	AgentInfo agentInfo = m_pInterface->Agent_GetInfo();

	//Use the navmesh to calculate the next navmesh point
	Elite::Vector2 nextTargetPos = m_pInterface->NavMesh_GetClosestPathPoint(behaviorTarget);
	m_NavMeshTarget = nextTargetPos;

	//Simple Seek Behaviour (towards Target)
	steering.LinearVelocity = nextTargetPos - agentInfo.Position; //Desired Velocity
	steering.LinearVelocity.Normalize(); //Normalize Desired Velocity
	steering.LinearVelocity *= agentInfo.MaxLinearSpeed; //Rescale to Max Speed

	steering.AngularVelocity = m_AngularVelocity;
	steering.AngularVelocity *= agentInfo.MaxAngularSpeed;

	steering.AutoOrient = false; //Setting AutoOrient to TRue overrides the AngularVelocity
	steering.RunMode = m_CanRun; //If RunMode is True > MaxLinSpd is increased for a limited time (till your stamina runs out)


	return steering;
}

//This function should only be used for rendering debug elements
void Plugin::Render(float dt) const
{

	//This Render function should only contain calls to Interface->Draw_... functions
	m_pInterface->Draw_SolidCircle(m_Target, .7f, { 0,0 }, { 1, 0, 0 });


	// Draw lines between the checkpoints
	for(size_t i{}; i < m_Checkpoints.size(); ++i)
	{
		const Checkpoint& current = m_Checkpoints[i];
		const Checkpoint& next = m_Checkpoints[(i + 1) % m_Checkpoints.size()];
		const Elite::Vector3 lineColor{ 0,0,0 };

		if(i == 0)
		{
			// Draw the first checkpoint
			m_pInterface->Draw_SolidCircle(current.Location, 5.0f, {}, { 0.5f, 0.1f, 0.1f });
		}

		m_pInterface->Draw_Segment(current.Location, next.Location, lineColor);
	}

	// Draw line to steering target & circle
	m_pInterface->Draw_Segment(m_pInterface->Agent_GetInfo().Position, m_SteeringTarget, { 1, 0, 0 }, m_pInterface->NextDepthSlice());
	m_pInterface->Draw_SolidCircle(m_SteeringTarget, .5f, {}, { 1, 0, 0 }, m_pInterface->NextDepthSlice());

	// Draw line to navmeshtarget & circle
	m_pInterface->Draw_Segment(m_pInterface->Agent_GetInfo().Position, m_NavMeshTarget, { 0, 1, 0 }, m_pInterface->NextDepthSlice());
	m_pInterface->Draw_SolidCircle(m_NavMeshTarget, .5f, {}, { 0, 1, 0 }, m_pInterface->NextDepthSlice());
}

std::vector<HouseInfo> Plugin::GetHousesInFOV() const
{
	std::vector<HouseInfo> vHousesInFOV = {};

	HouseInfo hi = {};
	for(int i = 0;; ++i)
	{
		if(m_pInterface->Fov_GetHouseByIndex(i, hi))
		{
			vHousesInFOV.push_back(hi);
			continue;
		}

		break;
	}

	return vHousesInFOV;
}

std::vector<EntityInfo> Plugin::GetEntitiesInFOV() const
{
	std::vector<EntityInfo> vEntitiesInFOV = {};

	EntityInfo ei = {};
	for(int i = 0;; ++i)
	{
		if(m_pInterface->Fov_GetEntityByIndex(i, ei))
		{
			vEntitiesInFOV.push_back(ei);
			continue;
		}

		break;
	}

	return vEntitiesInFOV;
}

Blackboard* Plugin::CreateBlackboard()
{
	Blackboard* pBlackboard{ new Blackboard() };


	// INPUT DATA
	pBlackboard->AddData(BB_HOUSES_PTR, &m_Houses);  // Stores all houses we have seen before
	pBlackboard->AddData(BB_ITEMS_PTR, &m_Items);
	pBlackboard->AddData(BB_ENEMIES_PTR, &m_Enemies);
	pBlackboard->AddData(BB_PURGEZONES_PTR, &m_PurgeZones);

	pBlackboard->AddData(BB_AGENT_INFO_PTR, &m_pInterface->Agent_GetInfo());  // Agent doesnt get hashed
	pBlackboard->AddData(BB_WORLD_INFO_PTR, &m_pInterface->World_GetInfo());  // Worldinfo doesnt get hashed
	pBlackboard->AddData(BB_EXAM_INTERFACE_PTR, m_pInterface);

	// Extra INPUT
	pBlackboard->AddData(BB_CHECKPOINTS_PTR, &m_Checkpoints);
	pBlackboard->AddData(BB_CURRENT_TIME, m_CurrentTime);

	// OUTPUT DATA
	pBlackboard->AddData(BB_STEERING_TARGET, m_SteeringTarget);
	pBlackboard->AddData(BB_CAN_RUN, m_CanRun);
	pBlackboard->AddData(BB_ANGULAR_VELOCITY, m_AngularVelocity);
	//pBlackboard->AddData(BB_LOOK_DIRECTION, Elite::Vector2());

	return pBlackboard;
}


BehaviorTree* Plugin::CreateBehaviortree(Blackboard* pBlackboard) const
{
	// Order of the behaviortree should be from highest priority to lowest priority
	// Ex. Running away from enemy has a higher priority than going to a house
	return new BehaviorTree(m_pBlackboard,
		new BehaviorSelector({
			// Healing
			new BehaviorSequence({
				new BehaviorConditional(BT_Conditions::LowHealth),
				new BehaviorConditional(BT_Conditions::HasMedkit),
				new BehaviorAction(BT_Actions::UseMedkit),
				new BehaviorConditional(BT_Conditions::Continue)  // Fails the sequence so this doesnt "stop" the tree;
			}),

			// Eating
			new BehaviorSequence({
				new BehaviorConditional(BT_Conditions::LowEnergy),
				new BehaviorConditional(BT_Conditions::HasFood),
				new BehaviorAction(BT_Actions::UseFood),
				new BehaviorConditional(BT_Conditions::Continue)  // Fails the sequence so this doesnt "stop" the tree;
			}),

			new BehaviorSequence({
				new BehaviorConditional(BT_Conditions::RecentlyBitten),
				new BehaviorAction(BT_Actions::BurstSprint),
				new BehaviorConditional(BT_Conditions::Continue)  // Fails the sequence so this doesnt "stop" the tree;
			}),

			// Below other important stuff, but they are all set to continue;
			// Purge zones
			new BehaviorSequence({
				new BehaviorConditional(BT_Conditions::IsInPurgeZone),
				new BehaviorAction(BT_Actions::FleeFromPurgeZones)
			}),

			// Enemies
			new BehaviorSequence({
				new BehaviorConditional(BT_Conditions::EnemyNearby),
				new BehaviorSelector({
					new BehaviorSequence({
						new BehaviorConditional(BT_Conditions::HasShotgun),
						new BehaviorConditional(BT_Conditions::HasShotgunAmmo),
						new BehaviorAction(BT_Actions::LookAtClosestEnemy),
						new BehaviorAction(BT_Actions::BurstSprint),
						new BehaviorConditional(BT_Conditions::IsLookingAtClosestEnemy),
						new BehaviorAction(BT_Actions::UseShotgun)  // LOOK AT + USE WEAPON
					}),
					new BehaviorSequence({
						new BehaviorConditional(BT_Conditions::HasPistol),
						new BehaviorConditional(BT_Conditions::HasPistolAmmo),
						new BehaviorAction(BT_Actions::LookAtClosestEnemy),
						new BehaviorAction(BT_Actions::BurstSprint),
						new BehaviorConditional(BT_Conditions::IsLookingAtClosestEnemy),
						new BehaviorAction(BT_Actions::UsePistol)  // LOOK AT + USE WEAPON
					}),
					new BehaviorSelector({
						new BehaviorSequence({
							new BehaviorConditional(BT_Conditions::LowHealth),
							new BehaviorAction(BT_Actions::Sprint),
							new BehaviorConditional(BT_Conditions::Continue)
						}),
						new BehaviorSequence({
							new BehaviorAction(BT_Actions::BurstSprint),
							new BehaviorConditional(BT_Conditions::Continue)
						})
					})
				})
			}),

			// Searching area
			new BehaviorSequence({
				// Check items
				new BehaviorSelector({
					// Pistol
					new BehaviorSequence({
						// Check if there is a weapon nearby
						new BehaviorConditional(BT_Conditions::PistolNearby),
						new BehaviorSelector({
							new BehaviorSequence({
								// Check if we already have a weapon, or if we are low on ammo
								new BehaviorConditional(BT_Conditions::HasNoPistol),
								//new BehaviorAction(BT_Actions::AimTowardsSteeringTarget),
								new BehaviorAction(BT_Actions::GrabClosestPistol)
							}),
							new BehaviorSequence({
								new BehaviorConditional(BT_Conditions::HasPistol),
								new BehaviorConditional(BT_Conditions::LowPistolAmmo),
								//new BehaviorAction(BT_Actions::AimTowardsSteeringTarget),
								new BehaviorAction(BT_Actions::GrabClosestPistol)  // GOTO + GRAB IF IN RANGE
							})
						}),
					}),

					// Shotgun
					new BehaviorSequence({
						// Check if there is a weapon nearby
						new BehaviorConditional(BT_Conditions::ShotgunNearby),
						new BehaviorSelector({
							new BehaviorSequence({
								// Check if we already have a weapon, or if we are low on ammo
								new BehaviorConditional(BT_Conditions::HasNoShotgun),
								//new BehaviorAction(BT_Actions::AimTowardsSteeringTarget),
								new BehaviorAction(BT_Actions::GrabClosestShotgun)
							}),
							new BehaviorSequence({
								new BehaviorConditional(BT_Conditions::HasShotgun),
								new BehaviorConditional(BT_Conditions::LowShotgunAmmo),
								//new BehaviorAction(BT_Actions::AimTowardsSteeringTarget),
								new BehaviorAction(BT_Actions::GrabClosestShotgun)  // GOTO + GRAB IF IN RANGE
							})
						}),
					}),

					// Medkits
					new BehaviorSequence({
						new BehaviorConditional(BT_Conditions::MedkitNearby),
						new BehaviorSelector({
							new BehaviorSequence({
								new BehaviorConditional(BT_Conditions::HasNoMedkit),
								//new BehaviorAction(BT_Actions::AimTowardsSteeringTarget),
								new BehaviorAction(BT_Actions::GrabClosestMedkit),
							}),
							new BehaviorSequence({
								new BehaviorConditional(BT_Conditions::SlightlyDamaged),
								new BehaviorAction(BT_Actions::UseMedkit),
								//new BehaviorAction(BT_Actions::AimTowardsSteeringTarget),
								new BehaviorAction(BT_Actions::GrabClosestMedkit)
							})
						}),
					}),

					// Food
					new BehaviorSequence({
						new BehaviorConditional(BT_Conditions::FoodNearby),
						new BehaviorSelector({
							new BehaviorSequence({
								new BehaviorConditional(BT_Conditions::HasNoFood),  // By using selector, grabclosestfood will only exec if hasfood is false
								//new BehaviorAction(BT_Actions::AimTowardsSteeringTarget),
								new BehaviorAction(BT_Actions::GrabClosestFood)
							}),
							new BehaviorSequence({
								new BehaviorConditional(BT_Conditions::SlightlyUsedEnergy),  // True if player lost a little of energy, but its not low
								new BehaviorAction(BT_Actions::UseFood),
								//new BehaviorAction(BT_Actions::AimTowardsSteeringTarget),
								new BehaviorAction(BT_Actions::GrabClosestFood)
							})
						})
					}),

					// Garbage
					new BehaviorSequence({
						new BehaviorConditional(BT_Conditions::GarbageNearby),
						//new BehaviorAction(BT_Actions::AimTowardsSteeringTarget),
						new BehaviorAction(BT_Actions::DestroyClosestGarbage),
					})
				})
			}),

			// No items nearby, no immediate things to do, so we need to find new stuff
			// If we know about any unvisited houses, go visit them and search them
			// If we dont, we need to roam the map, also notice how the map has a limited radius of where the houses spawn, so stay in there
			new BehaviorSequence({
				new BehaviorConditional(BT_Conditions::LootableHouseNearby),
				new BehaviorAction(BT_Actions::GoToClosestLootableHouse),
				new BehaviorAction(BT_Actions::ScanArea), // Still want to rotate when going towards a lootable house

			}),
			new BehaviorSequence({
				new BehaviorSelector({
					new BehaviorConditional(BT_Conditions::HasAngularVelocity),  // Only if false, do scan
					new BehaviorAction(BT_Actions::ScanArea),
				}),
				new BehaviorAction(BT_Actions::GoToNextCheckpoint)
			}),

			// Shouldnt be reaching this if any of previous worked.
			new BehaviorSequence({
				// Only if condition returns true, will the next node be executed  (For sequences)
				new BehaviorConditional(BT_Conditions::Test),
				new BehaviorAction(BT_Actions::Test)
			}),

			})
	);
}

void Plugin::InitCheckpoints()
{
	// Create checkpoints spread over the map
	// Checkpoints are used to make the agent roam the map and find new stuff

	// Place the checkpoints in a circle around the center of the map (0 0)
	// Put the checkpoints in a 11/5 hendecagram order (star shaped)


	const float radius{ 250.0f };
	const int corners{ 11 };
	const int skips{ 4 };  // See internet for good values (star polygons)	

	// Generate random number using default random engine
	std::random_device rd;
	std::default_random_engine generator(rd());
	std::uniform_real_distribution<double> distribution(-M_PI, M_PI);

	float randomRotationOffset{ (float)distribution(generator) };

	for(size_t i = 0; i < 11; i++)
	{
		float angle{ float(i) * ((2.0f * float(M_PI)) / float(corners) * float(skips)) };
		angle += randomRotationOffset;

		Checkpoint checkpoint{};
		checkpoint.Location = Elite::Vector2{ radius * cos(angle), radius * sin(angle) };
		checkpoint.LastVisitTime = -1.0f;
		checkpoint.IsVisited = false;

		m_Checkpoints.push_back(checkpoint);
	}


	//const float circleRadius{ 200.0f };
	//const int steps{ 8 };
	//const float angleStep{ 360.0f / float(steps) };
	//for(int i = 0; i < steps; i++)
	//{
	//	const float angle{ i * angleStep };

	//	Elite::Vector2 checkpointLocation{};
	//	checkpointLocation.x = circleRadius * cos(angle);
	//	checkpointLocation.y = circleRadius * sin(angle);

	//	Checkpoint checkpoint{};
	//	checkpoint.Location = checkpointLocation;
	//	checkpoint.LastVisitTime = -1.0f;
	//	checkpoint.IsVisited = false;

	//	m_Checkpoints.push_back(checkpoint);
	//}

}

void Plugin::CheckForNewHouses()
{
	const float resetThreshold{ 180.0f };  // Time before a house gets reset;

	// Get all houses in FOV
	std::vector<HouseInfo> housesInFov = GetHousesInFOV();

	// Check if we have seen this house before
	for(const HouseInfo& houseInFov : housesInFov)
	{
		bool alreadyFound = false;
		for(const HouseInfoExtended& existingHouse : m_Houses)
		{
			if(houseInFov.Center == existingHouse.Center)
			{
				alreadyFound = true;
				break;
			}
		}

		// If we haven't seen this house before, add it to the list
		if(!alreadyFound)
		{
			HouseInfoExtended house{ houseInFov };
			m_Houses.push_back(house);
		}
	}

	// Loop through the list of found houses, if any of the houses is looted, and its a long time ago, we can reset the looted status
	for(HouseInfoExtended& house : m_Houses)
	{
		if(house.Looted)
		{
			const float timeSinceVisit{ m_CurrentTime - house.LastVisitTime };
			if(timeSinceVisit > resetThreshold)
			{
				house.Looted = false;
				house.LastVisitTime = -1.0f;
				house.EnteredTime = -1.0f;
			}
		}
	}
}

void Plugin::CheckForNewEntities()
{
	// Get all entities in the fov
	m_Enemies.clear();

	std::vector<EntityInfo> entitiesInFov = GetEntitiesInFOV();

	for(const EntityInfo& entity : entitiesInFov)
	{
		switch(entity.Type)
		{
			case eEntityType::ITEM:
				HandleNewItem(entity);
				break;
			case eEntityType::ENEMY:
				HandleNewEnemy(entity);
				break;
			case eEntityType::PURGEZONE:
				HandleNewPurgeZone(entity);
				break;
			default:
				break;
		}
	}


}

void Plugin::HandleNewItem(const EntityInfo& entityInfo)
{
	// Check if we havent found this item already
	for(ItemInfo& existingItem : m_Items)
	{
		if(entityInfo.Location == existingItem.Location)
		{
			existingItem.ItemHash = entityInfo.EntityHash;
			return;
		}
	}

	// If we havent found this item already, add it to the list
	// First get the data using the interface
	ItemInfo itemInfo{};
	m_pInterface->Item_GetInfo(entityInfo, itemInfo);

	// Add the item to the list
	m_Items.push_back(itemInfo);


}

void Plugin::HandleNewEnemy(const EntityInfo& entityInfo)
{
	// We always want to add enemies to the vector, since they move and their location changes

	// Get the data from this enemy
	EnemyInfo enemyInfo{};
	m_pInterface->Enemy_GetInfo(entityInfo, enemyInfo);

	EnemyInfoExtended extendedInfo{ enemyInfo };
	extendedInfo.LastSeenTime = m_CurrentTime;  // Add the current time as timestamp
	m_Enemies.push_back(extendedInfo);

}

void Plugin::HandleNewPurgeZone(const EntityInfo& entityInfo)
{

	// Check if we havent found this purge zone already
	for(PurgeZoneInfoExtended& existingPurgeZone : m_PurgeZones)
	{
		if(entityInfo.Location == existingPurgeZone.Center)
		{
			existingPurgeZone.ZoneHash = entityInfo.EntityHash;
			return;
		}
	}

	// If we havent found this purge zone already, add it to the list
	// First get the data using the interface
	PurgeZoneInfo purgeZoneInfo{};
	m_pInterface->PurgeZone_GetInfo(entityInfo, purgeZoneInfo);
	PurgeZoneInfoExtended extendedInfo{ purgeZoneInfo };

	extendedInfo.DetectionTime = m_CurrentTime;

	// Add the purge zone to the list
	m_PurgeZones.push_back(extendedInfo);



}

void Plugin::UpdateEntities()
{
	// Delete purge zones that have expired
	// Loop in reverse to prevent skipping when a delete happens
	const float purgeZoneDuration{ 8.0f };  // Time before deleting the purgezone 
	for(int i = m_PurgeZones.size() - 1; i >= 0; --i)
	{
		if(m_CurrentTime - m_PurgeZones[i].DetectionTime > purgeZoneDuration)
		{
			m_PurgeZones.erase(m_PurgeZones.begin() + i);
		}
	}


	// Old enemies will have to be removed from the vector when their last seen time is too long ago
	// Delete enemies that are too old
	// Loop in reverse to prevent skipping when a delete happens
	//const float maxTimeSinceLastSeenEnemy = 2.f;
	//for(int i = m_Enemies.size() - 1; i >= 0; --i)
	//{
	//	if(m_CurrentTime - m_Enemies[i].LastSeenTime > maxTimeSinceLastSeenEnemy)
	//	{
	//		m_Enemies.erase(m_Enemies.begin() + i);
	//	}
	//	if(m_Enemies[i].Health <= 0)
	//	{
	//		m_Enemies.erase(m_Enemies.begin() + i);
	//	}
	//}



}

void Plugin::UpdateOutputVariables(Blackboard* pBlackboard)
{
	// Updates the output variables since they arent passed by pointer
	bool result{ true };
	result &= pBlackboard->GetData(BB_STEERING_TARGET, m_SteeringTarget);
	result &= pBlackboard->GetData(BB_CAN_RUN, m_CanRun);
	result &= pBlackboard->GetData(BB_ANGULAR_VELOCITY, m_AngularVelocity);

	if(!result)
	{
		std::cout << "FAILED TO UPDATE OUTPUT VARIABLES\n";
	}

}

void Plugin::ResetOutputVariables(Blackboard* pBlackboard)
{
	m_SteeringTarget = m_pInterface->Agent_GetInfo().Position;  // Reset to current position, not 0
	m_AngularVelocity = 0;
	m_CanRun = false;


	bool result{ true };
	result &= pBlackboard->ChangeData(BB_STEERING_TARGET, m_SteeringTarget);
	result &= pBlackboard->ChangeData(BB_CAN_RUN, m_CanRun);
	result &= pBlackboard->ChangeData(BB_ANGULAR_VELOCITY, m_AngularVelocity);
}
