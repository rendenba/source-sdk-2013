#ifndef COVEN_PARSE_H
#define COVEN_PARSE_H
#ifdef _WIN32
#pragma once
#endif

#include "shareddefs.h"

#define MAX_COVEN_STRING	80
#define COVEN_PRINTNAME_MISSING "!!! Missing Coven printname!"

#define EFFECT_FLAG_MAG_AS_TIMER	(1 << 0)	//shows the timer wipe regardless of timer enabled
#define EFFECT_FLAG_SPLIT_DEF		(1 << 1)	//split buff (diff name and icon for diff teams)

#define ITEM_FLAG_MOVEMENT_CANCEL	(1 << 0)	//movement cancels the use of item

typedef enum {
	COVEN_SND_START,
	COVEN_SND_STOP,

	// Add new shoot sound types here

	NUM_COVEN_SOUND_TYPES,
} CovenSound_t;

class KeyValues;
class CHudTexture;

class CovenAmmo_t
{
public:
	CovenAmmo_t()
	{
		szAmmoName[0] = 0;
		iAmmoCount = 0;
	};

	char	szAmmoName[MAX_COVEN_STRING];
	int		iAmmoCount;
};

class CovenClassInfo_t
{
public:

	CovenClassInfo_t();

	// Each game can override this to get whatever values it wants from the script.
	virtual void Parse(KeyValues *pKeyValuesData);


public:
	bool					bParsedScript;
	bool					bLoadedHudElements;

	// SHARED
	char					szName[MAX_COVEN_STRING];
	char					szPrintName[MAX_COVEN_STRING];			// Name for showing in HUD, etc.
	char					szDescription[MAX_COVEN_STRING];		// description for HUD

	unsigned int			iClassNumber;							// index for less'ing
	char					szModelName[MAX_COVEN_STRING];
	CovenAbility_t			iAbilities[COVEN_MAX_ABILITIES];
	float					flConstitution;
	float					flStrength;
	float					flIntellect;
	float					flBaseSpeed;
	float					flAddConstitution;
	float					flAddStrength;
	float					flAddIntellect;
	int						iFlags;									// miscellaneous flags

	CUtlVector<CovenAmmo_t *>	tAmmo;
	CUtlVector<char *>		szWeapons;

	// CLIENT DLL
	// Sprite data, read from the data file

	// SERVER DLL

private:
	void ParseAbility(KeyValues *kv, const char *szName, CovenAbility_t &value);
};

class CovenAbilityInfo_t
{
public:

	CovenAbilityInfo_t();

	// Each game can override this to get whatever values it wants from the script.
	virtual void Parse(KeyValues *pKeyValuesData);


public:
	bool					bParsedScript;
	bool					bLoadedHudElements;

	// SHARED
	char					szName[MAX_COVEN_STRING];
	char					szPrintName[MAX_COVEN_STRING];			// Name for showing in HUD, etc.
	char					szDescription[MAX_COVEN_STRING];		// description for HUD
	float					flCooldown;
	int						iMagnitude;
	float					flDuration;
	float					flRange;
	float					flCost;
	float					flDrain;
	bool					bPassive;
	int						iFlags;									// miscellaneous flags

	// Sound blocks
	char					aSounds[NUM_COVEN_SOUND_TYPES][MAX_COVEN_STRING];

	// CLIENT DLL
	// Sprite data, read from the data file
	int						iSpriteCount;
	CHudTexture				*abilityIconActive;

	// SERVER DLL
};

typedef enum
{
	SHOW_NEVER,
	SHOW_ALWAYS,
	SHOW_TEAM_SLAYERS,
	SHOW_TEAM_VAMPIRES
} StatusEffectShow_t;

class CovenStatusEffectInfo_t
{
public:

	CovenStatusEffectInfo_t();

	// Each game can override this to get whatever values it wants from the script.
	virtual void Parse(KeyValues *pKeyValuesData);


public:
	bool					bParsedScript;
	bool					bLoadedHudElements;

	// SHARED
	char					szName[MAX_COVEN_STRING];
	char					szPrintName[MAX_COVEN_STRING];			// Name for showing in HUD, etc.
	char					szAltPrintName[MAX_COVEN_STRING];		// Vampire name and other is Slayer Name
	int						iFlags;									// miscellaneous flags

	// CLIENT DLL
	StatusEffectShow_t		iShowMagnitude;
	StatusEffectShow_t		iShowTimer;
	bool					bIsDebuff;
	bool					bAltIsDebuff;
	// Sprite data, read from the data file
	int						iSpriteCount;
	CHudTexture				*statusIcon;
	CHudTexture				*altStatusIcon;
	CUtlVector<float>		flDataVariables;

	// SERVER DLL
};

class CovenBuildingInfo_t
{
public:
	CovenBuildingInfo_t();

	// Each game can override this to get whatever values it wants from the script.
	virtual void Parse(KeyValues *pKeyValuesData);

public:
	bool					bParsedScript;
	bool					bLoadedHudElements;

	// SHARED
	char					szName[MAX_COVEN_STRING];
	char					szPrintName[MAX_COVEN_STRING];			// Name for showing in HUD, etc.
	char					szModelName[MAX_COVEN_STRING];
	int						iFlags;									// miscellaneous flags
	int						iMaxLevel;
	CUtlVector<int>			iHealths;
	CUtlVector<int>			iXPs;
	CUtlVector<int>			iCosts;
	CUtlVector<int>			iAmmo1;
	CUtlVector<int>			iAmmo2;
	CUtlVector<int>			iAmmo3;
	// CLIENT DLL
	// Sprite data, read from the data file
	// SERVER DLL
};

class CovenItemInfo_t
{
public:
	CovenItemInfo_t();

	// Each game can override this to get whatever values it wants from the script.
	virtual void Parse(KeyValues *pKeyValuesData);

public:
	bool					bParsedScript;
	bool					bLoadedHudElements;

	// SHARED
	char					szName[MAX_COVEN_STRING];
	char					szPrintName[MAX_COVEN_STRING];			// Name for showing in HUD, etc.
	char					szModelName[MAX_COVEN_STRING];
	char					szIconName[MAX_COVEN_STRING];			// "Class" name for hud effects
	int						iFlags;									// miscellaneous flags
	int						iMagnitude;
	float					flDuration;
	int						iCost;
	float					flUseTime;
	int						iCarry;
	float					flMaximum;								// some sort of maximum effect
	char					aSounds[NUM_COVEN_SOUND_TYPES][MAX_COVEN_STRING];
	// CLIENT DLL
	// Sprite data, read from the data file
	CHudTexture				*hudIcon;
	CHudTexture				*hudIconOff;
	// SERVER DLL
};

void PrecacheAbilities(IFileSystem *filesystem);
void PrecacheClasses(IFileSystem *filesystem);
void PrecacheStatusEffects(IFileSystem *filesystem);
void PrecacheBuildings(IFileSystem *filesystem);
void PrecacheItems(IFileSystem *filesystem);

#ifdef CLIENT_DLL
void LoadAbilitySprites(CovenAbilityInfo_t *info);
void LoadStatusEffectSprites(CovenStatusEffectInfo_t *info);
void LoadItemSprites(CovenItemInfo_t *info);
#endif

CovenClassInfo_t			*GetCovenClassData(CovenClassID_t iClass);
CovenClassInfo_t			*GetCovenClassData(int iPartialClassNum, int iTeam); //client-side must use this accessor! (they don't receive a full class descriptor)

CovenAbilityInfo_t			*GetCovenAbilityData(CovenAbility_t iAbility);

CovenStatusEffectInfo_t		*GetCovenStatusEffectData(CovenStatus_t iStatus);

CovenBuildingInfo_t			*GetCovenBuildingData(BuildingType_t iBuildingType);

CovenItemInfo_t				*GetCovenItemData(CovenItemID_t iItemType);
#endif