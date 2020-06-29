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
const char *pCovenSoundCategories[NUM_COVEN_ABILITY_SOUND_TYPES] =
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
	"innerlight"
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
	"has_gas"
};

static CUtlMap<unsigned int, CovenClassInfo_t *> m_CovenClassInfoDatabase;
static CUtlVector<CovenAbilityInfo_t *> m_CovenAbilityInfoDatabase;
static CUtlVector<CovenStatusEffectInfo_t *> m_CovenStatusEffectInfoDatabase;

static CovenClassInfo_t gNullCovenClassInfo;
static CovenAbilityInfo_t gNullCovenAbilityInfo;
static CovenStatusEffectInfo_t gNullCovenStatusEffectInfo;


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
	memset(aSounds, 0, sizeof(aSounds));

	iSpriteCount = 0;
	abilityIconActive = 0;
}

CovenStatusEffectInfo_t::CovenStatusEffectInfo_t()
{
	bParsedScript = false;
	bLoadedHudElements = false;

	szName[0] = 0;
	szPrintName[0] = 0;

	bShowMagnitude = false;
	bShowTimer = false;
	iFlags = 0;

	iSpriteCount = 0;
	statusIcon = 0;
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
	flCost = pKeyValuesData->GetFloat("cost", 10.0f);
	flDrain = pKeyValuesData->GetFloat("drain", 0.0f);
	bPassive = (pKeyValuesData->GetInt("passive", 0) != 0) ? true : false;

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
	memset(aSounds, 0, sizeof(aSounds));
	KeyValues *pSoundData = pKeyValuesData->FindKey("SoundData");
	if (pSoundData)
	{
		for (int i = ABILITY_SND_START; i < NUM_COVEN_ABILITY_SOUND_TYPES; i++)
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

void CovenStatusEffectInfo_t::Parse(KeyValues *pKeyValuesData)
{
	// Okay, we tried at least once to look this up...
	bParsedScript = true;

	Q_strncpy(szPrintName, pKeyValuesData->GetString("printname", COVEN_PRINTNAME_MISSING), MAX_COVEN_STRING);
	bShowMagnitude = (pKeyValuesData->GetInt("showmagnitude", 0) != 0) ? true : false;
	bShowTimer = (pKeyValuesData->GetInt("showtimer", 0) != 0) ? true : false;

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
		KeyValues *ability = new KeyValues("statuseffectdata");
		if (ability->LoadFromFile(filesystem, tempfile, "GAME"))
		{
			CovenStatusEffectInfo_t *info = new CovenStatusEffectInfo_t();
			Q_snprintf(info->szName, sizeof(info->szName), pCovenStatusEffects[i]);
			info->Parse(ability);
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