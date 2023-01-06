#pragma once
#include "IExamPlugin.h"
#include "Exam_HelperStructs.h"
#include "DataExtensions.h"

class IBaseInterface;
class IExamInterface;
class BehaviorTree;
class Blackboard;

class Plugin:public IExamPlugin
{
public:
	Plugin();
	virtual ~Plugin();

	void Initialize(IBaseInterface* pInterface, PluginInfo& info) override;
	void DllInit() override;
	void DllShutdown() override;

	void InitGameDebugParams(GameDebugParams& params) override;
	void Update(float dt) override;

	SteeringPlugin_Output UpdateSteering(float dt) override;
	void Render(float dt) const override;

private:
	//Interface, used to request data from/perform actions with the AI Framework
	IExamInterface* m_pInterface = nullptr;
	std::vector<HouseInfo> GetHousesInFOV() const;
	std::vector<EntityInfo> GetEntitiesInFOV() const;

	Elite::Vector2 m_Target = {};
	bool m_EnableBreakpoint = false; // Debug purpose -> When Return key is pressed, will break the program (by setting breakpoint after if)
	bool m_GrabItem = false; //Demo purpose
	bool m_UseItem = false; //Demo purpose
	bool m_RemoveItem = false; //Demo purpose
	float m_AngSpeed = 0.f; //Demo purpose

	UINT m_InventorySlot = 0;

	float m_CurrentTime = 0.0f;  // Holds track of the current time, increased every time updatesteering is called;

	// Own AI stuff
	Blackboard* m_pBlackboard;
	BehaviorTree* m_pBehaviorTree;

	// DATA STORAGE (Inputed in blackboard for the behaviortree to act on)
	std::vector<HouseInfoExtended> m_Houses;
	std::vector<ItemInfo> m_Items;
	std::vector<EnemyInfoExtended> m_Enemies;
	std::vector<PurgeZoneInfoExtended> m_PurgeZones;

	// Extra Data
	std::vector<Checkpoint> m_Checkpoints;  // Holds checkpoints for the agent to go to, and explore the map

	// OUTPUT VARS (Changed by the behaviortree for the plugin to act on)
	Elite::Vector2 m_SteeringTarget{};
	Elite::Vector2 m_NavMeshTarget{}; // for render / debug purposes
	float m_AngularVelocity{};  // -1, 0, 1 (direction in which agent wants to rotate)
	bool m_CanRun = false;
	bool m_AimToTarget = false;

	Blackboard* CreateBlackboard();
	BehaviorTree* CreateBehaviortree(Blackboard* pBlackboard) const;

	void InitCheckpoints();

	void CheckForNewHouses();
	void CheckForNewEntities();
	void HandleNewItem(const EntityInfo& entityInfo);
	void HandleNewEnemy(const EntityInfo& entityInfo);
	void HandleNewPurgeZone(const EntityInfo& entityInfo);
	void UpdateEntities();

	void UpdateOutputVariables(Blackboard* pBlackboard);
	void ResetOutputVariables(Blackboard* pBlackboard);
};

//ENTRY
//This is the first function that is called by the host program
//The plugin returned by this function is also the plugin used by the host program
extern "C"
{
	__declspec (dllexport) IPluginBase* Register()
	{
		return new Plugin();
	}
}