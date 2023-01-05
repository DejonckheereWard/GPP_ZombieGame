#pragma once

#include "stdafx.h"
#include "Exam_HelperStructs.h"



// Extend houseinfo

struct HouseInfoExtended: public HouseInfo
{
	// Adds to the existing data from houses

	// Constructor to update houseinfo to extend edition
	HouseInfoExtended(const HouseInfo& houseInfo):
		HouseInfo(houseInfo)
	{
	}

	float VisitedTime = 0.0f;
	bool IsVisited = false;
};


struct PurgeZoneInfoExtended: public PurgeZoneInfo
{
	// Adds to the existing data from purgezones

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

	// Constructor to update enemyinfo to extend edition
	EnemyInfoExtended(const EnemyInfo& enemyInfo):
		EnemyInfo(enemyInfo)
	{
	}

	float LastSeenTime{ -1.0f };
};