#include "cbase.h"
#include <KeyValues.h>
#include <tier0/mem.h>
#include "filesystem.h"
#include "utldict.h"
#include "coven_parse.h"

#ifdef CLIENT_DLL
#include "hud.h"
void FreeHudTextureList(CUtlDict< CHudTexture *, int >& list);
static CHudTexture *FindHudTextureInDict(CUtlDict< CHudTexture *, int >& list, const char *psz)
{
	int idx = list.Find(psz);
	if (idx == list.InvalidIndex())
		return NULL;

	return list[idx];
}
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// The sound categories found in the weapon classname.txt files
// This needs to match the enum in coven_parse.h
const char *pCovenSoundCategories[NUM_COVEN_SOUND_TYPES] =
{
	"start",
	"stop"
};

//BB: this must be in the same order as shareddefs
static const unsigned short iCovenTeamCounts[COVEN_MAX_TEAMS] =
{
	0,
	0,
	COVEN_CLASSCOUNT_SLAYERS,
	COVEN_CLASSCOUNT_VAMPIRES
};

//BB: this must be in the same order as shareddefs
static const char *pCovenClasses[COVEN_MAX_CLASSCOUNT] =
{
	"NULL",
	"avenger",
	"reaver",
	"hellion",
	"fiend",
	"gore",
	"degen"
};

//BB: this must be in the same order as shareddefs
static const char *pCovenAbilities[COVEN_ABILITY_COUNT] =
{
	"None",
	"haste",
	"battleyell",
	"sneak",
	"bloodlust",
	"leap",
	"charge",
	"phase",
	"dreadscream",
	"berserk",
	"intimidatingshout",
	"detonateblood",
	"dodge",
	"sheerwill",
	"revenge",
	"gorge",
	"masochist",
	"gutcheck",
	"buildturret",
	"builddispenser",
	"tripmine",
	"darkwill",
	"dash",
	"innerlight",
	"demolition"
};

//BB: this must be in the same order as shareddefs
static const char *pCovenStatusEffects[COVEN_STATUS_COUNT] =
{
	"cappoint",
	"levelup",
	"haste",
	"battleyell",
	"statboost",
	"berserk",
	"masochist",
	"gutcheck",
	"holywater",
	"bloodlust",
	"slow",
	"stun",
	"phase",
	"has_cts",
	"dodge",
	"weakness",
	"has_gas",
	"innerlight"
};

//BB: this must be in the same order as shareddefs
static const char *pBuildingTypes[BUILDING_TYPE_COUNT] =
{
	"None",
	"ammocrate",
	"turret",
	"supplydepot",
	"purchase_item",
	"purchase_stimpack",
	"purchase_medkit",
	"purchase_pills",
	"purchase_grenade",
	"purchase_stungrenade",
	"purchase_holywater",
	"purchase_slam"
};

//BB: this must be in the same order as shareddefs
static const char *pItemTypes[COVEN_ITEM_MAXCOUNT] =
{
	"stimpack",
	"medkit",
	"pills",
	"fraggrenade",
	"stungrenade",
	"holywater",
	"slam",
	"gasoline"
};

static CUtlMap<unsigned int, CovenClassInfo_t *> m_CovenClassInfoDatabase;
static CUtlVector<CovenAbilityInfo_t *> m_CovenAbilityInfoDatabase;
static CUtlVector<CovenStatusEffectInfo_t *> m_CovenStatusEffectInfoDatabase;
static CUtlVector<CovenBuildingInfo_t *> m_CovenBuildingInfoDatabase;
static CUtlVector<CovenItemInfo_t *> m_CovenItemInfoDatabase;
static CUtlVector<CovenSupplyDepotInfo_t *> m_CovenSupplyDepotInfoDatabase;

static CovenClassInfo_t gNullCovenClassInfo;
static CovenAbilityInfo_t gNullCovenAbilityInfo;
static CovenStatusEffectInfo_t gNullCovenStatusEffectInfo;
static CovenBuildingInfo_t gNullCovenBuildingInfo;
static CovenItemInfo_t gNullCovenItemInfo;
static CovenSupplyDepotInfo_t gNullSupplyDepotInfo;

CovenClassInfo_t::CovenClassInfo_t()
{
	bParsedScript = false;
	bLoadedHudElements = false;

	szName[0] = 0;
	szPrintName[0] = 0;

	iClassNumber = 0;
	szModelName[0] = 0;
	Q_memset(iAbilities, 0, sizeof(iAbilities));
	flConstitution = 0.0f;
	flStrength = 0.0f;
	flIntellect = 0.0f;
	flBaseSpeed = 0.0f;
	flAddConstitution = 0.0f;
	flAddStrength = 0.0f;
	flAddIntellect = 0.0f;
	iFlags = 0;
}

CovenAbilityInfo_t::CovenAbilityInfo_t()
{
	bParsedScript = false;
	bLoadedHudElements = false;

	szName[0] = 0;
	szPrintName[0] = 0;
	szDescription[0] = 0;

	flDuration = 0.0f;
	flCooldown = 0.0f;
	iMagnitude = 0;
	flRange = 0.0f;
	flCost = 0.0f;
	flDrain = 0.0f;
	bPassive = false;
	iFlags = 0;
	Q_memset(aSounds, 0, sizeof(aSounds));

	iSpriteCount = 0;
	abilityIconActive = 0;
}

CovenStatusEffectInfo_t::CovenStatusEffectInfo_t()
{
	bParsedScript = false;
	bLoadedHudElements = false;

	szName[0] = 0;
	szPrintName[0] = 0;
	szAltPrintName[0] = 0;

	bIsDebuff = false;
	bAltIsDebuff = false;
	iShowMagnitude = SHOW_NEVER;
	iShowTimer = SHOW_NEVER;
	iFlags = 0;

	iSpriteCount = 0;
	statusIcon = 0;
	altStatusIcon = 0;
}

CovenBuildingInfo_t::CovenBuildingInfo_t()
{
	bParsedScript = false;
	bLoadedHudElements = false;

	szName[0] = 0;
	szPrintName[0] = 0;
	szModelName[0] = 0;

	iFlags = 0;
	iMaxLevel = 0;
	iControllerStrengths.AddToTail(25);
	iMaxAngVel.AddToTail(8100); //90 x 90
	iHealths.AddToTail(150);
	iXPs.AddToTail(200);
	flScaleForce.AddToTail(0.5f);
}

CovenItemInfo_t::CovenItemInfo_t()
{
	bParsedScript = false;
	bLoadedHudElements = false;

	szName[0] = 0;
	szPrintName[0] = 0;
	szModelName[0] = 0;
	szIconName[0] = 0;

	iFlags = 0;
	iMagnitude = 0;
	flDuration = 0.0f;
	iCost = -1;
	flUseTime = 0.0f;
	iCarry = 0;
	flMaximum = 0.0f;
	Q_memset(aSounds, 0, sizeof(aSounds));

	hudIcon = 0;
	hudIconOff = 0;
}

CovenSupplyDepotInfo_t::CovenSupplyDepotInfo_t()
{
	bParsedScript = false;
	bLoadedHudElements = false;

	szName[0] = 0;
	szModelName[0] = 0;

	vOffsetPosition.Init();
	qOffsetAngle.Init();

	iFlags = 0;
}

void CovenClassInfo_t::Parse(KeyValues *pKeyValuesData)
{
	// Okay, we tried at least once to look this up...
	bParsedScript = true;

	Q_strncpy(szPrintName, pKeyValuesData->GetString("printname", COVEN_PRINTNAME_MISSING), MAX_COVEN_STRING);
	Q_strncpy(szDescription, pKeyValuesData->GetString("description"), MAX_COVEN_STRING);
	Q_strncpy(szModelName, pKeyValuesData->GetString("modelname"), MAX_COVEN_STRING);

	flConstitution = pKeyValuesData->GetFloat("base_constitution", 25);
	flStrength = pKeyValuesData->GetFloat("base_strength", 15);
	flIntellect = pKeyValuesData->GetFloat("base_intellect", 10);
	flBaseSpeed = pKeyValuesData->GetFloat("base_speed", 210.0f);

	KeyValues *pLevelUpData = pKeyValuesData->FindKey("LevelData");
	if (pLevelUpData)
	{
		flAddConstitution = pLevelUpData->GetFloat("constitution");
		flAddStrength = pLevelUpData->GetFloat("strength");
		flAddIntellect = pLevelUpData->GetFloat("intellect");
	}

	// LAME old way to specify item flags.
	// Weapon scripts should use the flag names.
	iFlags = pKeyValuesData->GetInt("flags", 0);

	/*for (int i = 0; i < ARRAYSIZE(g_ItemFlags); i++)
	{
		int iVal = pKeyValuesData->GetInt(g_ItemFlags[i].m_pFlagName, -1);
		if (iVal == 0)
		{
			iFlags &= ~g_ItemFlags[i].m_iFlagValue;
		}
		else if (iVal == 1)
		{
			iFlags |= g_ItemFlags[i].m_iFlagValue;
		}
	}*/
	KeyValues *pWeaponData = pKeyValuesData->FindKey("WeaponData");
	if (pWeaponData)
	{
		for (KeyValues *sub = pWeaponData->GetFirstSubKey(); sub != NULL; sub = sub->GetNextKey())
		{
			if (!Q_stricmp(sub->GetName(), "weapon"))
			{
				//char weapon[MAX_COVEN_STRING];
				szWeapons.AddToTail();
				szWeapons[szWeapons.Count() - 1] = new char[80];
				Q_strncpy(szWeapons[szWeapons.Count() - 1], sub->GetString(), MAX_COVEN_STRING);
			}
		}
	}
	KeyValues *pAmmoData = pKeyValuesData->FindKey("AmmoData");
	if (pAmmoData)
	{
		for (KeyValues *sub = pAmmoData->GetFirstSubKey(); sub != NULL; sub = sub->GetNextKey())
		{
			CovenAmmo_t *ammo = new CovenAmmo_t();
			Q_strncpy(ammo->szAmmoName, sub->GetName(), MAX_COVEN_STRING);
			ammo->iAmmoCount = sub->GetInt();
			tAmmo.AddToTail(ammo);
		}
	}
	for (int i = 0; i < COVEN_MAX_ABILITIES; i++)
	{
		char temp[MAX_COVEN_STRING];
		Q_snprintf(temp, sizeof(temp), "ability%d", i + 1);
		ParseAbility(pKeyValuesData, temp, iAbilities[i]);
	}
}

void CovenClassInfo_t::ParseAbility(KeyValues *kv, const char *szName, CovenAbility_t &value)
{
	const char *ability = kv->GetString(szName, "None");
	for (int i = 0; i < COVEN_ABILITY_COUNT; i++)
	{
		if (!Q_stricmp(pCovenAbilities[i], ability))
		{
			value = (CovenAbility_t)i;
			break;
		}
	}
}

float CovenAbilityInfo_t::GetDataVariable(int iNum)
{
	if (iNum < flDataVariables.Count())
		return flDataVariables[iNum];
	return 0.0f;
}

void CovenAbilityInfo_t::Parse(KeyValues *pKeyValuesData)
{
	// Okay, we tried at least once to look this up...
	bParsedScript = true;

	// Classname
	Q_strncpy(szPrintName, pKeyValuesData->GetString("printname", COVEN_PRINTNAME_MISSING), MAX_COVEN_STRING);
	const char *description = pKeyValuesData->GetString("description");
	if (description && description[0])
	{
		Q_strncpy(szDescription, description, MAX_COVEN_STRING);
	}
	flCooldown = pKeyValuesData->GetFloat("cooldown", 3.0f);
	iMagnitude = pKeyValuesData->GetInt("magnitude", 1);
	flDuration = pKeyValuesData->GetFloat("duration", 5.0f);
	flRange = pKeyValuesData->GetFloat("range", 300.0f);
	flRange *= flRange;
	flCost = pKeyValuesData->GetFloat("cost", 10.0f);
	flDrain = pKeyValuesData->GetFloat("drain", 0.0f);
	bPassive = (pKeyValuesData->GetInt("passive", 0) != 0) ? true : false;
	KeyValues *pVariables = pKeyValuesData->FindKey("variables");
	if (pVariables)
	{
		for (KeyValues *sub = pVariables->GetFirstSubKey(); sub != NULL; sub = sub->GetNextKey())
		{
			flDataVariables.AddToTail(sub->GetFloat());
		}
	}
	// LAME old way to specify item flags.
	// Weapon scripts should use the flag names.
	iFlags = pKeyValuesData->GetInt("flags", 0);

	/*for (int i = 0; i < ARRAYSIZE(g_ItemFlags); i++)
	{
		int iVal = pKeyValuesData->GetInt(g_ItemFlags[i].m_pFlagName, -1);
		if (iVal == 0)
		{
			iFlags &= ~g_ItemFlags[i].m_iFlagValue;
		}
		else if (iVal == 1)
		{
			iFlags |= g_ItemFlags[i].m_iFlagValue;
		}
	}*/

	// Now read the sounds
	Q_memset(aSounds, 0, sizeof(aSounds));
	KeyValues *pSoundData = pKeyValuesData->FindKey("SoundData");
	if (pSoundData)
	{
		for (int i = COVEN_SND_START; i < NUM_COVEN_SOUND_TYPES; i++)
		{
			const char *soundname = pSoundData->GetString(pCovenSoundCategories[i]);
			if (soundname && soundname[0])
			{
				Q_strncpy(aSounds[i], soundname, MAX_COVEN_STRING);
				CBaseEntity::PrecacheScriptSound(aSounds[i]);
			}
		}
	}
}

float CovenStatusEffectInfo_t::GetDataVariable(int iNum)
{
	if (iNum < flDataVariables.Count())
		return flDataVariables[iNum];
	return 0.0f;
}

void CovenStatusEffectInfo_t::Parse(KeyValues *pKeyValuesData)
{
	// Okay, we tried at least once to look this up...
	bParsedScript = true;

	Q_strncpy(szPrintName, pKeyValuesData->GetString("printname", COVEN_PRINTNAME_MISSING), MAX_COVEN_STRING);
	Q_strncpy(szAltPrintName, pKeyValuesData->GetString("altprintname", COVEN_PRINTNAME_MISSING), MAX_COVEN_STRING);
	bIsDebuff = (pKeyValuesData->GetInt("debuff", 0) != 0) ? true : false;
	bAltIsDebuff = (pKeyValuesData->GetInt("altdebuff", 0) != 0) ? true : false;
	iShowMagnitude = (StatusEffectShow_t)pKeyValuesData->GetInt("showmagnitude", 0);
	iShowTimer = (StatusEffectShow_t)pKeyValuesData->GetInt("showtimer", 0);
	KeyValues *pVariables = pKeyValuesData->FindKey("variables");
	if (pVariables)
	{
		for (KeyValues *sub = pVariables->GetFirstSubKey(); sub != NULL; sub = sub->GetNextKey())
		{
			flDataVariables.AddToTail(sub->GetFloat());
		}
	}

	// LAME old way to specify item flags.
	// Weapon scripts should use the flag names.
	iFlags = pKeyValuesData->GetInt("flags", 0);

	/*for (int i = 0; i < ARRAYSIZE(g_ItemFlags); i++)
	{
		int iVal = pKeyValuesData->GetInt(g_ItemFlags[i].m_pFlagName, -1);
		if (iVal == 0)
		{
			iFlags &= ~g_ItemFlags[i].m_iFlagValue;
		}
		else if (iVal == 1)
		{
			iFlags |= g_ItemFlags[i].m_iFlagValue;
		}
	}*/
}

void CovenBuildingInfo_t::Parse(KeyValues *pKeyValuesData)
{
	// Okay, we tried at least once to look this up...
	bParsedScript = true;

	Q_strncpy(szPrintName, pKeyValuesData->GetString("printname", COVEN_PRINTNAME_MISSING), MAX_COVEN_STRING);
	iMaxLevel = pKeyValuesData->GetInt("maxlevel");
	Q_strncpy(szModelName, pKeyValuesData->GetString("modelname"), MAX_COVEN_STRING);
	KeyValues *pHealthInfo = pKeyValuesData->FindKey("health");
	if (pHealthInfo)
	{
		iHealths.Purge();
		for (KeyValues *sub = pHealthInfo->GetFirstSubKey(); sub != NULL; sub = sub->GetNextKey())
		{
			iHealths.AddToTail(sub->GetInt());
		}
	}
	KeyValues *pXPInfo = pKeyValuesData->FindKey("xp");
	if (pXPInfo)
	{
		iXPs.Purge();
		for (KeyValues *sub = pXPInfo->GetFirstSubKey(); sub != NULL; sub = sub->GetNextKey())
		{
			iXPs.AddToTail(sub->GetInt());
		}
	}
	KeyValues *pAmmo1Info = pKeyValuesData->FindKey("ammo1");
	if (pAmmo1Info)
	{
		for (KeyValues *sub = pAmmo1Info->GetFirstSubKey(); sub != NULL; sub = sub->GetNextKey())
		{
			iAmmo1.AddToTail(sub->GetInt());
		}
	}
	KeyValues *pAmmo2Info = pKeyValuesData->FindKey("ammo2");
	if (pAmmo2Info)
	{
		for (KeyValues *sub = pAmmo2Info->GetFirstSubKey(); sub != NULL; sub = sub->GetNextKey())
		{
			iAmmo2.AddToTail(sub->GetInt());
		}
	}
	KeyValues *pAmmo3Info = pKeyValuesData->FindKey("ammo3");
	if (pAmmo3Info)
	{
		for (KeyValues *sub = pAmmo3Info->GetFirstSubKey(); sub != NULL; sub = sub->GetNextKey())
		{
			iAmmo3.AddToTail(sub->GetInt());
		}
	}
	KeyValues *pControllerStrengthInfo = pKeyValuesData->FindKey("controller");
	if (pControllerStrengthInfo)
	{
		iControllerStrengths.Purge();
		for (KeyValues *sub = pControllerStrengthInfo->GetFirstSubKey(); sub != NULL; sub = sub->GetNextKey())
		{
			iControllerStrengths.AddToTail(sub->GetInt());
		}
	}
	KeyValues *pMaxAngVels = pKeyValuesData->FindKey("maxangvel");
	if (pMaxAngVels)
	{
		iMaxAngVel.Purge();
		for (KeyValues *sub = pMaxAngVels->GetFirstSubKey(); sub != NULL; sub = sub->GetNextKey())
		{
			iMaxAngVel.AddToTail(sub->GetInt());
		}
	}
	KeyValues *pScaleForce = pKeyValuesData->FindKey("scaleforce");
	if (pScaleForce)
	{
		flScaleForce.Purge();
		for (KeyValues *sub = pScaleForce->GetFirstSubKey(); sub != NULL; sub = sub->GetNextKey())
		{
			flScaleForce.AddToTail(sub->GetFloat());
		}
	}
}

void CovenItemInfo_t::Parse(KeyValues *pKeyValuesData)
{
	// Okay, we tried at least once to look this up...
	bParsedScript = true;

	Q_strncpy(szPrintName, pKeyValuesData->GetString("printname", COVEN_PRINTNAME_MISSING), MAX_COVEN_STRING);
	Q_strncpy(szModelName, pKeyValuesData->GetString("modelname"), MAX_COVEN_STRING);
	Q_strncpy(szIconName, pKeyValuesData->GetString("classname", COVEN_PRINTNAME_MISSING), MAX_COVEN_STRING);
	iMagnitude = pKeyValuesData->GetInt("magnitude", 1);
	iCost = pKeyValuesData->GetInt("cost", 10);
	flDuration = pKeyValuesData->GetFloat("duration", 5.0f);
	flUseTime = pKeyValuesData->GetFloat("usetime");
	flMaximum = pKeyValuesData->GetFloat("maximum");
	iCarry = pKeyValuesData->GetInt("maxcarry", 1);

	// LAME old way to specify item flags.
	// Weapon scripts should use the flag names.
	iFlags = pKeyValuesData->GetInt("flags", 0);

	/*for (int i = 0; i < ARRAYSIZE(g_ItemFlags); i++)
	{
	int iVal = pKeyValuesData->GetInt(g_ItemFlags[i].m_pFlagName, -1);
	if (iVal == 0)
	{
	iFlags &= ~g_ItemFlags[i].m_iFlagValue;
	}
	else if (iVal == 1)
	{
	iFlags |= g_ItemFlags[i].m_iFlagValue;
	}
	}*/

	// Now read the sounds
	Q_memset(aSounds, 0, sizeof(aSounds));
	KeyValues *pSoundData = pKeyValuesData->FindKey("SoundData");
	if (pSoundData)
	{
		for (int i = COVEN_SND_START; i < NUM_COVEN_SOUND_TYPES; i++)
		{
			const char *soundname = pSoundData->GetString(pCovenSoundCategories[i]);
			if (soundname && soundname[0])
			{
				Q_strncpy(aSounds[i], soundname, MAX_COVEN_STRING);
				CBaseEntity::PrecacheScriptSound(aSounds[i]);
			}
		}
	}
}

void CovenSupplyDepotInfo_t::Parse(KeyValues *pKeyValuesData)
{
	// Okay, we tried at least once to look this up...
	bParsedScript = true;

	Q_strncpy(szModelName, pKeyValuesData->GetString("modelname"), MAX_COVEN_STRING);

	float locs[3];
	UTIL_StringToVector(locs, pKeyValuesData->GetString("offset_position", "0 0 0"));
	vOffsetPosition.Init(locs[0], locs[1], locs[2]);
	UTIL_StringToVector(locs, pKeyValuesData->GetString("offset_angles", "0 0 0"));
	qOffsetAngle.Init(locs[0], locs[1], locs[2]);
	KeyValues *pItems = pKeyValuesData->FindKey("items");
	if (pItems)
	{
		for (KeyValues *sub = pItems->GetFirstSubKey(); sub != NULL; sub = sub->GetNextKey())
		{
			CovenItem_t *item = new CovenItem_t();
			Q_snprintf(item->szItemName, sizeof(item->szItemName), sub->GetName());
			UTIL_StringToVector(locs, sub->GetString("rel_location", "0 0 0"));
			item->vRelPosition.Init(locs[0], locs[1], locs[2]);
			UTIL_StringToVector(locs, sub->GetString("rel_angles", "0 0 0"));
			item->qRelAngle.Init(locs[0], locs[1], locs[2]);
			ItemInfo.AddToTail(item);
		}
	}

	// LAME old way to specify item flags.
	// Weapon scripts should use the flag names.
	iFlags = pKeyValuesData->GetInt("flags", 0);
}

void PrecacheClasses(IFileSystem *filesystem)
{
	SetDefLessFunc(m_CovenClassInfoDatabase);
	m_CovenClassInfoDatabase.Insert(0, &gNullCovenClassInfo);
	unsigned int indexNumberOffset = 0;
	for (int i = 2; i < COVEN_MAX_TEAMS; i++)
	{
		for (int j = 1; j <= iCovenTeamCounts[i]; j++)
		{
			unsigned int indexNumber = j + indexNumberOffset;
			unsigned int fullNumber = j | (1 << (i + 2));
			char tempfile[MAX_PATH];
			Q_snprintf(tempfile, sizeof(tempfile), "scripts/classes/%s.txt", pCovenClasses[indexNumber]);
			KeyValues *covenclass = new KeyValues("classdata");
			if (covenclass->LoadFromFile(filesystem, tempfile, "GAME"))
			{
				CovenClassInfo_t *info = new CovenClassInfo_t();
				Q_snprintf(info->szName, sizeof(info->szName), pCovenClasses[indexNumber]);
				info->iClassNumber = fullNumber;
				info->Parse(covenclass);
				m_CovenClassInfoDatabase.Insert(fullNumber, info);
			}
			else
			{
				Warning("Error loading class file: %s!\n", tempfile);
				m_CovenClassInfoDatabase.Insert(fullNumber, &gNullCovenClassInfo);
			}
		}
		indexNumberOffset += iCovenTeamCounts[i];
	}
}

void PrecacheAbilities(IFileSystem *filesystem)
{
	m_CovenAbilityInfoDatabase.AddToTail(&gNullCovenAbilityInfo);
	for (int i = 1; i < COVEN_ABILITY_COUNT; i++)
	{
		char tempfile[MAX_PATH];
		Q_snprintf(tempfile, sizeof(tempfile), "scripts/abilities/%s.txt", pCovenAbilities[i]);
		KeyValues *ability = new KeyValues("abilitydata");
		if (ability->LoadFromFile(filesystem, tempfile, "GAME"))
		{
			CovenAbilityInfo_t *info = new CovenAbilityInfo_t();
			Q_snprintf(info->szName, sizeof(info->szName), pCovenAbilities[i]);
			info->Parse(ability);
			m_CovenAbilityInfoDatabase.AddToTail(info);
#ifdef CLIENT_DLL
			LoadAbilitySprites(info);
#endif
		}
		else
		{
			Warning("Error loading ability file: %s!\n", tempfile);
			m_CovenAbilityInfoDatabase.AddToTail(&gNullCovenAbilityInfo);
		}
	}
}

void PrecacheStatusEffects(IFileSystem *filesystem)
{
	for (int i = 0; i < COVEN_STATUS_COUNT; i++)
	{
		char tempfile[MAX_PATH];
		Q_snprintf(tempfile, sizeof(tempfile), "scripts/statuseffects/%s.txt", pCovenStatusEffects[i]);
		KeyValues *effect = new KeyValues("statuseffectdata");
		if (effect->LoadFromFile(filesystem, tempfile, "GAME"))
		{
			CovenStatusEffectInfo_t *info = new CovenStatusEffectInfo_t();
			Q_snprintf(info->szName, sizeof(info->szName), pCovenStatusEffects[i]);
			info->Parse(effect);
			m_CovenStatusEffectInfoDatabase.AddToTail(info);
#ifdef CLIENT_DLL
			LoadStatusEffectSprites(info);
#endif
		}
		else
		{
			Warning("Error loading status effect file: %s!\n", tempfile);
			m_CovenStatusEffectInfoDatabase.AddToTail(&gNullCovenStatusEffectInfo);
		}
	}
}

void PrecacheBuildings(IFileSystem *filesystem)
{
	m_CovenBuildingInfoDatabase.AddToTail(&gNullCovenBuildingInfo);
	for (int i = 1; i < BUILDING_TYPE_COUNT; i++)
	{
		char tempfile[MAX_PATH];
		Q_snprintf(tempfile, sizeof(tempfile), "scripts/buildings/%s.txt", pBuildingTypes[i]);
		KeyValues *building = new KeyValues("buildingdata");
		if (building->LoadFromFile(filesystem, tempfile, "GAME"))
		{
			CovenBuildingInfo_t *info = new CovenBuildingInfo_t();
			Q_snprintf(info->szName, sizeof(info->szName), pBuildingTypes[i]);
			info->Parse(building);
			m_CovenBuildingInfoDatabase.AddToTail(info);
		}
		else
		{
			Warning("Error loading building file: %s!\n", tempfile);
			m_CovenBuildingInfoDatabase.AddToTail(&gNullCovenBuildingInfo);
		}
	}
}

void PrecacheItems(IFileSystem *filesystem)
{
	for (int i = 0; i < COVEN_ITEM_MAXCOUNT; i++)
	{
		char tempfile[MAX_PATH];
		Q_snprintf(tempfile, sizeof(tempfile), "scripts/items/%s.txt", pItemTypes[i]);
		KeyValues *item = new KeyValues("itemdata");
		if (item->LoadFromFile(filesystem, tempfile, "GAME"))
		{
			CovenItemInfo_t *info = new CovenItemInfo_t();
			Q_snprintf(info->szName, sizeof(info->szName), pItemTypes[i]);
			info->Parse(item);
			m_CovenItemInfoDatabase.AddToTail(info);
#ifdef CLIENT_DLL
			LoadItemSprites(info);
#endif
		}
		else
		{
			Warning("Error loading item file: %s!\n", tempfile);
			m_CovenItemInfoDatabase.AddToTail(&gNullCovenItemInfo);
		}
	}
}

void PrecacheSupplyDepots(IFileSystem *filesystem)
{
	m_CovenSupplyDepotInfoDatabase.AddToTail(&gNullSupplyDepotInfo);
	char tempfile[MAX_PATH];
	Q_snprintf(tempfile, sizeof(tempfile), "scripts/buildings/supplydepot_manifest.txt");
	KeyValues *manifest = new KeyValues("supplydepot_manifest");
	if (manifest->LoadFromFile(filesystem, tempfile, "GAME"))
	{
		for (KeyValues *sub = manifest->GetFirstSubKey(); sub != NULL; sub = sub->GetNextKey())
		{
			Q_snprintf(tempfile, sizeof(tempfile), "%s", sub->GetString());
			KeyValues *pSupplyDepot = new KeyValues("supplydepot");
			if (pSupplyDepot->LoadFromFile(filesystem, tempfile, "GAME"))
			{
				CovenSupplyDepotInfo_t *info = new CovenSupplyDepotInfo_t();
				info->Parse(pSupplyDepot);
				m_CovenSupplyDepotInfoDatabase.AddToTail(info);
			}
			else
			{
				Warning("Error loading supply depot file: %s!\n", tempfile);
			}
		}
	}
	else
	{
		Warning("Error loading supply depot manifest file: %s!\n", tempfile);
	}
}

#ifdef CLIENT_DLL
void LoadAbilitySprites(CovenAbilityInfo_t *info)
{
	if (info->bLoadedHudElements)
		return;

	info->bLoadedHudElements = true;

	info->abilityIconActive = NULL;

	char sz[128];
	Q_snprintf(sz, sizeof(sz), "scripts/abilities/%s", info->szName);

	CUtlDict< CHudTexture *, int > tempList;

	LoadHudTextures(tempList, sz, g_pGameRules->GetEncryptionKey());

	if (!tempList.Count())
	{
		Warning("Unable to load texture data for: %s\n", sz);
		return;
	}

	CHudTexture *p;
	p = FindHudTextureInDict(tempList, "abilitytexture");
	if (p)
	{
		info->abilityIconActive = gHUD.AddUnsearchableHudIconToList(*p);
	}

	FreeHudTextureList(tempList);
}

void LoadStatusEffectSprites(CovenStatusEffectInfo_t *info)
{
	if (info->bLoadedHudElements)
		return;

	info->bLoadedHudElements = true;

	info->statusIcon = NULL;
	info->altStatusIcon = NULL;

	char sz[128];
	Q_snprintf(sz, sizeof(sz), "scripts/statuseffects/%s", info->szName);

	CUtlDict< CHudTexture *, int > tempList;

	LoadHudTextures(tempList, sz, g_pGameRules->GetEncryptionKey());

	if (!tempList.Count())
	{
		Warning("Unable to load texture data for: %s\n", sz);
		return;
	}

	CHudTexture *p;
	p = FindHudTextureInDict(tempList, "statuseffecttexture");
	if (p)
	{
		info->statusIcon = gHUD.AddUnsearchableHudIconToList(*p);
	}
	p = FindHudTextureInDict(tempList, "altstatuseffecttexture");
	if (p)
	{
		info->altStatusIcon = gHUD.AddUnsearchableHudIconToList(*p);
	}

	FreeHudTextureList(tempList);
}

void LoadItemSprites(CovenItemInfo_t *info)
{
	if (info->bLoadedHudElements)
		return;

	info->bLoadedHudElements = true;

	info->hudIcon = NULL;
	info->hudIconOff = NULL;

	char sz[128];
	Q_snprintf(sz, sizeof(sz), "scripts/items/%s", info->szName);

	CUtlDict< CHudTexture *, int > tempList;

	LoadHudTextures(tempList, sz, g_pGameRules->GetEncryptionKey());

	if (!tempList.Count())
	{
		Warning("Unable to load texture data for: %s\n", sz);
		return;
	}

	CHudTexture *p;
	p = FindHudTextureInDict(tempList, "hudicon");
	if (p)
	{
		info->hudIcon = gHUD.AddUnsearchableHudIconToList(*p);
	}
	p = FindHudTextureInDict(tempList, "hudiconoff");
	if (p)
	{
		info->hudIconOff = gHUD.AddUnsearchableHudIconToList(*p);
	}

	FreeHudTextureList(tempList);
}
#endif

CovenClassInfo_t *GetCovenClassData(CovenClassID_t iClass)
{
	unsigned short idx = m_CovenClassInfoDatabase.Find(iClass);
	if (idx == m_CovenClassInfoDatabase.InvalidIndex())
		return &gNullCovenClassInfo;
	return m_CovenClassInfoDatabase[idx];
}

CovenClassInfo_t *GetCovenClassData(int iPartialClassNum, int iTeam)
{
	CovenClassID_t iClassNum = (CovenClassID_t)(iPartialClassNum | (1 << (iTeam + 2)));
	return GetCovenClassData(iClassNum);
}

CovenAbilityInfo_t *GetCovenAbilityData(CovenAbility_t iAbility)
{
	return m_CovenAbilityInfoDatabase[iAbility];
}

CovenStatusEffectInfo_t *GetCovenStatusEffectData(CovenStatus_t iStatus)
{
	return m_CovenStatusEffectInfoDatabase[iStatus];
}

CovenBuildingInfo_t *GetCovenBuildingData(BuildingType_t iBuildingType)
{
	return m_CovenBuildingInfoDatabase[iBuildingType];
}

CovenItemInfo_t *GetCovenItemData(CovenItemID_t iItemType)
{
	return m_CovenItemInfoDatabase[iItemType];
}

CovenSupplyDepotInfo_t *GetCovenSupplyDepotData(int iDepotType)
{
	return m_CovenSupplyDepotInfoDatabase[iDepotType];
}

int CovenSupplyDepotDataLength(void)
{
	return m_CovenSupplyDepotInfoDatabase.Count();
}