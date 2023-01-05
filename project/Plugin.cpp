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
	//delete m_pBehaviorTree;
	//delete m_pBlackboard;
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
	params.AutoGrabClosestItem = true; //A call to Item_Grab(...) returns the closest item that can be grabbed. (EntityInfo argument is ignored)
	params.StartingDifficultyStage = 1;
	params.InfiniteStamina = false;
	params.SpawnDebugPistol = true;
	params.SpawnDebugShotgun = true;
	params.SpawnPurgeZonesOnMiddleClick = true;
	params.PrintDebugMessages = true;
	params.ShowDebugItemNames = true;
	params.Seed = 63;
}

//Only Active in DEBUG Mode
//(=Use only for Debug Purposes)
void Plugin::Update(float dt)
{
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

	CheckForNewHouses();
	CheckForNewEntities();

	// Get entities from the fov and store these in the blackboard
	std::vector<EntityInfo> entitiesInFov{ GetEntitiesInFOV() };
	m_pBlackboard->ChangeData(BB_ENTITIES_IN_FOV_PTR, &entitiesInFov);

	// Get houses
	std::vector<HouseInfo> housesInFov{ GetHousesInFOV() };
	m_pBlackboard->ChangeData(BB_HOUSES_IN_FOV_PTR, &housesInFov);



	m_pBehaviorTree->Update(dt);
	//m_pBehaviorTree->GetBlackboard();

	Elite::Vector2 behaviorTarget{};
	m_pBlackboard->GetData(BB_STEERING_TARGET, behaviorTarget);
	m_Target = behaviorTarget;
	auto steering = SteeringPlugin_Output();

	//Use the Interface (IAssignmentInterface) to 'interface' with the AI_Framework
	auto agentInfo = m_pInterface->Agent_GetInfo();



	//Use the navmesh to calculate the next navmesh point
	//auto nextTargetPos = m_pInterface->NavMesh_GetClosestPathPoint(checkpointLocation);

	//OR, Use the mouse target
	auto nextTargetPos = m_pInterface->NavMesh_GetClosestPathPoint(behaviorTarget); //Uncomment this to use mouse position as guidance

	auto vHousesInFOV = GetHousesInFOV();//uses m_pInterface->Fov_GetHouseByIndex(...)
	auto vEntitiesInFOV = GetEntitiesInFOV(); //uses m_pInterface->Fov_GetEntityByIndex(...)

	for(auto& e : vEntitiesInFOV)
	{
		if(e.Type == eEntityType::PURGEZONE)
		{
			PurgeZoneInfo zoneInfo;
			m_pInterface->PurgeZone_GetInfo(e, zoneInfo);
			//std::cout << "Purge Zone in FOV:" << e.Location.x << ", "<< e.Location.y << "---Radius: "<< zoneInfo.Radius << std::endl;
		}
	}


	//INVENTORY USAGE DEMO
	//********************

	if(m_GrabItem)
	{
		ItemInfo item;
		//Item_Grab > When DebugParams.AutoGrabClosestItem is TRUE, the Item_Grab function returns the closest item in range
		//Keep in mind that DebugParams are only used for debugging purposes, by default this flag is FALSE
		//Otherwise, use GetEntitiesInFOV() to retrieve a vector of all entities in the FOV (EntityInfo)
		//Item_Grab gives you the ItemInfo back, based on the passed EntityHash (retrieved by GetEntitiesInFOV)
		if(m_pInterface->Item_Grab({}, item))
		{
			//Once grabbed, you can add it to a specific inventory slot
			//Slot must be empty
			m_pInterface->Inventory_AddItem(m_InventorySlot, item);
		}
	}

	if(m_UseItem)
	{
		//Use an item (make sure there is an item at the given inventory slot)
		m_pInterface->Inventory_UseItem(m_InventorySlot);
	}

	if(m_RemoveItem)
	{
		//Remove an item from a inventory slot
		m_pInterface->Inventory_RemoveItem(m_InventorySlot);
	}

	//Simple Seek Behaviour (towards Target)
	steering.LinearVelocity = nextTargetPos - agentInfo.Position; //Desired Velocity
	steering.LinearVelocity.Normalize(); //Normalize Desired Velocity
	steering.LinearVelocity *= agentInfo.MaxLinearSpeed; //Rescale to Max Speed

	if(Distance(nextTargetPos, agentInfo.Position) < 2.f)
	{
		steering.LinearVelocity = Elite::ZeroVector2;
	}

	//steering.AngularVelocity = m_AngSpeed; //Rotate your character to inspect the world while walking
	steering.AutoOrient = true; //Setting AutoOrient to TRue overrides the AngularVelocity

	steering.RunMode = m_CanRun; //If RunMode is True > MaxLinSpd is increased for a limited time (till your stamina runs out)

	//SteeringPlugin_Output is works the exact same way a SteeringBehaviour output

//@End (Demo Purposes)
	m_GrabItem = false; //Reset State
	m_UseItem = false;
	m_RemoveItem = false;

	return steering;
}

//This function should only be used for rendering debug elements
void Plugin::Render(float dt) const
{
	//This Render function should only contain calls to Interface->Draw_... functions
	m_pInterface->Draw_SolidCircle(m_Target, .7f, { 0,0 }, { 1, 0, 0 });
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

Blackboard* Plugin::CreateBlackboard() const
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

	// OUTPUT DATA
	pBlackboard->AddData(BB_STEERING_TARGET, Elite::Vector2());
	pBlackboard->AddData(BB_LOOK_DIRECTION, Elite::Vector2());

	// ADD SLOTS FOR THE VARIABLES HERE - OLD
	pBlackboard->AddData(BB_ENTITIES_IN_FOV_PTR, (std::vector<EntityInfo>*)nullptr);
	pBlackboard->AddData(BB_HOUSES_IN_FOV_PTR, (std::vector<HouseInfo>*)nullptr);
	pBlackboard->AddData(BB_HOUSES_VISITED_PTR, (std::deque<Elite::Vector2>*)nullptr);

	pBlackboard->AddData(BB_ITEM_INFO_PTRS, std::vector<ItemInfo*>{});
	pBlackboard->AddData(BB_LOOK_DIRECTION, Elite::Vector2());

	return pBlackboard;
}


BehaviorTree* Plugin::CreateBehaviortree(Blackboard* pBlackboard) const
{
	/// <summary>
	/// 
	/// </summary>
	/// <param name="pBlackboard"></param>
	/// <returns></returns>
	return new BehaviorTree(m_pBlackboard,
		new BehaviorSelector(
			{
				new BehaviorSelector({
					new BehaviorSequence({
						new BehaviorAction(BT_Actions::GrabClosestItem)
					}),
					new BehaviorSequence({
						new BehaviorConditional(BT_Conditions::ItemInVision),
						new BehaviorAction(BT_Actions::GoToClosestItem)

					}),
				}),
			//new BehaviorAction(BT_ACTIONS::Scan),
				new BehaviorSequence(
				{
					new BehaviorConditional(BT_Conditions::ReachedTarget),
					new BehaviorAction(BT_Actions::RandomWander),
				}),
				new BehaviorSequence(
				{
					new BehaviorConditional(BT_Conditions::Test),
					new BehaviorAction(BT_Actions::Test)
				})
			}
		)
	);
}

void Plugin::CheckForNewHouses()
{
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
}

void Plugin::CheckForNewEntities()
{
	// Get all entities in the fov

	std::vector<EntityInfo> entitiesInFov = GetEntitiesInFOV();

	for(const EntityInfo& entity : entitiesInFov)
	{
		switch(entity.Type)
		{
			case eEntityType::ITEM:
			{
				// Check if we have seen this item before
				for(const ItemInfo& existingItem : m_Items)
				{
					if(entity.Location == existingItem.Location)
					{
						break;
					}
				}




				break;
			}
			case eEntityType::ENEMY:
				break;
			case eEntityType::PURGEZONE:
				break;
			default:
				break;
		}
	}


}

void Plugin::HandleNewItem(const EntityInfo& entityInfo)
{
	// Check if we havent found this item already
	for(const ItemInfo& existingItem : m_Items)
	{
		if(entityInfo.Location == existingItem.Location)
		{
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
	// Old enemies will have to be removed from the vector when their last seen time is too long ago

	const float maxTimeSinceLastSeen = 2.f;

	// Get the data from this enemy
	EnemyInfo enemyInfo{};
	m_pInterface->Enemy_GetInfo(entityInfo, enemyInfo);

	EnemyInfoExtended extendedInfo{ enemyInfo };
	extendedInfo.LastSeenTime = m_CurrentTime;  // Add the current time as timestamp
	m_Enemies.push_back(extendedInfo);


	// Delete enemies that are too old
	// Loop in reverse to prevent skipping when a delete happens
	for(int i = m_Enemies.size() - 1; i >= 0; --i)
	{
		if(m_CurrentTime - m_Enemies[i].LastSeenTime > maxTimeSinceLastSeen)
		{
			m_Enemies.erase(m_Enemies.begin() + i);
		}
	}
}

void Plugin::HandleNewPurgeZone(const EntityInfo& entityInfo)
{
	const float purgeZoneDuration{ 5.0f };  // Time before deleting the purgezone

	// Check if we havent found this purge zone already
	for(const PurgeZoneInfo& existingPurgeZone : m_PurgeZones)
	{
		if(entityInfo.Location == existingPurgeZone.Center)
		{
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

	// Delete purge zones that have expired
	// Loop in reverse to prevent skipping when a delete happens



}
