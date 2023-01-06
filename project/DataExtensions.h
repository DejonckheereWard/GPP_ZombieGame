#pragma once

#include "stdafx.h"
#include "Exam_HelperStructs.h"



// Extend houseinfo

struct HouseInfoExtended: public HouseInfo
{
	// Adds to the existing data from houses
	HouseInfoExtended() = default;

	// Constructor to update houseinfo to extend edition
	HouseInfoExtended(const HouseInfo& houseInfo):
		HouseInfo(houseInfo)
	{
	}

	float LastVisitTime = -1.0f;
	float EnteredTime = -1.0f;
	bool Looted = false;
};


struct PurgeZoneInfoExtended: public PurgeZoneInfo
{
	// Adds to the existing data from purgezones
	PurgeZoneInfoExtended() = default;

	// Constructor to update purgezoneinfo to extend edition
	PurgeZoneInfoExtended(const PurgeZoneInfo& purgeZoneInfo):
		PurgeZoneInfo(purgeZoneInfo)
	{
	}

	float DetectionTime{ -1.0f };  // Holds track WHEN we saw the purgezone, as it will dissapear soon after "exploding"
};

struct EnemyInfoExtended: public EnemyInfo
{
	// Adds to the existing data from enemies
	EnemyInfoExtended() = default;

	// Constructor to update enemyinfo to extend edition
	EnemyInfoExtended(const EnemyInfo& enemyInfo):
		EnemyInfo(enemyInfo)
	{
	}

	float LastSeenTime{ -1.0f };
};


struct Checkpoint
{
	Elite::Vector2 Location{};
	bool IsVisited{ false };
	float LastVisitTime{ -1.0f };  // Timestamp of last visit
};