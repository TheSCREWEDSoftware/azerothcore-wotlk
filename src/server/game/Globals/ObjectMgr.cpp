/*
 * This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "ObjectMgr.h"
#include "AchievementMgr.h"
#include "ArenaTeamMgr.h"
#include "CharacterCache.h"
#include "Chat.h"
#include "Common.h"
#include "Config.h"
#include "Containers.h"
#include "CreatureAIFactory.h"
#include "DBCStructure.h"
#include "DatabaseEnv.h"
#include "DisableMgr.h"
#include "GameEventMgr.h"
#include "GameObjectAIFactory.h"
#include "GameTime.h"
#include "GossipDef.h"
#include "GroupMgr.h"
#include "GuildMgr.h"
#include "LFGMgr.h"
#include "Log.h"
#include "MapMgr.h"
#include "Pet.h"
#include "PoolMgr.h"
#include "RaceMgr.h"
#include "ReputationMgr.h"
#include "ScriptMgr.h"
#include "Spell.h"
#include "SpellMgr.h"
#include "SpellScript.h"
#include "SpellScriptLoader.h"
#include "StringConvert.h"
#include "Tokenize.h"
#include "Transport.h"
#include "Unit.h"
#include "Util.h"
#include "Vehicle.h"
#include "World.h"
#include <boost/algorithm/string.hpp>
#include <numeric>

#include "ItemEnchantmentMgr.h"

ScriptMapMap sSpellScripts;
ScriptMapMap sEventScripts;
ScriptMapMap sWaypointScripts;

std::string GetScriptsTableNameByType(ScriptsType type)
{
    std::string res = "";
    switch (type)
    {
        case SCRIPTS_SPELL:
            res = "spell_scripts";
            break;
        case SCRIPTS_EVENT:
            res = "event_scripts";
            break;
        case SCRIPTS_WAYPOINT:
            res = "waypoint_scripts";
            break;
        default:
            break;
    }
    return res;
}

ScriptMapMap* GetScriptsMapByType(ScriptsType type)
{
    ScriptMapMap* res = nullptr;
    switch (type)
    {
        case SCRIPTS_SPELL:
            res = &sSpellScripts;
            break;
        case SCRIPTS_EVENT:
            res = &sEventScripts;
            break;
        case SCRIPTS_WAYPOINT:
            res = &sWaypointScripts;
            break;
        default:
            break;
    }
    return res;
}

std::string GetScriptCommandName(ScriptCommands command)
{
    std::string res = "";
    switch (command)
    {
        case SCRIPT_COMMAND_TALK:
            res = "SCRIPT_COMMAND_TALK";
            break;
        case SCRIPT_COMMAND_EMOTE:
            res = "SCRIPT_COMMAND_EMOTE";
            break;
        case SCRIPT_COMMAND_FIELD_SET:
            res = "SCRIPT_COMMAND_FIELD_SET";
            break;
        case SCRIPT_COMMAND_MOVE_TO:
            res = "SCRIPT_COMMAND_MOVE_TO";
            break;
        case SCRIPT_COMMAND_FLAG_SET:
            res = "SCRIPT_COMMAND_FLAG_SET";
            break;
        case SCRIPT_COMMAND_FLAG_REMOVE:
            res = "SCRIPT_COMMAND_FLAG_REMOVE";
            break;
        case SCRIPT_COMMAND_TELEPORT_TO:
            res = "SCRIPT_COMMAND_TELEPORT_TO";
            break;
        case SCRIPT_COMMAND_QUEST_EXPLORED:
            res = "SCRIPT_COMMAND_QUEST_EXPLORED";
            break;
        case SCRIPT_COMMAND_KILL_CREDIT:
            res = "SCRIPT_COMMAND_KILL_CREDIT";
            break;
        case SCRIPT_COMMAND_RESPAWN_GAMEOBJECT:
            res = "SCRIPT_COMMAND_RESPAWN_GAMEOBJECT";
            break;
        case SCRIPT_COMMAND_TEMP_SUMMON_CREATURE:
            res = "SCRIPT_COMMAND_TEMP_SUMMON_CREATURE";
            break;
        case SCRIPT_COMMAND_OPEN_DOOR:
            res = "SCRIPT_COMMAND_OPEN_DOOR";
            break;
        case SCRIPT_COMMAND_CLOSE_DOOR:
            res = "SCRIPT_COMMAND_CLOSE_DOOR";
            break;
        case SCRIPT_COMMAND_ACTIVATE_OBJECT:
            res = "SCRIPT_COMMAND_ACTIVATE_OBJECT";
            break;
        case SCRIPT_COMMAND_REMOVE_AURA:
            res = "SCRIPT_COMMAND_REMOVE_AURA";
            break;
        case SCRIPT_COMMAND_CAST_SPELL:
            res = "SCRIPT_COMMAND_CAST_SPELL";
            break;
        case SCRIPT_COMMAND_PLAY_SOUND:
            res = "SCRIPT_COMMAND_PLAY_SOUND";
            break;
        case SCRIPT_COMMAND_CREATE_ITEM:
            res = "SCRIPT_COMMAND_CREATE_ITEM";
            break;
        case SCRIPT_COMMAND_DESPAWN_SELF:
            res = "SCRIPT_COMMAND_DESPAWN_SELF";
            break;
        case SCRIPT_COMMAND_LOAD_PATH:
            res = "SCRIPT_COMMAND_LOAD_PATH";
            break;
        case SCRIPT_COMMAND_CALLSCRIPT_TO_UNIT:
            res = "SCRIPT_COMMAND_CALLSCRIPT_TO_UNIT";
            break;
        case SCRIPT_COMMAND_KILL:
            res = "SCRIPT_COMMAND_KILL";
            break;
        // AzerothCore only
        case SCRIPT_COMMAND_ORIENTATION:
            res = "SCRIPT_COMMAND_ORIENTATION";
            break;
        case SCRIPT_COMMAND_EQUIP:
            res = "SCRIPT_COMMAND_EQUIP";
            break;
        case SCRIPT_COMMAND_MODEL:
            res = "SCRIPT_COMMAND_MODEL";
            break;
        case SCRIPT_COMMAND_CLOSE_GOSSIP:
            res = "SCRIPT_COMMAND_CLOSE_GOSSIP";
            break;
        case SCRIPT_COMMAND_PLAYMOVIE:
            res = "SCRIPT_COMMAND_PLAYMOVIE";
            break;
        default:
            {
                char sz[32];
                snprintf(sz, sizeof(sz), "Unknown command: %d", command);
                res = sz;
                break;
            }
    }
    return res;
}

std::string ScriptInfo::GetDebugInfo() const
{
    char sz[256];
    snprintf(sz, sizeof(sz), "%s ('%s' script id: %u)", GetScriptCommandName(command).c_str(), GetScriptsTableNameByType(type).c_str(), id);
    return std::string(sz);
}

bool normalizePlayerName(std::string& name)
{
    if (name.empty())
        return false;

    if (name.find(" ") != std::string::npos)
        return false;

    std::wstring tmp;
    if (!Utf8toWStr(name, tmp))
        return false;

    wstrToLower(tmp);
    if (!tmp.empty())
        tmp[0] = wcharToUpper(tmp[0]);

    if (!WStrToUtf8(tmp, name))
        return false;

    return true;
}

LanguageDesc lang_description[LANGUAGES_COUNT] =
{
    { LANG_ADDON,           0, 0                       },
    { LANG_UNIVERSAL,       0, 0                       },
    { LANG_ORCISH,        669, SKILL_LANG_ORCISH       },
    { LANG_DARNASSIAN,    671, SKILL_LANG_DARNASSIAN   },
    { LANG_TAURAHE,       670, SKILL_LANG_TAURAHE      },
    { LANG_DWARVISH,      672, SKILL_LANG_DWARVEN      },
    { LANG_COMMON,        668, SKILL_LANG_COMMON       },
    { LANG_DEMONIC,       815, SKILL_LANG_DEMON_TONGUE },
    { LANG_TITAN,         816, SKILL_LANG_TITAN        },
    { LANG_THALASSIAN,    813, SKILL_LANG_THALASSIAN   },
    { LANG_DRACONIC,      814, SKILL_LANG_DRACONIC     },
    { LANG_KALIMAG,       817, SKILL_LANG_OLD_TONGUE   },
    { LANG_GNOMISH,      7340, SKILL_LANG_GNOMISH      },
    { LANG_TROLL,        7341, SKILL_LANG_TROLL        },
    { LANG_GUTTERSPEAK, 17737, SKILL_LANG_GUTTERSPEAK  },
    { LANG_DRAENEI,     29932, SKILL_LANG_DRAENEI      },
    { LANG_ZOMBIE,          0, 0                       },
    { LANG_GNOMISH_BINARY,  0, 0                       },
    { LANG_GOBLIN_BINARY,   0, 0                       }
};

LanguageDesc const* GetLanguageDescByID(uint32 lang)
{
    for (uint8 i = 0; i < LANGUAGES_COUNT; ++i)
    {
        if (uint32(lang_description[i].lang_id) == lang)
            return &lang_description[i];
    }

    return nullptr;
}

bool SpellClickInfo::IsFitToRequirements(Unit const* clicker, Unit const* clickee) const
{
    Player const* playerClicker = clicker->ToPlayer();
    if (!playerClicker)
        return true;

    Unit const* summoner = nullptr;
    // Check summoners for party
    if (clickee->IsSummon())
        summoner = clickee->ToTempSummon()->GetSummonerUnit();
    if (!summoner)
        summoner = clickee;

    // This only applies to players
    switch (userType)
    {
        case SPELL_CLICK_USER_FRIEND:
            if (!playerClicker->IsFriendlyTo(summoner))
                return false;
            break;
        case SPELL_CLICK_USER_RAID:
            if (!playerClicker->IsInRaidWith(summoner))
                return false;
            break;
        case SPELL_CLICK_USER_PARTY:
            if (!playerClicker->IsInPartyWith(summoner))
                return false;
            break;
        default:
            break;
    }

    return true;
}

ObjectMgr::ObjectMgr():
    _auctionId(1),
    _equipmentSetGuid(1),
    _mailId(1),
    _voidItemId(1),
    _hiPetNumber(1),
    _creatureSpawnId(1),
    _gameObjectSpawnId(1),
    DBCLocaleIndex(LOCALE_enUS)
{
    for (uint8 i = 0; i < MAX_CLASSES; ++i)
    {
        _playerClassInfo[i] = nullptr;
    }

    // Initialize default spawn groups
    _spawnGroupDataStore[0] = {0, "Default Group", SPAWNGROUP_MAP_UNSET, SpawnGroupFlags(SPAWNGROUP_FLAG_SYSTEM)};
    _spawnGroupDataStore[1] = {1, "Legacy Group", SPAWNGROUP_MAP_UNSET, SpawnGroupFlags(SPAWNGROUP_FLAG_SYSTEM | SPAWNGROUP_FLAG_COMPATIBILITY_MODE)};
}

ObjectMgr::~ObjectMgr()
{
    for (QuestMap::iterator i = _questTemplates.begin(); i != _questTemplates.end(); ++i)
        delete i->second;

    for (PetLevelInfoContainer::iterator i = _petInfoStore.begin(); i != _petInfoStore.end(); ++i)
        delete[] i->second;

    // free only if loaded
    for (int class_ = 0; class_ < MAX_CLASSES; ++class_)
    {
        if (_playerClassInfo[class_])
            delete[] _playerClassInfo[class_]->levelInfo;
        delete _playerClassInfo[class_];
    }

    for (int race = 0; race < sRaceMgr->GetMaxRaces(); ++race)
    {
        for (int class_ = 0; class_ < MAX_CLASSES; ++class_)
        {
            if (_playerInfo[race][class_])
                delete[] _playerInfo[race][class_]->levelInfo;
            delete _playerInfo[race][class_];
        }
    }

    for (CacheVendorItemContainer::iterator itr = _cacheVendorItemStore.begin(); itr != _cacheVendorItemStore.end(); ++itr)
        itr->second.Clear();

    for (DungeonEncounterContainer::iterator itr = _dungeonEncounterStore.begin(); itr != _dungeonEncounterStore.end(); ++itr)
        for (DungeonEncounterList::iterator encounterItr = itr->second.begin(); encounterItr != itr->second.end(); ++encounterItr)
            delete *encounterItr;

    for (DungeonProgressionRequirementsContainer::iterator itr = _accessRequirementStore.begin(); itr != _accessRequirementStore.end(); ++itr)
    {
        std::unordered_map<uint8, DungeonProgressionRequirements*> difficulties = itr->second;
        for (auto difficultiesItr = difficulties.begin(); difficultiesItr != difficulties.end(); ++difficultiesItr)
        {
            for (auto questItr = difficultiesItr->second->quests.begin(); questItr != difficultiesItr->second->quests.end(); ++questItr)
            {
                delete* questItr;
            }

            for (auto achievementItr = difficultiesItr->second->achievements.begin(); achievementItr != difficultiesItr->second->achievements.end(); ++achievementItr)
            {
                delete* achievementItr;
            }

            for (auto itemsItr = difficultiesItr->second->items.begin(); itemsItr != difficultiesItr->second->items.end(); ++itemsItr)
            {
                delete* itemsItr;
            }

            delete difficultiesItr->second;
        }
    }
}

ObjectMgr* ObjectMgr::instance()
{
    static ObjectMgr instance;
    return &instance;
}

void ObjectMgr::AddLocaleString(std::string&& s, LocaleConstant locale, std::vector<std::string>& data)
{
    if (!s.empty())
    {
        if (data.size() <= std::size_t(locale))
            data.resize(locale + 1);

        data[locale] = std::move(s);
    }
}

void ObjectMgr::LoadCreatureLocales()
{
    uint32 oldMSTime = getMSTime();

    _creatureLocaleStore.clear();                              // need for reload case

    //                                               0      1       2     3
    QueryResult result = WorldDatabase.Query("SELECT entry, locale, Name, Title FROM creature_template_locale");
    if (!result)
        return;

    do
    {
        Field* fields = result->Fetch();

        uint32 ID = fields[0].Get<uint32>();

        LocaleConstant locale = GetLocaleByName(fields[1].Get<std::string>());
        if (locale == LOCALE_enUS)
            continue;

        CreatureLocale& data = _creatureLocaleStore[ID];
        AddLocaleString(fields[2].Get<std::string>(), locale, data.Name);
        AddLocaleString(fields[3].Get<std::string>(), locale, data.Title);
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} Creature Locale Strings in {} ms", (unsigned long)_creatureLocaleStore.size(), GetMSTimeDiffToNow(oldMSTime));
}

void ObjectMgr::LoadGossipMenuItemsLocales()
{
    uint32 oldMSTime = getMSTime();

    _gossipMenuItemsLocaleStore.clear();                              // need for reload case

    //                                               0       1            2       3           4
    QueryResult result = WorldDatabase.Query("SELECT MenuID, OptionID, Locale, OptionText, BoxText FROM gossip_menu_option_locale");

    if (!result)
        return;

    do
    {
        Field* fields = result->Fetch();

        uint32 MenuID = fields[0].Get<uint32>();
        uint16 OptionID = fields[1].Get<uint16>();

        LocaleConstant locale = GetLocaleByName(fields[2].Get<std::string>());
        if (locale == LOCALE_enUS)
            continue;

        GossipMenuItemsLocale& data = _gossipMenuItemsLocaleStore[MAKE_PAIR32(MenuID, OptionID)];
        AddLocaleString(fields[3].Get<std::string>(), locale, data.OptionText);
        AddLocaleString(fields[4].Get<std::string>(), locale, data.BoxText);
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} Gossip Menu Option Locale Strings in {} ms", (uint32)_gossipMenuItemsLocaleStore.size(), GetMSTimeDiffToNow(oldMSTime));
}

void ObjectMgr::LoadPetNamesLocales()
{
    uint32 oldMSTime = getMSTime();

    //                                                  0     1      2    3
    QueryResult result = WorldDatabase.Query("SELECT Locale, Word, Entry, Half FROM pet_name_generation_locale");

    if (!result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 pet name locales parts. DB table `pet_name_generation_locale` is empty!");
        LOG_INFO("server.loading", " ");
        return;
    }

    uint32 count = 0;

    do
    {
        Field* fields = result->Fetch();
        LocaleConstant locale = GetLocaleByName(fields[0].Get<std::string>());
        std::string word = fields[1].Get<std::string>();

        uint32 entry = fields[2].Get<uint32>();
        bool half = fields[3].Get<bool>();
        std::pair<uint32, LocaleConstant> pairkey = std::make_pair(entry, locale);
        if (half)
        {
            _petHalfLocaleName1[pairkey].push_back(word);
        }
        else
        {
            _petHalfLocaleName0[pairkey].push_back(word);
        }
        ++count;
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} Pet Name Locales Parts in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

void ObjectMgr::LoadPointOfInterestLocales()
{
    uint32 oldMSTime = getMSTime();

    _pointOfInterestLocaleStore.clear();                              // need for reload case

    //                                               0   1       2
    QueryResult result = WorldDatabase.Query("SELECT ID, locale, Name FROM points_of_interest_locale");

    if (!result)
        return;

    do
    {
        Field* fields = result->Fetch();

        uint32 ID = fields[0].Get<uint32>();

        LocaleConstant locale = GetLocaleByName(fields[1].Get<std::string>());
        if (locale == LOCALE_enUS)
            continue;

        PointOfInterestLocale& data = _pointOfInterestLocaleStore[ID];
        AddLocaleString(fields[2].Get<std::string>(), locale, data.Name);
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} Points Of Interest Locale Strings in {} ms", (uint32)_pointOfInterestLocaleStore.size(), GetMSTimeDiffToNow(oldMSTime));
}

void ObjectMgr::LoadCreatureTemplates()
{
    uint32 oldMSTime = getMSTime();

//                                                        0      1                   2                   3                   4            5            6     7        8
    QueryResult result = WorldDatabase.Query("SELECT entry, difficulty_entry_1, difficulty_entry_2, difficulty_entry_3, KillCredit1, KillCredit2, name, subname, IconName, "
//                        9               10        11        12   13       14       15          16         17          18            19               20      21
                         "gossip_menu_id, minlevel, maxlevel, exp, faction, npcflag, speed_walk, speed_run, speed_swim, speed_flight, detection_range, `rank`, dmgschool, "
//                        22              23              24               25            26             27          28          29           30            31
                         "DamageModifier, BaseAttackTime, RangeAttackTime, BaseVariance, RangeVariance, unit_class, unit_flags, unit_flags2, dynamicflags, family, "
//                        32    33          34      35              36
                         "type, type_flags, lootid, pickpocketloot, skinloot, "
//                        37              38         39       40       41      42            43          44        45          46
                         "PetSpellDataId, VehicleId, mingold, maxgold, AIName, MovementType, ctm.Ground, ctm.Swim, ctm.Flight, ctm.Rooted, "
//                        47         48          49                         50           51              52            53             54                  55            56          57
                         "ctm.Chase, ctm.Random, ctm.InteractionPauseTimer, HoverHeight, HealthModifier, ManaModifier, ArmorModifier, ExperienceModifier, RacialLeader, movementId, RegenHealth, "
//                       58                     59           60
                         "CreatureImmunitiesId, flags_extra, ScriptName "
                         "FROM creature_template ct LEFT JOIN creature_template_movement ctm ON ct.entry = ctm.CreatureId ORDER BY entry DESC;");

    if (!result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 creature template definitions. DB table `creature_template` is empty.");
        return;
    }

    _creatureTemplateStore.rehash(result->GetRowCount());
    _creatureTemplateStoreFast.clear();

    uint32 count = 0;
    do
    {
        Field* fields = result->Fetch();
        LoadCreatureTemplate(fields);
        ++count;
    } while (result->NextRow());

    // We load the creature models after loading but before checking
    LoadCreatureTemplateModels();

    sScriptMgr->OnAfterDatabaseLoadCreatureTemplates(_creatureTemplateStoreFast);

    LoadCreatureTemplateResistances();
    LoadCreatureTemplateSpells();

    // Checking needs to be done after loading because of the difficulty self referencing
    for (CreatureTemplateContainer::iterator itr = _creatureTemplateStore.begin(); itr != _creatureTemplateStore.end(); ++itr)
    {
        CheckCreatureTemplate(&itr->second);
        itr->second.InitializeQueryData();
    }

    LOG_INFO("server.loading", ">> Loaded {} Creature Definitions in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

/**
* @brief Loads a creature template from a database result
*
* @param fields Database result
* @param triggerHook If true, will trigger the OnAfterDatabaseLoadCreatureTemplates hook. Useful if you are not calling the hook yourself.
*/
void ObjectMgr::LoadCreatureTemplate(Field* fields, bool triggerHook)
{
    uint32 entry = fields[0].Get<uint32>();

    CreatureTemplate& creatureTemplate = _creatureTemplateStore[entry];

    // enlarge the fast cache as necessary
    if (_creatureTemplateStoreFast.size() < entry + 1)
    {
        _creatureTemplateStoreFast.resize(entry + 1, nullptr);
    }

    // load a pointer to this creatureTemplate into the fast cache
    _creatureTemplateStoreFast[entry] = &creatureTemplate;

    // build the creatureTemplate
    creatureTemplate.Entry = entry;

    for (uint8 i = 0; i < MAX_DIFFICULTY - 1; ++i)
    {
        creatureTemplate.DifficultyEntry[i] = fields[1 + i].Get<uint32>();
    }

    for (uint8 i = 0; i < MAX_KILL_CREDIT; ++i)
    {
        creatureTemplate.KillCredit[i] = fields[4 + i].Get<uint32>();
    }
    creatureTemplate.Name             = fields[6].Get<std::string>();
    creatureTemplate.SubName          = fields[7].Get<std::string>();
    creatureTemplate.IconName         = fields[8].Get<std::string>();
    creatureTemplate.GossipMenuId     = fields[9].Get<uint32>();
    creatureTemplate.minlevel         = fields[10].Get<uint8>();
    creatureTemplate.maxlevel         = fields[11].Get<uint8>();
    creatureTemplate.expansion        = uint32(fields[12].Get<int16>());
    creatureTemplate.faction          = uint32(fields[13].Get<uint16>());
    creatureTemplate.npcflag          = fields[14].Get<uint32>();
    creatureTemplate.speed_walk       = fields[15].Get<float>();
    creatureTemplate.speed_run        = fields[16].Get<float>();
    creatureTemplate.speed_swim       = fields[17].Get<float>();
    creatureTemplate.speed_flight     = fields[18].Get<float>();
    creatureTemplate.detection_range  = fields[19].Get<float>();
    creatureTemplate.rank             = uint32(fields[20].Get<uint8>());
    creatureTemplate.dmgschool        = uint32(fields[21].Get<int8>());
    creatureTemplate.DamageModifier   = fields[22].Get<float>();
    creatureTemplate.BaseAttackTime   = fields[23].Get<uint32>();
    creatureTemplate.RangeAttackTime  = fields[24].Get<uint32>();
    creatureTemplate.BaseVariance     = fields[25].Get<float>();
    creatureTemplate.RangeVariance    = fields[26].Get<float>();
    creatureTemplate.unit_class       = uint32(fields[27].Get<uint8>());
    creatureTemplate.unit_flags       = fields[28].Get<uint32>();
    creatureTemplate.unit_flags2      = fields[29].Get<uint32>();
    creatureTemplate.dynamicflags     = fields[30].Get<uint32>();
    creatureTemplate.family           = uint32(fields[31].Get<uint8>());
    creatureTemplate.type             = uint32(fields[32].Get<uint8>());
    creatureTemplate.type_flags       = fields[33].Get<uint32>();
    creatureTemplate.lootid           = fields[34].Get<uint32>();
    creatureTemplate.pickpocketLootId = fields[35].Get<uint32>();
    creatureTemplate.SkinLootId       = fields[36].Get<uint32>();

    for (uint8 i = SPELL_SCHOOL_HOLY; i < MAX_SPELL_SCHOOL; ++i)
    {
        creatureTemplate.resistance[i] = 0;
    }

    for (uint8 i = 0; i < MAX_CREATURE_SPELLS; ++i)
    {
        creatureTemplate.spells[i] = 0;
    }

    creatureTemplate.PetSpellDataId = fields[37].Get<uint32>();
    creatureTemplate.VehicleId      = fields[38].Get<uint32>();
    creatureTemplate.mingold        = fields[39].Get<uint32>();
    creatureTemplate.maxgold        = fields[40].Get<uint32>();
    creatureTemplate.AIName         = fields[41].Get<std::string>();
    creatureTemplate.MovementType   = uint32(fields[42].Get<uint8>());
    if (!fields[43].IsNull())
    {
        creatureTemplate.Movement.Ground = static_cast<CreatureGroundMovementType>(fields[43].Get<uint8>());
    }

    creatureTemplate.Movement.Swim = fields[44].Get<bool>();
    if (!fields[45].IsNull())
    {
        creatureTemplate.Movement.Flight = static_cast<CreatureFlightMovementType>(fields[45].Get<uint8>());
    }

    creatureTemplate.Movement.Rooted = fields[46].Get<bool>();
    if (!fields[47].IsNull())
    {
        creatureTemplate.Movement.Chase = static_cast<CreatureChaseMovementType>(fields[47].Get<uint8>());
    }
    if (!fields[48].IsNull())
    {
        creatureTemplate.Movement.Random = static_cast<CreatureRandomMovementType>(fields[48].Get<uint8>());
    }
    if (!fields[49].IsNull())
    {
        creatureTemplate.Movement.InteractionPauseTimer = fields[49].Get<uint32>();
    }

    creatureTemplate.HoverHeight           = fields[50].Get<float>();
    creatureTemplate.ModHealth             = fields[51].Get<float>();
    creatureTemplate.ModMana               = fields[52].Get<float>();
    creatureTemplate.ModArmor              = fields[53].Get<float>();
    creatureTemplate.ModExperience         = fields[54].Get<float>();
    creatureTemplate.RacialLeader          = fields[55].Get<bool>();
    creatureTemplate.movementId            = fields[56].Get<uint32>();
    creatureTemplate.RegenHealth           = fields[57].Get<bool>();
    creatureTemplate.CreatureImmunitiesId  = fields[58].Get<int32>();
    creatureTemplate.flags_extra           = fields[59].Get<uint32>();
    creatureTemplate.ScriptID              = GetScriptId(fields[60].Get<std::string>());

    // Warn about deprecated immunity flags that should be moved to `creature_immunities` table
    if (creatureTemplate.flags_extra & CREATURE_FLAG_EXTRA_NO_TAUNT)
        LOG_WARN("server.loading", "Creature (Entry: {}) has deprecated flags_extra bit NO_TAUNT (0x100) set. This will be migrated to the `creature_immunities` table in a future update.", entry);
    if (creatureTemplate.flags_extra & CREATURE_FLAG_EXTRA_AVOID_AOE)
        LOG_WARN("server.loading", "Creature (Entry: {}) has deprecated flags_extra bit AVOID_AOE (0x400000) set. This will be migrated to the `creature_immunities` table in a future update.", entry);
    if (creatureTemplate.flags_extra & CREATURE_FLAG_EXTRA_IMMUNITY_KNOCKBACK)
        LOG_WARN("server.loading", "Creature (Entry: {}) has deprecated flags_extra bit IMMUNITY_KNOCKBACK (0x40000000) set. This will be migrated to the `creature_immunities` table in a future update.", entry);

    // useful if the creature template load is being triggered from outside this class
    if (triggerHook)
    {
        sScriptMgr->OnAfterDatabaseLoadCreatureTemplates(_creatureTemplateStoreFast);
    }
}

void ObjectMgr::LoadCreatureTemplateModels()
{
    uint32 oldMSTime = getMSTime();

    //                                               0           1                  2             3
    QueryResult result = WorldDatabase.Query("SELECT CreatureID, CreatureDisplayID, DisplayScale, Probability FROM creature_template_model ORDER BY Idx ASC");

    if (!result)
    {
        LOG_INFO("server.loading", ">> Loaded 0 creature template model definitions. DB table `creature_template_model` is empty.");
        return;
    }

    uint32 count = 0;
    do
    {
        Field* fields = result->Fetch();

        uint32 creatureId = fields[0].Get<uint32>();
        uint32 creatureDisplayId = fields[1].Get<uint32>();
        float displayScale = fields[2].Get<float>();
        float probability = fields[3].Get<float>();

        CreatureTemplate const* cInfo = GetCreatureTemplate(creatureId);
        if (!cInfo)
        {
            LOG_ERROR("sql.sql", "Creature template (Entry: {}) does not exist but has a record in `creature_template_model`", creatureId);
            continue;
        }

        CreatureDisplayInfoEntry const* displayEntry = sCreatureDisplayInfoStore.LookupEntry(creatureDisplayId);
        if (!displayEntry)
        {
            LOG_ERROR("sql.sql", "Creature (Entry: {}) lists non-existing CreatureDisplayID id ({}), this can crash the client.", creatureId, creatureDisplayId);
            continue;
        }

        CreatureModelInfo const* modelInfo = GetCreatureModelInfo(creatureDisplayId);
        if (!modelInfo)
            LOG_ERROR("sql.sql", "No model data exist for `CreatureDisplayID` = {} listed by creature (Entry: {}).", creatureDisplayId, creatureId);

        if (displayScale <= 0.0f)
            displayScale = 1.0f;

        const_cast<CreatureTemplate*>(cInfo)->Models.emplace_back(creatureDisplayId, displayScale, probability);

        ++count;
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} creature template models in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
}

void ObjectMgr::LoadCreatureTemplateResistances()
{
    uint32 oldMSTime = getMSTime();

    //                                               0           1       2
    QueryResult result = WorldDatabase.Query("SELECT CreatureID, School, Resistance FROM creature_template_resistance");

    if (!result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 creature template resistance definitions. DB table `creature_template_resistance` is empty.");
        LOG_INFO("server.loading", " ");
        return;
    }

    uint32 count = 0;

    do
    {
        Field* fields = result->Fetch();

        uint32 creatureID = fields[0].Get<uint32>();
        uint8 school = fields[1].Get<uint8>();

        if (school == SPELL_SCHOOL_NORMAL || school >= MAX_SPELL_SCHOOL)
        {
            LOG_ERROR("sql.sql", "creature_template_resistance has resistance definitions for creature {} but this school {} doesn't exist", creatureID, school);
            continue;
        }

        CreatureTemplateContainer::iterator itr = _creatureTemplateStore.find(creatureID);
        if (itr == _creatureTemplateStore.end())
        {
            LOG_ERROR("sql.sql", "creature_template_resistance has resistance definitions for creature {} but this creature doesn't exist", creatureID);
            continue;
        }

        CreatureTemplate& creatureTemplate = itr->second;
        creatureTemplate.resistance[school] = fields[2].Get<int16>();

        ++count;
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} Creature Template Resistances in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

void ObjectMgr::LoadCreatureTemplateSpells()
{
    uint32 oldMSTime = getMSTime();

    //                                               0           1       2
    QueryResult result = WorldDatabase.Query("SELECT CreatureID, `Index`, Spell FROM creature_template_spell");

    if (!result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 creature template spell definitions. DB table `creature_template_spell` is empty.");
        LOG_INFO("server.loading", " ");
        return;
    }

    uint32 count = 0;

    do
    {
        Field* fields = result->Fetch();

        uint32 creatureID = fields[0].Get<uint32>();
        uint8 index = fields[1].Get<uint8>();

        if (index >= MAX_CREATURE_SPELLS)
        {
            LOG_ERROR("sql.sql", "creature_template_spell has spell definitions for creature {} with a incorrect index {}", creatureID, index);
            continue;
        }

        CreatureTemplateContainer::iterator itr = _creatureTemplateStore.find(creatureID);
        if (itr == _creatureTemplateStore.end())
        {
            LOG_ERROR("sql.sql", "creature_template_spell has spell definitions for creature {} but this creature doesn't exist", creatureID);
            continue;
        }

        CreatureTemplate& creatureTemplate = itr->second;
        creatureTemplate.spells[index] = fields[2].Get<uint32>();

        ++count;
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} Creature Template Spells in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

// AnimKit.dbc id of a creature_addon/creature_template_addon anim kit column, 0 when unknown
static uint16 LoadAnimKit(uint16 animKitId, char const* column)
{
    if (animKitId && !sAnimKitStore.LookupEntry(animKitId))
    {
        LOG_ERROR("sql.sql", "Creature addon {} {} does not exist in AnimKit.dbc, ignored.", column, animKitId);
        return 0;
    }

    return animKitId;
}

void ObjectMgr::LoadCreatureTemplateAddons()
{
    uint32 oldMSTime = getMSTime();

    //                                                0       1       2      3       4       5              6               7      8          9                10
    QueryResult result = WorldDatabase.Query("SELECT entry, path_id, mount, bytes1, bytes2, emote, visibilityDistanceType, auras, aiAnimKit, movementAnimKit, meleeAnimKit FROM creature_template_addon");

    if (!result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 creature template addon definitions. DB table `creature_template_addon` is empty.");
        LOG_INFO("server.loading", " ");
        return;
    }

    uint32 count = 0;
    do
    {
        Field* fields = result->Fetch();

        uint32 entry = fields[0].Get<uint32>();

        if (!GetCreatureTemplate(entry))
        {
            LOG_ERROR("sql.sql", "Creature template (Entry: {}) does not exist but has a record in `creature_template_addon`", entry);
            continue;
        }

        CreatureAddon& creatureAddon = _creatureTemplateAddonStore[entry];

        creatureAddon.path_id = fields[1].Get<uint32>();
        creatureAddon.mount   = fields[2].Get<uint32>();
        creatureAddon.bytes1  = fields[3].Get<uint32>();
        creatureAddon.bytes2  = fields[4].Get<uint32>();
        creatureAddon.emote   = fields[5].Get<uint32>();
        creatureAddon.visibilityDistanceType = VisibilityDistanceType(fields[6].Get<uint8>());
        creatureAddon.aiAnimKit       = LoadAnimKit(fields[8].Get<uint16>(), "aiAnimKit");
        creatureAddon.movementAnimKit = LoadAnimKit(fields[9].Get<uint16>(), "movementAnimKit");
        creatureAddon.meleeAnimKit    = LoadAnimKit(fields[10].Get<uint16>(), "meleeAnimKit");

        for (std::string_view aura : Acore::Tokenize(fields[7].Get<std::string_view>(), ' ', false))
        {
            SpellInfo const* spellInfo = nullptr;

            if (Optional<uint32> spellId = Acore::StringTo<uint32>(aura))
            {
                spellInfo = sSpellMgr->GetSpellInfo(*spellId);
            }

            if (!spellInfo)
            {
                LOG_ERROR("sql.sql", "Creature (Entry: {}) has wrong spell '{}' defined in `auras` field in `creature_template_addon`.", entry, aura);
                continue;
            }

            if (std::find(creatureAddon.auras.begin(), creatureAddon.auras.end(), spellInfo->Id) != creatureAddon.auras.end())
            {
                LOG_ERROR("sql.sql", "Creature (Entry: {}) has duplicate aura (spell {}) in `auras` field in `creature_template_addon`.", entry, spellInfo->Id);
                continue;
            }

            if (spellInfo->GetDuration() > 0)
            {
                LOG_DEBUG/*ERROR*/("sql.sql", "Creature (Entry: {}) has temporary aura (spell {}) in `auras` field in `creature_template_addon`.", entry, spellInfo->Id);
                // continue;
            }

            creatureAddon.auras.push_back(spellInfo->Id);
        }

        if (creatureAddon.mount)
        {
            if (!sCreatureDisplayInfoStore.LookupEntry(creatureAddon.mount))
            {
                LOG_ERROR("sql.sql", "Creature (Entry: {}) has invalid displayInfoId ({}) for mount defined in `creature_template_addon`", entry, creatureAddon.mount);
                creatureAddon.mount = 0;
            }
        }

        if (!sEmotesStore.LookupEntry(creatureAddon.emote))
        {
            LOG_ERROR("sql.sql", "Creature (Entry: {}) has invalid emote ({}) defined in `creature_addon`.", entry, creatureAddon.emote);
            creatureAddon.emote = 0;
        }

        if (creatureAddon.visibilityDistanceType >= VisibilityDistanceType::Max)
        {
            LOG_ERROR("sql.sql", "Creature (Entry: {}) has invalid visibilityDistanceType ({}) defined in `creature_template_addon`.", entry, AsUnderlyingType(creatureAddon.visibilityDistanceType));
            creatureAddon.visibilityDistanceType = VisibilityDistanceType::Normal;
        }
        ++count;
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} Creature Template Addons in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

/**
 * @brief Load config option Creatures.CustomIDs into Store
 */
void ObjectMgr::LoadCreatureCustomIDs()
{
    // Hack for modules
    std::string stringCreatureIds = sConfigMgr->GetOption<std::string>("Creatures.CustomIDs", "190010,55005,999991,25462,98888,601014,34567,34568");
    std::vector<std::string_view> CustomCreatures = Acore::Tokenize(stringCreatureIds, ',', false);

    for (auto& itr : CustomCreatures)
    {
        _creatureCustomIDsStore.push_back(Acore::StringTo<uint32>(itr).value());
    }
}

void ObjectMgr::CheckCreatureTemplate(CreatureTemplate const* cInfo)
{
    if (!cInfo)
        return;

    bool ok = true;                                     // bool to allow continue outside this loop
    for (uint32 diff = 0; diff < MAX_DIFFICULTY - 1 && ok; ++diff)
    {
        if (!cInfo->DifficultyEntry[diff])
            continue;
        ok = false;                                     // will be set to true at the end of this loop again

        CreatureTemplate const* difficultyInfo = GetCreatureTemplate(cInfo->DifficultyEntry[diff]);
        if (!difficultyInfo)
        {
            LOG_ERROR("sql.sql", "Creature (Entry: {}) has `difficulty_entry_{}`={} but creature entry {} does not exist.",
                             cInfo->Entry, diff + 1, cInfo->DifficultyEntry[diff], cInfo->DifficultyEntry[diff]);
            continue;
        }

        bool ok2 = true;
        for (uint32 diff2 = 0; diff2 < MAX_DIFFICULTY - 1 && ok2; ++diff2)
        {
            ok2 = false;
            if (_difficultyEntries[diff2].find(cInfo->Entry) != _difficultyEntries[diff2].end())
            {
                LOG_ERROR("sql.sql", "Creature (Entry: {}) is listed as `difficulty_entry_{}` of another creature, but itself lists {} in `difficulty_entry_{}`.",
                                 cInfo->Entry, diff2 + 1, cInfo->DifficultyEntry[diff], diff + 1);
                continue;
            }

            if (_difficultyEntries[diff2].find(cInfo->DifficultyEntry[diff]) != _difficultyEntries[diff2].end())
            {
                LOG_ERROR("sql.sql", "Creature (Entry: {}) already listed as `difficulty_entry_{}` for another entry.", cInfo->DifficultyEntry[diff], diff2 + 1);
                continue;
            }

            if (_hasDifficultyEntries[diff2].find(cInfo->DifficultyEntry[diff]) != _hasDifficultyEntries[diff2].end())
            {
                LOG_ERROR("sql.sql", "Creature (Entry: {}) has `difficulty_entry_{}`={} but creature entry {} has itself a value in `difficulty_entry_{}`.",
                                 cInfo->Entry, diff + 1, cInfo->DifficultyEntry[diff], cInfo->DifficultyEntry[diff], diff2 + 1);
                continue;
            }
            ok2 = true;
        }
        if (!ok2)
            continue;

        if (cInfo->expansion > difficultyInfo->expansion)
        {
            LOG_ERROR("sql.sql", "Creature (Entry: {}, expansion {}) has different `expansion` in difficulty {} mode (Entry: {}, expansion {}).",
                             cInfo->Entry, cInfo->expansion, diff + 1, cInfo->DifficultyEntry[diff], difficultyInfo->expansion);
        }

        if (cInfo->faction != difficultyInfo->faction)
        {
            LOG_ERROR("sql.sql", "Creature (Entry: {}, faction {}) has different `faction` in difficulty {} mode (Entry: {}, faction {}).",
                             cInfo->Entry, cInfo->faction, diff + 1, cInfo->DifficultyEntry[diff], difficultyInfo->faction);
        }

        if (cInfo->unit_class != difficultyInfo->unit_class)
        {
            LOG_ERROR("sql.sql", "Creature (Entry: {}, class {}) has different `unit_class` in difficulty {} mode (Entry: {}, class {}).",
                             cInfo->Entry, cInfo->unit_class, diff + 1, cInfo->DifficultyEntry[diff], difficultyInfo->unit_class);
            continue;
        }

        if (cInfo->npcflag != difficultyInfo->npcflag)
        {
            LOG_ERROR("sql.sql", "Creature (Entry: {}) has different `npcflag` in difficulty {} mode (Entry: {}).", cInfo->Entry, diff + 1, cInfo->DifficultyEntry[diff]);
            continue;
        }

        if (cInfo->family != difficultyInfo->family)
        {
            LOG_ERROR("sql.sql", "Creature (Entry: {}, family {}) has different `family` in difficulty {} mode (Entry: {}, family {}).",
                             cInfo->Entry, cInfo->family, diff + 1, cInfo->DifficultyEntry[diff], difficultyInfo->family);
        }

        if (cInfo->type != difficultyInfo->type)
        {
            LOG_ERROR("sql.sql", "Creature (Entry: {}, type {}) has different `type` in difficulty {} mode (Entry: {}, type {}).",
                             cInfo->Entry, cInfo->type, diff + 1, cInfo->DifficultyEntry[diff], difficultyInfo->type);
        }

        if (!cInfo->VehicleId && difficultyInfo->VehicleId)
        {
            LOG_ERROR("sql.sql", "Creature (Entry: {}, VehicleId {}) has different `VehicleId` in difficulty {} mode (Entry: {}, VehicleId {}).",
                             cInfo->Entry, cInfo->VehicleId, diff + 1, cInfo->DifficultyEntry[diff], difficultyInfo->VehicleId);
        }

        // Xinef: check dmg school
        if (cInfo->dmgschool != difficultyInfo->dmgschool)
        {
            LOG_ERROR("sql.sql", "Creature (Entry: {}) has different `dmgschool` in difficulty {} mode (Entry: {})", cInfo->Entry, diff + 1, cInfo->DifficultyEntry[diff]);
        }

        if (!difficultyInfo->AIName.empty())
        {
            LOG_ERROR("sql.sql", "Creature (Entry: {}) lists difficulty {} mode entry {} with `AIName` filled in. `AIName` of difficulty 0 mode creature is always used instead.",
                             cInfo->Entry, diff + 1, cInfo->DifficultyEntry[diff]);
            continue;
        }

        if (difficultyInfo->ScriptID)
        {
            LOG_ERROR("sql.sql", "Creature (Entry: {}) lists difficulty {} mode entry {} with `ScriptName` filled in. `ScriptName` of difficulty 0 mode creature is always used instead.",
                             cInfo->Entry, diff + 1, cInfo->DifficultyEntry[diff]);
            continue;
        }

        _hasDifficultyEntries[diff].insert(cInfo->Entry);
        _difficultyEntries[diff].insert(cInfo->DifficultyEntry[diff]);
        ok = true;
    }

    if (!cInfo->AIName.empty() && !sCreatureAIRegistry->HasItem(cInfo->AIName))
    {
        LOG_ERROR("sql.sql", "Creature (Entry: {}) has non-registered `AIName` '{}' set, removing", cInfo->Entry, cInfo->AIName);
        const_cast<CreatureTemplate*>(cInfo)->AIName.clear();
    }

    FactionTemplateEntry const* factionTemplate = sFactionTemplateStore.LookupEntry(cInfo->faction);
    if (!factionTemplate)
        LOG_ERROR("sql.sql", "Creature (Entry: {}) has non-existing faction template ({}).", cInfo->Entry, cInfo->faction);

    for (int k = 0; k < MAX_KILL_CREDIT; ++k)
    {
        if (cInfo->KillCredit[k])
        {
            if (!GetCreatureTemplate(cInfo->KillCredit[k]))
            {
                LOG_ERROR("sql.sql", "Creature (Entry: {}) lists non-existing creature entry {} in `KillCredit{}`.", cInfo->Entry, cInfo->KillCredit[k], k + 1);
                const_cast<CreatureTemplate*>(cInfo)->KillCredit[k] = 0;
            }
        }
    }

    if (!cInfo->Models.size())
        LOG_ERROR("sql.sql", "Creature (Entry: {}) does not have any existing display id in creature_template_model.", cInfo->Entry);
    else
    {
        float const totalProbability = std::accumulate(cInfo->Models.begin(), cInfo->Models.end(), 0.0f, [](float sum, CreatureModel const& model) { return sum + model.Probability; });

        if (totalProbability <= 0.0f)
        { // There are many cases in official data of all models having a probability of 0. Believe to be treated equivalent to equal chance ONLY if all are zeroed
            if (totalProbability == 0.0f)
                LOG_DEBUG("sql.sql", "Creature (Entry: {}) has zero total chance for all models in creature_template_model. Setting all to 1.0.", cInfo->Entry);
            else // Custom, likely bad data
                LOG_ERROR("sql.sql", "Creature (Entry: {}) has less than zero total chance for all models in creature_template_model. Setting all to 1.0.", cInfo->Entry);

            auto& models = const_cast<CreatureTemplate*>(cInfo)->Models;
            for (auto& model : models)
                model.Probability = 1.0f;
        }
    }

    if (!cInfo->unit_class || ((1 << (cInfo->unit_class - 1)) & CLASSMASK_ALL_CREATURES) == 0)
    {
        LOG_ERROR("sql.sql", "Creature (Entry: {}) has invalid unit_class ({}) in creature_template. Set to 1 (CLASS_WARRIOR).", cInfo->Entry, cInfo->unit_class);
        const_cast<CreatureTemplate*>(cInfo)->unit_class = CLASS_WARRIOR;
    }

    if (cInfo->dmgschool >= MAX_SPELL_SCHOOL)
    {
        LOG_ERROR("sql.sql", "Creature (Entry: {}) has invalid spell school value ({}) in `dmgschool`.", cInfo->Entry, cInfo->dmgschool);
        const_cast<CreatureTemplate*>(cInfo)->dmgschool = SPELL_SCHOOL_NORMAL;
    }

    if (cInfo->BaseAttackTime == 0)
        const_cast<CreatureTemplate*>(cInfo)->BaseAttackTime  = BASE_ATTACK_TIME;

    if (cInfo->RangeAttackTime == 0)
        const_cast<CreatureTemplate*>(cInfo)->RangeAttackTime = BASE_ATTACK_TIME;

    if (cInfo->speed_walk == 0.0f)
    {
        LOG_ERROR("sql.sql", "Creature (Entry: {}) has wrong value ({}) in speed_walk, set to 1.", cInfo->Entry, cInfo->speed_walk);
        const_cast<CreatureTemplate*>(cInfo)->speed_walk = 1.0f;
    }

    if (cInfo->speed_run == 0.0f)
    {
        LOG_ERROR("sql.sql", "Creature (Entry: {}) has wrong value ({}) in speed_run, set to 1.14286.", cInfo->Entry, cInfo->speed_run);
        const_cast<CreatureTemplate*>(cInfo)->speed_run = 1.14286f;
    }

    if (cInfo->type && !sCreatureTypeStore.LookupEntry(cInfo->type))
    {
        LOG_ERROR("sql.sql", "Creature (Entry: {}) has invalid creature type ({}) in `type`.", cInfo->Entry, cInfo->type);
        const_cast<CreatureTemplate*>(cInfo)->type = CREATURE_TYPE_HUMANOID;
    }

    // Must exist in DBC or use hidden miscellaneous family
    if (cInfo->family && !sCreatureFamilyStore.LookupEntry(cInfo->family) && cInfo->family != CREATURE_FAMILY_NOT_SPECIFIED)
    {
        LOG_ERROR("sql.sql", "Creature (Entry: {}) has invalid creature family ({}) in `family`.", cInfo->Entry, cInfo->family);
        const_cast<CreatureTemplate*>(cInfo)->family = 0;
    }

    CheckCreatureMovement("creature_template_movement", cInfo->Entry, const_cast<CreatureTemplate*>(cInfo)->Movement);

    if (cInfo->HoverHeight < 0.0f)
    {
        LOG_ERROR("sql.sql", "Creature (Entry: {}) has wrong value ({}) in `HoverHeight`", cInfo->Entry, cInfo->HoverHeight);
        const_cast<CreatureTemplate*>(cInfo)->HoverHeight = 1.0f;
    }

    if (cInfo->VehicleId)
    {
        VehicleEntry const* vehId = sVehicleStore.LookupEntry(cInfo->VehicleId);
        if (!vehId)
        {
            LOG_ERROR("sql.sql", "Creature (Entry: {}) has a non-existing VehicleId ({}). This *WILL* cause the client to freeze!", cInfo->Entry, cInfo->VehicleId);
            const_cast<CreatureTemplate*>(cInfo)->VehicleId = 0;
        }
    }

    if (cInfo->PetSpellDataId)
    {
        CreatureSpellDataEntry const* spellDataId = sCreatureSpellDataStore.LookupEntry(cInfo->PetSpellDataId);
        if (!spellDataId)
            LOG_ERROR("sql.sql", "Creature (Entry: {}) has non-existing PetSpellDataId ({}).", cInfo->Entry, cInfo->PetSpellDataId);
    }

    for (uint8 j = 0; j < MAX_CREATURE_SPELLS; ++j)
    {
        if (cInfo->spells[j] && !sSpellMgr->GetSpellInfo(cInfo->spells[j]))
        {
            LOG_ERROR("sql.sql", "Creature (Entry: {}) has non-existing Spell{} ({}), set to 0.", cInfo->Entry, j + 1, cInfo->spells[j]);
            const_cast<CreatureTemplate*>(cInfo)->spells[j] = 0;
        }
    }

    if (cInfo->MovementType >= MAX_DB_MOTION_TYPE)
    {
        LOG_ERROR("sql.sql", "Creature (Entry: {}) has wrong movement generator type ({}), ignored and set to IDLE.", cInfo->Entry, cInfo->MovementType);
        const_cast<CreatureTemplate*>(cInfo)->MovementType = IDLE_MOTION_TYPE;
    }

    if (cInfo->expansion > (MAX_EXPANSIONS - 1))
    {
        LOG_ERROR("sql.sql", "Table `creature_template` lists creature (Entry: {}) with expansion {}. Ignored and set to 0.", cInfo->Entry, cInfo->expansion);
        const_cast<CreatureTemplate*>(cInfo)->expansion = 0;
    }

    if (uint32 badFlags = (cInfo->flags_extra & ~CREATURE_FLAG_EXTRA_DB_ALLOWED))
    {
        LOG_ERROR("sql.sql", "Table `creature_template` lists creature (Entry: {}) with disallowed `flags_extra` {}, removing incorrect flag.", cInfo->Entry, badFlags);
        const_cast<CreatureTemplate*>(cInfo)->flags_extra &= CREATURE_FLAG_EXTRA_DB_ALLOWED;
    }

    const_cast<CreatureTemplate*>(cInfo)->DamageModifier *= Creature::_GetDamageMod(cInfo->rank);

    // Hack for modules
    for (auto& itr : _creatureCustomIDsStore)
    {
        if (cInfo->Entry == itr)
            return;
    }

    if ((cInfo->GossipMenuId && !(cInfo->npcflag & UNIT_NPC_FLAG_GOSSIP)) && !(cInfo->flags_extra & CREATURE_FLAG_EXTRA_MODULE))
    {
        LOG_ERROR("sql.sql", "Creature (Entry: {}) has assigned gossip menu {}, but npcflag does not include UNIT_NPC_FLAG_GOSSIP (1).", cInfo->Entry, cInfo->GossipMenuId);
    }
    else if ((!cInfo->GossipMenuId && (cInfo->npcflag & UNIT_NPC_FLAG_GOSSIP)) && !(cInfo->flags_extra & CREATURE_FLAG_EXTRA_MODULE))
    {
        LOG_INFO("sql.sql", "Creature (Entry: {}) has npcflag UNIT_NPC_FLAG_GOSSIP (1), but gossip menu is unassigned.", cInfo->Entry);
    }
}

void ObjectMgr::CheckCreatureMovement(char const* table, uint64 id, CreatureMovementData& creatureMovement)
{
    if (creatureMovement.Ground >= CreatureGroundMovementType::Max)
    {
        LOG_ERROR("sql.sql", "`{}`.`Ground` wrong value ({}) for Id {}, setting to Run.", table, uint32(creatureMovement.Ground), id);
        creatureMovement.Ground = CreatureGroundMovementType::Run;
    }

    if (creatureMovement.Flight >= CreatureFlightMovementType::Max)
    {
        LOG_ERROR("sql.sql", "`{}`.`Flight` wrong value ({}) for Id {}, setting to None.", table, uint32(creatureMovement.Flight), id);
        creatureMovement.Flight = CreatureFlightMovementType::None;
    }

    if (creatureMovement.Chase >= CreatureChaseMovementType::Max)
    {
        LOG_ERROR("sql.sql", "`{}`.`Chase` wrong value ({}) for Id {}, setting to Run.", table, uint32(creatureMovement.Chase), id);
        creatureMovement.Chase = CreatureChaseMovementType::Run;
    }

    if (creatureMovement.Random >= CreatureRandomMovementType::Max)
    {
        LOG_ERROR("sql.sql", "`{}`.`Random` wrong value ({}) for Id {}, setting to Walk.", table, uint32(creatureMovement.Random), id);
        creatureMovement.Random = CreatureRandomMovementType::Walk;
    }
}

void ObjectMgr::LoadCreatureAddons()
{
    uint32 oldMSTime = getMSTime();

    //                                                0       1       2      3       4       5             6                7
    QueryResult result = WorldDatabase.Query("SELECT guid, path_id, mount, bytes1, bytes2, emote, visibilityDistanceType, auras, aiAnimKit, movementAnimKit, meleeAnimKit FROM creature_addon");

    if (!result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 creature addon definitions. DB table `creature_addon` is empty.");
        LOG_INFO("server.loading", " ");
        return;
    }

    uint32 count = 0;
    do
    {
        Field* fields = result->Fetch();

        ObjectGuid::LowType guid = fields[0].Get<uint32>();

        CreatureData const* creData = GetCreatureData(guid);
        if (!creData)
        {
            LOG_ERROR("sql.sql", "Creature (GUID: {}) does not exist but has a record in `creature_addon`", guid);
            continue;
        }

        CreatureAddon& creatureAddon = _creatureAddonStore[guid];

        creatureAddon.path_id = fields[1].Get<uint32>();
        if (creData->movementType == WAYPOINT_MOTION_TYPE && !creatureAddon.path_id)
        {
            const_cast<CreatureData*>(creData)->movementType = IDLE_MOTION_TYPE;
            LOG_ERROR("sql.sql", "Creature (GUID {}) has movement type set to WAYPOINT_MOTION_TYPE but no path assigned", guid);
        }

        creatureAddon.mount   = fields[2].Get<uint32>();
        creatureAddon.bytes1  = fields[3].Get<uint32>();
        creatureAddon.bytes2  = fields[4].Get<uint32>();
        creatureAddon.emote   = fields[5].Get<uint32>();
        creatureAddon.visibilityDistanceType = VisibilityDistanceType(fields[6].Get<uint8>());
        creatureAddon.aiAnimKit       = LoadAnimKit(fields[8].Get<uint16>(), "aiAnimKit");
        creatureAddon.movementAnimKit = LoadAnimKit(fields[9].Get<uint16>(), "movementAnimKit");
        creatureAddon.meleeAnimKit    = LoadAnimKit(fields[10].Get<uint16>(), "meleeAnimKit");

        for (std::string_view aura : Acore::Tokenize(fields[7].Get<std::string_view>(), ' ', false))
        {
            SpellInfo const* spellInfo = nullptr;

            if (Optional<uint32> spellId = Acore::StringTo<uint32>(aura))
            {
                spellInfo = sSpellMgr->GetSpellInfo(*spellId);
            }

            if (!spellInfo)
            {
                LOG_ERROR("sql.sql", "Creature (GUID: {}) has wrong spell '{}' defined in `auras` field in `creature_addon`.", guid, aura);
                continue;
            }

            if (std::find(creatureAddon.auras.begin(), creatureAddon.auras.end(), spellInfo->Id) != creatureAddon.auras.end())
            {
                LOG_ERROR("sql.sql", "Creature (GUID: {}) has duplicate aura (spell {}) in `auras` field in `creature_addon`.", guid, spellInfo->Id);
                continue;
            }

            if (spellInfo->GetDuration() > 0)
            {
                LOG_DEBUG/*ERROR*/("sql.sql", "Creature (Entry: {}) has temporary aura (spell {}) in `auras` field in `creature_template_addon`.", guid, spellInfo->Id);
                // continue;
            }

            creatureAddon.auras.push_back(spellInfo->Id);
        }

        if (creatureAddon.mount)
        {
            if (!sCreatureDisplayInfoStore.LookupEntry(creatureAddon.mount))
            {
                LOG_ERROR("sql.sql", "Creature (GUID: {}) has invalid displayInfoId ({}) for mount defined in `creature_addon`", guid, creatureAddon.mount);
                creatureAddon.mount = 0;
            }
        }

        if (!sEmotesStore.LookupEntry(creatureAddon.emote))
        {
            LOG_ERROR("sql.sql", "Creature (GUID: {}) has invalid emote ({}) defined in `creature_addon`.", guid, creatureAddon.emote);
            creatureAddon.emote = 0;
        }

        if (creatureAddon.visibilityDistanceType >= VisibilityDistanceType::Max)
        {
            LOG_ERROR("sql.sql", "Creature (GUID: {}) has invalid visibilityDistanceType ({}) defined in `creature_addon`.", guid, AsUnderlyingType(creatureAddon.visibilityDistanceType));
            creatureAddon.visibilityDistanceType = VisibilityDistanceType::Normal;
        }

        ++count;
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} Creature Addons in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

void ObjectMgr::LoadGameObjectAddons()
{
    uint32 oldMSTime = getMSTime();

    //                                               0     1                 2                 3                 4                 5                 6
    QueryResult result = WorldDatabase.Query("SELECT guid, parent_rotation0, parent_rotation1, parent_rotation2, parent_rotation3, invisibilityType, invisibilityValue FROM gameobject_addon");

    if (!result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 gameobject addon definitions. DB table `gameobject_addon` is empty.");
        LOG_INFO("server.loading", " ");
        return;
    }

    uint32 count = 0;
    do
    {
        Field* fields = result->Fetch();

        ObjectGuid::LowType guid = fields[0].Get<uint32>();

        GameObjectData const* goData = GetGameObjectData(guid);
        if (!goData)
        {
            LOG_ERROR("sql.sql", "GameObject (GUID: {}) does not exist but has a record in `gameobject_addon`", guid);
            continue;
        }

        GameObjectAddon& gameObjectAddon = _gameObjectAddonStore[guid];
        gameObjectAddon.ParentRotation = QuaternionData(fields[1].Get<float>(), fields[2].Get<float>(), fields[3].Get<float>(), fields[4].Get<float>());
        gameObjectAddon.invisibilityType = InvisibilityType(fields[5].Get<uint8>());
        gameObjectAddon.InvisibilityValue = fields[6].Get<uint32>();

        if (gameObjectAddon.invisibilityType >= TOTAL_INVISIBILITY_TYPES)
        {
            LOG_ERROR("sql.sql", "GameObject (GUID: {}) has invalid InvisibilityType in `gameobject_addon`", guid);
            gameObjectAddon.invisibilityType = INVISIBILITY_GENERAL;
            gameObjectAddon.InvisibilityValue = 0;
        }

        if (gameObjectAddon.invisibilityType && !gameObjectAddon.InvisibilityValue)
        {
            LOG_ERROR("sql.sql", "GameObject (GUID: {}) has InvisibilityType set but has no InvisibilityValue in `gameobject_addon`, set to 1", guid);
            gameObjectAddon.InvisibilityValue = 1;
        }

        if (!gameObjectAddon.ParentRotation.IsUnit())
        {
            LOG_ERROR("sql.sql", "GameObject (GUID: {}) has invalid parent rotation in `gameobject_addon`, set to default", guid);
            gameObjectAddon.ParentRotation = QuaternionData();
        }

        ++count;
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} Gameobject Addons in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

GameObjectAddon const* ObjectMgr::GetGameObjectAddon(ObjectGuid::LowType lowguid)
{
    GameObjectAddonContainer::const_iterator itr = _gameObjectAddonStore.find(lowguid);
    if (itr != _gameObjectAddonStore.end())
        return &(itr->second);

    return nullptr;
}

CreatureAddon const* ObjectMgr::GetCreatureAddon(ObjectGuid::LowType lowguid)
{
    CreatureAddonContainer::const_iterator itr = _creatureAddonStore.find(lowguid);
    if (itr != _creatureAddonStore.end())
        return &(itr->second);

    return nullptr;
}

CreatureAddon const* ObjectMgr::GetCreatureTemplateAddon(uint32 entry)
{
    CreatureAddonContainer::const_iterator itr = _creatureTemplateAddonStore.find(entry);
    if (itr != _creatureTemplateAddonStore.end())
        return &(itr->second);

    return nullptr;
}

CreatureMovementData const* ObjectMgr::GetCreatureMovementOverride(ObjectGuid::LowType spawnId) const
{
    return Acore::Containers::MapGetValuePtr(_creatureMovementOverrides, spawnId);
}

EquipmentInfo const* ObjectMgr::GetEquipmentInfo(uint32 entry, int8& id)
{
    EquipmentInfoContainer::const_iterator itr = _equipmentInfoStore.find(entry);
    if (itr == _equipmentInfoStore.end())
        return nullptr;

    if (itr->second.empty())
        return nullptr;

    if (id == -1) // select a random element
    {
        EquipmentInfoContainerInternal::const_iterator ritr = itr->second.begin();
        std::advance(ritr, urand(0u, itr->second.size() - 1));
        id = std::distance(itr->second.begin(), ritr) + 1;
        return &ritr->second;
    }
    else
    {
        EquipmentInfoContainerInternal::const_iterator itr2 = itr->second.find(id);
        if (itr2 != itr->second.end())
            return &itr2->second;
    }

    return nullptr;
}

void ObjectMgr::LoadEquipmentTemplates()
{
    uint32 oldMSTime = getMSTime();

    //                                                 0         1       2       3       4
    QueryResult result = WorldDatabase.Query("SELECT CreatureID, ID, ItemID1, ItemID2, ItemID3 FROM creature_equip_template");

    if (!result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 creature equipment templates. DB table `creature_equip_template` is empty!");
        LOG_INFO("server.loading", " ");
        return;
    }

    uint32 count = 0;
    do
    {
        Field* fields = result->Fetch();

        uint32 entry = fields[0].Get<uint32>();

        if (!GetCreatureTemplate(entry))
        {
            LOG_ERROR("sql.sql", "Creature template (CreatureID: {}) does not exist but has a record in `creature_equip_template`", entry);
            continue;
        }

        uint8 id = fields[1].Get<uint8>();
        if (!id)
        {
            LOG_ERROR("sql.sql", "Creature equipment template with id 0 found for creature {}, skipped.", entry);
            continue;
        }

        EquipmentInfo& equipmentInfo = _equipmentInfoStore[entry][id];

        equipmentInfo.ItemEntry[0] = fields[2].Get<uint32>();
        equipmentInfo.ItemEntry[1] = fields[3].Get<uint32>();
        equipmentInfo.ItemEntry[2] = fields[4].Get<uint32>();

        for (uint8 i = 0; i < MAX_EQUIPMENT_ITEMS; ++i)
        {
            if (!equipmentInfo.ItemEntry[i])
                continue;

            ItemEntry const* dbcItem = sItemStore.LookupEntry(equipmentInfo.ItemEntry[i]);

            if (!dbcItem)
            {
                LOG_ERROR("sql.sql", "Unknown item (ID={}) in creature_equip_template.ItemID{} for CreatureID = {} and ID = {}, forced to 0.",
                                 equipmentInfo.ItemEntry[i], i + 1, entry, id);
                equipmentInfo.ItemEntry[i] = 0;
                continue;
            }

            if (dbcItem->InventoryType != INVTYPE_WEAPON &&
                dbcItem->InventoryType != INVTYPE_SHIELD &&
                dbcItem->InventoryType != INVTYPE_RANGED &&
                dbcItem->InventoryType != INVTYPE_2HWEAPON &&
                dbcItem->InventoryType != INVTYPE_WEAPONMAINHAND &&
                dbcItem->InventoryType != INVTYPE_WEAPONOFFHAND &&
                dbcItem->InventoryType != INVTYPE_HOLDABLE &&
                dbcItem->InventoryType != INVTYPE_THROWN &&
                dbcItem->InventoryType != INVTYPE_RANGEDRIGHT)
            {
                LOG_ERROR("sql.sql", "Item (ID={}) in creature_equip_template.ItemID{} for CreatureID = {} and ID = {} is not equipable in a hand, forced to 0.",
                                 equipmentInfo.ItemEntry[i], i + 1, entry, id);
                equipmentInfo.ItemEntry[i] = 0;
            }
        }

        ++count;
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} Equipment Templates in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

void ObjectMgr::LoadCreatureMovementOverrides()
{
    uint32 oldMSTime = getMSTime();

    _creatureMovementOverrides.clear();

    // Load the data from creature_movement_override and if NULL fallback to creature_template_movement
    QueryResult result = WorldDatabase.Query("SELECT cmo.SpawnId,"
                                             "COALESCE(cmo.Ground, ctm.Ground),"
                                             "COALESCE(cmo.Swim, ctm.Swim),"
                                             "COALESCE(cmo.Flight, ctm.Flight),"
                                             "COALESCE(cmo.Rooted, ctm.Rooted),"
                                             "COALESCE(cmo.Chase, ctm.Chase),"
                                             "COALESCE(cmo.Random, ctm.Random),"
                                             "COALESCE(cmo.InteractionPauseTimer, ctm.InteractionPauseTimer) "
                                             "FROM creature_movement_override AS cmo "
                                             "LEFT JOIN creature AS c ON c.guid = cmo.SpawnId "
                                             "LEFT JOIN creature_template_movement AS ctm ON ctm.CreatureId = c.id");
    if (!result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 creature movement overrides. DB table `creature_movement_override` is empty!");
        return;
    }

    do
    {
        Field*              fields  = result->Fetch();
        ObjectGuid::LowType spawnId = fields[0].Get<uint32>();
        if (!GetCreatureData(spawnId))
        {
            LOG_ERROR("sql.sql", "Creature (GUID: {}) does not exist but has a record in `creature_movement_override`", spawnId);
            continue;
        }

        CreatureMovementData& movement = _creatureMovementOverrides[spawnId];
        if (!fields[1].IsNull())
        {
            movement.Ground = static_cast<CreatureGroundMovementType>(fields[1].Get<uint8>());
        }

        if (!fields[2].IsNull())
        {
            movement.Swim = fields[2].Get<bool>();
        }

        if (!fields[3].IsNull())
        {
            movement.Flight = static_cast<CreatureFlightMovementType>(fields[3].Get<uint8>());
        }

        if (!fields[4].IsNull())
        {
            movement.Rooted = fields[4].Get<bool>();
        }

        if (!fields[5].IsNull())
        {
            movement.Chase = static_cast<CreatureChaseMovementType>(fields[5].Get<uint8>());
        }

        if (!fields[6].IsNull())
        {
            movement.Random = static_cast<CreatureRandomMovementType>(fields[6].Get<uint8>());
        }

        if (!fields[7].IsNull())
        {
            movement.InteractionPauseTimer = fields[7].Get<uint32>();
        }

        CheckCreatureMovement("creature_movement_override", spawnId, movement);
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} Movement Overrides in {} ms", _creatureMovementOverrides.size(), GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

CreatureModelInfo const* ObjectMgr::GetCreatureModelInfo(uint32 modelId) const
{
    CreatureModelContainer::const_iterator itr = _creatureModelStore.find(modelId);
    if (itr != _creatureModelStore.end())
        return &(itr->second);

    return nullptr;
}

CreatureModel const* ObjectMgr::ChooseDisplayId(CreatureTemplate const* cinfo, CreatureData const* data /*= nullptr*/)
{
    // Load creature model (display id)
    if (data && data->displayid)
        if (CreatureModel const* model = cinfo->GetModelWithDisplayId(data->displayid))
            return model;

    if (!cinfo->HasFlagsExtra(CREATURE_FLAG_EXTRA_TRIGGER))
        if (CreatureModel const* model = cinfo->GetRandomValidModel())
            return model;

    // Triggers by default receive the invisible model
    return cinfo->GetFirstInvisibleModel();
}

void ObjectMgr::ChooseCreatureFlags(CreatureTemplate const* cinfo, uint32& npcflag, uint32& unit_flags, uint32& dynamicflags, CreatureData const* data /*= nullptr*/)
{
    npcflag = cinfo->npcflag;
    unit_flags = cinfo->unit_flags;
    dynamicflags = cinfo->dynamicflags;

    if (data)
    {
        if (data->npcflag)
            npcflag = data->npcflag;

        if (data->unit_flags)
            unit_flags = data->unit_flags;

        if (data->dynamicflags)
            dynamicflags = data->dynamicflags;
    }
}

CreatureModelInfo const* ObjectMgr::GetCreatureModelRandomGender(CreatureModel* model, CreatureTemplate const* creatureTemplate) const
{
    CreatureModelInfo const* modelInfo = GetCreatureModelInfo(model->CreatureDisplayID);
    if (!modelInfo)
        return nullptr;

    // If a model for another gender exists, 50% chance to use it
    if (modelInfo->modelid_other_gender != 0 && urand(0, 1) == 0)
    {
        CreatureModelInfo const* minfo_tmp = GetCreatureModelInfo(modelInfo->modelid_other_gender);
        if (!minfo_tmp)
            LOG_ERROR("sql.sql", "Model (Entry: {}) has modelid_other_gender {} not found in table `creature_model_info`. ", model->CreatureDisplayID, modelInfo->modelid_other_gender);
        else
        {
            // Model ID changed
            model->CreatureDisplayID = modelInfo->modelid_other_gender;
            if (creatureTemplate)
            {
                auto itr = std::find_if(creatureTemplate->Models.begin(), creatureTemplate->Models.end(), [&](CreatureModel const& templateModel)
                    {
                        return templateModel.CreatureDisplayID == modelInfo->modelid_other_gender;
                    });
                if (itr != creatureTemplate->Models.end())
                    *model = *itr;
            }
            return minfo_tmp;
        }
    }

    return modelInfo;
}

void ObjectMgr::LoadCreatureModelInfo()
{
    uint32 oldMSTime = getMSTime();

    //                                                   0             1             2          3               4
    QueryResult result = WorldDatabase.Query("SELECT DisplayID, BoundingRadius, CombatReach, Gender, DisplayID_Other_Gender FROM creature_model_info");

    if (!result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 creature model definitions. DB table `creature_model_info` is empty.");
        LOG_INFO("server.loading", " ");
        return;
    }

    _creatureModelStore.rehash(result->GetRowCount());
    uint32 count = 0;

    // List of ModelDataIDs that use Invisible models
    uint32 triggerCreatureModelDataID[14] = { 1731, 1752, 2206, 2296, 2372, 2382, 2481, 2512, 2513, 2611, 2636, 2790, 3230, 3274 };

    do
    {
        Field* fields = result->Fetch();

        uint32 displayId = fields[0].Get<uint32>();
        CreatureDisplayInfoEntry const* creatureDisplay = sCreatureDisplayInfoStore.LookupEntry(displayId);
        uint32 modelId = fields[0].Get<uint32>();

        CreatureModelInfo& modelInfo = _creatureModelStore[modelId];

        modelInfo.bounding_radius      = fields[1].Get<float>();
        modelInfo.combat_reach         = fields[2].Get<float>();
        modelInfo.gender               = fields[3].Get<uint8>();
        modelInfo.modelid_other_gender = fields[4].Get<uint32>();
        modelInfo.is_trigger           = false;

        // Checks

        if (!sCreatureDisplayInfoStore.LookupEntry(modelId))
            LOG_ERROR("sql.sql", "Table `creature_model_info` has model for not existed display id ({}).", modelId);

        if (modelInfo.gender > GENDER_NONE)
        {
            LOG_ERROR("sql.sql", "Table `creature_model_info` has wrong gender ({}) for display id ({}).", uint32(modelInfo.gender), modelId);
            modelInfo.gender = GENDER_MALE;
        }

        if (modelInfo.modelid_other_gender && !sCreatureDisplayInfoStore.LookupEntry(modelInfo.modelid_other_gender))
        {
            LOG_ERROR("sql.sql", "Table `creature_model_info` has not existed alt.gender model ({}) for existed display id ({}).", modelInfo.modelid_other_gender, modelId);
            modelInfo.modelid_other_gender = 0;
        }

        if (modelInfo.combat_reach < 0.1f)
            modelInfo.combat_reach = DEFAULT_COMBAT_REACH;

        if (CreatureModelDataEntry const* modelData = creatureDisplay ? sCreatureModelDataStore.LookupEntry(creatureDisplay->ModelID) : nullptr)
        {
            for (uint32 i = 0; i < 14; i++)
            {
                if (modelData->ID == triggerCreatureModelDataID[i])
                {
                    modelInfo.is_trigger = true;
                    break;
                }
            }
        }

        ++count;
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} Creature Model Based Info in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

void ObjectMgr::LoadPlayerTotemModels()
{
    uint32 oldMSTime = getMSTime();

    QueryResult result = WorldDatabase.Query("SELECT TotemID, RaceID, ModelID from player_totem_model");

    if (!result)
    {
        LOG_INFO("server.loading", ">> Loaded 0 player totem model records. DB table `player_totem_model` is empty.");
        return;
    }

    uint32 count = 0;
    do
    {
        Field* fields = result->Fetch();

        SummonSlot totemSlot = SummonSlot(fields[0].Get<uint8>());
        uint8 race = fields[1].Get<uint8>();
        uint32 displayId = fields[2].Get<uint32>();

        if (totemSlot < SUMMON_SLOT_TOTEM_FIRE || totemSlot >= MAX_TOTEM_SLOT)
        {
            LOG_ERROR("sql.sql", "Wrong TotemSlot {} in `player_totem_model` table, skipped.", totemSlot);
            continue;
        }

        ChrRacesEntry const* raceEntry = sChrRacesStore.LookupEntry(race);
        if (!raceEntry)
        {
            LOG_ERROR("sql.sql", "Race {} defined in `player_totem_model` does not exists, skipped.", uint32(race));
            continue;
        }

        CreatureDisplayInfoEntry const* displayEntry = sCreatureDisplayInfoStore.LookupEntry(displayId);
        if (!displayEntry)
        {
            LOG_ERROR("sql.sql", "TotemSlot: {} defined in `player_totem_model` has non-existing model ({}), skipped.", totemSlot, displayId);
            continue;
        }

        _playerTotemModel[std::make_pair(totemSlot, Races(race))] = displayId;
        ++count;
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} player totem model records in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

uint32 ObjectMgr::GetModelForTotem(SummonSlot totemSlot, Races race) const
{
    auto itr = _playerTotemModel.find(std::make_pair(totemSlot, race));
    if (itr != _playerTotemModel.end())
        return itr->second;

    LOG_ERROR("misc", "TotemSlot {} with RaceID ({}) have no totem model data defined, set to default model.", totemSlot, race);
    return 0;
}

void ObjectMgr::LoadPlayerShapeshiftModels()
{
    uint32 oldMSTime = getMSTime();

    QueryResult result = WorldDatabase.Query("SELECT ShapeshiftID, RaceID, CustomizationID, GenderID, ModelID from player_shapeshift_model");

    if (!result)
    {
        LOG_INFO("server.loading", ">> Loaded 0 player shapeshift model records. DB table `player_shapeshift_model` is empty.");
        return;
    }

    uint32 count = 0;
    do
    {
        Field* fields = result->Fetch();

        ShapeshiftForm shapeshiftForm = ShapeshiftForm(fields[0].Get<uint8>());
        uint8 race = fields[1].Get<uint8>();
        uint8 customizationID = fields[2].Get<uint8>();
        uint8 genderID = Gender(fields[3].Get<uint8>());
        uint32 modelId = fields[4].Get<uint32>();

        ChrRacesEntry const* raceEntry = sChrRacesStore.LookupEntry(race);
        if (!raceEntry)
        {
            LOG_ERROR("sql.sql", "Race {} defined in `player_shapeshift_model` does not exists, skipped.", uint32(race));
            continue;
        }

        CreatureDisplayInfoEntry const* displayEntry = sCreatureDisplayInfoStore.LookupEntry(modelId);
        if (!displayEntry)
        {
            LOG_ERROR("sql.sql", "ShapeshiftForm: {}, Race: {} defined in `player_shapeshift_model` has non-existing model ({}), skipped.", shapeshiftForm, race, modelId);
            continue;
        }

        _playerShapeshiftModel[std::make_tuple(shapeshiftForm, race, customizationID, genderID)] = modelId;
        ++count;
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} player totem model records in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

uint32 ObjectMgr::GetModelForShapeshift(ShapeshiftForm form, Player* player) const
{
    uint8 customizationID;

    if (player->GetTeamId() == TEAM_ALLIANCE)
        customizationID = player->GetByteValue(PLAYER_BYTES, 3); // Use Hair Color
    else
        customizationID = player->GetByteValue(PLAYER_BYTES, 0); // Use Skin Color

    // getGender() tracks the active display model; real gender lives in PLAYER_BYTES_3
    uint8 gender = player->GetByteValue(PLAYER_BYTES_3, PLAYER_BYTES_3_OFFSET_GENDER);

    auto itr = _playerShapeshiftModel.find(std::make_tuple(form, player->getRace(), customizationID, gender));
    if (itr != _playerShapeshiftModel.end())
        return itr->second; // Explicit combination

    itr = _playerShapeshiftModel.find(std::make_tuple(form, player->getRace(), customizationID, GENDER_NONE));
    if (itr != _playerShapeshiftModel.end())
        return itr->second; // Combination applied to both genders

    itr = _playerShapeshiftModel.find(std::make_tuple(form, player->getRace(), 255, gender));
    if (itr != _playerShapeshiftModel.end())
        return itr->second; // Default gender-dependent model

    itr = _playerShapeshiftModel.find(std::make_tuple(form, player->getRace(), 255, GENDER_NONE));
    if (itr != _playerShapeshiftModel.end())
        return itr->second; // Last resort

    LOG_DEBUG("entities.player", "ShapeshiftForm {} with RaceID ({}) have no shapeshift model data defined, using fallback data.", form, player->getRace());
    return 0;
}

void ObjectMgr::LoadLinkedRespawn()
{
    uint32 oldMSTime = getMSTime();

    _linkedRespawnStore.clear();
    //                                                 0        1          2
    QueryResult result = WorldDatabase.Query("SELECT guid, linkedGuid, linkType FROM linked_respawn ORDER BY guid ASC");

    if (!result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 linked respawns. DB table `linked_respawn` is empty.");
        LOG_INFO("server.loading", " ");
        return;
    }

    do
    {
        Field* fields = result->Fetch();

        ObjectGuid::LowType guidLow = fields[0].Get<uint32>();
        ObjectGuid::LowType linkedGuidLow = fields[1].Get<uint32>();
        uint8  linkType = fields[2].Get<uint8>();

        ObjectGuid guid, linkedGuid;
        bool error = false;
        switch (linkType)
        {
            case CREATURE_TO_CREATURE:
                {
                    CreatureData const* slave = GetCreatureData(guidLow);
                    if (!slave)
                    {
                        LOG_ERROR("sql.sql", "LinkedRespawn: Creature (guid) {} not found in creature table", guidLow);
                        error = true;
                        break;
                    }

                    CreatureData const* master = GetCreatureData(linkedGuidLow);
                    if (!master)
                    {
                        LOG_ERROR("sql.sql", "LinkedRespawn: Creature (linkedGuid) {} not found in creature table", linkedGuidLow);
                        error = true;
                        break;
                    }

                    MapEntry const* const map = sMapStore.LookupEntry(master->mapid);
                    if (!map || !map->Instanceable() || (master->mapid != slave->mapid))
                    {
                        LOG_ERROR("sql.sql", "LinkedRespawn: Creature '{}' linking to Creature '{}' on an unpermitted map.", guidLow, linkedGuidLow);
                        error = true;
                        break;
                    }

                    if (!(master->spawnMask & slave->spawnMask))  // they must have a possibility to meet (normal/heroic difficulty)
                    {
                        LOG_ERROR("sql.sql", "LinkedRespawn: Creature '{}' linking to Creature '{}' with not corresponding spawnMask", guidLow, linkedGuidLow);
                        error = true;
                        break;
                    }

                    guid = ObjectGuid::Create<HighGuid::Creature>(slave->mapid, slave->id, guidLow);
                    linkedGuid = ObjectGuid::Create<HighGuid::Creature>(master->mapid, master->id, linkedGuidLow);
                    break;
                }
            case CREATURE_TO_GO:
                {
                    CreatureData const* slave = GetCreatureData(guidLow);
                    if (!slave)
                    {
                        LOG_ERROR("sql.sql", "LinkedRespawn: Creature (guid) {} not found in creature table", guidLow);
                        error = true;
                        break;
                    }

                    GameObjectData const* master = GetGameObjectData(linkedGuidLow);
                    if (!master)
                    {
                        LOG_ERROR("sql.sql", "LinkedRespawn: Gameobject (linkedGuid) {} not found in gameobject table", linkedGuidLow);
                        error = true;
                        break;
                    }

                    MapEntry const* const map = sMapStore.LookupEntry(master->mapid);
                    if (!map || !map->Instanceable() || (master->mapid != slave->mapid))
                    {
                        LOG_ERROR("sql.sql", "LinkedRespawn: Creature '{}' linking to Gameobject '{}' on an unpermitted map.", guidLow, linkedGuidLow);
                        error = true;
                        break;
                    }

                    if (!(master->spawnMask & slave->spawnMask))  // they must have a possibility to meet (normal/heroic difficulty)
                    {
                        LOG_ERROR("sql.sql", "LinkedRespawn: Creature '{}' linking to Gameobject '{}' with not corresponding spawnMask", guidLow, linkedGuidLow);
                        error = true;
                        break;
                    }

                    guid = ObjectGuid::Create<HighGuid::Creature>(slave->mapid, slave->id, guidLow);
                    linkedGuid = ObjectGuid::Create<HighGuid::GameObject>(master->mapid, master->id, linkedGuidLow);
                    break;
                }
            case GO_TO_GO:
                {
                    GameObjectData const* slave = GetGameObjectData(guidLow);
                    if (!slave)
                    {
                        LOG_ERROR("sql.sql", "LinkedRespawn: Gameobject (guid) {} not found in gameobject table", guidLow);
                        error = true;
                        break;
                    }

                    GameObjectData const* master = GetGameObjectData(linkedGuidLow);
                    if (!master)
                    {
                        LOG_ERROR("sql.sql", "LinkedRespawn: Gameobject (linkedGuid) {} not found in gameobject table", linkedGuidLow);
                        error = true;
                        break;
                    }

                    MapEntry const* const map = sMapStore.LookupEntry(master->mapid);
                    if (!map || !map->Instanceable() || (master->mapid != slave->mapid))
                    {
                        LOG_ERROR("sql.sql", "LinkedRespawn: Gameobject '{}' linking to Gameobject '{}' on an unpermitted map.", guidLow, linkedGuidLow);
                        error = true;
                        break;
                    }

                    if (!(master->spawnMask & slave->spawnMask))  // they must have a possibility to meet (normal/heroic difficulty)
                    {
                        LOG_ERROR("sql.sql", "LinkedRespawn: Gameobject '{}' linking to Gameobject '{}' with not corresponding spawnMask", guidLow, linkedGuidLow);
                        error = true;
                        break;
                    }

                    guid = ObjectGuid::Create<HighGuid::GameObject>(slave->mapid, slave->id, guidLow);
                    linkedGuid = ObjectGuid::Create<HighGuid::GameObject>(master->mapid, master->id, linkedGuidLow);
                    break;
                }
            case GO_TO_CREATURE:
                {
                    GameObjectData const* slave = GetGameObjectData(guidLow);
                    if (!slave)
                    {
                        LOG_ERROR("sql.sql", "LinkedRespawn: Gameobject (guid) {} not found in gameobject table", guidLow);
                        error = true;
                        break;
                    }

                    CreatureData const* master = GetCreatureData(linkedGuidLow);
                    if (!master)
                    {
                        LOG_ERROR("sql.sql", "LinkedRespawn: Creature (linkedGuid) {} not found in creature table", linkedGuidLow);
                        error = true;
                        break;
                    }

                    MapEntry const* const map = sMapStore.LookupEntry(master->mapid);
                    if (!map || !map->Instanceable() || (master->mapid != slave->mapid))
                    {
                        LOG_ERROR("sql.sql", "LinkedRespawn: Gameobject '{}' linking to Creature '{}' on an unpermitted map.", guidLow, linkedGuidLow);
                        error = true;
                        break;
                    }

                    if (!(master->spawnMask & slave->spawnMask))  // they must have a possibility to meet (normal/heroic difficulty)
                    {
                        LOG_ERROR("sql.sql", "LinkedRespawn: Gameobject '{}' linking to Creature '{}' with not corresponding spawnMask", guidLow, linkedGuidLow);
                        error = true;
                        break;
                    }

                    guid = ObjectGuid::Create<HighGuid::GameObject>(slave->mapid, slave->id, guidLow);
                    linkedGuid = ObjectGuid::Create<HighGuid::Creature>(master->mapid, master->id, linkedGuidLow);
                    break;
                }
        }

        if (!error)
            _linkedRespawnStore[guid] = linkedGuid;
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} Linked Respawns In {} ms", uint64(_linkedRespawnStore.size()), GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

bool ObjectMgr::SetCreatureLinkedRespawn(ObjectGuid::LowType guidLow, ObjectGuid::LowType linkedGuidLow)
{
    if (!guidLow)
        return false;

    CreatureData const* master = GetCreatureData(guidLow);
    ObjectGuid guid = ObjectGuid::Create<HighGuid::Creature>(master->mapid, master->id, guidLow);

    if (!linkedGuidLow) // we're removing the linking
    {
        _linkedRespawnStore.erase(guid);
        WorldDatabasePreparedStatement* stmt = WorldDatabase.GetPreparedStatement(WORLD_DEL_CRELINKED_RESPAWN);
        stmt->SetData(0, guidLow);
        WorldDatabase.Execute(stmt);
        return true;
    }

    CreatureData const* slave = GetCreatureData(linkedGuidLow);
    if (!slave)
    {
        LOG_ERROR("sql.sql", "Creature '{}' linking to non-existent creature '{}'.", guidLow, linkedGuidLow);
        return false;
    }

    MapEntry const* map = sMapStore.LookupEntry(master->mapid);
    if (!map || !map->Instanceable() || (master->mapid != slave->mapid))
    {
        LOG_ERROR("sql.sql", "Creature '{}' linking to '{}' on an unpermitted map.", guidLow, linkedGuidLow);
        return false;
    }

    if (!(master->spawnMask & slave->spawnMask))  // they must have a possibility to meet (normal/heroic difficulty)
    {
        LOG_ERROR("sql.sql", "LinkedRespawn: Creature '{}' linking to '{}' with not corresponding spawnMask", guidLow, linkedGuidLow);
        return false;
    }

    ObjectGuid linkedGuid = ObjectGuid::Create<HighGuid::Creature>(slave->mapid, slave->id, linkedGuidLow);

    _linkedRespawnStore[guid] = linkedGuid;
    WorldDatabasePreparedStatement* stmt = WorldDatabase.GetPreparedStatement(WORLD_REP_CREATURE_LINKED_RESPAWN);
    stmt->SetData(0, guidLow);
    stmt->SetData(1, linkedGuidLow);
    WorldDatabase.Execute(stmt);
    return true;
}

void ObjectMgr::LoadTempSummons()
{
    uint32 oldMSTime = getMSTime();

    //                                                    0           1             2        3      4           5           6           7            8           9
    QueryResult result = WorldDatabase.Query("SELECT summonerId, summonerType, groupId, entry, position_x, position_y, position_z, orientation, summonType, summonTime FROM creature_summon_groups");

    if (!result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 temp summons. DB table `creature_summon_groups` is empty.");
        return;
    }

    uint32 count = 0;
    do
    {
        Field* fields = result->Fetch();

        uint32 summonerId               = fields[0].Get<uint32>();
        SummonerType summonerType       = SummonerType(fields[1].Get<uint8>());
        uint8 group                     = fields[2].Get<uint8>();

        switch (summonerType)
        {
            case SUMMONER_TYPE_CREATURE:
                if (!GetCreatureTemplate(summonerId))
                {
                    LOG_ERROR("sql.sql", "Table `creature_summon_groups` has summoner with non existing entry {} for creature summoner type, skipped.", summonerId);
                    continue;
                }
                break;
            case SUMMONER_TYPE_GAMEOBJECT:
                if (!GetGameObjectTemplate(summonerId))
                {
                    LOG_ERROR("sql.sql", "Table `creature_summon_groups` has summoner with non existing entry {} for gameobject summoner type, skipped.", summonerId);
                    continue;
                }
                break;
            case SUMMONER_TYPE_MAP:
                if (!sMapStore.LookupEntry(summonerId))
                {
                    LOG_ERROR("sql.sql", "Table `creature_summon_groups` has summoner with non existing entry {} for map summoner type, skipped.", summonerId);
                    continue;
                }
                break;
            default:
                LOG_ERROR("sql.sql", "Table `creature_summon_groups` has unhandled summoner type {} for summoner {}, skipped.", summonerType, summonerId);
                continue;
        }

        TempSummonData data;
        data.entry                      = fields[3].Get<uint32>();

        if (!GetCreatureTemplate(data.entry))
        {
            LOG_ERROR("sql.sql", "Table `creature_summon_groups` has creature in group [Summoner ID: {}, Summoner Type: {}, Group ID: {}] with non existing creature entry {}, skipped.", summonerId, summonerType, group, data.entry);
            continue;
        }

        float posX                      = fields[4].Get<float>();
        float posY                      = fields[5].Get<float>();
        float posZ                      = fields[6].Get<float>();
        float orientation               = fields[7].Get<float>();

        data.pos.Relocate(posX, posY, posZ, orientation);

        data.type                       = TempSummonType(fields[8].Get<uint8>());

        if (data.type > TEMPSUMMON_MANUAL_DESPAWN)
        {
            LOG_ERROR("sql.sql", "Table `creature_summon_groups` has unhandled temp summon type {} in group [Summoner ID: {}, Summoner Type: {}, Group ID: {}] for creature entry {}, skipped.", data.type, summonerId, summonerType, group, data.entry);
            continue;
        }

        data.time                       = fields[9].Get<uint32>();

        TempSummonGroupKey key(summonerId, summonerType, group);
        _tempSummonDataStore[key].push_back(data);

        ++count;
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} Temporary Summons in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

void ObjectMgr::LoadGameObjectSummons()
{
    uint32 oldMSTime = getMSTime();

    _goSummonDataStore.clear();

    //                                                    0           1             2        3      4           5           6           7            8          9          10         11           12
    QueryResult result = WorldDatabase.Query("SELECT summonerId, summonerType, groupId, entry, position_x, position_y, position_z, orientation, rotation0, rotation1, rotation2, rotation3, respawnTime FROM gameobject_summon_groups");

    if (!result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 gameobject summons. DB table `gameobject_summon_groups` is empty.");
        return;
    }

    uint32 count = 0;
    do
    {
        Field* fields = result->Fetch();

        uint32 summonerId               = fields[0].Get<uint32>();
        SummonerType summonerType       = SummonerType(fields[1].Get<uint8>());
        uint8 group                     = fields[2].Get<uint8>();

        switch (summonerType)
        {
            case SUMMONER_TYPE_CREATURE:
                if (!GetCreatureTemplate(summonerId))
                {
                    LOG_ERROR("sql.sql", "Table `gameobject_summon_groups` has summoner with non existing entry {} for creature summoner type, skipped.", summonerId);
                    continue;
                }
                break;
            case SUMMONER_TYPE_GAMEOBJECT:
                if (!GetGameObjectTemplate(summonerId))
                {
                    LOG_ERROR("sql.sql", "Table `gameobject_summon_groups` has summoner with non existing entry {} for gameobject summoner type, skipped.", summonerId);
                    continue;
                }
                break;
            case SUMMONER_TYPE_MAP:
                if (!sMapStore.LookupEntry(summonerId))
                {
                    LOG_ERROR("sql.sql", "Table `gameobject_summon_groups` has summoner with non existing entry {} for map summoner type, skipped.", summonerId);
                    continue;
                }
                break;
            default:
                LOG_ERROR("sql.sql", "Table `gameobject_summon_groups` has unhandled summoner type {} for summoner {}, skipped.", summonerType, summonerId);
                continue;
        }

        GameObjectSummonData data;
        data.entry                      = fields[3].Get<uint32>();

        if (!GetGameObjectTemplate(data.entry))
        {
            LOG_ERROR("sql.sql", "Table `gameobject_summon_groups` has gameobject in group [Summoner ID: {}, Summoner Type: {}, Group ID: {}] with non existing gameobject entry {}, skipped.", summonerId, summonerType, group, data.entry);
            continue;
        }

        float posX                      = fields[4].Get<float>();
        float posY                      = fields[5].Get<float>();
        float posZ                      = fields[6].Get<float>();
        float orientation               = fields[7].Get<float>();

        data.pos.Relocate(posX, posY, posZ, orientation);

        data.rot                        = G3D::Quat(fields[8].Get<float>(), fields[9].Get<float>(), fields[10].Get<float>(), fields[11].Get<float>());
        data.respawnTime                = fields[12].Get<uint32>();

        TempSummonGroupKey key(summonerId, summonerType, group);
        _goSummonDataStore[key].push_back(data);

        ++count;
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} Gameobject Summons in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

namespace
{
// Worlds imported before #348 collapsed a TDB phase group into PhaseId (only when the source
// PhaseId was 0). Move values that are group-only back; ids that are both a phase and a group
// cannot be disambiguated from the collapsed column and need the clean-world rebuild.
void MigrateLegacySpawnPhase(SpawnData& data, char const* table, uint32 spawnId)
{
    if (!data.phaseId || IsValidPhaseId(data.phaseId))
        return;

    if (IsValidPhaseGroupId(data.phaseId) && !data.phaseGroup)
    {
        LOG_DEBUG("sql.sql", "Table `{}` has spawn {} with phase group {} in `PhaseId`; moved to `PhaseGroup` (#348).",
                  table, spawnId, data.phaseId);
        data.phaseGroup = data.phaseId;
        data.phaseId = 0;
        return;
    }

    LOG_ERROR("sql.sql", "Table `{}` has spawn {} with non-existing phase id {} in `PhaseId`, set to 0.",
              table, spawnId, data.phaseId);
    data.phaseId = 0;
}
}

void ObjectMgr::LoadCreatures()
{
    uint32 oldMSTime = getMSTime();

    //                                                     0         1   2        3            4           5           6            7              8             9
    QueryResult result = WorldDatabase.Query("SELECT creature.guid, id, map, equipment_id, position_x, position_y, position_z, orientation, spawntimesecs, wander_distance, "
                         //      10            11       12          13           14         15         16          17             18                 19                    20
                         "currentwaypoint, curhealth, curmana, MovementType, spawnMask, phaseMask, eventEntry, pool_entry, creature.npcflag, creature.unit_flags, creature.dynamicflags, "
                         //       21                     22
                         "creature.ScriptName, creature.PhaseId, creature.PhaseGroup "
                         "FROM creature "
                         "LEFT OUTER JOIN game_event_creature ON creature.guid = game_event_creature.guid "
                         "LEFT OUTER JOIN pool_creature ON creature.guid = pool_creature.guid");

    if (!result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 creatures. DB table `creature` is empty.");
        LOG_INFO("server.loading", " ");
        return;
    }

    if (sWorld->getBoolConfig(CONFIG_CALCULATE_CREATURE_ZONE_AREA_DATA))
        LOG_INFO("server.loading", "Calculating zone and area fields. This may take a moment...");

    // Build single time for check spawnmask
    std::map<uint32, uint32> spawnMasks;
    for (uint32 i = 0; i < sMapStore.GetNumRows(); ++i)
        if (sMapStore.LookupEntry(i))
            for (int k = 0; k < MAX_DIFFICULTY; ++k)
                if (GetMapDifficultyData(i, Difficulty(k)))
                    spawnMasks[i] |= (1 << k);

    _creatureDataStore.rehash(result->GetRowCount());
    uint32 count = 0;
    do
    {
        Field* fields = result->Fetch();

        ObjectGuid::LowType spawnId     = fields[0].Get<uint32>();
        uint32 creatureId               = fields[1].Get<uint32>();

        CreatureTemplate const* cInfo = GetCreatureTemplate(creatureId);
        if (!cInfo)
        {
            LOG_ERROR("sql.sql", "Table `creature` has creature (SpawnId: {}) with non existing creature entry {} in `id` field, skipped.", spawnId, creatureId);
            continue;
        }
        CreatureData& data      = _creatureDataStore[spawnId];
        data.spawnId            = spawnId;
        data.id                 = creatureId;
        data.mapid              = fields[2].Get<uint16>();
        data.equipmentId        = fields[3].Get<int8>();
        data.posX               = fields[4].Get<float>();
        data.posY               = fields[5].Get<float>();
        data.posZ               = fields[6].Get<float>();
        data.orientation        = fields[7].Get<float>();
        data.spawntimesecs      = fields[8].Get<uint32>();
        data.wander_distance    = fields[9].Get<float>();
        data.currentwaypoint    = fields[10].Get<uint32>();
        data.curhealth          = fields[11].Get<uint32>();
        data.curmana            = fields[12].Get<uint32>();
        data.movementType       = fields[13].Get<uint8>();
        data.spawnMask          = fields[14].Get<uint8>();
        data.phaseMask          = fields[15].Get<uint32>();
        int16 gameEvent         = fields[16].Get<int16>();
        data.poolId             = fields[17].Get<uint32>();
        data.npcflag            = fields[18].Get<uint32>();
        data.unit_flags         = fields[19].Get<uint32>();
        data.dynamicflags       = fields[20].Get<uint32>();
        data.ScriptId           = GetScriptId(fields[21].Get<std::string>());
        data.phaseId            = fields[22].Get<uint32>();
        data.phaseGroup         = fields[23].Get<uint32>();
        data.spawnGroupId       = 0;

        MigrateLegacySpawnPhase(data, "creature", spawnId);
        if (data.phaseGroup && !IsValidPhaseGroupId(data.phaseGroup))
        {
            LOG_ERROR("sql.sql", "Table `creature` has creature (SpawnId: {}) with non-existing phase group id {} in `PhaseGroup` field, set to 0.", spawnId, data.phaseGroup);
            data.phaseGroup = 0;
        }

        if (!data.ScriptId)
            data.ScriptId = cInfo->ScriptID;

        MapEntry const* mapEntry = sMapStore.LookupEntry(data.mapid);
        if (!mapEntry)
        {
            LOG_ERROR("sql.sql", "Table `creature` have creature (SpawnId: {}) that spawned at not existed map (Id: {}), skipped.", spawnId, data.mapid);
            continue;
        }

        // pussywizard: 7 days means no reaspawn, so set it to 14 days, because manual id reset may be late
        if (mapEntry->IsRaid() && data.spawntimesecs >= 7 * DAY && data.spawntimesecs < 14 * DAY)
            data.spawntimesecs = 14 * DAY;

        // Skip spawnMask check for transport maps
        if (!_transportMaps.count(data.mapid))
        {
            if (data.spawnMask & ~spawnMasks[data.mapid])
                LOG_ERROR("sql.sql", "Table `creature` have creature (SpawnId: {}) that have wrong spawn mask {} including not supported difficulty modes for map (Id: {}).",
                    spawnId, data.spawnMask, data.mapid);
        }
        else
            data.spawnGroupId = 1; // force compatibility group for transport spawns

        bool ok = true;
        for (uint32 diff = 0; diff < MAX_DIFFICULTY - 1 && ok; ++diff)
        {
            if (_difficultyEntries[diff].find(data.id) != _difficultyEntries[diff].end())
            {
                LOG_ERROR("sql.sql", "Table `creature` have creature (SpawnId: {}) that listed as difficulty {} template (Entry: {}) in `creature_template`, skipped.",
                                 spawnId, diff + 1, data.id);
                ok = false;
            }
        }
        if (!ok)
            continue;

        // -1 random, 0 no equipment,
        if (data.equipmentId != 0)
        {
            if (!GetEquipmentInfo(data.id, data.equipmentId))
            {
                LOG_ERROR("sql.sql", "Table `creature` have creature (Entry: {}) with equipment_id {} not found in table `creature_equip_template`, set to no equipment.",
                    data.id, data.equipmentId);
                data.equipmentId = 0;
            }
        }
        if (cInfo->HasFlagsExtra(CREATURE_FLAG_EXTRA_INSTANCE_BIND))
        {
            if (!mapEntry->IsDungeon())
                LOG_ERROR("sql.sql", "Table `creature` have creature (SpawnId: {} Entry: {}) with `creature_template`.`flags_extra` including CREATURE_FLAG_EXTRA_INSTANCE_BIND but creature are not in instance.",
                    spawnId, data.id);
        }
        if (data.movementType >= MAX_DB_MOTION_TYPE)
        {
            LOG_ERROR("sql.sql", "Table `creature` has creature (SpawnId: {} Entry: {}) with wrong movement generator type ({}), ignored and set to IDLE.", spawnId, data.id, data.movementType);
            data.movementType = IDLE_MOTION_TYPE;
        }
        if (data.wander_distance < 0.0f)
        {
            LOG_ERROR("sql.sql", "Table `creature` have creature (SpawnId: {} Entry: {}) with `wander_distance`< 0, set to 0.", spawnId, data.id);
            data.wander_distance = 0.0f;
        }
        else if (data.movementType == RANDOM_MOTION_TYPE)
        {
            if (data.wander_distance == 0.0f)
            {
                LOG_ERROR("sql.sql", "Table `creature` have creature (SpawnId: {} Entry: {}) with `MovementType`=1 (random movement) but with `wander_distance`=0, replace by idle movement type (0).",
                    spawnId, data.id);
                data.movementType = IDLE_MOTION_TYPE;
            }
        }
        else if (data.movementType == IDLE_MOTION_TYPE)
        {
            if (data.wander_distance != 0.0f)
            {
                LOG_ERROR("sql.sql", "Table `creature` have creature (SpawnId: {} Entry: {}) with `MovementType`=0 (idle) have `wander_distance`<>0, set to 0.", spawnId, data.id);
                data.wander_distance = 0.0f;
            }
        }

        if (data.phaseMask == 0)
        {
            LOG_ERROR("sql.sql", "Table `creature` have creature (SpawnId: {} Entry: {}) with `phaseMask`=0 (not visible for anyone), set to 1.", spawnId, data.id);
            data.phaseMask = 1;
        }

        if (sWorld->getBoolConfig(CONFIG_CALCULATE_CREATURE_ZONE_AREA_DATA))
        {
            uint32 zoneId = sMapMgr->GetZoneId(data.phaseMask, data.mapid, data.posX, data.posY, data.posZ);
            uint32 areaId = sMapMgr->GetAreaId(data.phaseMask, data.mapid, data.posX, data.posY, data.posZ);

            WorldDatabasePreparedStatement* stmt = WorldDatabase.GetPreparedStatement(WORLD_UPD_CREATURE_ZONE_AREA_DATA);

            stmt->SetData(0, zoneId);
            stmt->SetData(1, areaId);
            stmt->SetData(2, spawnId);

            WorldDatabase.Execute(stmt);
        }

        // Add to grid if not managed by the game event. Pooled spawns are in
        // the grid data too; the grid loader filters them by pool state.
        if (gameEvent == 0)
            AddCreatureToGrid(spawnId, &data);

        ++count;
    } while (result->NextRow());

    // Load alternate entries from creature_multispawn
    QueryResult variantResult = WorldDatabase.Query("SELECT spawnId, entry FROM creature_multispawn ORDER BY spawnId");
    if (variantResult)
    {
        uint32 variantCount = 0;
        do
        {
            Field* fields = variantResult->Fetch();
            ObjectGuid::LowType spawnId = fields[0].Get<uint32>();
            uint32 entry = fields[1].Get<uint32>();

            auto creatureDataIt = _creatureDataStore.find(spawnId);
            if (creatureDataIt == _creatureDataStore.end())
            {
                LOG_ERROR("sql.sql", "Table `creature_multispawn` has entry for non-existing creature spawn (SpawnId: {}), skipped.", spawnId);
                continue;
            }

            CreatureData* data = &creatureDataIt->second;

            CreatureTemplate const* variantInfo = GetCreatureTemplate(entry);
            if (!variantInfo)
            {
                LOG_ERROR("sql.sql", "Table `creature_multispawn` has creature (SpawnId: {}) with non-existing creature entry {}, skipped.", spawnId, entry);
                continue;
            }

            // Check difficulty entries for variant
            bool diffOk = true;
            for (uint32 diff = 0; diff < MAX_DIFFICULTY - 1 && diffOk; ++diff)
            {
                if (_difficultyEntries[diff].find(entry) != _difficultyEntries[diff].end())
                {
                    LOG_ERROR("sql.sql", "Table `creature_multispawn` has creature (SpawnId: {}) with entry {} listed as difficulty template in `creature_template`, skipped.",
                        spawnId, entry);
                    diffOk = false;
                }
            }
            if (!diffOk)
                continue;

            // Validate equipment for variant entry
            if (data->equipmentId != 0 && !GetEquipmentInfo(entry, data->equipmentId))
            {
                LOG_ERROR("sql.sql", "Table `creature_multispawn` has creature (SpawnId: {}) with entry {} where equipment_id {} not found in `creature_equip_template`.",
                    spawnId, entry, data->equipmentId);
            }

            // Populate id2/id3 fields
            if (!data->id2)
            {
                data->id2 = entry;
                ++variantCount;
            }
            else if (!data->id3)
            {
                data->id3 = entry;
                ++variantCount;
            }
            else
                LOG_ERROR("sql.sql", "Table `creature_multispawn` has more than 2 variant entries for creature (SpawnId: {}), extra entry {} skipped.", spawnId, entry);
        } while (variantResult->NextRow());

        LOG_INFO("server.loading", ">> Loaded {} creature spawn variants", variantCount);
    }

    LOG_INFO("server.loading", ">> Loaded {} Creatures in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

// Loads a single creature spawn from DB into the cache.
// Creature::LoadCreatureFromDB() reads from cache (GetCreatureData()), not from DB directly,
// so this must be called first for spawns not loaded at startup.
CreatureData const* ObjectMgr::LoadCreatureDataFromDB(ObjectGuid::LowType spawnId)
{
    CreatureData const* data = GetCreatureData(spawnId);
    if (data)
        return data;

    QueryResult result = WorldDatabase.Query("SELECT creature.guid, id, map, equipment_id, "
        "position_x, position_y, position_z, orientation, spawntimesecs, wander_distance, "
        "currentwaypoint, curhealth, curmana, MovementType, spawnMask, phaseMask, "
        "creature.npcflag, creature.unit_flags, creature.dynamicflags, creature.ScriptName, creature.PhaseId, creature.PhaseGroup "
        "FROM creature WHERE creature.guid = {}", spawnId);

    if (!result)
        return nullptr;

    Field* fields = result->Fetch();
    uint32 creatureId = fields[1].Get<uint32>();

    CreatureTemplate const* cInfo = GetCreatureTemplate(creatureId);
    if (!cInfo)
    {
        LOG_ERROR("sql.sql", "Table `creature` has creature (SpawnId: {}) with non-existing creature entry {} in `id` field, skipped.", spawnId, creatureId);
        return nullptr;
    }

    CreatureData& creatureData    = _creatureDataStore[spawnId];
    creatureData.id               = creatureId;
    creatureData.mapid            = fields[2].Get<uint16>();
    creatureData.equipmentId      = fields[3].Get<int8>();
    creatureData.posX             = fields[4].Get<float>();
    creatureData.posY             = fields[5].Get<float>();
    creatureData.posZ             = fields[6].Get<float>();
    creatureData.orientation      = fields[7].Get<float>();
    creatureData.spawntimesecs    = fields[8].Get<uint32>();
    creatureData.wander_distance  = fields[9].Get<float>();
    creatureData.currentwaypoint  = fields[10].Get<uint32>();
    creatureData.curhealth        = fields[11].Get<uint32>();
    creatureData.curmana          = fields[12].Get<uint32>();
    creatureData.movementType     = fields[13].Get<uint8>();
    creatureData.spawnMask        = fields[14].Get<uint8>();
    creatureData.phaseMask        = fields[15].Get<uint32>();
    creatureData.npcflag          = fields[16].Get<uint32>();
    creatureData.unit_flags       = fields[17].Get<uint32>();
    creatureData.dynamicflags     = fields[18].Get<uint32>();
    creatureData.ScriptId         = GetScriptId(fields[19].Get<std::string>());
    creatureData.phaseId          = fields[20].Get<uint32>();
    creatureData.phaseGroup       = fields[21].Get<uint32>();
    creatureData.spawnGroupId     = 0;

    MigrateLegacySpawnPhase(creatureData, "creature", spawnId);
    if (creatureData.phaseGroup && !IsValidPhaseGroupId(creatureData.phaseGroup))
    {
        LOG_ERROR("sql.sql", "Table `creature` has creature (SpawnId: {}) with non-existing phase group id {} in `PhaseGroup` field, set to 0.", spawnId, creatureData.phaseGroup);
        creatureData.phaseGroup = 0;
    }

    if (!creatureData.ScriptId)
        creatureData.ScriptId = cInfo->ScriptID;

    // Load alternate entries from creature_multispawn
    QueryResult variantResult = WorldDatabase.Query("SELECT entry FROM creature_multispawn WHERE spawnId = {} ORDER BY entry", spawnId);
    if (variantResult)
    {
        do
        {
            uint32 variantEntry = variantResult->Fetch()[0].Get<uint32>();
            if (!GetCreatureTemplate(variantEntry))
            {
                LOG_ERROR("sql.sql", "Table `creature_multispawn` has creature (SpawnId: {}) with non-existing entry {}, skipped.", spawnId, variantEntry);
                continue;
            }

            // Check difficulty entries for variant
            bool diffOk = true;
            for (uint32 diff = 0; diff < MAX_DIFFICULTY - 1 && diffOk; ++diff)
            {
                if (_difficultyEntries[diff].find(variantEntry) != _difficultyEntries[diff].end())
                {
                    LOG_ERROR("sql.sql", "Table `creature_multispawn` has creature (SpawnId: {}) with entry {} listed as difficulty template in `creature_template`, skipped.",
                        spawnId, variantEntry);
                    diffOk = false;
                }
            }
            if (!diffOk)
                continue;

            // Validate equipment for variant entry
            if (creatureData.equipmentId != 0 && !GetEquipmentInfo(variantEntry, creatureData.equipmentId))
            {
                LOG_ERROR("sql.sql", "Table `creature_multispawn` has creature (SpawnId: {}) with entry {} where equipment_id {} not found in `creature_equip_template`.",
                    spawnId, variantEntry, creatureData.equipmentId);
            }

            if (!creatureData.id2)
                creatureData.id2 = variantEntry;
            else if (!creatureData.id3)
                creatureData.id3 = variantEntry;
            else
                LOG_ERROR("sql.sql", "Table `creature_multispawn` has more than 2 variant entries for creature (SpawnId: {}), extra entry {} skipped.", spawnId, variantEntry);
        } while (variantResult->NextRow());
    }

    MapEntry const* mapEntry = sMapStore.LookupEntry(creatureData.mapid);
    if (!mapEntry)
    {
        LOG_ERROR("sql.sql", "Table `creature` has creature (SpawnId: {}) that spawned at non-existing map (Id: {}), skipped.", spawnId, creatureData.mapid);
        _creatureDataStore.erase(spawnId);
        return nullptr;
    }

    if (mapEntry->IsRaid() && creatureData.spawntimesecs >= 7 * DAY && creatureData.spawntimesecs < 14 * DAY)
        creatureData.spawntimesecs = 14 * DAY;

    bool ok = true;
    for (uint32 diff = 0; diff < MAX_DIFFICULTY - 1 && ok; ++diff)
    {
        if (_difficultyEntries[diff].find(creatureId) != _difficultyEntries[diff].end())
        {
            LOG_ERROR("sql.sql", "Table `creature` has creature (SpawnId: {}) that is listed as difficulty {} template (Entry: {}) in `creature_template`, skipped.",
                spawnId, diff + 1, creatureId);
            ok = false;
        }
    }

    if (!ok)
    {
        _creatureDataStore.erase(spawnId);
        return nullptr;
    }

    if (creatureData.equipmentId != 0)
    {
        if (!GetEquipmentInfo(creatureId, creatureData.equipmentId))
        {
            LOG_ERROR("sql.sql", "Table `creature` has creature (Entry: {}) with equipment_id {} not found in table `creature_equip_template`, set to no equipment.",
                creatureId, creatureData.equipmentId);
            creatureData.equipmentId = 0;
        }
    }

    if (creatureData.movementType >= MAX_DB_MOTION_TYPE)
    {
        LOG_ERROR("sql.sql", "Table `creature` has creature (SpawnId: {} Entry: {}) with wrong movement generator type ({}), set to IDLE.",
            spawnId, creatureId, creatureData.movementType);
        creatureData.movementType = IDLE_MOTION_TYPE;
    }

    if (creatureData.wander_distance < 0.0f)
    {
        LOG_ERROR("sql.sql", "Table `creature` has creature (SpawnId: {} Entry: {}) with `wander_distance`< 0, set to 0.",
            spawnId, creatureId);
        creatureData.wander_distance = 0.0f;
    }
    else if (creatureData.movementType == RANDOM_MOTION_TYPE)
    {
        if (creatureData.wander_distance == 0.0f)
        {
            LOG_ERROR("sql.sql", "Table `creature` has creature (SpawnId: {} Entry: {}) with `MovementType`=1 (random movement) but with `wander_distance`=0, replace by idle movement type (0).",
                spawnId, creatureId);
            creatureData.movementType = IDLE_MOTION_TYPE;
        }
    }
    else if (creatureData.movementType == IDLE_MOTION_TYPE)
    {
        if (creatureData.wander_distance != 0.0f)
        {
            LOG_ERROR("sql.sql", "Table `creature` has creature (SpawnId: {} Entry: {}) with `MovementType`=0 (idle) have `wander_distance`<>0, set to 0.",
                spawnId, creatureId);
            creatureData.wander_distance = 0.0f;
        }
    }

    if (creatureData.phaseMask == 0)
    {
        LOG_ERROR("sql.sql", "Table `creature` has creature (SpawnId: {} Entry: {}) with `phaseMask`=0 (not visible for anyone), set to 1.",
            spawnId, creatureId);
        creatureData.phaseMask = 1;
    }

    return &creatureData;
}

void ObjectMgr::LoadCreatureSparring()
{
    uint32 oldMSTime = getMSTime();

    QueryResult result = WorldDatabase.Query("SELECT GUID, SparringPCT FROM creature_sparring");

    if (!result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 sparring data. DB table `creature_sparring` is empty.");
        LOG_INFO("server.loading", " ");
        return;
    }

    uint32 count = 0;
    do
    {
        Field* fields = result->Fetch();

        ObjectGuid::LowType spawnId     = fields[0].Get<uint32>();
        float sparringHealthPct         = fields[1].Get<float>();

        if (!GetCreatureData(spawnId))
        {
            LOG_ERROR("sql.sql", "Entry {} has a record in `creature_sparring` but doesn't exist in `creatures` table");
            continue;
        }

        _creatureSparringStore[spawnId].push_back(sparringHealthPct);

        ++count;
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} sparring data in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

void ObjectMgr::AddCreatureToGrid(ObjectGuid::LowType guid, CreatureData const* data)
{
    uint8 mask = data->spawnMask;
    for (uint8 i = 0; mask != 0; i++, mask >>= 1)
    {
        if (mask & 1)
        {
            GridCoord gridCoord = Acore::ComputeGridCoord(data->posX, data->posY);
            CellObjectGuids& cell_guids = _mapObjectGuidsStore[MAKE_PAIR32(data->mapid, i)][gridCoord.GetId()];
            cell_guids.creatures.insert(guid);
        }
    }
}

void ObjectMgr::RemoveCreatureFromGrid(ObjectGuid::LowType guid, CreatureData const* data)
{
    uint8 mask = data->spawnMask;
    for (uint8 i = 0; mask != 0; i++, mask >>= 1)
    {
        if (mask & 1)
        {
            GridCoord gridCoord = Acore::ComputeGridCoord(data->posX, data->posY);
            CellObjectGuids& cell_guids = _mapObjectGuidsStore[MAKE_PAIR32(data->mapid, i)][gridCoord.GetId()];
            cell_guids.creatures.erase(guid);
        }
    }
}

ObjectGuid::LowType ObjectMgr::AddGOData(uint32 entry, uint32 mapId, float x, float y, float z, float o, uint32 spawntimedelay, float rotation0, float rotation1, float rotation2, float rotation3)
{
    GameObjectTemplate const* goinfo = GetGameObjectTemplate(entry);
    if (!goinfo)
        return 0;

    Map* map = sMapMgr->CreateBaseMap(mapId);
    if (!map)
        return 0;

    ObjectGuid::LowType spawnId = GenerateGameObjectSpawnId();

    GameObjectData& data = NewGOData(spawnId);
    data.spawnId        = spawnId;
    data.id             = entry;
    data.mapid          = mapId;
    data.posX           = x;
    data.posY           = y;
    data.posZ           = z;
    data.orientation    = o;
    data.rotation.x     = rotation0;
    data.rotation.y     = rotation1;
    data.rotation.z     = rotation2;
    data.rotation.w     = rotation3;
    data.spawntimesecs  = spawntimedelay;
    data.animprogress   = 100;
    data.spawnMask      = 1;
    data.go_state       = GO_STATE_READY;
    data.phaseMask      = PHASEMASK_NORMAL;
    data.artKit         = goinfo->type == GAMEOBJECT_TYPE_CAPTURE_POINT ? 21 : 0;
    data.dbData = false;
    data.spawnGroupId   = 0;

    AddGameobjectToGrid(spawnId, &data);

    // Spawn if necessary (loaded grids only)
    // We use spawn coords to spawn
    if (!map->Instanceable() && map->IsGridLoaded(x, y))
    {
        GameObject* go = IsGameObjectStaticTransport(data.id) ? new StaticTransport() : new GameObject();
        if (!go->LoadGameObjectFromDB(spawnId, map))
        {
            LOG_ERROR("sql.sql", "AddGOData: cannot add gameobject entry {} to map", entry);
            delete go;
            return 0;
        }
    }

    LOG_DEBUG("maps", "AddGOData: spawnId {} entry {} map {} x {} y {} z {} o {}", spawnId, entry, mapId, x, y, z, o);

    return spawnId;
}

ObjectGuid::LowType ObjectMgr::AddCreData(uint32 entry, uint32 mapId, float x, float y, float z, float o, uint32 spawntimedelay)
{
    CreatureTemplate const* cInfo = GetCreatureTemplate(entry);
    if (!cInfo)
        return 0;

    uint32 level = cInfo->minlevel == cInfo->maxlevel ? cInfo->minlevel : urand(cInfo->minlevel, cInfo->maxlevel); // Only used for extracting creature base stats
    CreatureBaseStats const* stats = GetCreatureBaseStats(level, cInfo->unit_class);
    Map* map = sMapMgr->CreateBaseMap(mapId);
    if (!map)
        return 0;

    ObjectGuid::LowType spawnId = GenerateCreatureSpawnId();
    CreatureData& data = NewOrExistCreatureData(spawnId);
    data.spawnId = spawnId;
    data.spawnMask = spawnId;
    data.id = entry;
    data.id2 = 0;
    data.id3 = 0;
    data.mapid = mapId;
    data.displayid = 0;
    data.equipmentId = 0;
    data.posX = x;
    data.posY = y;
    data.posZ = z;
    data.orientation = o;
    data.spawntimesecs = spawntimedelay;
    data.wander_distance = 0;
    data.currentwaypoint = 0;
    data.curhealth = stats->GenerateHealth(cInfo);
    data.curmana = stats->GenerateMana(cInfo);
    data.movementType = cInfo->MovementType;
    data.spawnMask = 1;
    data.phaseMask = PHASEMASK_NORMAL;
    data.dbData = false;
    data.npcflag = cInfo->npcflag;
    data.unit_flags = cInfo->unit_flags;
    data.dynamicflags = cInfo->dynamicflags;

    AddCreatureToGrid(spawnId, &data);

    // Spawn if necessary (loaded grids only)
    if (!map->Instanceable() && map->IsGridLoaded(x, y))
    {
        Creature* creature = new Creature();
        if (!creature->LoadCreatureFromDB(spawnId, map, true, true))
        {
            LOG_ERROR("sql.sql", "AddCreature: Cannot add creature entry {} to map", entry);
            delete creature;
            return 0;
        }
    }

    return spawnId;
}

void ObjectMgr::LoadGameobjects()
{
    uint32 oldMSTime = getMSTime();

    //                                                0                1   2    3           4           5           6
    QueryResult result = WorldDatabase.Query("SELECT gameobject.guid, id, map, position_x, position_y, position_z, orientation, "
                         //   7          8          9          10         11             12            13     14         15         16          17
                         "rotation0, rotation1, rotation2, rotation3, spawntimesecs, animprogress, state, spawnMask, phaseMask, eventEntry, pool_entry, "
                         //   18                19
                         "ScriptName, gameobject.PhaseId, gameobject.PhaseGroup "
                         "FROM gameobject LEFT OUTER JOIN game_event_gameobject ON gameobject.guid = game_event_gameobject.guid "
                         "LEFT OUTER JOIN pool_gameobject ON gameobject.guid = pool_gameobject.guid");

    if (!result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 gameobjects. DB table `gameobject` is empty.");
        LOG_INFO("server.loading", " ");
        return;
    }

    if (sWorld->getBoolConfig(CONFIG_CALCULATE_GAMEOBJECT_ZONE_AREA_DATA))
        LOG_INFO("server.loading", "Calculating zone and area fields. This may take a moment...");

    // build single time for check spawnmask
    std::map<uint32, uint32> spawnMasks;
    for (uint32 i = 0; i < sMapStore.GetNumRows(); ++i)
        if (sMapStore.LookupEntry(i))
            for (int k = 0; k < MAX_DIFFICULTY; ++k)
                if (GetMapDifficultyData(i, Difficulty(k)))
                    spawnMasks[i] |= (1 << k);

    _gameObjectDataStore.rehash(result->GetRowCount());
    do
    {
        Field* fields = result->Fetch();

        ObjectGuid::LowType guid    = fields[0].Get<uint32>();
        uint32 entry                = fields[1].Get<uint32>();

        GameObjectTemplate const* gInfo = GetGameObjectTemplate(entry);
        if (!gInfo)
        {
            LOG_ERROR("sql.sql", "Table `gameobject` has gameobject (GUID: {}) with non existing gameobject entry {}, skipped.", guid, entry);
            continue;
        }

        if (!gInfo->displayId)
        {
            switch (gInfo->type)
            {
                case GAMEOBJECT_TYPE_TRAP:
                case GAMEOBJECT_TYPE_SPELL_FOCUS:
                    break;
                default:
                    LOG_ERROR("sql.sql", "Gameobject (GUID: {} Entry {} GoType: {}) doesn't have a displayId ({}), not loaded.", guid, entry, gInfo->type, gInfo->displayId);
                    break;
            }
        }

        if (gInfo->displayId && !sGameObjectDisplayInfoStore.LookupEntry(gInfo->displayId))
        {
            LOG_ERROR("sql.sql", "Gameobject (GUID: {} Entry {} GoType: {}) has an invalid displayId ({}), not loaded.", guid, entry, gInfo->type, gInfo->displayId);
            continue;
        }

        GameObjectData& data = _gameObjectDataStore[guid];

        data.spawnId        = guid;
        data.id             = entry;
        data.mapid          = fields[2].Get<uint16>();
        data.posX           = fields[3].Get<float>();
        data.posY           = fields[4].Get<float>();
        data.posZ           = fields[5].Get<float>();
        data.orientation    = fields[6].Get<float>();
        data.rotation.x     = fields[7].Get<float>();
        data.rotation.y     = fields[8].Get<float>();
        data.rotation.z     = fields[9].Get<float>();
        data.rotation.w     = fields[10].Get<float>();
        data.spawntimesecs  = fields[11].Get<int32>();
        data.ScriptId       = GetScriptId(fields[18].Get<std::string>());
        data.spawnGroupId   = 0;
        if (!data.ScriptId)
            data.ScriptId = gInfo->ScriptId;

        MapEntry const* mapEntry = sMapStore.LookupEntry(data.mapid);
        if (!mapEntry)
        {
            LOG_ERROR("sql.sql", "Table `gameobject` has gameobject (GUID: {} Entry: {}) spawned on a non-existed map (Id: {}), skip", guid, data.id, data.mapid);
            continue;
        }

        if (data.spawntimesecs == 0 && gInfo->IsDespawnAtAction())
        {
            LOG_ERROR("sql.sql", "Table `gameobject` has gameobject (GUID: {} Entry: {}) with `spawntimesecs` (0) value, but the gameobejct is marked as despawnable at action.", guid, data.id);
        }

        data.animprogress   = fields[12].Get<uint8>();
        data.artKit         = 0;

        uint32 go_state     = fields[13].Get<uint8>();
        if (go_state >= MAX_GO_STATE)
        {
            LOG_ERROR("sql.sql", "Table `gameobject` has gameobject (GUID: {} Entry: {}) with invalid `state` ({}) value, skip", guid, data.id, go_state);
            continue;
        }
        data.go_state       = GOState(go_state);

        data.spawnMask      = fields[14].Get<uint8>();

        if (!_transportMaps.count(data.mapid))
        {
            if (data.spawnMask & ~spawnMasks[data.mapid])
                LOG_ERROR("sql.sql", "Table `gameobject` has gameobject (GUID: {} Entry: {}) that has wrong spawn mask {} including not supported difficulty modes for map (Id: {}), skip", guid, data.id, data.spawnMask, data.mapid);
        }
        else
            data.spawnGroupId = 1; // force compatibility group for transport spawns

        data.phaseMask      = fields[15].Get<uint32>();
        int16 gameEvent     = fields[16].Get<int16>();
        data.poolId         = fields[17].Get<uint32>();
        data.phaseId        = fields[19].Get<uint32>();
        data.phaseGroup     = fields[20].Get<uint32>();

        MigrateLegacySpawnPhase(data, "gameobject", guid);
        if (data.phaseGroup && !IsValidPhaseGroupId(data.phaseGroup))
        {
            LOG_ERROR("sql.sql", "Table `gameobject` has gameobject (GUID: {} Entry: {}) with non-existing phase group id {} in `PhaseGroup` field, set to 0.", guid, data.id, data.phaseGroup);
            data.phaseGroup = 0;
        }

        if (data.rotation.x < -1.0f || data.rotation.x > 1.0f)
        {
            LOG_ERROR("sql.sql", "Table `gameobject` has gameobject (GUID: {} Entry: {}) with invalid rotationX ({}) value, skip", guid, data.id, data.rotation.x);
            continue;
        }

        if (data.rotation.y < -1.0f || data.rotation.y > 1.0f)
        {
            LOG_ERROR("sql.sql", "Table `gameobject` has gameobject (GUID: {} Entry: {}) with invalid rotationY ({}) value, skip", guid, data.id, data.rotation.y);
            continue;
        }

        if (data.rotation.z < -1.0f || data.rotation.z > 1.0f)
        {
            LOG_ERROR("sql.sql", "Table `gameobject` has gameobject (GUID: {} Entry: {}) with invalid rotationZ ({}) value, skip", guid, data.id, data.rotation.z);
            continue;
        }

        if (data.rotation.w < -1.0f || data.rotation.w > 1.0f)
        {
            LOG_ERROR("sql.sql", "Table `gameobject` has gameobject (GUID: {} Entry: {}) with invalid rotationW ({}) value, skip", guid, data.id, data.rotation.w);
            continue;
        }

        if (fabs(data.rotation.x * data.rotation.x + data.rotation.y * data.rotation.y +
                 data.rotation.z * data.rotation.z + data.rotation.w * data.rotation.w - 1.0f) >= 1e-5f)
        {
            LOG_ERROR("sql.sql", "Table `gameobject` has gameobject (GUID: {} Entry: {}) with invalid rotation quaternion (non-unit), defaulting to orientation on Z axis only", guid, data.id);
            data.rotation = G3D::Quat(G3D::Matrix3::fromEulerAnglesZYX(data.orientation, 0.0f, 0.0f));
        }

        if (!MapMgr::IsValidMapCoord(data.mapid, data.posX, data.posY, data.posZ, data.orientation))
        {
            LOG_ERROR("sql.sql", "Table `gameobject` has gameobject (GUID: {} Entry: {}) with invalid coordinates, skip", guid, data.id);
            continue;
        }

        if (data.phaseMask == 0)
        {
            LOG_ERROR("sql.sql", "Table `gameobject` has gameobject (GUID: {} Entry: {}) with `phaseMask`=0 (not visible for anyone), set to 1.", guid, data.id);
            data.phaseMask = 1;
        }

        if (sWorld->getBoolConfig(CONFIG_CALCULATE_GAMEOBJECT_ZONE_AREA_DATA))
        {
            uint32 zoneId = sMapMgr->GetZoneId(data.phaseMask, data.mapid, data.posX, data.posY, data.posZ);
            uint32 areaId = sMapMgr->GetAreaId(data.phaseMask, data.mapid, data.posX, data.posY, data.posZ);

            WorldDatabasePreparedStatement* stmt = WorldDatabase.GetPreparedStatement(WORLD_UPD_GAMEOBJECT_ZONE_AREA_DATA);

            stmt->SetData(0, zoneId);
            stmt->SetData(1, areaId);
            stmt->SetData(2, guid);

            WorldDatabase.Execute(stmt);
        }

        if (gameEvent == 0)                      // if not this is to be managed by GameEvent System
            AddGameobjectToGrid(guid, &data);
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} Gameobjects in {} ms", (unsigned long)_gameObjectDataStore.size(), GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

// Loads a single gameobject spawn from DB into the cache.
// GameObject::LoadGameObjectFromDB() reads from cache (GetGameObjectData()), not from DB directly,
// so this must be called first for spawns not loaded at startup.
GameObjectData const* ObjectMgr::LoadGameObjectDataFromDB(ObjectGuid::LowType spawnId)
{
    GameObjectData const* data = GetGameObjectData(spawnId);
    if (data)
        return data;

    QueryResult result = WorldDatabase.Query("SELECT gameobject.guid, id, map, position_x, position_y, position_z, orientation, "
        "rotation0, rotation1, rotation2, rotation3, spawntimesecs, animprogress, state, spawnMask, phaseMask, "
        "ScriptName, gameobject.PhaseId, gameobject.PhaseGroup "
        "FROM gameobject WHERE gameobject.guid = {}", spawnId);

    if (!result)
        return nullptr;

    Field* fields = result->Fetch();
    uint32 entry = fields[1].Get<uint32>();

    GameObjectTemplate const* gInfo = GetGameObjectTemplate(entry);
    if (!gInfo)
    {
        LOG_ERROR("sql.sql", "Table `gameobject` has gameobject (GUID: {}) with non-existing gameobject entry {}, skipped.", spawnId, entry);
        return nullptr;
    }

    if (gInfo->displayId && !sGameObjectDisplayInfoStore.LookupEntry(gInfo->displayId))
    {
        LOG_ERROR("sql.sql", "Table `gameobject` has gameobject (GUID: {} Entry {} GoType: {}) with an invalid displayId ({}), not loaded.",
            spawnId, entry, gInfo->type, gInfo->displayId);
        return nullptr;
    }

    GameObjectData& goData  = _gameObjectDataStore[spawnId];
    goData.id               = entry;
    goData.mapid            = fields[2].Get<uint16>();
    goData.posX             = fields[3].Get<float>();
    goData.posY             = fields[4].Get<float>();
    goData.posZ             = fields[5].Get<float>();
    goData.orientation      = fields[6].Get<float>();
    goData.rotation.x       = fields[7].Get<float>();
    goData.rotation.y       = fields[8].Get<float>();
    goData.rotation.z       = fields[9].Get<float>();
    goData.rotation.w       = fields[10].Get<float>();
    goData.spawntimesecs    = fields[11].Get<int32>();
    goData.animprogress     = fields[12].Get<uint8>();
    goData.artKit           = 0;
    goData.ScriptId         = GetScriptId(fields[16].Get<std::string>());

    if (!goData.ScriptId)
        goData.ScriptId = gInfo->ScriptId;

    MapEntry const* mapEntry = sMapStore.LookupEntry(goData.mapid);
    if (!mapEntry)
    {
        LOG_ERROR("sql.sql", "Table `gameobject` has gameobject (GUID: {} Entry: {}) spawned on a non-existing map (Id: {}), skipped.", spawnId, entry, goData.mapid);
        _gameObjectDataStore.erase(spawnId);
        return nullptr;
    }

    if (goData.spawntimesecs == 0 && gInfo->IsDespawnAtAction())
    {
        LOG_ERROR("sql.sql", "Table `gameobject` has gameobject (GUID: {} Entry: {}) with `spawntimesecs` (0) value, but the gameobject is marked as despawnable at action.", spawnId, entry);
    }

    uint32 go_state = fields[13].Get<uint8>();
    if (go_state >= MAX_GO_STATE)
    {
        LOG_ERROR("sql.sql", "Table `gameobject` has gameobject (GUID: {} Entry: {}) with invalid `state` ({}) value, skipped.", spawnId, entry, go_state);
        _gameObjectDataStore.erase(spawnId);
        return nullptr;
    }
    goData.go_state = GOState(go_state);

    goData.spawnMask        = fields[14].Get<uint8>();
    goData.phaseMask        = fields[15].Get<uint32>();
    goData.phaseId          = fields[17].Get<uint32>();
    goData.phaseGroup       = fields[18].Get<uint32>();

    MigrateLegacySpawnPhase(goData, "gameobject", spawnId);
    if (goData.phaseGroup && !IsValidPhaseGroupId(goData.phaseGroup))
    {
        LOG_ERROR("sql.sql", "Table `gameobject` has gameobject (GUID: {} Entry: {}) with non-existing phase group id {} in `PhaseGroup` field, set to 0.", spawnId, entry, goData.phaseGroup);
        goData.phaseGroup = 0;
    }

    if (goData.rotation.x < -1.0f || goData.rotation.x > 1.0f)
    {
        LOG_ERROR("sql.sql", "Table `gameobject` has gameobject (GUID: {} Entry: {}) with invalid rotationX ({}) value, skipped.", spawnId, entry, goData.rotation.x);
        _gameObjectDataStore.erase(spawnId);
        return nullptr;
    }

    if (goData.rotation.y < -1.0f || goData.rotation.y > 1.0f)
    {
        LOG_ERROR("sql.sql", "Table `gameobject` has gameobject (GUID: {} Entry: {}) with invalid rotationY ({}) value, skipped.", spawnId, entry, goData.rotation.y);
        _gameObjectDataStore.erase(spawnId);
        return nullptr;
    }

    if (goData.rotation.z < -1.0f || goData.rotation.z > 1.0f)
    {
        LOG_ERROR("sql.sql", "Table `gameobject` has gameobject (GUID: {} Entry: {}) with invalid rotationZ ({}) value, skipped.", spawnId, entry, goData.rotation.z);
        _gameObjectDataStore.erase(spawnId);
        return nullptr;
    }

    if (goData.rotation.w < -1.0f || goData.rotation.w > 1.0f)
    {
        LOG_ERROR("sql.sql", "Table `gameobject` has gameobject (GUID: {} Entry: {}) with invalid rotationW ({}) value, skipped.", spawnId, entry, goData.rotation.w);
        _gameObjectDataStore.erase(spawnId);
        return nullptr;
    }

    if (fabs(goData.rotation.x * goData.rotation.x + goData.rotation.y * goData.rotation.y +
             goData.rotation.z * goData.rotation.z + goData.rotation.w * goData.rotation.w - 1.0f) >= 1e-5f)
    {
        LOG_ERROR("sql.sql", "Table `gameobject` has gameobject (GUID: {} Entry: {}) with invalid rotation quaternion (non-unit), defaulting to orientation on Z axis only", spawnId, entry);
        goData.rotation = G3D::Quat(G3D::Matrix3::fromEulerAnglesZYX(goData.orientation, 0.0f, 0.0f));
    }

    if (!MapMgr::IsValidMapCoord(goData.mapid, goData.posX, goData.posY, goData.posZ, goData.orientation))
    {
        LOG_ERROR("sql.sql", "Table `gameobject` has gameobject (GUID: {} Entry: {}) with invalid coordinates, skipped.", spawnId, entry);
        _gameObjectDataStore.erase(spawnId);
        return nullptr;
    }

    if (goData.phaseMask == 0)
    {
        LOG_ERROR("sql.sql", "Table `gameobject` has gameobject (GUID: {} Entry: {}) with `phaseMask`=0 (not visible for anyone), set to 1.", spawnId, entry);
        goData.phaseMask = 1;
    }

    return &goData;
}

void ObjectMgr::AddGameobjectToGrid(ObjectGuid::LowType guid, GameObjectData const* data)
{
    uint8 mask = data->spawnMask;
    for (uint8 i = 0; mask != 0; i++, mask >>= 1)
    {
        if (mask & 1)
        {
            GridCoord gridCoord = Acore::ComputeGridCoord(data->posX, data->posY);
            CellObjectGuids& cell_guids = _mapObjectGuidsStore[MAKE_PAIR32(data->mapid, i)][gridCoord.GetId()];
            cell_guids.gameobjects.insert(guid);
        }
    }
}

void ObjectMgr::RemoveGameobjectFromGrid(ObjectGuid::LowType guid, GameObjectData const* data)
{
    uint8 mask = data->spawnMask;
    for (uint8 i = 0; mask != 0; i++, mask >>= 1)
    {
        if (mask & 1)
        {
            GridCoord gridCoord = Acore::ComputeGridCoord(data->posX, data->posY);
            CellObjectGuids& cell_guids = _mapObjectGuidsStore[MAKE_PAIR32(data->mapid, i)][gridCoord.GetId()];
            cell_guids.gameobjects.erase(guid);
        }
    }
}

void ObjectMgr::LoadItemLocales()
{
    uint32 oldMSTime = getMSTime();

    _itemLocaleStore.clear();                                 // need for reload case

    QueryResult result = WorldDatabase.Query("SELECT ID, locale, Name, Description FROM item_template_locale");
    if (!result)
        return;

    do
    {
        Field* fields = result->Fetch();

        uint32 ID = fields[0].Get<uint32>();

        LocaleConstant locale = GetLocaleByName(fields[1].Get<std::string>());
        if (locale == LOCALE_enUS)
            continue;

        ItemLocale& data = _itemLocaleStore[ID];
        AddLocaleString(fields[2].Get<std::string>(), locale, data.Name);
        AddLocaleString(fields[3].Get<std::string>(), locale, data.Description);
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} Item Locale Strings in {} ms", (uint32)_itemLocaleStore.size(), GetMSTimeDiffToNow(oldMSTime));
}

ServerConfigs const qualityToBuyValueConfig[MAX_ITEM_QUALITY] =
{
    RATE_BUYVALUE_ITEM_POOR,                                    // ITEM_QUALITY_POOR
    RATE_BUYVALUE_ITEM_NORMAL,                                  // ITEM_QUALITY_NORMAL
    RATE_BUYVALUE_ITEM_UNCOMMON,                                // ITEM_QUALITY_UNCOMMON
    RATE_BUYVALUE_ITEM_RARE,                                    // ITEM_QUALITY_RARE
    RATE_BUYVALUE_ITEM_EPIC,                                    // ITEM_QUALITY_EPIC
    RATE_BUYVALUE_ITEM_LEGENDARY,                               // ITEM_QUALITY_LEGENDARY
    RATE_BUYVALUE_ITEM_ARTIFACT,                                // ITEM_QUALITY_ARTIFACT
    RATE_BUYVALUE_ITEM_HEIRLOOM,                                // ITEM_QUALITY_HEIRLOOM
};

ServerConfigs const qualityToSellValueConfig[MAX_ITEM_QUALITY] =
{
    RATE_SELLVALUE_ITEM_POOR,                                   // ITEM_QUALITY_POOR
    RATE_SELLVALUE_ITEM_NORMAL,                                 // ITEM_QUALITY_NORMAL
    RATE_SELLVALUE_ITEM_UNCOMMON,                               // ITEM_QUALITY_UNCOMMON
    RATE_SELLVALUE_ITEM_RARE,                                   // ITEM_QUALITY_RARE
    RATE_SELLVALUE_ITEM_EPIC,                                   // ITEM_QUALITY_EPIC
    RATE_SELLVALUE_ITEM_LEGENDARY,                              // ITEM_QUALITY_LEGENDARY
    RATE_SELLVALUE_ITEM_ARTIFACT,                               // ITEM_QUALITY_ARTIFACT
    RATE_SELLVALUE_ITEM_HEIRLOOM,                               // ITEM_QUALITY_HEIRLOOM
};

// 6.x item helpers, from TrinityCore 6.x (ObjectMgr.cpp)
uint32 FillMaxDurability(uint32 itemClass, uint32 itemSubClass, uint32 inventoryType, uint32 quality, uint32 itemLevel)
{
    if (itemClass != ITEM_CLASS_ARMOR && itemClass != ITEM_CLASS_WEAPON)
        return 0;

    static float const qualityMultipliers[MAX_ITEM_QUALITY] =
    {
        0.92f, 0.92f, 0.92f, 1.11f, 1.32f, 1.61f, 0.0f, 0.0f
    };

    static float const armorMultipliers[MAX_INVTYPE] =
    {
        0.00f, // INVTYPE_NON_EQUIP
        0.60f, // INVTYPE_HEAD
        0.00f, // INVTYPE_NECK
        0.60f, // INVTYPE_SHOULDERS
        0.00f, // INVTYPE_BODY
        1.00f, // INVTYPE_CHEST
        0.33f, // INVTYPE_WAIST
        0.72f, // INVTYPE_LEGS
        0.48f, // INVTYPE_FEET
        0.33f, // INVTYPE_WRISTS
        0.33f, // INVTYPE_HANDS
        0.00f, // INVTYPE_FINGER
        0.00f, // INVTYPE_TRINKET
        0.00f, // INVTYPE_WEAPON
        0.72f, // INVTYPE_SHIELD
        0.00f, // INVTYPE_RANGED
        0.00f, // INVTYPE_CLOAK
        0.00f, // INVTYPE_2HWEAPON
        0.00f, // INVTYPE_BAG
        0.00f, // INVTYPE_TABARD
        1.00f, // INVTYPE_ROBE
        0.00f, // INVTYPE_WEAPONMAINHAND
        0.00f, // INVTYPE_WEAPONOFFHAND
        0.00f, // INVTYPE_HOLDABLE
        0.00f, // INVTYPE_AMMO
        0.00f, // INVTYPE_THROWN
        0.00f, // INVTYPE_RANGEDRIGHT
        0.00f, // INVTYPE_QUIVER
        0.00f, // INVTYPE_RELIC
    };

    static float const weaponMultipliers[MAX_ITEM_SUBCLASS_WEAPON] =
    {
        0.91f, // ITEM_SUBCLASS_WEAPON_AXE
        1.00f, // ITEM_SUBCLASS_WEAPON_AXE2
        1.00f, // ITEM_SUBCLASS_WEAPON_BOW
        1.00f, // ITEM_SUBCLASS_WEAPON_GUN
        0.91f, // ITEM_SUBCLASS_WEAPON_MACE
        1.00f, // ITEM_SUBCLASS_WEAPON_MACE2
        1.00f, // ITEM_SUBCLASS_WEAPON_POLEARM
        0.91f, // ITEM_SUBCLASS_WEAPON_SWORD
        1.00f, // ITEM_SUBCLASS_WEAPON_SWORD2
        0.00f, // ITEM_SUBCLASS_WEAPON_Obsolete
        1.00f, // ITEM_SUBCLASS_WEAPON_STAFF
        0.00f, // ITEM_SUBCLASS_WEAPON_EXOTIC
        0.00f, // ITEM_SUBCLASS_WEAPON_EXOTIC2
        0.66f, // ITEM_SUBCLASS_WEAPON_FIST
        0.00f, // ITEM_SUBCLASS_WEAPON_MISCELLANEOUS
        0.66f, // ITEM_SUBCLASS_WEAPON_DAGGER
        0.00f, // ITEM_SUBCLASS_WEAPON_THROWN
        0.00f, // ITEM_SUBCLASS_WEAPON_SPEAR
        1.00f, // ITEM_SUBCLASS_WEAPON_CROSSBOW
        0.66f, // ITEM_SUBCLASS_WEAPON_WAND
        0.66f, // ITEM_SUBCLASS_WEAPON_FISHING_POLE
    };

    float levelPenalty = 1.0f;
    if (itemLevel <= 28)
        levelPenalty = 0.966f - float(28u - itemLevel) / 54.0f;

    if (itemClass == ITEM_CLASS_ARMOR)
    {
        if (inventoryType > INVTYPE_ROBE)
            return 0;

        return 5 * uint32(round(25.0f * qualityMultipliers[quality] * armorMultipliers[inventoryType] * levelPenalty));
    }

    return 5 * uint32(round(18.0f * qualityMultipliers[quality] * weaponMultipliers[itemSubClass] * levelPenalty));
}

// from TrinityCore 6.x (ObjectMgr.cpp), on AC ItemTemplate fields
void FillDisenchantFields(uint32* disenchantID, uint32* requiredDisenchantSkill, ItemTemplate const& itemTemplate)
{
    *disenchantID = 0;
    *(int32*)requiredDisenchantSkill = -1;
    if ((itemTemplate.Flags & (ITEM_FLAG_CONJURED | ITEM_FLAG_NO_USER_DESTROY)) ||
        itemTemplate.Bonding == BIND_QUEST_ITEM || itemTemplate.Area || itemTemplate.Map ||
        itemTemplate.GetMaxStackSize() > 1 ||
        itemTemplate.Quality < ITEM_QUALITY_UNCOMMON || itemTemplate.Quality > ITEM_QUALITY_EPIC ||
        !(itemTemplate.Class == ITEM_CLASS_ARMOR || itemTemplate.Class == ITEM_CLASS_WEAPON) ||
        !(Item::GetSpecialPrice(&itemTemplate) || sItemCurrencyCostStore.LookupEntry(itemTemplate.ItemId)))
        return;

    for (ItemDisenchantLootEntry const* disenchant : sItemDisenchantLootStore)
    {
        if (disenchant->ItemClass == itemTemplate.Class &&
            disenchant->ItemQuality == itemTemplate.Quality &&
            disenchant->MinItemLevel <= itemTemplate.ItemLevel &&
            disenchant->MaxItemLevel >= itemTemplate.ItemLevel)
        {
            if (disenchant->ID == 60 || disenchant->ID == 61)   // epic item disenchant ilvl range 66-99 (classic)
            {
                if (itemTemplate.RequiredLevel > 60 || itemTemplate.RequiredSkillRank > 300)
                    continue;                                   // skip to epic item disenchant ilvl range 90-199 (TBC)
            }
            else if (disenchant->ID == 66 || disenchant->ID == 67)  // epic item disenchant ilvl range 90-199 (TBC)
            {
                if (itemTemplate.RequiredLevel <= 60 || (itemTemplate.RequiredSkill && itemTemplate.RequiredSkillRank <= 300))
                    continue;
            }

            *disenchantID = disenchant->ID;
            *requiredDisenchantSkill = disenchant->RequiredDisenchantSkill;
            return;
        }
    }
}

struct ItemSpecStats
{
    uint32 ItemType;
    uint32 ItemSpecStatTypes[MAX_ITEM_PROTO_STATS];
    uint32 ItemSpecStatCount;

    ItemSpecStats(ItemEntry const* item, ItemSparseEntry const* sparse) : ItemType(0), ItemSpecStatCount(0)
    {
        memset(ItemSpecStatTypes, -1, sizeof(ItemSpecStatTypes));

        if (item->Class == ITEM_CLASS_WEAPON)
        {
            ItemType = 5;
            switch (item->SubClass)
            {
                case ITEM_SUBCLASS_WEAPON_AXE:
                    AddStat(ITEM_SPEC_STAT_ONE_HANDED_AXE);
                    break;
                case ITEM_SUBCLASS_WEAPON_AXE2:
                    AddStat(ITEM_SPEC_STAT_TWO_HANDED_AXE);
                    break;
                case ITEM_SUBCLASS_WEAPON_BOW:
                    AddStat(ITEM_SPEC_STAT_BOW);
                    break;
                case ITEM_SUBCLASS_WEAPON_GUN:
                    AddStat(ITEM_SPEC_STAT_GUN);
                    break;
                case ITEM_SUBCLASS_WEAPON_MACE:
                    AddStat(ITEM_SPEC_STAT_ONE_HANDED_MACE);
                    break;
                case ITEM_SUBCLASS_WEAPON_MACE2:
                    AddStat(ITEM_SPEC_STAT_TWO_HANDED_MACE);
                    break;
                case ITEM_SUBCLASS_WEAPON_POLEARM:
                    AddStat(ITEM_SPEC_STAT_POLEARM);
                    break;
                case ITEM_SUBCLASS_WEAPON_SWORD:
                    AddStat(ITEM_SPEC_STAT_ONE_HANDED_SWORD);
                    break;
                case ITEM_SUBCLASS_WEAPON_SWORD2:
                    AddStat(ITEM_SPEC_STAT_TWO_HANDED_SWORD);
                    break;
                case ITEM_SUBCLASS_WEAPON_STAFF:
                    AddStat(ITEM_SPEC_STAT_STAFF);
                    break;
                case ITEM_SUBCLASS_WEAPON_FIST:
                    AddStat(ITEM_SPEC_STAT_FIST_WEAPON);
                    break;
                case ITEM_SUBCLASS_WEAPON_DAGGER:
                    AddStat(ITEM_SPEC_STAT_DAGGER);
                    break;
                case ITEM_SUBCLASS_WEAPON_THROWN:
                    AddStat(ITEM_SPEC_STAT_THROWN);
                    break;
                case ITEM_SUBCLASS_WEAPON_CROSSBOW:
                    AddStat(ITEM_SPEC_STAT_CROSSBOW);
                    break;
                case ITEM_SUBCLASS_WEAPON_WAND:
                    AddStat(ITEM_SPEC_STAT_WAND);
                    break;
                default:
                    break;
            }
        }
        else if (item->Class == ITEM_CLASS_ARMOR && item->SubClass > 5 && item->SubClass <= 11)
        {
            switch (item->SubClass)
            {
                case ITEM_SUBCLASS_ARMOR_CLOTH:
                    if (sparse->InventoryType != INVTYPE_CLOAK)
                    {
                        ItemType = 1;
                        break;
                    }

                    ItemType = 0;
                    AddStat(ITEM_SPEC_STAT_CLOAK);
                    break;
                case ITEM_SUBCLASS_ARMOR_LEATHER:
                    ItemType = 2;
                    break;
                case ITEM_SUBCLASS_ARMOR_MAIL:
                    ItemType = 3;
                    break;
                case ITEM_SUBCLASS_ARMOR_PLATE:
                    ItemType = 4;
                    break;
                default:
                    ItemType = 6;
                    if (item->SubClass == ITEM_SUBCLASS_ARMOR_SHIELD)
                        AddStat(ITEM_SPEC_STAT_SHIELD);
                    else if (item->SubClass > ITEM_SUBCLASS_ARMOR_SHIELD && item->SubClass <= ITEM_SUBCLASS_ARMOR_SIGIL)
                        AddStat(ITEM_SPEC_STAT_RELIC);
                    break;
            }
        }
        else
            ItemType = 0;

        for (uint32 i = 0; i < MAX_ITEM_PROTO_STATS; ++i)
            if (sparse->ItemStatType[i] != -1)
                AddModStat(sparse->ItemStatType[i]);
    }

    void AddStat(ItemSpecStat statType)
    {
        if (ItemSpecStatCount >= MAX_ITEM_PROTO_STATS)
            return;

        for (uint32 i = 0; i < MAX_ITEM_PROTO_STATS; ++i)
            if (ItemSpecStatTypes[i] == uint32(statType))
                return;

        ItemSpecStatTypes[ItemSpecStatCount++] = statType;
    }

    void AddModStat(int32 itemStatType)
    {
        switch (itemStatType)
        {
            case ITEM_MOD_AGILITY:
                AddStat(ITEM_SPEC_STAT_AGILITY);
                break;
            case ITEM_MOD_STRENGTH:
                AddStat(ITEM_SPEC_STAT_STRENGTH);
                break;
            case ITEM_MOD_INTELLECT:
                AddStat(ITEM_SPEC_STAT_INTELLECT);
                break;
            case ITEM_MOD_SPIRIT:
                AddStat(ITEM_SPEC_STAT_SPIRIT);
                break;
            case ITEM_MOD_DODGE_RATING:
                AddStat(ITEM_SPEC_STAT_DODGE);
                break;
            case ITEM_MOD_PARRY_RATING:
                AddStat(ITEM_SPEC_STAT_PARRY);
                break;
            case ITEM_MOD_CRIT_MELEE_RATING:
            case ITEM_MOD_CRIT_RANGED_RATING:
            case ITEM_MOD_CRIT_SPELL_RATING:
            case ITEM_MOD_CRIT_RATING:
                AddStat(ITEM_SPEC_STAT_CRIT);
                break;
            case ITEM_MOD_HASTE_MELEE_RATING:
            case ITEM_MOD_HASTE_RANGED_RATING:
            case ITEM_MOD_HASTE_SPELL_RATING:
            case ITEM_MOD_HASTE_RATING:
                AddStat(ITEM_SPEC_STAT_HASTE);
                break;
            case ITEM_MOD_HIT_RATING:
                AddStat(ITEM_SPEC_STAT_HIT);
                break;
            case ITEM_MOD_EXTRA_ARMOR:
                AddStat(ITEM_SPEC_STAT_BONUS_ARMOR);
                break;
            case ITEM_MOD_AGI_STR_INT:
                AddStat(ITEM_SPEC_STAT_AGILITY);
                AddStat(ITEM_SPEC_STAT_STRENGTH);
                AddStat(ITEM_SPEC_STAT_INTELLECT);
                break;
            case ITEM_MOD_AGI_STR:
                AddStat(ITEM_SPEC_STAT_AGILITY);
                AddStat(ITEM_SPEC_STAT_STRENGTH);
                break;
            case ITEM_MOD_AGI_INT:
                AddStat(ITEM_SPEC_STAT_AGILITY);
                AddStat(ITEM_SPEC_STAT_INTELLECT);
                break;
            case ITEM_MOD_STR_INT:
                AddStat(ITEM_SPEC_STAT_STRENGTH);
                AddStat(ITEM_SPEC_STAT_INTELLECT);
                break;
        }
    }
};

void ObjectMgr::LoadItemTemplates()
{
    uint32 oldMSTime = getMSTime();

    _itemTemplateStore.clear();
    _itemTemplateStore.reserve(sItemSparseStore.GetNumRows());

    // 6.x: items are Item.db2 + Item-sparse.db2 rows (and their hotfixes); the world database only adds server data
    LocaleConstant const locale = sWorld->GetDefaultDbcLocale();
    for (ItemSparseEntry const* sparse : sItemSparseStore)
    {
        ItemEntry const* db2Data = sItemStore.LookupEntry(sparse->ID);
        if (!db2Data)
            continue;

        ItemTemplate& itemTemplate = _itemTemplateStore[sparse->ID];
        itemTemplate.BasicData = db2Data;
        itemTemplate.ExtendedData = sparse;

        itemTemplate.ItemId = sparse->ID;
        itemTemplate.Class = db2Data->Class;
        itemTemplate.SubClass = db2Data->SubClass;
        itemTemplate.SoundOverrideSubclass = db2Data->SoundOverrideSubclass;
        itemTemplate.Name1 = sparse->Name->Str[locale];
        itemTemplate.DisplayInfoID = sDB2Manager.GetItemDisplayId(sparse->ID, 0);
        itemTemplate.Quality = sparse->Quality;
        itemTemplate.Flags = ItemFlags(sparse->Flags[0]);
        itemTemplate.Flags2 = ItemFlags2(sparse->Flags[1]);
        itemTemplate.Flags3 = sparse->Flags[2];
        itemTemplate.BuyCount = std::max(sparse->BuyCount, 1u);
        itemTemplate.BuyPrice = int32(sparse->BuyPrice);
        itemTemplate.SellPrice = sparse->SellPrice;
        itemTemplate.PriceRandomValue = sparse->Unk1;
        itemTemplate.PriceVariance = sparse->Unk2;
        itemTemplate.InventoryType = sparse->InventoryType;
        itemTemplate.AllowableClass = uint32(sparse->AllowableClass);
        itemTemplate.AllowableRace = uint32(sparse->AllowableRace);
        itemTemplate.ItemLevel = sparse->ItemLevel;
        itemTemplate.RequiredLevel = uint32(std::max(sparse->RequiredLevel, 0));
        itemTemplate.RequiredSkill = sparse->RequiredSkill;
        itemTemplate.RequiredSkillRank = sparse->RequiredSkillRank;
        itemTemplate.RequiredSpell = sparse->RequiredSpell;
        itemTemplate.RequiredHonorRank = sparse->RequiredHonorRank;
        itemTemplate.RequiredCityRank = sparse->RequiredCityRank;
        itemTemplate.RequiredReputationFaction = sparse->RequiredReputationFaction;
        itemTemplate.RequiredReputationRank = sparse->RequiredReputationRank;
        itemTemplate.MaxCount = int32(sparse->MaxCount);
        itemTemplate.Stackable = sparse->Stackable ? int32(sparse->Stackable) : 1;
        itemTemplate.ContainerSlots = std::min<uint32>(sparse->ContainerSlots, MAX_BAG_SIZE);

        // runtime stats come from the item's bonus data; these are the base ones
        itemTemplate.StatsCount = MAX_ITEM_PROTO_STATS;
        for (uint8 i = 0; i < MAX_ITEM_PROTO_STATS; ++i)
        {
            itemTemplate.ItemStat[i].ItemStatType = sparse->ItemStatType[i] == -1 ? 0 : uint32(sparse->ItemStatType[i]);
            itemTemplate.ItemStat[i].ItemStatValue = sparse->ItemStatValue[i];
            itemTemplate.ItemStat[i].ItemStatAllocation = sparse->ItemStatAllocation[i];
            itemTemplate.ItemStat[i].ItemStatSocketCostMultiplier = sparse->ItemStatSocketCostMultiplier[i];
        }

        itemTemplate.ScalingStatDistribution = sparse->ScalingStatDistribution;
        itemTemplate.ScalingStatValue = 0;
        itemTemplate.Delay = sparse->Delay;
        itemTemplate.StatScalingFactor = sparse->StatScalingFactor;

        // 6.x armor and weapon damage derive from the item level
        for (uint8 i = 0; i < MAX_ITEM_PROTO_DAMAGES; ++i)
            itemTemplate.Damage[i] = _Damage();
        itemTemplate.GetBaseDamage(itemTemplate.Damage[0].DamageMin, itemTemplate.Damage[0].DamageMax);
        itemTemplate.Damage[0].DamageType = sparse->DamageType;
        itemTemplate.Armor = itemTemplate.GetBaseArmor();
        itemTemplate.HolyRes = 0;
        itemTemplate.FireRes = 0;
        itemTemplate.NatureRes = 0;
        itemTemplate.FrostRes = 0;
        itemTemplate.ShadowRes = 0;
        itemTemplate.ArcaneRes = 0;
        itemTemplate.AmmoType = 0;
        itemTemplate.RangedModRange = sparse->RangedModRange;

        itemTemplate.Bonding = sparse->Bonding;
        itemTemplate.Description = sparse->Description->Str[locale];
        itemTemplate.PageText = sparse->PageText;
        itemTemplate.LanguageID = sparse->LanguageID;
        itemTemplate.PageMaterial = sparse->PageMaterial;
        itemTemplate.StartQuest = sparse->StartQuest;
        itemTemplate.LockID = sparse->LockID;
        itemTemplate.Material = sparse->Material;
        itemTemplate.Sheath = sparse->Sheath;
        itemTemplate.RandomProperty = int32(sparse->RandomProperty);
        itemTemplate.RandomSuffix = int32(sparse->RandomSuffix);
        itemTemplate.Block = 0;
        itemTemplate.ItemSet = sparse->ItemSet;
        itemTemplate.Area = sparse->Area;
        itemTemplate.Map = sparse->Map;
        itemTemplate.BagFamily = sparse->BagFamily;
        itemTemplate.TotemCategory = sparse->TotemCategory;
        for (uint8 i = 0; i < MAX_ITEM_PROTO_SOCKETS; ++i)
        {
            itemTemplate.Socket[i].Color = sparse->SocketColor[i];
            itemTemplate.Socket[i].Content = 0;
        }

        itemTemplate.socketBonus = sparse->SocketBonus;
        itemTemplate.GemProperties = sparse->GemProperties;
        itemTemplate.ArmorDamageModifier = sparse->ArmorDamageModifier;
        itemTemplate.Duration = sparse->Duration;
        itemTemplate.ItemLimitCategory = sparse->ItemLimitCategory;
        itemTemplate.HolidayId = sparse->HolidayID;

        itemTemplate.MaxDurability = FillMaxDurability(db2Data->Class, db2Data->SubClass, sparse->InventoryType, sparse->Quality, sparse->ItemLevel);
        FillDisenchantFields(&itemTemplate.DisenchantID, &itemTemplate.RequiredDisenchantSkill, itemTemplate);

        // server side data, from item_template_addon and item_script_names
        itemTemplate.ScriptId = 0;
        itemTemplate.FoodType = 0;
        itemTemplate.MinMoneyLoot = 0;
        itemTemplate.MaxMoneyLoot = 0;
        itemTemplate.FlagsCu = ItemFlagsCustom(0);
        itemTemplate.SpellPPMRate = 0.0f;

        for (uint8 i = 0; i < MAX_ITEM_PROTO_SPELLS; ++i)
        {
            itemTemplate.Spells[i] = _Spell();
            itemTemplate.Spells[i].SpellTrigger = ITEM_SPELLTRIGGER_ON_USE;
        }

        // the specializations that can use the item (ItemSpecOverride.db2, or ItemSpec.db2 from its stats)
        if (std::vector<ItemSpecOverrideEntry const*> const* itemSpecOverrides = sDB2Manager.GetItemSpecOverrides(sparse->ID))
        {
            for (ItemSpecOverrideEntry const* itemSpecOverride : *itemSpecOverrides)
                if (ChrSpecializationEntry const* specialization = sChrSpecializationStore.LookupEntry(itemSpecOverride->SpecID))
                    itemTemplate.Specializations[0].set(ItemTemplate::CalculateItemSpecBit(specialization));

            itemTemplate.Specializations[1] |= itemTemplate.Specializations[0];
        }
        else
        {
            ItemSpecStats itemSpecStats(db2Data, sparse);
            if (itemSpecStats.ItemSpecStatCount)
            {
                for (ItemSpecEntry const* itemSpec : sItemSpecStore)
                {
                    if (itemSpecStats.ItemType != itemSpec->ItemType)
                        continue;

                    bool hasPrimary = false;
                    bool hasSecondary = itemSpec->SecondaryStat == ITEM_SPEC_STAT_NONE;
                    for (uint32 i = 0; i < itemSpecStats.ItemSpecStatCount; ++i)
                    {
                        if (itemSpecStats.ItemSpecStatTypes[i] == itemSpec->PrimaryStat)
                            hasPrimary = true;
                        if (itemSpecStats.ItemSpecStatTypes[i] == itemSpec->SecondaryStat)
                            hasSecondary = true;
                    }

                    if (!hasPrimary || !hasSecondary)
                        continue;

                    if (ChrSpecializationEntry const* specialization = sChrSpecializationStore.LookupEntry(itemSpec->SpecID))
                        if ((1 << (specialization->ClassID - 1)) & sparse->AllowableClass)
                            itemTemplate.Specializations[itemSpec->MaxLevel > 40].set(ItemTemplate::CalculateItemSpecBit(specialization));
                }
            }
        }
    }

    // item effects (spells): ItemEffect.db2, kept in order; the first MAX_ITEM_PROTO_SPELLS also feed the AC spell slots
    for (ItemEffectEntry const* effectEntry : sItemEffectStore)
    {
        auto itemItr = _itemTemplateStore.find(effectEntry->ItemID);
        if (itemItr == _itemTemplateStore.end())
            continue;

        itemItr->second.Effects.push_back(effectEntry);
    }

    for (auto& [itemId, itemTemplate] : _itemTemplateStore)
    {
        std::sort(itemTemplate.Effects.begin(), itemTemplate.Effects.end(), [](ItemEffectEntry const* left, ItemEffectEntry const* right)
        {
            return left->OrderIndex < right->OrderIndex;
        });

        for (std::size_t i = 0; i < itemTemplate.Effects.size() && i < MAX_ITEM_PROTO_SPELLS; ++i)
        {
            ItemEffectEntry const* effect = itemTemplate.Effects[i];
            _Spell& spell = itemTemplate.Spells[i];
            spell.SpellId = int32(effect->SpellID);
            spell.SpellTrigger = effect->Trigger;
            spell.SpellCharges = effect->Charges;
            spell.SpellPPMRate = 0.0f;
            spell.SpellCooldown = effect->Cooldown;
            spell.SpellCategory = effect->Category;
            spell.SpellCategoryCooldown = effect->CategoryCooldown;

            if (spell.SpellId && spell.SpellCategory && spell.SpellCategoryCooldown)
                sSpellsByCategoryStore[spell.SpellCategory].emplace(true, uint32(spell.SpellId));
        }
    }

    // Check if item templates for DBC referenced character start outfit are present
    std::set<uint32> notFoundOutfit;
    for (CharStartOutfitEntry const* entry : sCharStartOutfitStore)
        for (int j = 0; j < MAX_OUTFIT_ITEMS; ++j)
            if (entry->ItemID[j] > 0 && !GetItemTemplate(entry->ItemID[j]))
                notFoundOutfit.insert(entry->ItemID[j]);

    for (uint32 itemId : notFoundOutfit)
        LOG_ERROR("sql.sql", "Item (Entry: {}) does not exist in Item-sparse.db2 but is referenced in `CharStartOutfit.dbc`", itemId);

    LOG_INFO("server.loading", ">> Loaded {} Item Templates in {} ms", uint32(_itemTemplateStore.size()), GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

void ObjectMgr::LoadItemTemplateAddon()
{
    uint32 oldMSTime = getMSTime();
    uint32 count = 0;

    PreparedQueryResult result = WorldDatabase.Query(WorldDatabase.GetPreparedStatement(WORLD_SEL_ITEM_TEMPLATE_ADDON));
    if (result)
    {
        do
        {
            Field* fields = result->Fetch();
            uint32 itemId = fields[0].Get<uint32>();
            auto itr = _itemTemplateStore.find(itemId);
            if (itr == _itemTemplateStore.end())
            {
                LOG_ERROR("sql.sql", "Item {} specified in `item_template_addon` does not exist, skipped.", itemId);
                continue;
            }

            uint32 minMoneyLoot = fields[3].Get<uint32>();
            uint32 maxMoneyLoot = fields[4].Get<uint32>();
            if (minMoneyLoot > maxMoneyLoot)
            {
                LOG_ERROR("sql.sql", "Minimum money loot specified in `item_template_addon` for item {} was greater than maximum amount, swapping.", itemId);
                std::swap(minMoneyLoot, maxMoneyLoot);
            }

            ItemTemplate& itemTemplate = itr->second;
            itemTemplate.FlagsCu = ItemFlagsCustom(fields[1].Get<uint32>());
            itemTemplate.FoodType = fields[2].Get<uint8>();
            itemTemplate.MinMoneyLoot = minMoneyLoot;
            itemTemplate.MaxMoneyLoot = maxMoneyLoot;
            itemTemplate.SpellPPMRate = fields[5].Get<float>();

            // chance on hit spells of the item proc at the item's rate
            for (_Spell& spell : itemTemplate.Spells)
                if (spell.SpellTrigger == ITEM_SPELLTRIGGER_CHANCE_ON_HIT)
                    spell.SpellPPMRate = itemTemplate.SpellPPMRate;

            if (itemTemplate.HasFlagCu(ITEM_FLAGS_CU_DURATION_REAL_TIME) && !itemTemplate.Duration)
            {
                LOG_ERROR("sql.sql", "Item (Entry {}) has flag ITEM_FLAGS_CU_DURATION_REAL_TIME but it does not have duration limit", itemId);
                itemTemplate.FlagsCu = ItemFlagsCustom(uint32(itemTemplate.FlagsCu) & ~ITEM_FLAGS_CU_DURATION_REAL_TIME);
            }

            ++count;
        } while (result->NextRow());
    }

    LOG_INFO("server.loading", ">> Loaded {} Item Addon Templates in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
}

void ObjectMgr::LoadItemScriptNames()
{
    uint32 oldMSTime = getMSTime();
    uint32 count = 0;

    PreparedQueryResult result = WorldDatabase.Query(WorldDatabase.GetPreparedStatement(WORLD_SEL_ITEM_SCRIPT_NAMES));
    if (result)
    {
        do
        {
            Field* fields = result->Fetch();
            uint32 itemId = fields[0].Get<uint32>();
            auto itr = _itemTemplateStore.find(itemId);
            if (itr == _itemTemplateStore.end())
            {
                LOG_ERROR("sql.sql", "Item {} specified in `item_script_names` does not exist, skipped.", itemId);
                continue;
            }

            itr->second.ScriptId = GetScriptId(fields[1].Get<std::string>());
            ++count;
        } while (result->NextRow());
    }

    LOG_INFO("server.loading", ">> Loaded {} Item Script Names in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
}

ItemTemplate const* ObjectMgr::GetItemTemplate(uint32 entry)
{
    return entry < _itemTemplateStoreFast.size() ? _itemTemplateStoreFast[entry] : nullptr;
}

void ObjectMgr::LoadItemSetNameLocales()
{
    uint32 oldMSTime = getMSTime();

    _itemSetNameLocaleStore.clear();                                 // need for reload case

    QueryResult result = WorldDatabase.Query("SELECT ID, locale, Name FROM item_set_names_locale");

    if (!result)
        return;

    do
    {
        Field* fields = result->Fetch();

        uint32 ID = fields[0].Get<uint32>();

        LocaleConstant locale = GetLocaleByName(fields[1].Get<std::string>());
        if (locale == LOCALE_enUS)
            continue;

        ItemSetNameLocale& data = _itemSetNameLocaleStore[ID];
        AddLocaleString(fields[2].Get<std::string>(), locale, data.Name);
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} Item Set Name Locale Strings in {} ms", uint32(_itemSetNameLocaleStore.size()), GetMSTimeDiffToNow(oldMSTime));
}

void ObjectMgr::LoadItemSetNames()
{
    uint32 oldMSTime = getMSTime();

    _itemSetNameStore.clear();                               // needed for reload case

    std::set<uint32> itemSetItems;

    // fill item set member ids
    for (uint32 entryId = 0; entryId < sItemSetStore.GetNumRows(); ++entryId)
    {
        ItemSetEntry const* setEntry = sItemSetStore.LookupEntry(entryId);
        if (!setEntry)
            continue;

        for (uint32 i = 0; i < MAX_ITEM_SET_ITEMS; ++i)
            if (setEntry->ItemID[i])
                itemSetItems.insert(setEntry->ItemID[i]);
    }

    //                                                  0        1            2
    QueryResult result = WorldDatabase.Query("SELECT `entry`, `name`, `InventoryType` FROM `item_set_names`");

    if (!result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 item set names. DB table `item_set_names` is empty.");
        LOG_INFO("server.loading", " ");
        return;
    }

    _itemSetNameStore.rehash(result->GetRowCount());
    uint32 count = 0;

    do
    {
        Field* fields = result->Fetch();

        uint32 entry = fields[0].Get<uint32>();
        if (itemSetItems.find(entry) == itemSetItems.end())
        {
            LOG_ERROR("sql.sql", "Item set name (Entry: {}) not found in ItemSet.dbc, data useless.", entry);
            continue;
        }

        ItemSetNameEntry& data = _itemSetNameStore[entry];
        data.name = fields[1].Get<std::string>();

        uint32 invType = fields[2].Get<uint8>();
        if (invType >= MAX_INVTYPE)
        {
            LOG_ERROR("sql.sql", "Item set name (Entry: {}) has wrong InventoryType value ({})", entry, invType);
            invType = INVTYPE_NON_EQUIP;
        }

        data.InventoryType = invType;
        itemSetItems.erase(entry);
        ++count;
    } while (result->NextRow());

    if (!itemSetItems.empty())
    {
        ItemTemplate const* pProto;
        for (std::set<uint32>::iterator itr = itemSetItems.begin(); itr != itemSetItems.end(); ++itr)
        {
            uint32 entry = *itr;
            // add data from item_template if available
            pProto = GetItemTemplate(entry);
            if (pProto)
            {
                LOG_ERROR("sql.sql", "Item set part (Entry: {}) does not have entry in `item_set_names`, adding data from `item_template`.", entry);
                ItemSetNameEntry& data = _itemSetNameStore[entry];
                data.name = pProto->Name1;
                data.InventoryType = pProto->InventoryType;
                ++count;
            }
            else
                LOG_ERROR("sql.sql", "Item set part (Entry: {}) does not have entry in `item_set_names`, set will not display properly.", entry);
        }
    }

    LOG_INFO("server.loading", ">> Loaded {} Item Set Names in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

void ObjectMgr::LoadVehicleTemplateAccessories()
{
    uint32 oldMSTime = getMSTime();

    _vehicleTemplateAccessoryStore.clear();                           // needed for reload case

    uint32 count = 0;

    //                                                  0             1              2          3           4             5
    QueryResult result = WorldDatabase.Query("SELECT `entry`, `accessory_entry`, `seat_id`, `minion`, `summontype`, `summontimer` FROM `vehicle_template_accessory`");

    if (!result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 vehicle template accessories. DB table `vehicle_template_accessory` is empty.");
        LOG_INFO("server.loading", " ");
        return;
    }

    do
    {
        Field* fields = result->Fetch();

        uint32 uiEntry      = fields[0].Get<uint32>();
        uint32 uiAccessory  = fields[1].Get<uint32>();
        int8   uiSeat       = int8(fields[2].Get<int8>());
        bool   bMinion      = fields[3].Get<bool>();
        uint8  uiSummonType = fields[4].Get<uint8>();
        uint32 uiSummonTimer = fields[5].Get<uint32>();

        if (!GetCreatureTemplate(uiEntry))
        {
            LOG_ERROR("sql.sql", "Table `vehicle_template_accessory`: creature template entry {} does not exist.", uiEntry);
            continue;
        }

        if (!GetCreatureTemplate(uiAccessory))
        {
            LOG_ERROR("sql.sql", "Table `vehicle_template_accessory`: Accessory {} does not exist.", uiAccessory);
            continue;
        }

        if (_spellClickInfoStore.find(uiEntry) == _spellClickInfoStore.end())
        {
            LOG_ERROR("sql.sql", "Table `vehicle_template_accessory`: creature template entry {} has no data in npc_spellclick_spells", uiEntry);
            continue;
        }

        _vehicleTemplateAccessoryStore[uiEntry].push_back(VehicleAccessory(uiAccessory, uiSeat, bMinion, uiSummonType, uiSummonTimer));

        ++count;
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} Vehicle Template Accessories in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

void ObjectMgr::LoadVehicleAccessories()
{
    uint32 oldMSTime = getMSTime();

    _vehicleAccessoryStore.clear();                           // needed for reload case

    uint32 count = 0;

    //                                                  0             1             2          3           4             5
    QueryResult result = WorldDatabase.Query("SELECT `guid`, `accessory_entry`, `seat_id`, `minion`, `summontype`, `summontimer` FROM `vehicle_accessory`");

    if (!result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 Vehicle Accessories in {} ms", GetMSTimeDiffToNow(oldMSTime));
        LOG_INFO("server.loading", " ");
        return;
    }

    do
    {
        Field* fields = result->Fetch();

        uint32 uiGUID       = fields[0].Get<uint32>();
        uint32 uiAccessory  = fields[1].Get<uint32>();
        int8   uiSeat       = int8(fields[2].Get<int16>());
        bool   bMinion      = fields[3].Get<bool>();
        uint8  uiSummonType = fields[4].Get<uint8>();
        uint32 uiSummonTimer = fields[5].Get<uint32>();

        if (!GetCreatureTemplate(uiAccessory))
        {
            LOG_ERROR("sql.sql", "Table `vehicle_accessory`: Accessory {} does not exist.", uiAccessory);
            continue;
        }

        _vehicleAccessoryStore[uiGUID].push_back(VehicleAccessory(uiAccessory, uiSeat, bMinion, uiSummonType, uiSummonTimer));

        ++count;
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} Vehicle Accessories in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

void ObjectMgr::LoadVehicleSeatAddon()
{
    uint32 oldMSTime = getMSTime();

    _vehicleSeatAddonStore.clear();                           // needed for reload case

    uint32 count = 0;

    //                                                0            1                  2             3             4             5             6
    QueryResult result = WorldDatabase.Query("SELECT `SeatEntry`, `SeatOrientation`, `ExitParamX`, `ExitParamY`, `ExitParamZ`, `ExitParamO`, `ExitParamValue` FROM `vehicle_seat_addon`");

    if (!result)
    {
        LOG_ERROR("server.loading", ">> Loaded 0 vehicle seat addons. DB table `vehicle_seat_addon` is empty.");
        return;
    }

    do
    {
        Field* fields = result->Fetch();

        uint32 seatID = fields[0].Get<uint32>();
        float orientation = fields[1].Get<float>();
        float exitX = fields[2].Get<float>();
        float exitY = fields[3].Get<float>();
        float exitZ = fields[4].Get<float>();
        float exitO = fields[5].Get<float>();
        uint8 exitParam = fields[6].Get<uint8>();

        if (!sVehicleSeatStore.LookupEntry(seatID))
        {
            LOG_ERROR("sql.sql", "Table `vehicle_seat_addon`: SeatID: {} does not exist in VehicleSeat.dbc. Skipping entry.", seatID);
            continue;
        }

        // Sanitizing values
        if (orientation > float(M_PI * 2))
        {
            LOG_ERROR("sql.sql", "Table `vehicle_seat_addon`: SeatID: {} is using invalid angle offset value ({}). Set Value to 0.", seatID, orientation);
            orientation = 0.0f;
        }

        if (exitParam >= AsUnderlyingType(VehicleExitParameters::VehicleExitParamMax))
        {
            LOG_ERROR("sql.sql", "Table `vehicle_seat_addon`: SeatID: {} is using invalid exit parameter value ({}). Setting to 0 (none).", seatID, exitParam);
            continue;
        }

        _vehicleSeatAddonStore[seatID] = VehicleSeatAddon(orientation, exitX, exitY, exitZ, exitO, exitParam);

        ++count;
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} Vehicle Seat Addon entries in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
}

void ObjectMgr::LoadPetLevelInfo()
{
    uint32 oldMSTime = getMSTime();

    //                                                 0               1      2   3     4    5    6    7     8    9      10       11
    QueryResult result = WorldDatabase.Query("SELECT creature_entry, level, hp, mana, str, agi, sta, inte, spi, armor, min_dmg, max_dmg FROM pet_levelstats");

    if (!result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 level pet stats definitions. DB table `pet_levelstats` is empty.");
        LOG_INFO("server.loading", " ");
        return;
    }

    uint32 count = 0;

    do
    {
        Field* fields = result->Fetch();

        uint32 creature_id = fields[0].Get<uint32>();
        if (!GetCreatureTemplate(creature_id))
        {
            LOG_ERROR("sql.sql", "Wrong creature id {} in `pet_levelstats` table, ignoring.", creature_id);
            continue;
        }

        uint32 current_level = fields[1].Get<uint8>();
        if (current_level > sWorld->getIntConfig(CONFIG_MAX_PLAYER_LEVEL))
        {
            if (current_level > STRONG_MAX_LEVEL)        // hardcoded level maximum
                LOG_ERROR("sql.sql", "Wrong (> {}) level {} in `pet_levelstats` table, ignoring.", STRONG_MAX_LEVEL, current_level);
            else
            {
                LOG_DEBUG("sql.sql", "Unused (> MaxPlayerLevel in worldserver.conf) level {} in `pet_levelstats` table, ignoring.", current_level);
                ++count;                                // make result loading percent "expected" correct in case disabled detail mode for example.
            }
            continue;
        }
        else if (current_level < 1)
        {
            LOG_ERROR("sql.sql", "Wrong (<1) level {} in `pet_levelstats` table, ignoring.", current_level);
            continue;
        }

        PetLevelInfo*& pInfoMapEntry = _petInfoStore[creature_id];

        if (!pInfoMapEntry)
            pInfoMapEntry = new PetLevelInfo[sWorld->getIntConfig(CONFIG_MAX_PLAYER_LEVEL)];

        // data for level 1 stored in [0] array element, ...
        PetLevelInfo* pLevelInfo = &pInfoMapEntry[current_level - 1];

        pLevelInfo->health = fields[2].Get<uint32>();
        pLevelInfo->mana   = fields[3].Get<uint32>();
        pLevelInfo->armor  = fields[9].Get<uint32>();
        pLevelInfo->min_dmg = fields[10].Get<uint32>();
        pLevelInfo->max_dmg = fields[11].Get<uint32>();
        for (uint8 i = 0; i < MAX_STATS; i++)
        {
            pLevelInfo->stats[i] = fields[i + 4].Get<uint32>();
        }

        ++count;
    } while (result->NextRow());

    // Fill gaps and check integrity
    for (PetLevelInfoContainer::iterator itr = _petInfoStore.begin(); itr != _petInfoStore.end(); ++itr)
    {
        PetLevelInfo* pInfo = itr->second;

        // fatal error if no level 1 data
        if (!pInfo || pInfo[0].health == 0)
        {
            LOG_ERROR("sql.sql", "Creature {} does not have pet stats data for Level 1!", itr->first);
            exit(1);
        }

        // fill level gaps
        for (uint8 level = 1; level < sWorld->getIntConfig(CONFIG_MAX_PLAYER_LEVEL); ++level)
        {
            if (pInfo[level].health == 0)
            {
                LOG_ERROR("sql.sql", "Creature {} has no data for Level {} pet stats data, using data of Level {}.", itr->first, level + 1, level);
                pInfo[level] = pInfo[level - 1];
            }
        }
    }

    LOG_INFO("server.loading", ">> Loaded {} Level Pet Stats Definitions in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

PetLevelInfo const* ObjectMgr::GetPetLevelInfo(uint32 creature_id, uint8 level) const
{
    if (level > sWorld->getIntConfig(CONFIG_MAX_PLAYER_LEVEL))
        level = sWorld->getIntConfig(CONFIG_MAX_PLAYER_LEVEL);

    PetLevelInfoContainer::const_iterator itr = _petInfoStore.find(creature_id);
    if (itr == _petInfoStore.end())
        return nullptr;

    return &itr->second[level - 1];                         // data for level 1 stored in [0] array element, ...
}

void ObjectMgr::PlayerCreateInfoAddItemHelper(uint32 race_, uint32 class_, uint32 itemId, int32 count)
{
    if (!_playerInfo[race_][class_])
        return;

    if (count > 0)
        _playerInfo[race_][class_]->item.push_back(PlayerCreateInfoItem(itemId, count));
    else
    {
        if (count < -1)
            LOG_ERROR("sql.sql", "Invalid count {} specified on item {} be removed from original player create info (use -1)!", count, itemId);

        for (uint32 gender = 0; gender < GENDER_NONE; ++gender)
        {
            if (CharStartOutfitEntry const* entry = GetCharStartOutfitEntry(race_, class_, gender))
            {
                bool found = false;
                for (uint8 x = 0; x < MAX_OUTFIT_ITEMS; ++x)
                {
                    if (entry->ItemID[x] > 0 && uint32(entry->ItemID[x]) == itemId)
                    {
                        found = true;
                        const_cast<CharStartOutfitEntry*>(entry)->ItemID[x] = 0;
                        break;
                    }
                }

                if (!found)
                    LOG_ERROR("sql.sql", "Item {} specified to be removed from original create info not found in dbc!", itemId);
            }
        }
    }
}

void ObjectMgr::LoadPlayerInfo()
{
    // Load playercreate
    {
        if (_playerInfo.empty() || _playerInfo.size() != sRaceMgr->GetMaxRaces())
        {
            _playerInfo.clear();
            _playerInfo.resize(sRaceMgr->GetMaxRaces());
            for (auto& classVec : _playerInfo)
                classVec.resize(MAX_CLASSES, nullptr);
        }

        uint32 oldMSTime = getMSTime();
        //                                                0     1      2    3        4          5           6
        QueryResult result = WorldDatabase.Query("SELECT race, class, map, zone, position_x, position_y, position_z, orientation FROM playercreateinfo");

        if (!result)
        {
            LOG_INFO("server.loading", " ");
            LOG_WARN("server.loading", ">> Loaded 0 player create definitions. DB table `playercreateinfo` is empty.");
            exit(1);
        }
        else
        {
            uint32 count = 0;

            do
            {
                Field* fields = result->Fetch();

                uint32 current_race  = fields[0].Get<uint8>();
                uint32 current_class = fields[1].Get<uint8>();
                uint32 mapId         = fields[2].Get<uint16>();
                uint32 areaId        = fields[3].Get<uint32>(); // zone
                float  positionX     = fields[4].Get<float>();
                float  positionY     = fields[5].Get<float>();
                float  positionZ     = fields[6].Get<float>();
                float  orientation   = fields[7].Get<float>();

                if (current_race >= sRaceMgr->GetMaxRaces())
                {
                    LOG_ERROR("sql.sql", "Wrong race {} in `playercreateinfo` table, ignoring.", current_race);
                    continue;
                }

                ChrRacesEntry const* rEntry = sChrRacesStore.LookupEntry(current_race);
                if (!rEntry)
                {
                    LOG_ERROR("sql.sql", "Wrong race {} in `playercreateinfo` table, ignoring.", current_race);
                    continue;
                }

                if (current_class >= MAX_CLASSES)
                {
                    LOG_ERROR("sql.sql", "Wrong class {} in `playercreateinfo` table, ignoring.", current_class);
                    continue;
                }

                if (!sChrClassesStore.LookupEntry(current_class))
                {
                    LOG_ERROR("sql.sql", "Wrong class {} in `playercreateinfo` table, ignoring.", current_class);
                    continue;
                }

                // accept DB data only for valid position (and non instanceable)
                if (!MapMgr::IsValidMapCoord(mapId, positionX, positionY, positionZ, orientation))
                {
                    LOG_ERROR("sql.sql", "Wrong home position for class {} race {} pair in `playercreateinfo` table, ignoring.", current_class, current_race);
                    continue;
                }

                if (sMapStore.LookupEntry(mapId)->Instanceable())
                {
                    LOG_ERROR("sql.sql", "Home position in instanceable map for class {} race {} pair in `playercreateinfo` table, ignoring.", current_class, current_race);
                    continue;
                }

                PlayerInfo* info = new PlayerInfo();
                info->mapId = mapId;
                info->areaId = areaId;
                info->positionX = positionX;
                info->positionY = positionY;
                info->positionZ = positionZ;
                info->orientation = orientation;
                info->displayId_m = rEntry->MaleDisplayID;
                info->displayId_f = rEntry->FemaleDisplayID;
                _playerInfo[current_race][current_class] = info;

                ++count;
            } while (result->NextRow());

            LOG_INFO("server.loading", ">> Loaded {} Player Create Definitions in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
            LOG_INFO("server.loading", " ");
        }
    }

    // Load playercreate items
    LOG_INFO("server.loading", "Loading Player Create Items Data...");
    {
        uint32 oldMSTime = getMSTime();
        //                                                0     1      2       3
        QueryResult result = WorldDatabase.Query("SELECT race, class, itemid, amount FROM playercreateinfo_item");

        if (!result)
        {
            LOG_WARN("server.loading", ">> Loaded 0 Custom Player Create Items. DB Table `playercreateinfo_item` Is Empty.");
            LOG_INFO("server.loading", " ");
        }
        else
        {
            uint32 count = 0;

            do
            {
                Field* fields = result->Fetch();

                uint32 current_race = fields[0].Get<uint8>();
                if (current_race >= sRaceMgr->GetMaxRaces())
                {
                    LOG_ERROR("sql.sql", "Wrong race {} in `playercreateinfo_item` table, ignoring.", current_race);
                    continue;
                }

                uint32 current_class = fields[1].Get<uint8>();
                if (current_class >= MAX_CLASSES)
                {
                    LOG_ERROR("sql.sql", "Wrong class {} in `playercreateinfo_item` table, ignoring.", current_class);
                    continue;
                }

                uint32 item_id = fields[2].Get<uint32>();

                if (!GetItemTemplate(item_id))
                {
                    LOG_ERROR("sql.sql", "Item id {} (race {} class {}) in `playercreateinfo_item` table but not listed in `item_template`, ignoring.", item_id, current_race, current_class);
                    continue;
                }

                int32 amount = fields[3].Get<int32>();

                if (!amount)
                {
                    LOG_ERROR("sql.sql", "Item id {} (class {} race {}) have amount == 0 in `playercreateinfo_item` table, ignoring.", item_id, current_race, current_class);
                    continue;
                }

                if (!current_race || !current_class)
                {
                    uint32 min_race = current_race ? current_race : 1;
                    uint32 max_race = current_race ? current_race + 1 : sRaceMgr->GetMaxRaces();
                    uint32 min_class = current_class ? current_class : 1;
                    uint32 max_class = current_class ? current_class + 1 : MAX_CLASSES;
                    for (uint32 r = min_race; r < max_race; ++r)
                        for (uint32 c = min_class; c < max_class; ++c)
                            PlayerCreateInfoAddItemHelper(r, c, item_id, amount);
                }
                else
                    PlayerCreateInfoAddItemHelper(current_race, current_class, item_id, amount);

                ++count;
            } while (result->NextRow());

            LOG_INFO("server.loading", ">> Loaded {} Custom Player Create Items in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
            LOG_INFO("server.loading", " ");
        }
    }

    LOG_INFO("server.loading", "Loading Player Create Skill Data...");
    {
        uint32 oldMSTime = getMSTime();
        uint32 count = 0;
        for (SkillRaceClassInfoEntry const* skill : sSkillRaceClassInfoStore)
        {
            if (skill->Availability != 1 || !sSkillLineStore.LookupEntry(skill->SkillID))
                continue;

            for (uint32 race = RACE_HUMAN; race < sRaceMgr->GetMaxRaces(); ++race)
            {
                if (skill->RaceMask && !(skill->RaceMask & (1u << (race - 1))))
                    continue;

                for (uint32 classId = CLASS_WARRIOR; classId < MAX_CLASSES; ++classId)
                {
                    if (skill->ClassMask && !(skill->ClassMask & (1u << (classId - 1))))
                        continue;

                    if (PlayerInfo* info = _playerInfo[race][classId])
                    {
                        info->skills.push_back(skill);
                        ++count;
                    }
                }
            }
        }
        LOG_INFO("server.loading", ">> Loaded {} Player Create Skills in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
    }

    // Load playercreate spells
    LOG_INFO("server.loading", "Loading Player Create Spell Data...");
    {
        uint32 oldMSTime = getMSTime();

        QueryResult result = WorldDatabase.Query("SELECT racemask, classmask, Spell FROM playercreateinfo_spell_custom");

        if (!result)
        {
            LOG_WARN("server.loading", ">> Loaded 0 player create spells. DB table `playercreateinfo_spell_custom` is empty.");
        }
        else
        {
            uint32 count = 0;

            do
            {
                Field* fields = result->Fetch();
                uint32 raceMask = fields[0].Get<uint32>();
                uint32 classMask = fields[1].Get<uint32>();
                uint32 spellId = fields[2].Get<uint32>();

                if (raceMask != 0 && !(raceMask & sRaceMgr->GetPlayableRaceMask()))
                {
                    LOG_ERROR("sql.sql", "Wrong race mask {} in `playercreateinfo_spell_custom` table, ignoring.", raceMask);
                    continue;
                }

                if (classMask != 0 && !(classMask & CLASSMASK_ALL_PLAYABLE))
                {
                    LOG_ERROR("sql.sql", "Wrong class mask {} in `playercreateinfo_spell_custom` table, ignoring.", classMask);
                    continue;
                }

                for (uint32 raceIndex = RACE_HUMAN; raceIndex < sRaceMgr->GetMaxRaces(); ++raceIndex)
                {
                    if (raceMask == 0 || ((1 << (raceIndex - 1)) & raceMask))
                    {
                        for (uint32 classIndex = CLASS_WARRIOR; classIndex < MAX_CLASSES; ++classIndex)
                        {
                            if (classMask == 0 || ((1 << (classIndex - 1)) & classMask))
                            {
                                if (PlayerInfo* info = _playerInfo[raceIndex][classIndex])
                                {
                                    info->customSpells.push_back(spellId);
                                    ++count;
                                }
                            }
                        }
                    }
                }
            } while (result->NextRow());

            LOG_INFO("server.loading", ">> Loaded {} Custom Player Create Spells in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
            LOG_INFO("server.loading", " ");
        }
    }

    // Load playercreate cast spell
    LOG_INFO("server.loading", "Loading Player Create Cast Spell Data...");
    {
        uint32 oldMSTime = getMSTime();

        QueryResult result = WorldDatabase.Query("SELECT raceMask, classMask, spell FROM playercreateinfo_cast_spell");

        if (!result)
        {
            LOG_WARN("server.loading", ">> Loaded 0 Player Create Cast Spells. DB Table `playercreateinfo_cast_spell` Is Empty.");
        }
        else
        {
            uint32 count = 0;

            do
            {
                Field* fields    = result->Fetch();
                uint32 raceMask  = fields[0].Get<uint32>();
                uint32 classMask = fields[1].Get<uint32>();
                uint32 spellId   = fields[2].Get<uint32>();

                if (raceMask != 0 && !(raceMask & sRaceMgr->GetPlayableRaceMask()))
                {
                    LOG_ERROR("sql.sql", "Wrong race mask {} in `playercreateinfo_cast_spell` table, ignoring.", raceMask);
                    continue;
                }

                if (classMask != 0 && !(classMask & CLASSMASK_ALL_PLAYABLE))
                {
                    LOG_ERROR("sql.sql", "Wrong class mask {} in `playercreateinfo_cast_spell` table, ignoring.", classMask);
                    continue;
                }

                for (uint32 raceIndex = RACE_HUMAN; raceIndex < sRaceMgr->GetMaxRaces(); ++raceIndex)
                {
                    if (raceMask == 0 || ((1 << (raceIndex - 1)) & raceMask))
                    {
                        for (uint32 classIndex = CLASS_WARRIOR; classIndex < MAX_CLASSES; ++classIndex)
                        {
                            if (classMask == 0 || ((1 << (classIndex - 1)) & classMask))
                            {
                                if (PlayerInfo* info = _playerInfo[raceIndex][classIndex])
                                {
                                    info->castSpells.push_back(spellId);
                                    ++count;
                                }
                            }
                        }
                    }
                }
            } while (result->NextRow());

            LOG_INFO("server.loading", ">> Loaded {} Player Create Cast Spells in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
            LOG_INFO("server.loading", " ");
        }
    }

    // Load playercreate actions
    LOG_INFO("server.loading", "Loading Player Create Action Data...");
    {
        uint32 oldMSTime = getMSTime();

        //                                                0     1      2       3       4
        QueryResult result = WorldDatabase.Query("SELECT race, class, button, action, type FROM playercreateinfo_action");

        if (!result)
        {
            LOG_WARN("server.loading", ">> Loaded 0 Player Create Actions. DB Table `playercreateinfo_action` Is Empty.");
            LOG_INFO("server.loading", " ");
        }
        else
        {
            uint32 count = 0;

            do
            {
                Field* fields = result->Fetch();

                uint32 current_race = fields[0].Get<uint8>();
                if (current_race >= sRaceMgr->GetMaxRaces())
                {
                    LOG_ERROR("sql.sql", "Wrong race {} in `playercreateinfo_action` table, ignoring.", current_race);
                    continue;
                }

                uint32 current_class = fields[1].Get<uint8>();
                if (current_class >= MAX_CLASSES)
                {
                    LOG_ERROR("sql.sql", "Wrong class {} in `playercreateinfo_action` table, ignoring.", current_class);
                    continue;
                }

                if (PlayerInfo* info = _playerInfo[current_race][current_class])
                    info->action.push_back(PlayerCreateInfoAction(fields[2].Get<uint16>(), fields[3].Get<uint32>(), fields[4].Get<uint16>()));

                ++count;
            } while (result->NextRow());

            LOG_INFO("server.loading", ">> Loaded {} Player Create Actions in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
            LOG_INFO("server.loading", " ");
        }
    }

    // Loading levels data (class/race dependent)
    LOG_INFO("server.loading", "Loading Player Create Level Stats Data...");
    {
        struct RaceStats
        {
            int16 StatModifier[MAX_STATS];
        };

        std::vector<RaceStats> raceStatModifiers;

        raceStatModifiers.resize(sRaceMgr->GetMaxRaces());

        uint32 oldMSTime = getMSTime();

        //                                                          0       1         2        3         4        5
        QueryResult raceStatsResult  = WorldDatabase.Query("SELECT Race, Strength, Agility, Stamina, Intellect, Spirit FROM player_race_stats");

        if (!raceStatsResult)
        {
            LOG_WARN("server.loading", ">> Loaded 0 race stats definitions. DB table `player_race_stats` is empty.");
            LOG_INFO("server.loading", " ");
            exit(1);
        }

        do
        {
            Field* fields = raceStatsResult->Fetch();

            uint32 current_race = fields[0].Get<uint8>();
            if (current_race >= sRaceMgr->GetMaxRaces())
            {
                LOG_ERROR("sql.sql", "Wrong race {} in `player_race_stats` table, ignoring.", current_race);
                continue;
            }

            for (uint32 i = 0; i < MAX_STATS; ++i)
                raceStatModifiers[current_race].StatModifier[i] = fields[i + 1].Get<int16>();

        } while (raceStatsResult->NextRow());

        //                                                 0      1       2         3        4         5        6       7        8
        QueryResult result = WorldDatabase.Query("SELECT Class, Level, Strength, Agility, Stamina, Intellect, Spirit, BaseHP, BaseMana FROM player_class_stats");

        if (!result)
        {
            LOG_ERROR("server.loading", ">> Loaded 0 level stats definitions. DB table `player_class_stats` is empty.");
            exit(1);
        }

        uint32 count = 0;

        do
        {
            Field* fields = result->Fetch();

            uint32 current_class = fields[0].Get<uint8>();
            if (current_class >= MAX_CLASSES)
            {
                LOG_ERROR("sql.sql", "Wrong class {} in `player_class_stats` table, ignoring.", current_class);
                continue;
            }

            uint32 current_level = fields[1].Get<uint8>();
            if (current_level > sWorld->getIntConfig(CONFIG_MAX_PLAYER_LEVEL))
            {
                if (current_level > STRONG_MAX_LEVEL) // hardcoded level maximum
                    LOG_ERROR("sql.sql", "Wrong (> {}) level {} in `player_class_stats` table, ignoring.", STRONG_MAX_LEVEL, current_level);
                else
                    LOG_DEBUG("sql.sql", "Unused (> MaxPlayerLevel in worldserver.conf) level {} in `player_class_stats` table, ignoring.", current_level);

                continue;
            }

            for (std::size_t race = 0; race < raceStatModifiers.size(); ++race)
            {
                if (PlayerInfo* info = _playerInfo[race][current_class])
                {
                    if (!info->levelInfo)
                        info->levelInfo = new PlayerLevelInfo[sWorld->getIntConfig(CONFIG_MAX_PLAYER_LEVEL)];

                    PlayerLevelInfo& levelInfo = info->levelInfo[current_level - 1];
                    for (int i = 0; i < MAX_STATS; ++i)
                        levelInfo.stats[i] = fields[i + 2].Get<uint16>() + raceStatModifiers[race].StatModifier[i];
                }
            }

            PlayerClassInfo* info = _playerClassInfo[current_class];
            if (!info)
            {
                info = new PlayerClassInfo();
                info->levelInfo = new PlayerClassLevelInfo[sWorld->getIntConfig(CONFIG_MAX_PLAYER_LEVEL)];
                _playerClassInfo[current_class] = info;
            }

            PlayerClassLevelInfo& levelInfo = info->levelInfo[current_level - 1];

            levelInfo.basehealth = fields[7].Get<uint32>();
            levelInfo.basemana = fields[8].Get<uint32>();

            ++count;
        } while (result->NextRow());

        // Fill gaps and check integrity
        for (int race = RACE_HUMAN; race < sRaceMgr->GetMaxRaces(); ++race)
        {
            // skip non existed races
            if (!sChrRacesStore.LookupEntry(race))
                continue;

            for (int class_ = 0; class_ < MAX_CLASSES; ++class_)
            {
                // skip non existed classes
                if (!sChrClassesStore.LookupEntry(class_))
                    continue;

                PlayerClassInfo* pClassInfo = _playerClassInfo[class_];
                PlayerInfo* info = _playerInfo[race][class_];
                if (!info)
                    continue;

                // skip expansion races if not playing with expansion
                if (sWorld->getIntConfig(CONFIG_EXPANSION) < EXPANSION_THE_BURNING_CRUSADE && (race == RACE_BLOODELF || race == RACE_DRAENEI))
                    continue;

                // skip expansion classes if not playing with expansion
                if (sWorld->getIntConfig(CONFIG_EXPANSION) < EXPANSION_WRATH_OF_THE_LICH_KING && class_ == CLASS_DEATH_KNIGHT)
                    continue;

                // fatal error if no initial stats data
                if (!info->levelInfo || (info->levelInfo[sWorld->getIntConfig(CONFIG_START_PLAYER_LEVEL) - 1].stats[0] == 0 && class_ != CLASS_DEATH_KNIGHT) || (info->levelInfo[sWorld->getIntConfig(CONFIG_START_HEROIC_PLAYER_LEVEL) - 1].stats[0] == 0 && class_ == CLASS_DEATH_KNIGHT))
                {
                    LOG_ERROR("sql.sql", "Race {} class {} initial level does not have stats data!", race, class_);
                    exit(1);
                }

                // fatal error if no initial health/mana data
                if (!pClassInfo->levelInfo || (pClassInfo->levelInfo[sWorld->getIntConfig(CONFIG_START_PLAYER_LEVEL) - 1].basehealth == 0 && class_ != CLASS_DEATH_KNIGHT) || (pClassInfo->levelInfo[sWorld->getIntConfig(CONFIG_START_HEROIC_PLAYER_LEVEL) - 1].basehealth == 0 && class_ == CLASS_DEATH_KNIGHT))
                {
                    LOG_ERROR("sql.sql", "Class {} initial level does not have health/mana data!", class_);
                    exit(1);
                }

                // fill level gaps for stats
                for (uint8 level = 1; level < sWorld->getIntConfig(CONFIG_MAX_PLAYER_LEVEL); ++level)
                {
                    if ((info->levelInfo[level].stats[0] == 0 && class_ != CLASS_DEATH_KNIGHT) || (level >= sWorld->getIntConfig(CONFIG_START_HEROIC_PLAYER_LEVEL) && info->levelInfo[level].stats[0] == 0 && class_ == CLASS_DEATH_KNIGHT))
                    {
                        LOG_ERROR("sql.sql", "Race {} class {} level {} does not have stats data. Using stats data of level {}.", race, class_, level + 1, level);
                        info->levelInfo[level] = info->levelInfo[level - 1];
                    }
                }

                // fill level gaps for health/mana
                for (uint8 level = 1; level < sWorld->getIntConfig(CONFIG_MAX_PLAYER_LEVEL); ++level)
                {
                    if ((pClassInfo->levelInfo[level].basehealth == 0 && class_ != CLASS_DEATH_KNIGHT) || (level >= sWorld->getIntConfig(CONFIG_START_HEROIC_PLAYER_LEVEL) && pClassInfo->levelInfo[level].basehealth == 0 && class_ == CLASS_DEATH_KNIGHT))
                    {
                        LOG_ERROR("sql.sql", "Class {} level {} does not have health/mana data. Using stats data of level {}.", class_, level + 1, level);
                        pClassInfo->levelInfo[level] = pClassInfo->levelInfo[level - 1];
                    }
                }
            }
        }

        LOG_INFO("server.loading", ">> Loaded {} Level Stats Definitions in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
        LOG_INFO("server.loading", " ");
    }

    // Loading xp per level data
    LOG_INFO("server.loading", "Loading Player Create XP Data...");
    {
        uint32 oldMSTime = getMSTime();

        _playerXPperLevel.resize(sWorld->getIntConfig(CONFIG_MAX_PLAYER_LEVEL));
        for (uint8 level = 0; level < sWorld->getIntConfig(CONFIG_MAX_PLAYER_LEVEL); ++level)
            _playerXPperLevel[level] = 0;

        //                                                 0    1
        QueryResult result  = WorldDatabase.Query("SELECT Level, Experience FROM player_xp_for_level");

        if (!result)
        {
            LOG_WARN("server.loading", ">> Loaded 0 xp for level definitions. DB table `player_xp_for_level` is empty.");
            LOG_INFO("server.loading", " ");
            exit(1);
        }

        uint32 count = 0;

        do
        {
            Field* fields = result->Fetch();

            uint32 current_level = fields[0].Get<uint8>();
            uint32 current_xp    = fields[1].Get<uint32>();

            if (current_level >= sWorld->getIntConfig(CONFIG_MAX_PLAYER_LEVEL))
            {
                if (current_level > STRONG_MAX_LEVEL)        // hardcoded level maximum
                    LOG_ERROR("sql.sql", "Wrong (> {}) level {} in `player_xp_for_level` table, ignoring.", STRONG_MAX_LEVEL, current_level);
                else
                {
                    LOG_DEBUG("sql.sql", "Unused (> MaxPlayerLevel in worldserver.conf) level {} in `player_xp_for_levels` table, ignoring.", current_level);
                    ++count;                                // make result loading percent "expected" correct in case disabled detail mode for example.
                }
                continue;
            }
            //PlayerXPperLevel
            _playerXPperLevel[current_level] = current_xp;
            ++count;
        } while (result->NextRow());

        // fill level gaps
        for (uint8 level = 1; level < sWorld->getIntConfig(CONFIG_MAX_PLAYER_LEVEL); ++level)
        {
            if (_playerXPperLevel[level] == 0)
            {
                LOG_ERROR("sql.sql", "Level {} does not have XP for level data. Using data of level [{}] + 100.", level + 1, level);
                _playerXPperLevel[level] = _playerXPperLevel[level - 1] + 100;
            }
        }

        LOG_INFO("server.loading", ">> Loaded {} XP For Level Definitions in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
        LOG_INFO("server.loading", " ");
    }
}

void ObjectMgr::GetPlayerClassLevelInfo(uint32 class_, uint8 level, PlayerClassLevelInfo* info) const
{
    if (level < 1 || class_ >= MAX_CLASSES)
        return;

    PlayerClassInfo const* pInfo = _playerClassInfo[class_];

    if (level > sWorld->getIntConfig(CONFIG_MAX_PLAYER_LEVEL))
        level = sWorld->getIntConfig(CONFIG_MAX_PLAYER_LEVEL);

    *info = pInfo->levelInfo[level - 1];
}

void ObjectMgr::GetPlayerLevelInfo(uint32 race, uint32 class_, uint8 level, PlayerLevelInfo* info) const
{
    if (level < 1 || race >= sRaceMgr->GetMaxRaces() || class_ >= MAX_CLASSES)
        return;

    PlayerInfo const* pInfo = _playerInfo[race][class_];
    if (!pInfo)
        return;

    if (level <= sWorld->getIntConfig(CONFIG_MAX_PLAYER_LEVEL))
        *info = pInfo->levelInfo[level - 1];
    else
        BuildPlayerLevelInfo(race, class_, level, info);
}

void ObjectMgr::BuildPlayerLevelInfo(uint8 race, uint8 _class, uint8 level, PlayerLevelInfo* info) const
{
    // base data (last known level)
    *info = _playerInfo[race][_class]->levelInfo[sWorld->getIntConfig(CONFIG_MAX_PLAYER_LEVEL) - 1];

    // if conversion from uint32 to uint8 causes unexpected behaviour, change lvl to uint32
    for (uint8 lvl = sWorld->getIntConfig(CONFIG_MAX_PLAYER_LEVEL) - 1; lvl < level; ++lvl)
    {
        switch (_class)
        {
            case CLASS_WARRIOR:
                info->stats[STAT_STRENGTH]  += (lvl > 23 ? 2 : (lvl > 1  ? 1 : 0));
                info->stats[STAT_STAMINA]   += (lvl > 23 ? 2 : (lvl > 1  ? 1 : 0));
                info->stats[STAT_AGILITY]   += (lvl > 36 ? 1 : (lvl > 6 && (lvl % 2) ? 1 : 0));
                info->stats[STAT_INTELLECT] += (lvl > 9 && !(lvl % 2) ? 1 : 0);
                info->stats[STAT_SPIRIT]    += (lvl > 9 && !(lvl % 2) ? 1 : 0);
                break;
            case CLASS_PALADIN:
                info->stats[STAT_STRENGTH]  += (lvl > 3  ? 1 : 0);
                info->stats[STAT_STAMINA]   += (lvl > 33 ? 2 : (lvl > 1 ? 1 : 0));
                info->stats[STAT_AGILITY]   += (lvl > 38 ? 1 : (lvl > 7 && !(lvl % 2) ? 1 : 0));
                info->stats[STAT_INTELLECT] += (lvl > 6 && (lvl % 2) ? 1 : 0);
                info->stats[STAT_SPIRIT]    += (lvl > 7 ? 1 : 0);
                break;
            case CLASS_HUNTER:
                info->stats[STAT_STRENGTH]  += (lvl > 4  ? 1 : 0);
                info->stats[STAT_STAMINA]   += (lvl > 4  ? 1 : 0);
                info->stats[STAT_AGILITY]   += (lvl > 33 ? 2 : (lvl > 1 ? 1 : 0));
                info->stats[STAT_INTELLECT] += (lvl > 8 && (lvl % 2) ? 1 : 0);
                info->stats[STAT_SPIRIT]    += (lvl > 38 ? 1 : (lvl > 9 && !(lvl % 2) ? 1 : 0));
                break;
            case CLASS_ROGUE:
                info->stats[STAT_STRENGTH]  += (lvl > 5  ? 1 : 0);
                info->stats[STAT_STAMINA]   += (lvl > 4  ? 1 : 0);
                info->stats[STAT_AGILITY]   += (lvl > 16 ? 2 : (lvl > 1 ? 1 : 0));
                info->stats[STAT_INTELLECT] += (lvl > 8 && !(lvl % 2) ? 1 : 0);
                info->stats[STAT_SPIRIT]    += (lvl > 38 ? 1 : (lvl > 9 && !(lvl % 2) ? 1 : 0));
                break;
            case CLASS_PRIEST:
                info->stats[STAT_STRENGTH]  += (lvl > 9 && !(lvl % 2) ? 1 : 0);
                info->stats[STAT_STAMINA]   += (lvl > 5  ? 1 : 0);
                info->stats[STAT_AGILITY]   += (lvl > 38 ? 1 : (lvl > 8 && (lvl % 2) ? 1 : 0));
                info->stats[STAT_INTELLECT] += (lvl > 22 ? 2 : (lvl > 1 ? 1 : 0));
                info->stats[STAT_SPIRIT]    += (lvl > 3  ? 1 : 0);
                break;
            case CLASS_SHAMAN:
                info->stats[STAT_STRENGTH]  += (lvl > 34 ? 1 : (lvl > 6 && (lvl % 2) ? 1 : 0));
                info->stats[STAT_STAMINA]   += (lvl > 4 ? 1 : 0);
                info->stats[STAT_AGILITY]   += (lvl > 7 && !(lvl % 2) ? 1 : 0);
                info->stats[STAT_INTELLECT] += (lvl > 5 ? 1 : 0);
                info->stats[STAT_SPIRIT]    += (lvl > 4 ? 1 : 0);
                break;
            case CLASS_MAGE:
                info->stats[STAT_STRENGTH]  += (lvl > 9 && !(lvl % 2) ? 1 : 0);
                info->stats[STAT_STAMINA]   += (lvl > 5  ? 1 : 0);
                info->stats[STAT_AGILITY]   += (lvl > 9 && !(lvl % 2) ? 1 : 0);
                info->stats[STAT_INTELLECT] += (lvl > 24 ? 2 : (lvl > 1 ? 1 : 0));
                info->stats[STAT_SPIRIT]    += (lvl > 33 ? 2 : (lvl > 2 ? 1 : 0));
                break;
            case CLASS_WARLOCK:
                info->stats[STAT_STRENGTH]  += (lvl > 9 && !(lvl % 2) ? 1 : 0);
                info->stats[STAT_STAMINA]   += (lvl > 38 ? 2 : (lvl > 3 ? 1 : 0));
                info->stats[STAT_AGILITY]   += (lvl > 9 && !(lvl % 2) ? 1 : 0);
                info->stats[STAT_INTELLECT] += (lvl > 33 ? 2 : (lvl > 2 ? 1 : 0));
                info->stats[STAT_SPIRIT]    += (lvl > 38 ? 2 : (lvl > 3 ? 1 : 0));
                break;
            case CLASS_DRUID:
                info->stats[STAT_STRENGTH]  += (lvl > 38 ? 2 : (lvl > 6 && (lvl % 2) ? 1 : 0));
                info->stats[STAT_STAMINA]   += (lvl > 32 ? 2 : (lvl > 4 ? 1 : 0));
                info->stats[STAT_AGILITY]   += (lvl > 38 ? 2 : (lvl > 8 && (lvl % 2) ? 1 : 0));
                info->stats[STAT_INTELLECT] += (lvl > 38 ? 3 : (lvl > 4 ? 1 : 0));
                info->stats[STAT_SPIRIT]    += (lvl > 38 ? 3 : (lvl > 5 ? 1 : 0));
        }
    }
}

void ObjectMgr::LoadQuests()
{
    uint32 oldMSTime = getMSTime();

    // For reload case
    for (QuestMap::const_iterator itr = _questTemplates.begin(); itr != _questTemplates.end(); ++itr)
        delete itr->second;
    _questTemplates.clear();

    mExclusiveQuestGroups.clear();
    _breadcrumbsForQuest.clear();
    _questAreaTriggerStore.clear();

    QueryResult result = WorldDatabase.Query("SELECT "
        //0  1          2           3               4         5            6            7                  8                9                   10
        "ID, QuestType, QuestLevel, QuestPackageID, MinLevel, QuestSortID, QuestInfoID, SuggestedGroupNum, RewardNextQuest, RewardXPDifficulty, RewardXPMultiplier, "
        //11          12                     13                     14                15                  16           17           18               19         20     21
        "RewardMoney, RewardMoneyDifficulty, RewardMoneyMultiplier, RewardBonusMoney, RewardDisplaySpell, RewardSpell, RewardHonor, RewardKillHonor, StartItem, Flags, FlagsEx, "
        //22          23             24         25                 26           27             28         29
        "RewardItem1, RewardAmount1, ItemDrop1, ItemDropQuantity1, RewardItem2, RewardAmount2, ItemDrop2, ItemDropQuantity2, "
        //30          31             32         33                 34           35             36         37
        "RewardItem3, RewardAmount3, ItemDrop3, ItemDropQuantity3, RewardItem4, RewardAmount4, ItemDrop4, ItemDropQuantity4, "
        //38                  39                         40                          41                   42                         43
        "RewardChoiceItemID1, RewardChoiceItemQuantity1, RewardChoiceItemDisplayID1, RewardChoiceItemID2, RewardChoiceItemQuantity2, RewardChoiceItemDisplayID2, "
        //44                  45                         46                          47                   48                         49
        "RewardChoiceItemID3, RewardChoiceItemQuantity3, RewardChoiceItemDisplayID3, RewardChoiceItemID4, RewardChoiceItemQuantity4, RewardChoiceItemDisplayID4, "
        //50                  51                         52                          53                   54                         55
        "RewardChoiceItemID5, RewardChoiceItemQuantity5, RewardChoiceItemDisplayID5, RewardChoiceItemID6, RewardChoiceItemQuantity6, RewardChoiceItemDisplayID6, "
        //56           57    58    59           60           61             62                 63                 64                 65             66
        "POIContinent, POIx, POIy, POIPriority, RewardTitle, RewardTalents, RewardArenaPoints, RewardSkillLineID, RewardNumSkillUps, PortraitGiver, PortraitTurnIn, "
        //67               68                   69                      70                71                   72
        "RewardFactionID1, RewardFactionValue1, RewardFactionOverride1, RewardFactionID2, RewardFactionValue2, RewardFactionOverride2, "
        //73               74                   75                      76                77                   78
        "RewardFactionID3, RewardFactionValue3, RewardFactionOverride3, RewardFactionID4, RewardFactionValue4, RewardFactionOverride4, "
        //79               80                   81                      82
        "RewardFactionID5, RewardFactionValue5, RewardFactionOverride5, RewardFactionFlags, "
        //83                84                  85                 86                  87                 88                  89                 90
        "RewardCurrencyID1, RewardCurrencyQty1, RewardCurrencyID2, RewardCurrencyQty2, RewardCurrencyID3, RewardCurrencyQty3, RewardCurrencyID4, RewardCurrencyQty4, "
        //91                 92                  93           94           95
        "AcceptedSoundKitID, CompleteSoundKitID, AreaGroupID, TimeAllowed, AllowableRaces, "
        //96       97              98                99               100                101                102                 103                 104
        "LogTitle, LogDescription, QuestDescription, AreaDescription, PortraitGiverText, PortraitGiverName, PortraitTurnInText, PortraitTurnInName, QuestCompletionLog"
        " FROM quest_template");
    if (!result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 quests definitions. DB table `quest_template` is empty.");
        LOG_INFO("server.loading", " ");
        return;
    }

    // create multimap previous quest for each existed quest
    // some quests can have many previous maps set by NextQuestId in previous quest
    // for example set of race quests can lead to single not race specific quest
    do
    {
        Field* fields = result->Fetch();

        Quest* newQuest = new Quest(fields);

        // a choice reward with zero quantity would be shown to the player but never given
        bool hasZeroCountChoice = false;
        for (uint8 j = 0; j < QUEST_REWARD_CHOICES_COUNT; ++j)
        {
            if (newQuest->RewardChoiceItemId[j] && !newQuest->RewardChoiceItemCount[j])
            {
                LOG_ERROR("sql.sql",
                    "Quest {} has `RewardChoiceItemId{}` = {} but `RewardChoiceItemCount{}` = 0, quest skipped.",
                    newQuest->GetQuestId(), j + 1, newQuest->RewardChoiceItemId[j], j + 1);
                hasZeroCountChoice = true;
            }
        }

        if (hasZeroCountChoice)
        {
            delete newQuest;
            continue;
        }

        _questTemplates[newQuest->GetQuestId()] = newQuest;
    } while (result->NextRow());

    // pussywizard:
    {
        uint32 max = 0;
        for (QuestMap::const_iterator itr = _questTemplates.begin(); itr != _questTemplates.end(); ++itr)
            if (itr->first > max)
                max = itr->first;
        if (max)
        {
            _questTemplatesFast.clear();
            _questTemplatesFast.resize(max + 1, nullptr);
            for (QuestMap::iterator itr = _questTemplates.begin(); itr != _questTemplates.end(); ++itr)
                _questTemplatesFast[itr->first] = itr->second;
        }
    }

    std::map<uint32, uint32> usedMailTemplates;

    // Load `quest_details`
    //                                   0   1       2       3       4       5            6            7            8
    result = WorldDatabase.Query("SELECT ID, Emote1, Emote2, Emote3, Emote4, EmoteDelay1, EmoteDelay2, EmoteDelay3, EmoteDelay4 FROM quest_details");

    if (!result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 quest details. DB table `quest_details` is empty.");
    }
    else
    {
        do
        {
            Field* fields = result->Fetch();
            uint32 questId = fields[0].Get<uint32>();

            auto itr = _questTemplates.find(questId);
            if (itr != _questTemplates.end())
                itr->second->LoadQuestDetails(fields);
            else
                LOG_ERROR("sql.sql", "Table `quest_details` has data for quest {} but such quest does not exist", questId);
        } while (result->NextRow());
    }

    // Load `quest_request_items`
    //                                   0   1                2                  3                     4                       5
    result = WorldDatabase.Query("SELECT ID, EmoteOnComplete, EmoteOnIncomplete, EmoteOnCompleteDelay, EmoteOnIncompleteDelay, CompletionText FROM quest_request_items");

    if (!result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 quest request items. DB table `quest_request_items` is empty.");
    }
    else
    {
        do
        {
            Field* fields = result->Fetch();
            uint32 questId = fields[0].Get<uint32>();

            auto itr = _questTemplates.find(questId);
            if (itr != _questTemplates.end())
                itr->second->LoadQuestRequestItems(fields);
            else
                LOG_ERROR("sql.sql", "Table `quest_request_items` has data for quest {} but such quest does not exist", questId);
        } while (result->NextRow());
    }

    // Load `quest_offer_reward`
    //                                   0   1       2       3       4       5            6            7            8            9
    result = WorldDatabase.Query("SELECT ID, Emote1, Emote2, Emote3, Emote4, EmoteDelay1, EmoteDelay2, EmoteDelay3, EmoteDelay4, RewardText FROM quest_offer_reward");

    if (!result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 quest reward emotes. DB table `quest_offer_reward` is empty.");
    }
    else
    {
        do
        {
            Field* fields = result->Fetch();
            uint32 questId = fields[0].Get<uint32>();

            auto itr = _questTemplates.find(questId);
            if (itr != _questTemplates.end())
                itr->second->LoadQuestOfferReward(fields);
            else
                LOG_ERROR("sql.sql", "Table `quest_offer_reward` has data for quest {} but such quest does not exist", questId);
        } while (result->NextRow());
    }

    // Load `quest_template_addon`
    //                                   0   1         2                 3              4            5            6               7                      8
    result = WorldDatabase.Query("SELECT ID, MaxLevel, AllowableClasses, SourceSpellID, PrevQuestID, NextQuestID, ExclusiveGroup, BreadcrumbForQuestId, RewardMailTemplateID, "
                                 //9                10               11                   12                     13                     14                   15                   16                 17                     18
                                 "RewardMailDelay, RequiredSkillID, RequiredSkillPoints, RequiredMinRepFaction, RequiredMaxRepFaction, RequiredMinRepValue, RequiredMaxRepValue, ProvidedItemCount, RewardMailSenderEntry, SpecialFlags FROM quest_template_addon LEFT JOIN quest_mail_sender ON Id=QuestId");

    if (!result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 quest template addons. DB table `quest_template_addon` is empty.");
    }
    else
    {
        do
        {
            Field* fields = result->Fetch();
            uint32 questId = fields[0].Get<uint32>();

            auto itr = _questTemplates.find(questId);
            if (itr != _questTemplates.end())
                itr->second->LoadQuestTemplateAddon(fields);
            else
                LOG_ERROR("sql.sql", "Table `quest_template_addon` has data for quest {} but such quest does not exist", questId);
        } while (result->NextRow());
    }

    // Load `quest_objectives` order by descending storage index to reduce resizes
    //                                   0   1        2     3             4         5       6      7         8
    result = WorldDatabase.Query("SELECT ID, QuestID, Type, StorageIndex, ObjectID, Amount, Flags, UnkFloat, Description FROM quest_objectives ORDER BY StorageIndex DESC");

    if (!result)
    {
        LOG_ERROR("server.loading", ">> Loaded 0 quest objectives. DB table `quest_objectives` is empty.");
    }
    else
    {
        do
        {
            Field* fields = result->Fetch();
            uint32 questId = fields[1].Get<uint32>();

            auto itr = _questTemplates.find(questId);
            if (itr != _questTemplates.end())
                itr->second->LoadQuestObjective(fields);
            else
                LOG_ERROR("server.loading", "Table `quest_objectives` has objective for quest {} but such quest does not exist", questId);
        } while (result->NextRow());
    }

    // Load `quest_visual_effect` join table with quest_objectives because visual effects are based on objective ID (core stores objectives by their index in quest)
    //                                   0     1     2          3        4
    result = WorldDatabase.Query("SELECT v.ID, o.ID, o.QuestID, v.Index, v.VisualEffect FROM quest_visual_effect AS v LEFT JOIN quest_objectives AS o ON v.ID = o.ID ORDER BY v.Index DESC");

    if (!result)
    {
        LOG_ERROR("server.loading", ">> Loaded 0 quest visual effects. DB table `quest_visual_effect` is empty.");
    }
    else
    {
        do
        {
            Field* fields = result->Fetch();
            uint32 vID = fields[0].Get<uint32>();
            uint32 oID = fields[1].Get<uint32>();

            if (!vID)
            {
                LOG_ERROR("server.loading", "Table `quest_visual_effect` has visual effect for null objective id");
                continue;
            }

            // objID will be null if match for table join is not found
            if (vID != oID)
            {
                LOG_ERROR("server.loading", "Table `quest_visual_effect` has visual effect for objective {} but such objective does not exist.", vID);
                continue;
            }

            uint32 questId = fields[2].Get<uint32>();

            // Do not throw error here because error for non existing quest is thrown while loading quest objectives. we do not need duplication
            auto itr = _questTemplates.find(questId);
            if (itr != _questTemplates.end())
                itr->second->LoadQuestObjectiveVisualEffect(fields);
        } while (result->NextRow());
    }

    // Post processing
    for (QuestMap::iterator iter = _questTemplates.begin(); iter != _questTemplates.end(); ++iter)
    {
        // skip post-loading checks for disabled quests
        if (sDisableMgr->IsDisabledFor(DISABLE_TYPE_QUEST, iter->first, nullptr))
            continue;

        Quest* qinfo = iter->second;

        // additional quest integrity checks (GO, creature_template and item_template must be loaded already)

        if (qinfo->GetQuestType() >= MAX_QUEST_TYPES)
            LOG_ERROR("sql.sql", "Quest {} has `Method` = {}, expected values are 0, 1 or 2.", qinfo->GetQuestId(), qinfo->GetQuestType());

        if (qinfo->SpecialFlags & ~QUEST_SPECIAL_FLAGS_DB_ALLOWED)
        {
            LOG_ERROR("sql.sql", "Quest {} has `SpecialFlags` = {} > max allowed value. Correct `SpecialFlags` to value <= {}",
                qinfo->GetQuestId(), qinfo->SpecialFlags, QUEST_SPECIAL_FLAGS_DB_ALLOWED);
            qinfo->SpecialFlags &= QUEST_SPECIAL_FLAGS_DB_ALLOWED;
        }

        if (qinfo->Flags & QUEST_FLAGS_DAILY && qinfo->Flags & QUEST_FLAGS_WEEKLY)
        {
            LOG_ERROR("sql.sql", "Weekly Quest {} is marked as daily quest in `Flags`, removed daily flag.", qinfo->GetQuestId());
            qinfo->Flags &= ~QUEST_FLAGS_DAILY;
        }

        if (qinfo->Flags & QUEST_FLAGS_DAILY)
        {
            if (!(qinfo->SpecialFlags & QUEST_SPECIAL_FLAGS_REPEATABLE))
            {
                LOG_DEBUG("sql.sql", "Daily Quest {} not marked as repeatable in `SpecialFlags`, added.", qinfo->GetQuestId());
                qinfo->SpecialFlags |= QUEST_SPECIAL_FLAGS_REPEATABLE;
            }
        }

        if (qinfo->Flags & QUEST_FLAGS_WEEKLY)
        {
            if (!(qinfo->SpecialFlags & QUEST_SPECIAL_FLAGS_REPEATABLE))
            {
                LOG_DEBUG("sql.sql", "Weekly Quest {} not marked as repeatable in `SpecialFlags`, added.", qinfo->GetQuestId());
                qinfo->SpecialFlags |= QUEST_SPECIAL_FLAGS_REPEATABLE;
            }
        }

        if (qinfo->SpecialFlags & QUEST_SPECIAL_FLAGS_MONTHLY)
        {
            if (!(qinfo->SpecialFlags & QUEST_SPECIAL_FLAGS_REPEATABLE))
            {
                LOG_DEBUG("sql.sql", "Monthly quest {} not marked as repeatable in `SpecialFlags`, added.", qinfo->GetQuestId());
                qinfo->SpecialFlags |= QUEST_SPECIAL_FLAGS_REPEATABLE;
            }
        }

        if (qinfo->Flags & QUEST_FLAGS_TRACKING)
        {
            // at auto-reward can be rewarded only RewardChoiceItemId[0]
            for (int j = 1; j < QUEST_REWARD_CHOICES_COUNT; ++j )
            {
                if (uint32 id = qinfo->RewardChoiceItemId[j])
                {
                    LOG_ERROR("sql.sql", "Quest {} has `RewardChoiceItemId{}` = {} but item from `RewardChoiceItemId{}` can't be rewarded with quest flag QUEST_FLAGS_TRACKING.",
                        qinfo->GetQuestId(), j+1, id, j+1);
                    // no changes, quest ignore this data
                }
            }
        }

        if (qinfo->MinLevel == -1 || qinfo->MinLevel > DEFAULT_MAX_LEVEL)
        {
            LOG_ERROR("sql.sql", "Quest {} should be disabled because `MinLevel` = {}", qinfo->GetQuestId(), int32(qinfo->MinLevel));
            // no changes needed, sending -1 in SMSG_QUEST_QUERY_RESPONSE is valid
        }

        // client quest log visual (area case)
        if (qinfo->QuestSortID > 0)
        {
            if (!sAreaTableStore.LookupEntry(qinfo->QuestSortID))
            {
                LOG_ERROR("sql.sql", "Quest {} has `QuestSortID` = {} (zone case) but zone with this id does not exist.",
                    qinfo->GetQuestId(), qinfo->QuestSortID);
                // no changes, quest not dependent from this value but can have problems at client
            }
        }
        // client quest log visual (sort case)
        if (qinfo->QuestSortID < 0)
        {
            QuestSortEntry const* qSort = sQuestSortStore.LookupEntry(-int32(qinfo->QuestSortID));
            if (!qSort)
            {
                LOG_ERROR("sql.sql", "Quest {} has `QuestSortID` = {} (sort case) but quest sort with this id does not exist.",
                    qinfo->GetQuestId(), qinfo->QuestSortID);
                // no changes, quest not dependent from this value but can have problems at client (note some may be 0, we must allow this so no check)
            }
            //check for proper RequiredSkillId value (skill case)
            if (uint32 skill_id = SkillByQuestSort(-int32(qinfo->QuestSortID)))
            {
                if (qinfo->RequiredSkillId != skill_id)
                {
                    LOG_ERROR("sql.sql", "Quest {} has `QuestSortID` = {} but `RequiredSkillId` does not have a corresponding value ({}).",
                        qinfo->GetQuestId(), qinfo->QuestSortID, skill_id);
                    //override, and force proper value here?
                }
            }
        }

        // AllowableClasses, can be 0/CLASSMASK_ALL_PLAYABLE to allow any class
        if (qinfo->AllowableClasses)
        {
            if (!(qinfo->AllowableClasses & CLASSMASK_ALL_PLAYABLE))
            {
                LOG_ERROR("sql.sql", "Quest {} does not contain any playable classes in `AllowableClasses` ({}), value set to 0 (all classes).", qinfo->GetQuestId(), qinfo->AllowableClasses);
                qinfo->AllowableClasses = 0;
            }
        }
        // AllowableRaces, can be -1/RACEMASK_ALL_PLAYABLE to allow any race
        if (qinfo->AllowableRaces != -1)
        {
            if (qinfo->AllowableRaces > 0 && !(uint32(qinfo->AllowableRaces) & RACEMASK_ALL_PLAYABLE))
            {
                LOG_ERROR("sql.sql", "Quest {} does not contain any playable races in `AllowableRaces` ({}), value set to 0 (all races).", qinfo->GetQuestId(), qinfo->AllowableRaces);
                qinfo->AllowableRaces = -1;
            }
        }
        // RequiredSkillId, can be 0
        if (qinfo->RequiredSkillId)
        {
            if (!sSkillLineStore.LookupEntry(qinfo->RequiredSkillId))
            {
                LOG_ERROR("sql.sql", "Quest {} has `RequiredSkillId` = {} but this skill does not exist",
                    qinfo->GetQuestId(), qinfo->RequiredSkillId);
            }
        }

        if (qinfo->RequiredSkillPoints)
        {
            if (qinfo->RequiredSkillPoints > sWorld->GetConfigMaxSkillValue())
            {
                LOG_ERROR("sql.sql", "Quest {} has `RequiredSkillPoints` = {} but max possible skill is {}, quest can't be done.",
                    qinfo->GetQuestId(), qinfo->RequiredSkillPoints, sWorld->GetConfigMaxSkillValue());
                // no changes, quest can't be done for this requirement
            }
        }
        // else Skill quests can have 0 skill level, this is ok

        if (qinfo->RequiredMinRepFaction && !sFactionStore.LookupEntry(qinfo->RequiredMinRepFaction))
        {
            LOG_ERROR("sql.sql", "Quest {} has `RequiredMinRepFaction` = {} but faction template {} does not exist, quest can't be done.",
                qinfo->GetQuestId(), qinfo->RequiredMinRepFaction, qinfo->RequiredMinRepFaction);
            // no changes, quest can't be done for this requirement
        }

        if (qinfo->RequiredMaxRepFaction && !sFactionStore.LookupEntry(qinfo->RequiredMaxRepFaction))
        {
            LOG_ERROR("sql.sql", "Quest {} has `RequiredMaxRepFaction` = {} but faction template {} does not exist, quest can't be done.",
                qinfo->GetQuestId(), qinfo->RequiredMaxRepFaction, qinfo->RequiredMaxRepFaction);
            // no changes, quest can't be done for this requirement
        }

        if (qinfo->RequiredMinRepValue && qinfo->RequiredMinRepValue > ReputationMgr::Reputation_Cap)
        {
            LOG_ERROR("sql.sql", "Quest {} has `RequiredMinRepValue` = {} but max reputation is {}, quest can't be done.",
                qinfo->GetQuestId(), qinfo->RequiredMinRepValue, ReputationMgr::Reputation_Cap);
            // no changes, quest can't be done for this requirement
        }

        if (qinfo->RequiredMinRepValue && qinfo->RequiredMaxRepValue && qinfo->RequiredMaxRepValue <= qinfo->RequiredMinRepValue)
        {
            LOG_ERROR("sql.sql", "Quest {} has `RequiredMaxRepValue` = {} and `RequiredMinRepValue` = {}, quest can't be done.",
                qinfo->GetQuestId(), qinfo->RequiredMaxRepValue, qinfo->RequiredMinRepValue);
            // no changes, quest can't be done for this requirement
        }

        if (!qinfo->RequiredMinRepFaction && qinfo->RequiredMinRepValue != 0)
        {
            LOG_ERROR("sql.sql", "Quest {} has `RequiredMinRepValue` = {} but `RequiredMinRepFaction` is 0, value has no effect",
                qinfo->GetQuestId(), qinfo->RequiredMinRepValue);
            // warning
        }

        if (!qinfo->RequiredMaxRepFaction && qinfo->RequiredMaxRepValue != 0)
        {
            LOG_ERROR("sql.sql", "Quest {} has `RequiredMaxRepValue` = {} but `RequiredMaxRepFaction` is 0, value has no effect",
                qinfo->GetQuestId(), qinfo->RequiredMaxRepValue);
            // warning
        }

        if (qinfo->RewardTitleId && !sCharTitlesStore.LookupEntry(qinfo->RewardTitleId))
        {
            LOG_ERROR("sql.sql", "Quest {} has `RewardTitleId` = {} but CharTitle Id {} does not exist, quest can't be rewarded with title.",
                qinfo->GetQuestId(), qinfo->RewardTitleId, qinfo->RewardTitleId);
            qinfo->RewardTitleId = 0;
            // quest can't reward this title
        }

        if (qinfo->SourceItemId)
        {
            if (!sObjectMgr->GetItemTemplate(qinfo->SourceItemId))
            {
                LOG_ERROR("sql.sql", "Quest {} has `SourceItemId` = {} but item with entry {} does not exist, quest can't be done.",
                    qinfo->GetQuestId(), qinfo->SourceItemId, qinfo->SourceItemId);
                qinfo->SourceItemId = 0;                       // quest can't be done for this requirement
            }
            else if (qinfo->SourceItemIdCount == 0)
            {
                LOG_ERROR("sql.sql", "Quest {} has `StartItem` = {} but `ProvidedItemCount` = 0, set to 1 but need fix in DB.",
                    qinfo->GetQuestId(), qinfo->SourceItemId);
                qinfo->SourceItemIdCount = 1;                    // update to 1 for allow quest work for backward compatibility with DB
            }
        }
        else if (qinfo->SourceItemIdCount>0)
        {
            LOG_ERROR("sql.sql", "Quest {} has `SourceItemId` = 0 but `SourceItemIdCount` = {}, useless value.",
                qinfo->GetQuestId(), qinfo->SourceItemIdCount);
            qinfo->SourceItemIdCount=0;                          // no quest work changes in fact
        }

        if (qinfo->SourceSpellID)
        {
            SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(qinfo->SourceSpellID);
            if (!spellInfo)
            {
                LOG_ERROR("sql.sql", "Quest {} has `SourceSpellid` = {} but spell {} doesn't exist, quest can't be done.",
                    qinfo->GetQuestId(), qinfo->SourceSpellID, qinfo->SourceSpellID);
                qinfo->SourceSpellID = 0;                        // quest can't be done for this requirement
            }
            else if (!SpellMgr::IsSpellValid(spellInfo))
            {
                LOG_ERROR("sql.sql", "Quest {} has `SourceSpellid` = {} but spell {} is broken, quest can't be done.",
                    qinfo->GetQuestId(), qinfo->SourceSpellID, qinfo->SourceSpellID);
                qinfo->SourceSpellID = 0;                        // quest can't be done for this requirement
            }
        }

        for (QuestObjective const& obj : qinfo->GetObjectives())
        {
            // Check storage index for objectives which store data
            if (obj.StorageIndex < 0)
            {
                switch (obj.Type)
                {
                    case QUEST_OBJECTIVE_MONSTER:
                    case QUEST_OBJECTIVE_ITEM:
                    case QUEST_OBJECTIVE_GAMEOBJECT:
                    case QUEST_OBJECTIVE_TALKTO:
                    case QUEST_OBJECTIVE_PLAYERKILLS:
                    case QUEST_OBJECTIVE_AREATRIGGER:
                    case QUEST_OBJECTIVE_WINPETBATTLEAGAINSTNPC:
                    case QUEST_OBJECTIVE_OBTAIN_CURRENCY:
                        LOG_ERROR("sql.sql", "Quest {} objective {} has invalid StorageIndex = {} for objective type {}", qinfo->GetQuestId(), obj.ID, obj.StorageIndex, obj.Type);
                        break;
                    default:
                        break;
                }
            }

            switch (obj.Type)
            {
                case QUEST_OBJECTIVE_ITEM:
                    qinfo->SetSpecialFlag(QUEST_SPECIAL_FLAGS_DELIVER);
                    if (!sObjectMgr->GetItemTemplate(obj.ObjectID))
                        LOG_ERROR("sql.sql", "Quest {} objective {} has non existing item entry {}, quest can't be done.",
                            qinfo->GetQuestId(), obj.ID, obj.ObjectID);
                    break;
                case QUEST_OBJECTIVE_MONSTER:
                    qinfo->SetSpecialFlag(QUEST_SPECIAL_FLAGS_KILL | QUEST_SPECIAL_FLAGS_CAST);
                    if (!sObjectMgr->GetCreatureTemplate(obj.ObjectID))
                        LOG_ERROR("sql.sql", "Quest {} objective {} has non existing creature entry {}, quest can't be done.",
                            qinfo->GetQuestId(), obj.ID, uint32(obj.ObjectID));
                    break;
                case QUEST_OBJECTIVE_GAMEOBJECT:
                    qinfo->SetSpecialFlag(QUEST_SPECIAL_FLAGS_KILL | QUEST_SPECIAL_FLAGS_CAST);
                    if (!sObjectMgr->GetGameObjectTemplate(obj.ObjectID))
                        LOG_ERROR("sql.sql", "Quest {} objective {} has non existing gameobject entry {}, quest can't be done.",
                            qinfo->GetQuestId(), obj.ID, uint32(obj.ObjectID));
                    break;
                case QUEST_OBJECTIVE_TALKTO:
                    // Need checks (is it creature only?)
                    qinfo->SetSpecialFlag(QUEST_SPECIAL_FLAGS_CAST | QUEST_SPECIAL_FLAGS_SPEAKTO);
                    break;
                case QUEST_OBJECTIVE_MIN_REPUTATION:
                case QUEST_OBJECTIVE_MAX_REPUTATION:
                    if (!sFactionStore.LookupEntry(obj.ObjectID))
                        LOG_ERROR("sql.sql", "Quest {} objective {} has non existing faction id {}", qinfo->GetQuestId(), obj.ID, obj.ObjectID);
                    break;
                case QUEST_OBJECTIVE_PLAYERKILLS:
                    qinfo->SetSpecialFlag(QUEST_SPECIAL_FLAGS_PLAYER_KILL);
                    if (obj.Amount <= 0)
                        LOG_ERROR("sql.sql", "Quest {} objective {} has invalid player kills count {}", qinfo->GetQuestId(), obj.ID, obj.Amount);
                    break;
                case QUEST_OBJECTIVE_CURRENCY:
                case QUEST_OBJECTIVE_HAVE_CURRENCY:
                case QUEST_OBJECTIVE_OBTAIN_CURRENCY:
                    if (!sCurrencyTypesStore.LookupEntry(obj.ObjectID))
                        LOG_ERROR("sql.sql", "Quest {} objective {} has non existing currency {}", qinfo->GetQuestId(), obj.ID, obj.ObjectID);
                    if (obj.Amount <= 0)
                        LOG_ERROR("sql.sql", "Quest {} objective {} has invalid currency amount {}", qinfo->GetQuestId(), obj.ID, obj.Amount);
                    break;
                case QUEST_OBJECTIVE_LEARNSPELL:
                    if (!sSpellMgr->GetSpellInfo(obj.ObjectID))
                        LOG_ERROR("sql.sql", "Quest {} objective {} has non existing spell id {}", qinfo->GetQuestId(), obj.ID, obj.ObjectID);
                    break;
                case QUEST_OBJECTIVE_WINPETBATTLEAGAINSTNPC:
                    if (obj.ObjectID && !sObjectMgr->GetCreatureTemplate(obj.ObjectID))
                        LOG_ERROR("sql.sql", "Quest {} objective {} has non existing creature entry {}, quest can't be done.",
                            qinfo->GetQuestId(), obj.ID, uint32(obj.ObjectID));
                    break;
                case QUEST_OBJECTIVE_DEFEATBATTLEPET:
                    if (!sBattlePetSpeciesStore.LookupEntry(obj.ObjectID))
                        LOG_ERROR("sql.sql", "Quest {} objective {} has non existing battlepet species id {}", qinfo->GetQuestId(), obj.ID, obj.ObjectID);
                    break;
                case QUEST_OBJECTIVE_CRITERIA_TREE:
                    if (!sCriteriaTreeStore.LookupEntry(obj.ObjectID))
                        LOG_ERROR("sql.sql", "Quest {} objective {} has non existing criteria tree id {}", qinfo->GetQuestId(), obj.ID, obj.ObjectID);
                    break;
                case QUEST_OBJECTIVE_AREATRIGGER:
                    if (GetAreaTrigger(uint32(obj.ObjectID)))
                        _questAreaTriggerStore[obj.ObjectID].insert(qinfo->ID);
                    else if (obj.ObjectID != -1)
                        LOG_ERROR("sql.sql", "Quest {} objective {} has non existing areatrigger id {}", qinfo->GetQuestId(), obj.ID, obj.ObjectID);
                    break;
                case QUEST_OBJECTIVE_MONEY:
                case QUEST_OBJECTIVE_WINPVPPETBATTLES:
                    break;
                default:
                    LOG_ERROR("sql.sql", "Quest {} objective {} has unhandled type {}", qinfo->GetQuestId(), obj.ID, obj.Type);
                    break;
            }
        }

        for (uint8 j = 0; j < QUEST_ITEM_DROP_COUNT; ++j)
        {
            uint32 id = qinfo->ItemDrop[j];
            if (id)
            {
                if (!sObjectMgr->GetItemTemplate(id))
                {
                    LOG_ERROR("sql.sql", "Quest {} has `ItemDrop{}` = {} but item with entry {} does not exist, quest can't be done.",
                        qinfo->GetQuestId(), j+1, id, id);
                    // no changes, quest can't be done for this requirement
                }
            }
            else
            {
                if (qinfo->ItemDropQuantity[j]>0)
                {
                    LOG_ERROR("sql.sql", "Quest {} has `ItemDrop{}` = 0 but `ItemDropQuantity{}` = {}.",
                        qinfo->GetQuestId(), j+1, j+1, qinfo->ItemDropQuantity[j]);
                    // no changes, quest ignore this data
                }
            }
        }

        for (uint8 j = 0; j < QUEST_REWARD_CHOICES_COUNT; ++j)
        {
            uint32 id = qinfo->RewardChoiceItemId[j];
            if (id)
            {
                if (!sObjectMgr->GetItemTemplate(id))
                {
                    LOG_ERROR("sql.sql", "Quest {} has `RewardChoiceItemId{}` = {} but item with entry {} does not exist, quest will not reward this item.",
                        qinfo->GetQuestId(), j+1, id, id);
                    qinfo->RewardChoiceItemId[j] = 0;          // no changes, quest will not reward this
                }

                if (!qinfo->RewardChoiceItemCount[j])
                {
                    LOG_ERROR("sql.sql", "Quest {} has `RewardChoiceItemId{}` = {} but `RewardChoiceItemCount{}` = 0, quest can't be done.",
                        qinfo->GetQuestId(), j+1, id, j+1);
                    // no changes, quest can't be done
                }
            }
            else if (qinfo->RewardChoiceItemCount[j]>0)
            {
                LOG_ERROR("sql.sql", "Quest {} has `RewardChoiceItemId{}` = 0 but `RewardChoiceItemCount{}` = {}.",
                    qinfo->GetQuestId(), j+1, j+1, qinfo->RewardChoiceItemCount[j]);
                // no changes, quest ignore this data
            }
        }

        for (uint8 j = 0; j < QUEST_REWARD_ITEM_COUNT; ++j)
        {
            uint32 id = qinfo->RewardItemId[j];
            if (id)
            {
                if (!sObjectMgr->GetItemTemplate(id))
                {
                    LOG_ERROR("sql.sql", "Quest {} has `RewardItemId{}` = {} but item with entry {} does not exist, quest will not reward this item.",
                        qinfo->GetQuestId(), j+1, id, id);
                    qinfo->RewardItemId[j] = 0;                // no changes, quest will not reward this item
                }

                if (!qinfo->RewardItemCount[j])
                {
                    LOG_ERROR("sql.sql", "Quest {} has `RewardItemId{}` = {} but `RewardItemCount{}` = 0, quest will not reward this item.",
                        qinfo->GetQuestId(), j+1, id, j+1);
                    // no changes
                }
            }
            else if (qinfo->RewardItemCount[j]>0)
            {
                LOG_ERROR("sql.sql", "Quest {} has `RewardItemId{}` = 0 but `RewardItemCount{}` = {}.",
                    qinfo->GetQuestId(), j+1, j+1, qinfo->RewardItemCount[j]);
                // no changes, quest ignore this data
            }
        }

        for (uint8 j = 0; j < QUEST_REWARD_REPUTATIONS_COUNT; ++j)
        {
            if (qinfo->RewardFactionId[j])
            {
                if (std::abs(qinfo->RewardFactionValue[j]) > 9)
                {
               LOG_ERROR("sql.sql", "Quest {} has RewardFactionValueId{} = {}. That is outside the range of valid values (-9 to 9).", qinfo->GetQuestId(), j+1, qinfo->RewardFactionValue[j]);
                }
                if (!sFactionStore.LookupEntry(qinfo->RewardFactionId[j]))
                {
                    LOG_ERROR("sql.sql", "Quest {} has `RewardFactionId{}` = {} but raw faction (faction.dbc) {} does not exist, quest will not reward reputation for this faction.", qinfo->GetQuestId(), j+1, qinfo->RewardFactionId[j], qinfo->RewardFactionId[j]);
                    qinfo->RewardFactionId[j] = 0;            // quest will not reward this
                }
            }

            else if (qinfo->RewardFactionOverride[j] != 0)
            {
                LOG_ERROR("sql.sql", "Quest {} has `RewardFactionId{}` = 0 but `RewardFactionValueIdOverride{}` = {}.",
                    qinfo->GetQuestId(), j+1, j+1, qinfo->RewardFactionOverride[j]);
                // no changes, quest ignore this data
            }
        }

        if (qinfo->RewardDisplaySpell)
        {
            SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(qinfo->RewardDisplaySpell);

            if (!spellInfo)
            {
                LOG_ERROR("sql.sql", "Quest {} has `RewardSpell` = {} but spell {} does not exist, spell removed as display reward.",
                    qinfo->GetQuestId(), qinfo->RewardDisplaySpell, qinfo->RewardDisplaySpell);
                qinfo->RewardDisplaySpell = 0;                        // no spell reward will display for this quest
            }

            else if (!SpellMgr::IsSpellValid(spellInfo))
            {
                LOG_ERROR("sql.sql", "Quest {} has `RewardSpell` = {} but spell {} is broken, quest will not have a spell reward.",
                    qinfo->GetQuestId(), qinfo->RewardDisplaySpell, qinfo->RewardDisplaySpell);
                qinfo->RewardDisplaySpell = 0;                        // no spell reward will display for this quest
            }
        }

        if (qinfo->RewardSpell > 0)
        {
            SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(qinfo->RewardSpell);

            if (!spellInfo)
            {
                LOG_ERROR("sql.sql", "Quest {} has `RewardSpellCast` = {} but spell {} does not exist, quest will not have a spell reward.",
                    qinfo->GetQuestId(), qinfo->RewardSpell, qinfo->RewardSpell);
                qinfo->RewardSpell = 0;                    // no spell will be cast on player
            }

            else if (!SpellMgr::IsSpellValid(spellInfo))
            {
                LOG_ERROR("sql.sql", "Quest {} has `RewardSpellCast` = {} but spell {} is broken, quest will not have a spell reward.",
                    qinfo->GetQuestId(), qinfo->RewardSpell, qinfo->RewardSpell);
                qinfo->RewardSpell = 0;                    // no spell will be cast on player
            }
        }

        if (qinfo->RewardMailTemplateId)
        {
            if (!sMailTemplateStore.LookupEntry(qinfo->RewardMailTemplateId))
            {
                LOG_ERROR("sql.sql", "Quest {} has `RewardMailTemplateId` = {} but mail template  {} does not exist, quest will not have a mail reward.",
                    qinfo->GetQuestId(), qinfo->RewardMailTemplateId, qinfo->RewardMailTemplateId);
                qinfo->RewardMailTemplateId = 0;               // no mail will send to player
                qinfo->RewardMailDelay = 0;                // no mail will send to player
                qinfo->RewardMailSenderEntry = 0;
            }
            else if (usedMailTemplates.find(qinfo->RewardMailTemplateId) != usedMailTemplates.end())
            {
                std::map<uint32, uint32>::const_iterator used_mt_itr = usedMailTemplates.find(qinfo->RewardMailTemplateId);
                LOG_ERROR("sql.sql", "Quest {} has `RewardMailTemplateId` = {} but mail template  {} already used for quest {}, quest will not have a mail reward.",
                    qinfo->GetQuestId(), qinfo->RewardMailTemplateId, qinfo->RewardMailTemplateId, used_mt_itr->second);
                qinfo->RewardMailTemplateId = 0;               // no mail will send to player
                qinfo->RewardMailDelay = 0;                // no mail will send to player
                qinfo->RewardMailSenderEntry = 0;
            }
            else
                usedMailTemplates[qinfo->RewardMailTemplateId] = qinfo->GetQuestId();
        }

        if (qinfo->NextQuestInChain)
        {
            QuestMap::iterator qNextItr = _questTemplates.find(qinfo->NextQuestInChain);
            if (qNextItr == _questTemplates.end())
            {
                LOG_ERROR("sql.sql", "Quest {} has `NextQuestInChain` = {} but quest {} does not exist, quest chain will not work.",
                    qinfo->GetQuestId(), qinfo->NextQuestInChain, qinfo->NextQuestInChain);
                qinfo->NextQuestInChain = 0;
            }
            else
                qNextItr->second->prevChainQuests.push_back(qinfo->GetQuestId());
        }

        for (uint8 j = 0; j < QUEST_REWARD_CURRENCY_COUNT; ++j)
        {
            if (qinfo->RewardCurrencyId[j])
            {
                if (qinfo->RewardCurrencyCount[j] == 0)
                {
                    LOG_ERROR("sql.sql", "Quest {} has `RewardCurrencyId{}` = {} but `RewardCurrencyCount{}` = 0, quest can't be done.",
                        qinfo->GetQuestId(), j+1, qinfo->RewardCurrencyId[j], j+1);
                    // no changes, quest can't be done for this requirement
                }

                if (!sCurrencyTypesStore.LookupEntry(qinfo->RewardCurrencyId[j]))
                {
                    LOG_ERROR("sql.sql", "Quest {} has `RewardCurrencyId{}` = {} but currency with entry {} does not exist, quest can't be done.",
                        qinfo->GetQuestId(), j+1, qinfo->RewardCurrencyId[j], qinfo->RewardCurrencyId[j]);
                    qinfo->RewardCurrencyCount[j] = 0;             // prevent incorrect work of quest
                }
            }
            else if (qinfo->RewardCurrencyCount[j] > 0)
            {
                LOG_ERROR("sql.sql", "Quest {} has `RewardCurrencyId{}` = 0 but `RewardCurrencyCount{}` = {}, quest can't be done.",
                    qinfo->GetQuestId(), j+1, j+1, qinfo->RewardCurrencyCount[j]);
                qinfo->RewardCurrencyCount[j] = 0;                 // prevent incorrect work of quest
            }
        }

        if (qinfo->SoundAccept)
        {
            if (!sSoundEntriesStore.LookupEntry(qinfo->SoundAccept))
            {
                LOG_ERROR("sql.sql", "Quest {} has `SoundAccept` = {} but sound {} does not exist, set to 0.",
                    qinfo->GetQuestId(), qinfo->SoundAccept, qinfo->SoundAccept);
                qinfo->SoundAccept = 0;                        // no sound will be played
            }
        }

        if (qinfo->SoundTurnIn)
        {
            if (!sSoundEntriesStore.LookupEntry(qinfo->SoundTurnIn))
            {
                LOG_ERROR("sql.sql", "Quest {} has `SoundTurnIn` = {} but sound {} does not exist, set to 0.",
                    qinfo->GetQuestId(), qinfo->SoundTurnIn, qinfo->SoundTurnIn);
                qinfo->SoundTurnIn = 0;                        // no sound will be played
            }
        }

        if (qinfo->RewardSkillId)
        {
            if (!sSkillLineStore.LookupEntry(qinfo->RewardSkillId))
            {
                LOG_ERROR("sql.sql", "Quest {} has `RewardSkillId` = {} but this skill does not exist",
                    qinfo->GetQuestId(), qinfo->RewardSkillId);
            }
            if (!qinfo->RewardSkillPoints)
            {
                LOG_ERROR("sql.sql", "Quest {} has `RewardSkillId` = {} but `RewardSkillPoints` is 0",
                    qinfo->GetQuestId(), qinfo->RewardSkillId);
            }
        }

        if (qinfo->RewardSkillPoints)
        {
            if (qinfo->RewardSkillPoints > sWorld->GetConfigMaxSkillValue())
            {
                LOG_ERROR("sql.sql", "Quest {} has `RewardSkillPoints` = {} but max possible skill is {}, quest can't be done.",
                    qinfo->GetQuestId(), qinfo->RewardSkillPoints, sWorld->GetConfigMaxSkillValue());
                // no changes, quest can't be done for this requirement
            }
            if (!qinfo->RewardSkillId)
            {
                LOG_ERROR("sql.sql", "Quest {} has `RewardSkillPoints` = {} but `RewardSkillId` is 0",
                    qinfo->GetQuestId(), qinfo->RewardSkillPoints);
            }
        }

        // fill additional data stores
        if (qinfo->PrevQuestID)
        {
            if (_questTemplates.find(std::abs(qinfo->GetPrevQuestId())) == _questTemplates.end())
                LOG_ERROR("sql.sql", "Quest {} has PrevQuestId {}, but no such quest", qinfo->GetQuestId(), qinfo->GetPrevQuestId());
            else
                qinfo->prevQuests.push_back(qinfo->PrevQuestID);
        }

        if (qinfo->NextQuestID)
        {
            QuestMap::iterator qNextItr = _questTemplates.find(std::abs(qinfo->GetNextQuestId()));
            if (qNextItr == _questTemplates.end())
                LOG_ERROR("sql.sql", "Quest {} has NextQuestId {}, but no such quest", qinfo->GetQuestId(), qinfo->GetNextQuestId());
            else
            {
                int32 signedQuestId = qinfo->NextQuestID < 0 ? -int32(qinfo->GetQuestId()) : int32(qinfo->GetQuestId());
                qNextItr->second->prevQuests.push_back(signedQuestId);
            }
        }

        if (qinfo->ExclusiveGroup)
            mExclusiveQuestGroups.insert(std::pair<int32, uint32>(qinfo->ExclusiveGroup, qinfo->GetQuestId()));
        if (uint32 breadcrumbForQuestId = qinfo->GetBreadcrumbForQuestId())
        {
            if (_questTemplates.find(breadcrumbForQuestId) == _questTemplates.end())
                LOG_ERROR("sql.sql", "Quest {} has BreadcrumbForQuestId {}, but no such quest exists", qinfo->GetQuestId(), breadcrumbForQuestId);
            else
                _breadcrumbsForQuest[breadcrumbForQuestId].push_back(qinfo->GetQuestId());

            if (qinfo->GetNextQuestId())
                LOG_ERROR("sql.sql", "Quest {} is a breadcrumb quest but also has NextQuestID {} set", qinfo->GetQuestId(), qinfo->GetNextQuestId());
            if (qinfo->GetExclusiveGroup())
                LOG_ERROR("sql.sql", "Quest {} is a breadcrumb quest but also has ExclusiveGroup {} set", qinfo->GetQuestId(), qinfo->GetExclusiveGroup());
        }

        if (qinfo->LimitTime)
            qinfo->SetSpecialFlag(QUEST_SPECIAL_FLAGS_TIMED);
    }

    for (auto const& [questId, quest] : _questTemplates)
    {
        if (sDisableMgr->IsDisabledFor(DISABLE_TYPE_QUEST, questId, nullptr))
            continue;

        uint32 breadcrumbForQuestId = quest->GetBreadcrumbForQuestId();
        if (!breadcrumbForQuestId)
            continue;

        std::set<uint32> visitedQuests;
        visitedQuests.insert(questId);

        while (breadcrumbForQuestId)
        {
            if (!visitedQuests.insert(breadcrumbForQuestId).second)
            {
                LOG_ERROR("sql.sql", "Breadcrumb quests {} and {} form a loop", questId, breadcrumbForQuestId);
                break;
            }

            Quest const* targetQuest = GetQuestTemplate(breadcrumbForQuestId);
            if (!targetQuest)
                break;

            breadcrumbForQuestId = targetQuest->GetBreadcrumbForQuestId();
        }
    }

    // check QUEST_SPECIAL_FLAGS_EXPLORATION_OR_EVENT for spell with SPELL_EFFECT_QUEST_COMPLETE
    for (uint32 i = 0; i < sSpellMgr->GetSpellInfoStoreSize(); ++i)
    {
        SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(i);
        if (!spellInfo)
            continue;

        for (uint8 j = 0; j < MAX_SPELL_EFFECTS; ++j)
        {
            if (spellInfo->Effects[j].Effect != SPELL_EFFECT_QUEST_COMPLETE)
                continue;

            uint32 quest_id = spellInfo->Effects[j].MiscValue;

            Quest const* quest = GetQuestTemplate(quest_id);

            // some quest referenced in spells not exist (outdated spells)
            if (!quest)
                continue;

            if (!quest->HasSpecialFlag(QUEST_SPECIAL_FLAGS_EXPLORATION_OR_EVENT))
            {
                LOG_ERROR("sql.sql", "Spell (id: {}) have SPELL_EFFECT_QUEST_COMPLETE for quest {}, but quest not have specialflag QUEST_SPECIAL_FLAGS_EXPLORATION_OR_EVENT. Quest flags must be fixed, quest modified to enable objective.", spellInfo->Id, quest_id);

                // this will prevent quest completing without objective
                // xinef: remove this, leave error but do not break the quest
                // const_cast<Quest*>(quest)->SetSpecialFlag(QUEST_SPECIAL_FLAGS_EXPLORATION_OR_EVENT);
            }
        }
    }

    LOG_INFO("server.loading", ">> Loaded {} Quests Definitions in {} ms", (unsigned long)_questTemplates.size(), GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

void ObjectMgr::LoadRealmNames()
{
    uint32 oldMSTime = getMSTime();
    _realmNameStore.clear();

    PreparedQueryResult result = LoginDatabase.Query(LoginDatabase.GetPreparedStatement(LOGIN_SEL_REALM_NAMES));
    if (!result)
    {
        LOG_INFO("server.loading", ">> Loaded 0 realm names. DB table `realmlist` is empty.");
        return;
    }

    do
    {
        Field* fields = result->Fetch();
        _realmNameStore[fields[0].Get<uint32>()] = fields[1].Get<std::string>();
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} realm names in {} ms.", uint32(_realmNameStore.size()), GetMSTimeDiffToNow(oldMSTime));
}

std::string ObjectMgr::GetRealmName(uint32 realmId) const
{
    auto iter = _realmNameStore.find(realmId);
    return iter != _realmNameStore.end() ? iter->second : "";
}

std::string ObjectMgr::GetNormalizedRealmName(uint32 realmId) const
{
    std::string name = GetRealmName(realmId);
    name.erase(std::remove_if(name.begin(), name.end(), ::isspace), name.end());
    return name;
}

void ObjectMgr::LoadQuestLocales()
{
    uint32 oldMSTime = getMSTime();

    _questLocaleStore.clear();                                // need for reload case

    //                                               0   1       2         3               4                 5                6                  7                  8                   9                   10
    QueryResult result = WorldDatabase.Query("SELECT ID, locale, LogTitle, LogDescription, QuestDescription, AreaDescription, PortraitGiverText, PortraitGiverName, PortraitTurnInText, PortraitTurnInName, QuestCompletionLog FROM quest_template_locale");

    if (!result)
        return;

    do
    {
        Field* fields = result->Fetch();

        uint32 ID = fields[0].Get<uint32>();

        LocaleConstant locale = GetLocaleByName(fields[1].Get<std::string>());
        if (locale == LOCALE_enUS)
            continue;

        QuestTemplateLocale& data = _questLocaleStore[ID];
        AddLocaleString(fields[2].Get<std::string>(), locale, data.LogTitle);
        AddLocaleString(fields[3].Get<std::string>(), locale, data.LogDescription);
        AddLocaleString(fields[4].Get<std::string>(), locale, data.QuestDescription);
        AddLocaleString(fields[5].Get<std::string>(), locale, data.AreaDescription);
        AddLocaleString(fields[6].Get<std::string>(), locale, data.PortraitGiverText);
        AddLocaleString(fields[7].Get<std::string>(), locale, data.PortraitGiverName);
        AddLocaleString(fields[8].Get<std::string>(), locale, data.PortraitTurnInText);
        AddLocaleString(fields[9].Get<std::string>(), locale, data.PortraitTurnInName);
        AddLocaleString(fields[10].Get<std::string>(), locale, data.QuestCompletionLog);
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} Quest Locale Strings in {} ms", (uint32)_questLocaleStore.size(), GetMSTimeDiffToNow(oldMSTime));
}

void ObjectMgr::LoadQuestObjectivesLocale()
{
    uint32 oldMSTime = getMSTime();

    _questObjectivesLocaleStore.clear();                      // need for reload case

    //                                               0   1       2
    QueryResult result = WorldDatabase.Query("SELECT ID, locale, Description FROM quest_objectives_locale");
    if (!result)
        return;

    do
    {
        Field* fields = result->Fetch();

        uint32 id = fields[0].Get<uint32>();

        LocaleConstant locale = GetLocaleByName(fields[1].Get<std::string>());
        if (locale == LOCALE_enUS)
            continue;

        AddLocaleString(fields[2].Get<std::string>(), locale, _questObjectivesLocaleStore[id].Description);
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} Quest Objectives Locale Strings in {} ms", (uint32)_questObjectivesLocaleStore.size(), GetMSTimeDiffToNow(oldMSTime));
}

void ObjectMgr::LoadScripts(ScriptsType type)
{
    uint32 oldMSTime = getMSTime();

    ScriptMapMap* scripts = GetScriptsMapByType(type);
    if (!scripts)
        return;

    std::string tableName = GetScriptsTableNameByType(type);
    if (tableName.empty())
        return;

    if (sScriptMgr->IsScriptScheduled())                    // function cannot be called when scripts are in use.
        return;

    LOG_INFO("server.loading", "Loading {}...", tableName);

    scripts->clear();                                       // need for reload support

    bool isSpellScriptTable = (type == SCRIPTS_SPELL);
    //                                                 0    1       2         3         4          5    6  7  8  9
    QueryResult result = WorldDatabase.Query("SELECT id, delay, command, datalong, datalong2, dataint, x, y, z, o{} FROM {}", isSpellScriptTable ? ", effIndex" : "", tableName);

    if (!result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 script definitions. DB table `{}` is empty!", tableName);
        LOG_INFO("server.loading", " ");
        return;
    }

    uint32 count = 0;

    do
    {
        Field* fields = result->Fetch();
        ScriptInfo tmp;
        tmp.type      = type;
        tmp.id           = fields[0].Get<uint32>();
        if (isSpellScriptTable)
            tmp.id      |= fields[10].Get<uint8>() << 24;
        tmp.delay        = fields[1].Get<uint32>();
        tmp.command      = ScriptCommands(fields[2].Get<uint32>());
        tmp.Raw.nData[0] = fields[3].Get<uint32>();
        tmp.Raw.nData[1] = fields[4].Get<uint32>();
        tmp.Raw.nData[2] = fields[5].Get<int32>();
        tmp.Raw.fData[0] = fields[6].Get<float>();
        tmp.Raw.fData[1] = fields[7].Get<float>();
        tmp.Raw.fData[2] = fields[8].Get<float>();
        tmp.Raw.fData[3] = fields[9].Get<float>();

        // generic command args check
        switch (tmp.command)
        {
            case SCRIPT_COMMAND_TALK:
                {
                    if (tmp.Talk.ChatType > CHAT_TYPE_WHISPER && tmp.Talk.ChatType != CHAT_MSG_RAID_BOSS_WHISPER)
                    {
                        LOG_ERROR("sql.sql", "Table `{}` has invalid talk type (datalong = {}) in SCRIPT_COMMAND_TALK for script id {}",
                                         tableName, tmp.Talk.ChatType, tmp.id);
                        continue;
                    }
                    if (!GetBroadcastText(uint32(tmp.Talk.TextID)))
                    {
                        LOG_ERROR("sql.sql", "Table `{}` has invalid talk text id (dataint = {}) in SCRIPT_COMMAND_TALK for script id {}",
                                         tableName, tmp.Talk.TextID, tmp.id);
                        continue;
                    }
                    break;
                }

            case SCRIPT_COMMAND_EMOTE:
                {
                    if (!sEmotesStore.LookupEntry(tmp.Emote.EmoteID))
                    {
                        LOG_ERROR("sql.sql", "Table `{}` has invalid emote id (datalong = {}) in SCRIPT_COMMAND_EMOTE for script id {}",
                                         tableName, tmp.Emote.EmoteID, tmp.id);
                        continue;
                    }
                    break;
                }

            case SCRIPT_COMMAND_TELEPORT_TO:
                {
                    if (!sMapStore.LookupEntry(tmp.TeleportTo.MapID))
                    {
                        LOG_ERROR("sql.sql", "Table `{}` has invalid map (Id: {}) in SCRIPT_COMMAND_TELEPORT_TO for script id {}",
                                         tableName, tmp.TeleportTo.MapID, tmp.id);
                        continue;
                    }

                    if (!Acore::IsValidMapCoord(tmp.TeleportTo.DestX, tmp.TeleportTo.DestY, tmp.TeleportTo.DestZ, tmp.TeleportTo.Orientation))
                    {
                        LOG_ERROR("sql.sql", "Table `{}` has invalid coordinates (X: {} Y: {} Z: {} O: {}) in SCRIPT_COMMAND_TELEPORT_TO for script id {}",
                                         tableName, tmp.TeleportTo.DestX, tmp.TeleportTo.DestY, tmp.TeleportTo.DestZ, tmp.TeleportTo.Orientation, tmp.id);
                        continue;
                    }
                    break;
                }

            case SCRIPT_COMMAND_QUEST_EXPLORED:
                {
                    Quest const* quest = GetQuestTemplate(tmp.QuestExplored.QuestID);
                    if (!quest)
                    {
                        LOG_ERROR("sql.sql", "Table `{}` has invalid quest (ID: {}) in SCRIPT_COMMAND_QUEST_EXPLORED in `datalong` for script id {}",
                                         tableName, tmp.QuestExplored.QuestID, tmp.id);
                        continue;
                    }

                    if (!quest->HasSpecialFlag(QUEST_SPECIAL_FLAGS_EXPLORATION_OR_EVENT))
                    {
                        LOG_ERROR("sql.sql", "Table `{}` has quest (ID: {}) in SCRIPT_COMMAND_QUEST_EXPLORED in `datalong` for script id {}, but quest not have specialflag QUEST_SPECIAL_FLAGS_EXPLORATION_OR_EVENT in quest flags. Script command or quest flags wrong. Quest modified to require objective.",
                                         tableName, tmp.QuestExplored.QuestID, tmp.id);

                        // this will prevent quest completing without objective
                        const_cast<Quest*>(quest)->SetSpecialFlag(QUEST_SPECIAL_FLAGS_EXPLORATION_OR_EVENT);

                        // continue; - quest objective requirement set and command can be allowed
                    }

                    if (float(tmp.QuestExplored.Distance) > DEFAULT_VISIBILITY_DISTANCE)
                    {
                        LOG_ERROR("sql.sql", "Table `{}` has too large distance ({}) for exploring objective complete in `datalong2` in SCRIPT_COMMAND_QUEST_EXPLORED in `datalong` for script id {}",
                                         tableName, tmp.QuestExplored.Distance, tmp.id);
                        continue;
                    }

                    if (tmp.QuestExplored.Distance && float(tmp.QuestExplored.Distance) > DEFAULT_VISIBILITY_DISTANCE)
                    {
                        LOG_ERROR("sql.sql", "Table `{}` has too large distance ({}) for exploring objective complete in `datalong2` in SCRIPT_COMMAND_QUEST_EXPLORED in `datalong` for script id {}, max distance is {} or 0 for disable distance check",
                                         tableName, tmp.QuestExplored.Distance, tmp.id, DEFAULT_VISIBILITY_DISTANCE);
                        continue;
                    }

                    if (tmp.QuestExplored.Distance && float(tmp.QuestExplored.Distance) < INTERACTION_DISTANCE)
                    {
                        LOG_ERROR("sql.sql", "Table `{}` has too small distance ({}) for exploring objective complete in `datalong2` in SCRIPT_COMMAND_QUEST_EXPLORED in `datalong` for script id {}, min distance is {} or 0 for disable distance check",
                                         tableName, tmp.QuestExplored.Distance, tmp.id, INTERACTION_DISTANCE);
                        continue;
                    }

                    break;
                }

            case SCRIPT_COMMAND_KILL_CREDIT:
                {
                    if (!GetCreatureTemplate(tmp.KillCredit.CreatureEntry))
                    {
                        LOG_ERROR("sql.sql", "Table `{}` has invalid creature (Entry: {}) in SCRIPT_COMMAND_KILL_CREDIT for script id {}",
                                         tableName, tmp.KillCredit.CreatureEntry, tmp.id);
                        continue;
                    }
                    break;
                }

            case SCRIPT_COMMAND_RESPAWN_GAMEOBJECT:
                {
                    GameObjectData const* data = GetGameObjectData(tmp.RespawnGameobject.GOGuid);
                    if (!data)
                    {
                        LOG_ERROR("sql.sql", "Table `{}` has invalid gameobject (GUID: {}) in SCRIPT_COMMAND_RESPAWN_GAMEOBJECT for script id {}",
                                         tableName, tmp.RespawnGameobject.GOGuid, tmp.id);
                        continue;
                    }

                    GameObjectTemplate const* info = GetGameObjectTemplate(data->id);
                    if (!info)
                    {
                        LOG_ERROR("sql.sql", "Table `{}` has gameobject with invalid entry (GUID: {} Entry: {}) in SCRIPT_COMMAND_RESPAWN_GAMEOBJECT for script id {}",
                                         tableName, tmp.RespawnGameobject.GOGuid, data->id, tmp.id);
                        continue;
                    }

                    if (info->type == GAMEOBJECT_TYPE_FISHINGNODE ||
                            info->type == GAMEOBJECT_TYPE_FISHINGHOLE ||
                            info->type == GAMEOBJECT_TYPE_DOOR        ||
                            info->type == GAMEOBJECT_TYPE_BUTTON      ||
                            info->type == GAMEOBJECT_TYPE_TRAP)
                    {
                        LOG_ERROR("sql.sql", "Table `{}` have gameobject type ({}) unsupported by command SCRIPT_COMMAND_RESPAWN_GAMEOBJECT for script id {}",
                                         tableName, info->entry, tmp.id);
                        continue;
                    }
                    break;
                }

            case SCRIPT_COMMAND_TEMP_SUMMON_CREATURE:
                {
                    if (!Acore::IsValidMapCoord(tmp.TempSummonCreature.PosX, tmp.TempSummonCreature.PosY, tmp.TempSummonCreature.PosZ, tmp.TempSummonCreature.Orientation))
                    {
                        LOG_ERROR("sql.sql", "Table `{}` has invalid coordinates (X: {} Y: {} Z: {} O: {}) in SCRIPT_COMMAND_TEMP_SUMMON_CREATURE for script id {}",
                                         tableName, tmp.TempSummonCreature.PosX, tmp.TempSummonCreature.PosY, tmp.TempSummonCreature.PosZ, tmp.TempSummonCreature.Orientation, tmp.id);
                        continue;
                    }

                    uint32 entry = tmp.TempSummonCreature.CreatureEntry;
                    if (!GetCreatureTemplate(entry))
                    {
                        LOG_ERROR("sql.sql", "Table `{}` has invalid creature (Entry: {}) in SCRIPT_COMMAND_TEMP_SUMMON_CREATURE for script id {}",
                                         tableName, tmp.TempSummonCreature.CreatureEntry, tmp.id);
                        continue;
                    }
                    break;
                }

            case SCRIPT_COMMAND_OPEN_DOOR:
            case SCRIPT_COMMAND_CLOSE_DOOR:
                {
                    GameObjectData const* data = GetGameObjectData(tmp.ToggleDoor.GOGuid);
                    if (!data)
                    {
                        LOG_ERROR("sql.sql", "Table `{}` has invalid gameobject (GUID: {}) in {} for script id {}",
                                         tableName, tmp.ToggleDoor.GOGuid, GetScriptCommandName(tmp.command), tmp.id);
                        continue;
                    }

                    GameObjectTemplate const* info = GetGameObjectTemplate(data->id);
                    if (!info)
                    {
                        LOG_ERROR("sql.sql", "Table `{}` has gameobject with invalid entry (GUID: {} Entry: {}) in {} for script id {}",
                                         tableName, tmp.ToggleDoor.GOGuid, data->id, GetScriptCommandName(tmp.command), tmp.id);
                        continue;
                    }

                    if (info->type != GAMEOBJECT_TYPE_DOOR)
                    {
                        LOG_ERROR("sql.sql", "Table `{}` has gameobject type ({}) non supported by command {} for script id {}",
                                         tableName, info->entry, GetScriptCommandName(tmp.command), tmp.id);
                        continue;
                    }

                    break;
                }

            case SCRIPT_COMMAND_REMOVE_AURA:
                {
                    if (!sSpellMgr->GetSpellInfo(tmp.RemoveAura.SpellID))
                    {
                        LOG_ERROR("sql.sql", "Table `{}` using non-existent spell (id: {}) in SCRIPT_COMMAND_REMOVE_AURA for script id {}",
                                         tableName, tmp.RemoveAura.SpellID, tmp.id);
                        continue;
                    }
                    if (tmp.RemoveAura.Flags & ~0x1)                    // 1 bits (0, 1)
                    {
                        LOG_ERROR("sql.sql", "Table `{}` using unknown flags in datalong2 ({}) in SCRIPT_COMMAND_REMOVE_AURA for script id {}",
                                         tableName, tmp.RemoveAura.Flags, tmp.id);
                        continue;
                    }
                    break;
                }

            case SCRIPT_COMMAND_CAST_SPELL:
                {
                    if (!sSpellMgr->GetSpellInfo(tmp.CastSpell.SpellID))
                    {
                        LOG_ERROR("sql.sql", "Table `{}` using non-existent spell (id: {}) in SCRIPT_COMMAND_CAST_SPELL for script id {}",
                                         tableName, tmp.CastSpell.SpellID, tmp.id);
                        continue;
                    }
                    if (tmp.CastSpell.Flags > 4)                      // targeting type
                    {
                        LOG_ERROR("sql.sql", "Table `{}` using unknown target in datalong2 ({}) in SCRIPT_COMMAND_CAST_SPELL for script id {}",
                                         tableName, tmp.CastSpell.Flags, tmp.id);
                        continue;
                    }
                    if (tmp.CastSpell.Flags != 4 && tmp.CastSpell.CreatureEntry & ~0x1)                      // 1 bit (0, 1)
                    {
                        LOG_ERROR("sql.sql", "Table `{}` using unknown flags in dataint ({}) in SCRIPT_COMMAND_CAST_SPELL for script id {}",
                                         tableName, tmp.CastSpell.CreatureEntry, tmp.id);
                        continue;
                    }
                    else if (tmp.CastSpell.Flags == 4 && !GetCreatureTemplate(tmp.CastSpell.CreatureEntry))
                    {
                        LOG_ERROR("sql.sql", "Table `{}` using invalid creature entry in dataint ({}) in SCRIPT_COMMAND_CAST_SPELL for script id {}",
                                         tableName, tmp.CastSpell.CreatureEntry, tmp.id);
                        continue;
                    }
                    break;
                }

            case SCRIPT_COMMAND_CREATE_ITEM:
                {
                    if (!GetItemTemplate(tmp.CreateItem.ItemEntry))
                    {
                        LOG_ERROR("sql.sql", "Table `{}` has nonexistent item (entry: {}) in SCRIPT_COMMAND_CREATE_ITEM for script id {}",
                                         tableName, tmp.CreateItem.ItemEntry, tmp.id);
                        continue;
                    }
                    if (!tmp.CreateItem.Amount)
                    {
                        LOG_ERROR("sql.sql", "Table `{}` SCRIPT_COMMAND_CREATE_ITEM but amount is {} for script id {}",
                                         tableName, tmp.CreateItem.Amount, tmp.id);
                        continue;
                    }
                    break;
                }
            default:
                break;
        }

        if (scripts->find(tmp.id) == scripts->end())
        {
            ScriptMap emptyMap;
            (*scripts)[tmp.id] = emptyMap;
        }
        (*scripts)[tmp.id].insert(std::pair<uint32, ScriptInfo>(tmp.delay, tmp));

        ++count;
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} script definitions in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

void ObjectMgr::LoadSpellScripts()
{
    LoadScripts(SCRIPTS_SPELL);

    // check ids
    for (ScriptMapMap::const_iterator itr = sSpellScripts.begin(); itr != sSpellScripts.end(); ++itr)
    {
        uint32 spellId = uint32(itr->first) & 0x00FFFFFF;
        SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId);

        if (!spellInfo)
        {
            LOG_ERROR("sql.sql", "Table `spell_scripts` has not existing spell (Id: {}) as script id", spellId);
            continue;
        }

        SpellEffIndex i = SpellEffIndex((uint32(itr->first) >> 24) & 0x000000FF);
        if (uint32(i) >= MAX_SPELL_EFFECTS)
        {
            LOG_ERROR("sql.sql", "Table `spell_scripts` has too high effect index {} for spell (Id: {}) as script id", uint32(i), spellId);
        }

        //check for correct spellEffect
        if (!spellInfo->Effects[i].Effect || (spellInfo->Effects[i].Effect != SPELL_EFFECT_SCRIPT_EFFECT && spellInfo->Effects[i].Effect != SPELL_EFFECT_DUMMY))
            LOG_ERROR("sql.sql", "Table `spell_scripts` - spell {} effect {} is not SPELL_EFFECT_SCRIPT_EFFECT or SPELL_EFFECT_DUMMY", spellId, uint32(i));
    }
}

void ObjectMgr::LoadEventScripts()
{
    LoadScripts(SCRIPTS_EVENT);

    std::set<uint32> evt_scripts;
    // Load all possible script entries from gameobjects
    GameObjectTemplateContainer const* gotc = GetGameObjectTemplates();
    for (GameObjectTemplateContainer::const_iterator itr = gotc->begin(); itr != gotc->end(); ++itr)
        if (uint32 eventId = itr->second.GetEventScriptId())
            evt_scripts.insert(eventId);

    // Load all possible script entries from spells
    for (uint32 i = 1; i < sSpellMgr->GetSpellInfoStoreSize(); ++i)
        if (SpellInfo const* spell = sSpellMgr->GetSpellInfo(i))
            for (uint8 j = 0; j < MAX_SPELL_EFFECTS; ++j)
                if (spell->Effects[j].Effect == SPELL_EFFECT_SEND_EVENT)
                    if (spell->Effects[j].MiscValue)
                        evt_scripts.insert(spell->Effects[j].MiscValue);

    for (std::size_t path_idx = 0; path_idx < sTaxiPathNodesByPath.size(); ++path_idx)
    {
        for (std::size_t node_idx = 0; node_idx < sTaxiPathNodesByPath[path_idx].size(); ++node_idx)
        {
            TaxiPathNodeEntry const* node = sTaxiPathNodesByPath[path_idx][node_idx];

            if (node->ArrivalEventID)
                evt_scripts.insert(node->ArrivalEventID);

            if (node->DepartureEventID)
                evt_scripts.insert(node->DepartureEventID);
        }
    }

    // Then check if all scripts are in above list of possible script entries
    for (ScriptMapMap::const_iterator itr = sEventScripts.begin(); itr != sEventScripts.end(); ++itr)
    {
        std::set<uint32>::const_iterator itr2 = evt_scripts.find(itr->first);
        if (itr2 == evt_scripts.end())
            LOG_ERROR("sql.sql", "Table `event_scripts` has script (Id: {}) not referring to any gameobject_template type 10 data2 field, type 3 data6 field, type 13 data 2 field or any spell effect {}",
                             itr->first, SPELL_EFFECT_SEND_EVENT);
    }
}

//Load WP Scripts
void ObjectMgr::LoadWaypointScripts()
{
    LoadScripts(SCRIPTS_WAYPOINT);

    std::set<uint32> actionSet;

    for (ScriptMapMap::const_iterator itr = sWaypointScripts.begin(); itr != sWaypointScripts.end(); ++itr)
        actionSet.insert(itr->first);

    WorldDatabasePreparedStatement* stmt = WorldDatabase.GetPreparedStatement(WORLD_SEL_WAYPOINT_DATA_ACTION);
    PreparedQueryResult result = WorldDatabase.Query(stmt);

    if (result)
    {
        do
        {
            Field* fields = result->Fetch();
            uint32 action = fields[0].Get<uint32>();

            actionSet.erase(action);
        } while (result->NextRow());
    }

    for (std::set<uint32>::iterator itr = actionSet.begin(); itr != actionSet.end(); ++itr)
        LOG_ERROR("sql.sql", "There is no waypoint which links to the waypoint script {}", *itr);
}

void ObjectMgr::LoadSpellScriptNames()
{
    uint32 oldMSTime = getMSTime();

    _spellScriptsStore.clear();                            // need for reload case

    QueryResult result = WorldDatabase.Query("SELECT spell_id, ScriptName FROM spell_script_names");

    if (!result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 spell script names. DB table `spell_script_names` is empty!");
        LOG_INFO("server.loading", " ");
        return;
    }

    uint32 count = 0;

    do
    {
        Field* fields = result->Fetch();

        int32 spellId          = fields[0].Get<int32>();
        std::string scriptName = fields[1].Get<std::string>();

        bool allRanks = false;
        if (spellId <= 0)
        {
            allRanks = true;
            spellId = -spellId;
        }

        SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId);
        if (!spellInfo)
        {
            LOG_ERROR("sql.sql", "Scriptname: `{}` spell (spell_id:{}) does not exist in `Spell.dbc`.", scriptName, fields[0].Get<int32>());
            continue;
        }

        if (allRanks)
        {
            if (sSpellMgr->GetFirstSpellInChain(spellId) != uint32(spellId))
            {
                LOG_ERROR("sql.sql", "Scriptname: `{}` spell (spell_id:{}) is not first rank of spell.", scriptName, fields[0].Get<int32>());
                continue;
            }
            while (spellInfo)
            {
                _spellScriptsStore.insert(SpellScriptsContainer::value_type(spellInfo->Id, GetScriptId(scriptName)));
                spellInfo = spellInfo->GetNextRankSpell();
            }
        }
        else
            _spellScriptsStore.insert(SpellScriptsContainer::value_type(spellInfo->Id, GetScriptId(scriptName)));
        ++count;
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} spell script names in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

void ObjectMgr::ValidateSpellScripts()
{
    uint32 oldMSTime = getMSTime();

    if (_spellScriptsStore.empty())
    {
        LOG_INFO("server.loading", ">> Validated 0 scripts.");
        LOG_INFO("server.loading", " ");
        return;
    }

    uint32 count = 0;

    for (SpellScriptsContainer::iterator itr = _spellScriptsStore.begin(); itr != _spellScriptsStore.end();)
    {
        SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(itr->first);
        std::vector<std::pair<SpellScriptLoader*, SpellScriptsContainer::iterator> > SpellScriptLoaders;
        sScriptMgr->CreateSpellScriptLoaders(itr->first, SpellScriptLoaders);
        itr = _spellScriptsStore.upper_bound(itr->first);

        for (std::vector<std::pair<SpellScriptLoader*, SpellScriptsContainer::iterator> >::iterator sitr = SpellScriptLoaders.begin(); sitr != SpellScriptLoaders.end(); ++sitr)
        {
            SpellScript* spellScript = sitr->first->GetSpellScript();
            AuraScript* auraScript = sitr->first->GetAuraScript();
            bool valid = true;
            if (!spellScript && !auraScript)
            {
                LOG_ERROR("sql.sql", "Functions GetSpellScript() and GetAuraScript() of script `{}` do not return objects - script skipped", GetScriptName(sitr->second->second));
                valid = false;
            }
            if (spellScript)
            {
                spellScript->_Init(&sitr->first->GetName(), spellInfo->Id);
                spellScript->_Register();
                if (!spellScript->_Validate(spellInfo))
                    valid = false;
                delete spellScript;
            }
            if (auraScript)
            {
                auraScript->_Init(&sitr->first->GetName(), spellInfo->Id);
                auraScript->_Register();
                if (!auraScript->_Validate(spellInfo))
                    valid = false;
                delete auraScript;
            }
            if (!valid)
            {
                _spellScriptsStore.erase(sitr->second);
            }
        }
        ++count;
    }

    LOG_INFO("server.loading", ">> Validated {} scripts in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

void ObjectMgr::InitializeSpellInfoPrecomputedData()
{
    uint32 limit = sSpellStore.GetNumRows();
    for(uint32 i = 0; i <= limit; ++i)
        if (SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(i))
        {
            const_cast<SpellInfo*>(spellInfo)->SetStackableWithRanks(spellInfo->ComputeIsStackableWithRanks());
            const_cast<SpellInfo*>(spellInfo)->SetCritCapable(spellInfo->ComputeIsCritCapable());
            const_cast<SpellInfo*>(spellInfo)->SetSpellValid(SpellMgr::ComputeIsSpellValid(spellInfo, false));
        }
}

void ObjectMgr::LoadPageTexts()
{
    uint32 oldMSTime = getMSTime();

    //                                               0     1       2
    QueryResult result = WorldDatabase.Query("SELECT ID, Text, NextPageID FROM page_text");

    if (!result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 page texts. DB table `page_text` is empty!");
        LOG_INFO("server.loading", " ");
        return;
    }

    uint32 count = 0;
    do
    {
        Field* fields = result->Fetch();

        PageText& pageText = _pageTextStore[fields[0].Get<uint32>()];

        pageText.Text     = fields[1].Get<std::string>();
        pageText.NextPage = fields[2].Get<uint32>();

        ++count;
    } while (result->NextRow());

    for (PageTextContainer::const_iterator itr = _pageTextStore.begin(); itr != _pageTextStore.end(); ++itr)
    {
        if (itr->second.NextPage)
        {
            PageTextContainer::const_iterator itr2 = _pageTextStore.find(itr->second.NextPage);
            if (itr2 == _pageTextStore.end())
                LOG_ERROR("sql.sql", "Page text (Id: {}) has not existing next page (Id: {})", itr->first, itr->second.NextPage);
        }
    }

    LOG_INFO("server.loading", ">> Loaded {} Page Texts in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

PageText const* ObjectMgr::GetPageText(uint32 pageEntry)
{
    PageTextContainer::const_iterator itr = _pageTextStore.find(pageEntry);
    if (itr != _pageTextStore.end())
        return &(itr->second);

    return nullptr;
}

void ObjectMgr::LoadPageTextLocales()
{
    uint32 oldMSTime = getMSTime();

    _pageTextLocaleStore.clear();                             // need for reload case

    //                                               0   1       2
    QueryResult result = WorldDatabase.Query("SELECT ID, locale, Text FROM page_text_locale");

    if (!result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 page texts. DB table `page_text_locale` is empty!");
        LOG_INFO("server.loading", " ");
        return;
    }

    do
    {
        Field* fields = result->Fetch();

        uint32 ID = fields[0].Get<uint32>();

        LocaleConstant locale = GetLocaleByName(fields[1].Get<std::string>());
        if (locale == LOCALE_enUS)
            continue;

        PageTextLocale& data = _pageTextLocaleStore[ID];
        AddLocaleString(fields[2].Get<std::string>(), locale, data.Text);
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} Page Text Locale Strings in {} ms", (uint32)_pageTextLocaleStore.size(), GetMSTimeDiffToNow(oldMSTime));
}

void ObjectMgr::LoadInstanceTemplate()
{
    uint32 oldMSTime = getMSTime();

    //                                                0     1       2        4
    QueryResult result = WorldDatabase.Query("SELECT map, parent, script, allowMount FROM instance_template");

    if (!result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 instance templates. DB table `instance_template` is empty!");
        LOG_INFO("server.loading", " ");
        return;
    }

    uint32 count = 0;
    do
    {
        Field* fields = result->Fetch();

        uint16 mapID = fields[0].Get<uint16>();

        if (!MapMgr::IsValidMAP(mapID, true))
        {
            LOG_ERROR("sql.sql", "ObjectMgr::LoadInstanceTemplate: bad mapid {} for template!", mapID);
            continue;
        }

        InstanceTemplate instanceTemplate;

        instanceTemplate.AllowMount = fields[3].Get<bool>();
        instanceTemplate.Parent     = uint32(fields[1].Get<uint16>());
        instanceTemplate.ScriptId   = GetScriptId(fields[2].Get<std::string>());

        _instanceTemplateStore[mapID] = instanceTemplate;

        ++count;
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} Instance Templates in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

InstanceTemplate const* ObjectMgr::GetInstanceTemplate(uint32 mapID)
{
    InstanceTemplateContainer::const_iterator itr = _instanceTemplateStore.find(uint16(mapID));
    if (itr != _instanceTemplateStore.end())
        return &(itr->second);

    return nullptr;
}

void ObjectMgr::LoadInstanceEncounters()
{
    uint32 oldMSTime = getMSTime();

    //                                                 0         1            2                3
    QueryResult result = WorldDatabase.Query("SELECT entry, creditType, creditEntry, lastEncounterDungeon FROM instance_encounters");
    if (!result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 instance encounters, table is empty!");
        LOG_INFO("server.loading", " ");
        return;
    }

    uint32 count = 0;
    std::map<uint32, DungeonEncounterEntry const*> dungeonLastBosses;
    do
    {
        Field* fields = result->Fetch();
        uint32 entry = fields[0].Get<uint32>();
        uint8 creditType = fields[1].Get<uint8>();
        uint32 creditEntry = fields[2].Get<uint32>();
        uint32 lastEncounterDungeon = fields[3].Get<uint16>();
        DungeonEncounterEntry const* dungeonEncounter = sDungeonEncounterStore.LookupEntry(entry);
        if (!dungeonEncounter)
        {
            LOG_ERROR("sql.sql", "Table `instance_encounters` has an invalid encounter id {}, skipped!", entry);
            continue;
        }

        if (lastEncounterDungeon && !sLFGMgr->GetLFGDungeonEntry(lastEncounterDungeon))
        {
            LOG_ERROR("sql.sql", "Table `instance_encounters` has an encounter {} ({}) marked as final for invalid dungeon id {}, skipped!", entry, dungeonEncounter->Name_lang, lastEncounterDungeon);
            continue;
        }

        std::map<uint32, DungeonEncounterEntry const*>::const_iterator itr = dungeonLastBosses.find(lastEncounterDungeon);
        if (lastEncounterDungeon)
        {
            if (itr != dungeonLastBosses.end())
            {
                LOG_ERROR("sql.sql", "Table `instance_encounters` specified encounter {} ({}) as last encounter but {} ({}) is already marked as one, skipped!", entry, dungeonEncounter->Name_lang, itr->second->ID, itr->second->Name_lang);
                continue;
            }

            dungeonLastBosses[lastEncounterDungeon] = dungeonEncounter;
        }

        switch (creditType)
        {
            case ENCOUNTER_CREDIT_KILL_CREATURE:
                {
                    CreatureTemplate const* creatureInfo = GetCreatureTemplate(creditEntry);
                    if (!creatureInfo)
                    {
                        LOG_ERROR("sql.sql", "Table `instance_encounters` has an invalid creature (entry {}) linked to the encounter {} ({}), skipped!", creditEntry, entry, dungeonEncounter->Name_lang);
                        continue;
                    }
                    const_cast<CreatureTemplate*>(creatureInfo)->flags_extra |= CREATURE_FLAG_EXTRA_DUNGEON_BOSS;
                    break;
                }
            case ENCOUNTER_CREDIT_CAST_SPELL:
                {
                    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(creditEntry);
                    if (!spellInfo)
                    {
                        LOG_ERROR("sql.sql", "Table `instance_encounters` has an invalid spell (entry {}) linked to the encounter {} ({}), skipped!", creditEntry, entry, dungeonEncounter->Name_lang);
                        continue;
                    }
                    const_cast<SpellInfo*>(spellInfo)->AttributesCu |= SPELL_ATTR0_CU_ENCOUNTER_REWARD;
                    break;
                }
            default:
                LOG_ERROR("sql.sql", "Table `instance_encounters` has an invalid credit type ({}) for encounter {} ({}), skipped!", creditType, entry, dungeonEncounter->Name_lang);
                continue;
        }

        // 5.4.8: Difficulty.dbc id, -1 for every difficulty of the map
        if (int32(dungeonEncounter->DifficultyID) < 0)
        {
            for (uint8 difficulty = 0; difficulty < MAX_DIFFICULTY; ++difficulty)
                _dungeonEncounterStore[MAKE_PAIR32(dungeonEncounter->MapID, difficulty)].push_back(new DungeonEncounter(dungeonEncounter, EncounterCreditType(creditType), creditEntry, lastEncounterDungeon));
        }
        else
            _dungeonEncounterStore[MAKE_PAIR32(dungeonEncounter->MapID, dungeonEncounter->DifficultyID)].push_back(new DungeonEncounter(dungeonEncounter, EncounterCreditType(creditType), creditEntry, lastEncounterDungeon));
        ++count;
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} Instance Encounters in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

GossipText const* ObjectMgr::GetGossipText(uint32 Text_ID) const
{
    GossipTextContainer::const_iterator itr = _gossipTextStore.find(Text_ID);
    if (itr != _gossipTextStore.end())
        return &itr->second;
    return nullptr;
}

void ObjectMgr::LoadGossipText()
{
    uint32 oldMSTime = getMSTime();

    QueryResult result = WorldDatabase.Query("SELECT ID, "
                         "text0_0, text0_1, BroadcastTextID0, lang0, Probability0, em0_0, em0_1, em0_2, em0_3, em0_4, em0_5, "
                         "text1_0, text1_1, BroadcastTextID1, lang1, Probability1, em1_0, em1_1, em1_2, em1_3, em1_4, em1_5, "
                         "text2_0, text2_1, BroadcastTextID2, lang2, Probability2, em2_0, em2_1, em2_2, em2_3, em2_4, em2_5, "
                         "text3_0, text3_1, BroadcastTextID3, lang3, Probability3, em3_0, em3_1, em3_2, em3_3, em3_4, em3_5, "
                         "text4_0, text4_1, BroadcastTextID4, lang4, Probability4, em4_0, em4_1, em4_2, em4_3, em4_4, em4_5, "
                         "text5_0, text5_1, BroadcastTextID5, lang5, Probability5, em5_0, em5_1, em5_2, em5_3, em5_4, em5_5, "
                         "text6_0, text6_1, BroadcastTextID6, lang6, Probability6, em6_0, em6_1, em6_2, em6_3, em6_4, em6_5, "
                         "text7_0, text7_1, BroadcastTextID7, lang7, Probability7, em7_0, em7_1, em7_2, em7_3, em7_4, em7_5 "
                         "FROM npc_text");

    if (!result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 npc texts, table is empty!");
        LOG_INFO("server.loading", " ");
        return;
    }

    _gossipTextStore.rehash(result->GetRowCount());

    uint32 count = 0;
    uint8 cic;

    do
    {
        cic = 0;

        Field* fields = result->Fetch();

        uint32 id = fields[cic++].Get<uint32>();
        if (!id)
        {
            LOG_ERROR("sql.sql", "Table `npc_text` has record wit reserved id 0, ignore.");
            continue;
        }

        GossipText& gText = _gossipTextStore[id];

        for (uint8 i = 0; i < MAX_GOSSIP_TEXT_OPTIONS; ++i)
        {
            gText.Options[i].Text_0           = fields[cic++].Get<std::string>();
            gText.Options[i].Text_1           = fields[cic++].Get<std::string>();
            gText.Options[i].BroadcastTextID  = fields[cic++].Get<uint32>();
            gText.Options[i].Language         = fields[cic++].Get<uint8>();
            gText.Options[i].Probability      = fields[cic++].Get<float>();

            for (uint8 j = 0; j < MAX_GOSSIP_TEXT_EMOTES; ++j)
            {
                gText.Options[i].Emotes[j]._Delay = fields[cic++].Get<uint16>();
                gText.Options[i].Emotes[j]._Emote = fields[cic++].Get<uint16>();
            }
        }

        for (uint8 i = 0; i < MAX_GOSSIP_TEXT_OPTIONS; i++)
        {
            if (gText.Options[i].BroadcastTextID)
            {
                if (!GetBroadcastText(gText.Options[i].BroadcastTextID))
                {
                    LOG_ERROR("sql.sql", "GossipText (Id: {}) in table `npc_text` has non-existing or incompatible BroadcastTextID{} {}.", id, i, gText.Options[i].BroadcastTextID);
                    gText.Options[i].BroadcastTextID = 0;
                }
            }
        }

        count++;
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} Npc Texts in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

void ObjectMgr::LoadNpcTextLocales()
{
    uint32 oldMSTime = getMSTime();

    _npcTextLocaleStore.clear();                              // need for reload case

    QueryResult result = WorldDatabase.Query("SELECT ID, Locale, "
                         //   2        3        4        5        6        7        8        9        10       11       12       13       14       15       16       17
                         "Text0_0, Text0_1, Text1_0, Text1_1, Text2_0, Text2_1, Text3_0, Text3_1, Text4_0, Text4_1, Text5_0, Text5_1, Text6_0, Text6_1, Text7_0, Text7_1 "
                         "FROM npc_text_locale");

    if (!result)
        return;

    do
    {
        Field* fields = result->Fetch();

        uint32 ID = fields[0].Get<uint32>();

        LocaleConstant locale = GetLocaleByName(fields[1].Get<std::string>());
        if (locale == LOCALE_enUS)
            continue;

        NpcTextLocale& data = _npcTextLocaleStore[ID];
        for (uint8 i = 0; i < MAX_GOSSIP_TEXT_OPTIONS; ++i)
        {
            AddLocaleString(fields[2 + i * 2].Get<std::string>(), locale, data.Text_0[i]);
            AddLocaleString(fields[3 + i * 2].Get<std::string>(), locale, data.Text_1[i]);
        }
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} Npc Text Locale Strings in {} ms", (uint32)_npcTextLocaleStore.size(), GetMSTimeDiffToNow(oldMSTime));
}

void ObjectMgr::LoadQuestAreaTriggers()
{
    uint32 oldMSTime = getMSTime();

    _questAreaTriggerStore.clear();                           // need for reload case

    QueryResult result = WorldDatabase.Query("SELECT id, quest FROM areatrigger_involvedrelation");

    if (!result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 quest trigger points. DB table `areatrigger_involvedrelation` is empty.");
        LOG_INFO("server.loading", " ");
        return;
    }

    uint32 count = 0;

    do
    {
        ++count;

        Field* fields = result->Fetch();

        uint32 trigger_ID = fields[0].Get<uint32>();
        uint32 quest_ID   = fields[1].Get<uint32>();

        AreaTrigger const* atEntry = GetAreaTrigger(trigger_ID);
        if (!atEntry)
        {
            LOG_ERROR("sql.sql", "Area trigger (ID:{}) does not exist in `AreaTrigger.dbc`.", trigger_ID);
            continue;
        }

        Quest const* quest = GetQuestTemplate(quest_ID);

        if (!quest)
        {
            LOG_ERROR("sql.sql", "Table `areatrigger_involvedrelation` has record (id: {}) for not existing quest {}", trigger_ID, quest_ID);
            continue;
        }

        if (!quest->HasSpecialFlag(QUEST_SPECIAL_FLAGS_EXPLORATION_OR_EVENT))
        {
            LOG_ERROR("sql.sql", "Table `areatrigger_involvedrelation` has record (id: {}) for not quest {}, but quest not have specialflag QUEST_SPECIAL_FLAGS_EXPLORATION_OR_EVENT. Trigger or quest flags must be fixed, quest modified to require objective.", trigger_ID, quest_ID);

            // this will prevent quest completing without objective
            const_cast<Quest*>(quest)->SetSpecialFlag(QUEST_SPECIAL_FLAGS_EXPLORATION_OR_EVENT);

            // continue; - quest modified to required objective and trigger can be allowed.
        }

        _questAreaTriggerStore[trigger_ID].insert(quest_ID);
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} Quest Trigger Points in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

QuestGreeting const* ObjectMgr::GetQuestGreeting(TypeID type, uint32 id) const
{
    uint8 typeIndex;
    if (type == TYPEID_UNIT)
        typeIndex = 0;
    else if (type == TYPEID_GAMEOBJECT)
        typeIndex = 1;
    else
        return nullptr;

    std::pair<uint32, uint8> pairKey = std::make_pair(id, typeIndex);
    QuestGreetingContainer::const_iterator itr = _questGreetingStore.find(pairKey);
    if (itr == _questGreetingStore.end())
        return nullptr;

    return &itr->second;
}

void ObjectMgr::LoadQuestGreetings()
{
    uint32 oldMSTime = getMSTime();

    _questGreetingStore.clear(); // For reload case

    //                                                0   1          2                3             4
    QueryResult result = WorldDatabase.Query("SELECT ID, Type, GreetEmoteType, GreetEmoteDelay, Greeting FROM quest_greeting");
    if (!result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 quest greetings. DB table `quest_greeting` is empty.");
        return;
    }

    do
    {
        Field* fields = result->Fetch();

        uint32 id = fields[0].Get<uint32>();
        uint8 type = fields[1].Get<uint8>();
        switch (type)
        {
            case 0: // Creature
                if (!GetCreatureTemplate(id))
                {
                    LOG_ERROR("sql.sql", "Table `quest_greeting`: creature template entry {} does not exist.", id);
                    continue;
                }
                break;
            case 1: // GameObject
                if (!GetGameObjectTemplate(id))
                {
                    LOG_ERROR("sql.sql", "Table `quest_greeting`: gameobject template entry {} does not exist.", id);
                    continue;
                }
                break;
            default:
                LOG_ERROR("sql.sql", "Table `quest_greeting` has unknown type {} for id {}, skipped.", type, id);
                continue;
        }

        std::pair<uint32, uint8> pairKey = std::make_pair(id, type);
        QuestGreeting& data = _questGreetingStore[pairKey];

        data.EmoteType = fields[2].Get<uint16>();
        data.EmoteDelay = fields[3].Get<uint32>();
        AddLocaleString(fields[4].Get<std::string>(), LOCALE_enUS, data.Greeting);
    }
    while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} quest_greeting in {} ms", _questGreetingStore.size(), GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

void ObjectMgr::LoadQuestGreetingsLocales()
{
    uint32 oldMSTime = getMSTime();

    //                                               0     1      2       3
    QueryResult result = WorldDatabase.Query("SELECT ID, Type, Locale, Greeting FROM quest_greeting_locale");
    if (!result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 quest_greeting locales. DB table `quest_greeting_locale` is empty.");
        return;
    }

    uint32 localeCount = 0;
    do
    {
        Field* fields = result->Fetch();

        uint32 id = fields[0].Get<uint32>();
        uint8 type = fields[1].Get<uint8>();
        switch (type)
        {
        case 0: // Creature
            if (!GetCreatureTemplate(id))
            {
                LOG_ERROR("sql.sql", "Table `quest_greeting_locale`: creature template entry {} does not exist.", id);
                continue;
            }
            break;
        case 1: // GameObject
            if (!GetGameObjectTemplate(id))
            {
                LOG_ERROR("sql.sql", "Table `quest_greeting_locale`: gameobject template entry {} does not exist.", id);
                continue;
            }
            break;
        default:
            continue;
        }

        std::pair<uint32, uint8> pairKey = std::make_pair(id, type);
        QuestGreeting& data = _questGreetingStore[pairKey];

        QuestGreetingContainer::iterator qgc = _questGreetingStore.find(pairKey);
        if (qgc == _questGreetingStore.end())
        {
            LOG_ERROR("sql.sql", "QuestGreeting (Id: {} Type: {}) found in table `quest_greeting_locale` but does not exist in `quest_greeting`. Skipped!", id, type);
            continue;
        }

        LocaleConstant locale = GetLocaleByName(fields[2].Get<std::string>());
        if (locale == LOCALE_enUS)
            continue;

        AddLocaleString(fields[3].Get<std::string>(), locale, data.Greeting);
        localeCount++;
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} quest greeting Locale Strings in {} ms", localeCount, GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

void ObjectMgr::LoadQuestOfferRewardLocale()
{
    uint32 oldMSTime = getMSTime();

    _questOfferRewardLocaleStore.clear(); // need for reload case

    //                                               0     1          2
    QueryResult result = WorldDatabase.Query("SELECT Id, locale, RewardText FROM quest_offer_reward_locale");
    if (!result)
        return;

    do
    {
        Field* fields = result->Fetch();

        uint32 id = fields[0].Get<uint32>();
        std::string localeName = fields[1].Get<std::string>();

        LocaleConstant locale = GetLocaleByName(localeName);
        if (locale == LOCALE_enUS)
            continue;

        QuestOfferRewardLocale& data = _questOfferRewardLocaleStore[id];
        AddLocaleString(fields[2].Get<std::string>(), locale, data.RewardText);
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} Quest Offer Reward Locale Strings in {} ms", _questOfferRewardLocaleStore.size(), GetMSTimeDiffToNow(oldMSTime));
}

void ObjectMgr::LoadQuestRequestItemsLocale()
{
    uint32 oldMSTime = getMSTime();

    _questRequestItemsLocaleStore.clear(); // need for reload case

    //                                               0     1          2
    QueryResult result = WorldDatabase.Query("SELECT Id, locale, CompletionText FROM quest_request_items_locale");
    if (!result)
        return;

    do
    {
        Field* fields = result->Fetch();

        uint32 id = fields[0].Get<uint32>();
        std::string localeName = fields[1].Get<std::string>();

        LocaleConstant locale = GetLocaleByName(localeName);
        if (locale == LOCALE_enUS)
            continue;

        QuestRequestItemsLocale& data = _questRequestItemsLocaleStore[id];
        AddLocaleString(fields[2].Get<std::string>(), locale, data.CompletionText);
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} Quest Request Items Locale Strings in {} ms", _questRequestItemsLocaleStore.size(), GetMSTimeDiffToNow(oldMSTime));
}

void ObjectMgr::LoadTavernAreaTriggers()
{
    uint32 oldMSTime = getMSTime();

    _tavernAreaTriggerStore.clear();                          // need for reload case

    QueryResult result = WorldDatabase.Query("SELECT id, faction FROM areatrigger_tavern");

    if (!result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 tavern triggers. DB table `areatrigger_tavern` is empty.");
        LOG_INFO("server.loading", " ");
        return;
    }

    uint32 count = 0;

    do
    {
        ++count;

        Field* fields = result->Fetch();

        uint32 Trigger_ID      = fields[0].Get<uint32>();

        AreaTrigger const* atEntry = GetAreaTrigger(Trigger_ID);
        if (!atEntry)
        {
            LOG_ERROR("sql.sql", "Area trigger (ID:{}) does not exist in `AreaTrigger.dbc`.", Trigger_ID);
            continue;
        }

        uint32 faction         = fields[1].Get<uint32>();

        _tavernAreaTriggerStore.emplace(Trigger_ID, faction);
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} Tavern Triggers in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

void ObjectMgr::LoadAreaTriggerScripts()
{
    uint32 oldMSTime = getMSTime();

    _areaTriggerScriptStore.clear();                            // need for reload case
    QueryResult result = WorldDatabase.Query("SELECT entry, ScriptName FROM areatrigger_scripts");

    if (!result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 Areatrigger Scripts. DB Table `areatrigger_scripts` Is Empty.");
        LOG_INFO("server.loading", " ");
        return;
    }

    uint32 count = 0;

    do
    {
        ++count;

        Field* fields = result->Fetch();

        uint32 Trigger_ID      = fields[0].Get<uint32>();
        std::string scriptName = fields[1].Get<std::string>();

        AreaTrigger const* atEntry = GetAreaTrigger(Trigger_ID);
        if (!atEntry)
        {
            LOG_ERROR("sql.sql", "Area trigger (ID:{}) does not exist in `AreaTrigger.dbc`.", Trigger_ID);
            continue;
        }
        _areaTriggerScriptStore[Trigger_ID] = GetScriptId(scriptName);
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} Areatrigger Scripts in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

uint32 ObjectMgr::GetNearestTaxiNode(WorldLocation const& loc, uint32 teamId)
{
    return GetNearestTaxiNode(loc.GetPositionX(), loc.GetPositionY(), loc.GetPositionZ(), loc.GetMapId(), teamId);
}

uint32 ObjectMgr::GetNearestTaxiNode(float x, float y, float z, uint32 mapid, uint32 teamId)
{
    bool found = false;
    float dist = 10000;
    uint32 id = 0;

    for (uint32 i = 1; i < sTaxiNodesStore.GetNumRows(); ++i)
    {
        TaxiNodesEntry const* node = sTaxiNodesStore.LookupEntry(i);

        if (!node || node->MapID != mapid || (!node->MountCreatureID[teamId == TEAM_ALLIANCE ? 1 : 0] && node->MountCreatureID[0] != 32981)) // dk flight
            continue;

        uint8  field   = (uint8)((i - 1) / 32);
        uint32 submask = 1 << ((i - 1) % 32);

        // skip not taxi network nodes
        if (field >= TaxiMaskSize || (sTaxiNodesMask[field] & submask) == 0)
        {
            continue;
        }

        float dist2 = (node->Pos.X - x) * (node->Pos.X - x) + (node->Pos.Y - y) * (node->Pos.Y - y) + (node->Pos.Z - z) * (node->Pos.Z - z);
        if (found)
        {
            if (dist2 < dist)
            {
                dist = dist2;
                id = i;
            }
        }
        else
        {
            found = true;
            dist = dist2;
            id = i;
        }
    }

    return id;
}

void ObjectMgr::GetTaxiPath(uint32 source, uint32 destination, uint32& path, uint32& cost)
{
    TaxiPathSetBySource::iterator src_i = sTaxiPathSetBySource.find(source);
    if (src_i == sTaxiPathSetBySource.end())
    {
        path = 0;
        cost = 0;
        return;
    }

    TaxiPathSetForSource& pathSet = src_i->second;

    TaxiPathSetForSource::iterator dest_i = pathSet.find(destination);
    if (dest_i == pathSet.end())
    {
        path = 0;
        cost = 0;
        return;
    }

    cost = dest_i->second.price;
    path = dest_i->second.ID;
}

uint32 ObjectMgr::GetTaxiMountDisplayId(uint32 id, TeamId teamId, bool allowed_alt_team /* = false */)
{
    CreatureModel mountModel;
    CreatureTemplate const* mount_info = nullptr;

    // select mount creature id
    TaxiNodesEntry const* node = sTaxiNodesStore.LookupEntry(id);
    if (node)
    {
        uint32 mount_entry = node->MountCreatureID[teamId == TEAM_ALLIANCE ? 1 : 0];

        // Fix for Alliance not being able to use Acherus taxi
        // only one mount type for both sides
        if (mount_entry == 0 && allowed_alt_team)
        {
            // Simply reverse the selection. At least one team in theory should have a valid mount ID to choose.
            mount_entry = node->MountCreatureID[teamId];
        }

        mount_info = GetCreatureTemplate(mount_entry);
        if (mount_info)
        {
            CreatureModel const* model = mount_info->GetRandomValidModel();
            if (!model)
            {
                LOG_ERROR("sql.sql", "No displayid found for the taxi mount with the entry {}! Can't load it!", mount_entry);
                return 0;
            }
            mountModel = *model;
        }
    }

    // minfo is not actually used but the mount_id was updated
    GetCreatureModelRandomGender(&mountModel, mount_info);

    return mountModel.CreatureDisplayID;
}

void ObjectMgr::LoadAreaTriggers()
{
    uint32 oldMSTime = getMSTime();

    _areaTriggerStore.clear();

    QueryResult result = WorldDatabase.Query("SELECT entry, map, x, y, z, radius, length, width, height, orientation FROM areatrigger");

    if (!result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 area trigger definitions. DB table `areatrigger` is empty.");
        LOG_INFO("server.loading", " ");
        return;
    }

    uint32 count = 0;

    do
    {
        Field* fields = result->Fetch();

        ++count;

        AreaTrigger at;

        at.entry = fields[0].Get<uint32>();
        at.map = fields[1].Get<uint32>();
        at.x = fields[2].Get<float>();
        at.y = fields[3].Get<float>();
        at.z = fields[4].Get<float>();
        at.radius = fields[5].Get<float>();
        at.length = fields[6].Get<float>();
        at.width = fields[7].Get<float>();
        at.height = fields[8].Get<float>();
        at.orientation = fields[9].Get<float>();

        MapEntry const* mapEntry = sMapStore.LookupEntry(at.map);
        if (!mapEntry)
        {
            LOG_ERROR("sql.sql", "Area trigger (ID:{}) map (ID: {}) does not exist in `Map.dbc`.", at.entry, at.map);
            continue;
        }

        _areaTriggerStore[at.entry] = at;
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} Area Trigger Definitions in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

void ObjectMgr::LoadTerrainWorldMaps()
{
    uint32 oldMSTime = getMSTime();
    PreparedQueryResult result = WorldDatabase.Query(WorldDatabase.GetPreparedStatement(WORLD_SEL_TERRAIN_WORLD_MAP));
    if (!result)
    {
        LOG_INFO("server.loading", ">> Loaded 0 terrain world maps. DB table `terrain_worldmap` is empty.");
        return;
    }

    uint32 count = 0;
    do
    {
        Field* fields = result->Fetch();
        uint32 swapMap = fields[0].Get<uint32>();
        uint32 worldMapArea = fields[1].Get<uint32>();
        if (!sMapStore.LookupEntry(swapMap))
        {
            LOG_ERROR("sql.sql", "TerrainSwapMap {} defined in `terrain_worldmap` does not exist, skipped.", swapMap);
            continue;
        }

        TerrainSwapInfo& info = _terrainSwapInfos[swapMap];
        info.Id = swapMap;
        info.UiMapPhaseIDs.push_back(worldMapArea);
        ++count;
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} terrain world maps in {} ms.", count, GetMSTimeDiffToNow(oldMSTime));
}

void ObjectMgr::LoadTerrainSwapDefaults()
{
    uint32 oldMSTime = getMSTime();
    PreparedQueryResult result = WorldDatabase.Query(WorldDatabase.GetPreparedStatement(WORLD_SEL_TERRAIN_SWAP_DEFAULTS));
    if (!result)
    {
        LOG_INFO("server.loading", ">> Loaded 0 terrain swap defaults. DB table `terrain_swap_defaults` is empty.");
        return;
    }

    uint32 count = 0;
    do
    {
        Field* fields = result->Fetch();
        uint32 mapId = fields[0].Get<uint32>();
        uint32 swapMap = fields[1].Get<uint32>();
        if (!sMapStore.LookupEntry(mapId) || !sMapStore.LookupEntry(swapMap))
        {
            LOG_ERROR("sql.sql", "Terrain swap {}/{} defined in `terrain_swap_defaults` references a missing map, skipped.", mapId, swapMap);
            continue;
        }

        TerrainSwapInfo& info = _terrainSwapInfos[swapMap];
        info.Id = swapMap;
        _terrainSwapsByMap[mapId].push_back(&info);
        ++count;
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} terrain swap defaults in {} ms.", count, GetMSTimeDiffToNow(oldMSTime));
}

ObjectMgr::TerrainSwapInfo const* ObjectMgr::GetTerrainSwapInfo(uint32 id) const
{
    auto itr = _terrainSwapInfos.find(id);
    return itr != _terrainSwapInfos.end() ? &itr->second : nullptr;
}

void ObjectMgr::LoadPhaseAreas()
{
    uint32 oldMSTime = getMSTime();

    _phasesByArea.clear();

    WorldDatabasePreparedStatement* stmt = WorldDatabase.GetPreparedStatement(WORLD_SEL_PHASE_AREA);
    PreparedQueryResult result = WorldDatabase.Query(stmt);

    if (!result)
    {
        LOG_INFO("server.loading", ">> Loaded 0 phase area definitions. DB table `phase_area` is empty.");
        LOG_INFO("server.loading", " ");
        return;
    }

    uint32 count = 0;

    do
    {
        Field* fields = result->Fetch();

        uint32 areaId = fields[0].Get<uint32>();
        uint32 phaseId = fields[1].Get<uint32>();
        uint32 phaseGroup = fields[2].Get<uint32>();

        if (!sAreaTableStore.LookupEntry(areaId))
        {
            LOG_ERROR("sql.sql", "Table `phase_area` has AreaId {} not found in `AreaTable.dbc`, skipped.", areaId);
            continue;
        }

        if ((!phaseId && !phaseGroup) || (phaseId && !IsValidPhaseId(phaseId)) || (phaseGroup && !IsValidPhaseGroupId(phaseGroup)))
        {
            LOG_ERROR("sql.sql", "Table `phase_area` has invalid PhaseId {} / PhaseGroup {} for area {}, skipped.", phaseId, phaseGroup, areaId);
            continue;
        }

        _phasesByArea[areaId].push_back({ phaseId, phaseGroup });
        ++count;
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} phase area definitions in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

void ObjectMgr::LoadAreaTriggerTeleports()
{
    uint32 oldMSTime = getMSTime();

    _areaTriggerTeleportStore.clear();                                  // need for reload case

    //                                               0        1              2                  3                  4                   5
    QueryResult result = WorldDatabase.Query("SELECT ID,  target_map, target_position_x, target_position_y, target_position_z, target_orientation FROM areatrigger_teleport");

    if (!result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 area trigger teleport definitions. DB table `areatrigger_teleport` is empty.");
        LOG_INFO("server.loading", " ");
        return;
    }

    uint32 count = 0;

    do
    {
        Field* fields = result->Fetch();

        ++count;

        uint32 Trigger_ID = fields[0].Get<uint32>();

        AreaTriggerTeleport at;

        at.target_mapId             = fields[1].Get<uint16>();
        at.target_X                 = fields[2].Get<float>();
        at.target_Y                 = fields[3].Get<float>();
        at.target_Z                 = fields[4].Get<float>();
        at.target_Orientation       = fields[5].Get<float>();

        AreaTrigger const* atEntry = GetAreaTrigger(Trigger_ID);
        if (!atEntry)
        {
            LOG_ERROR("sql.sql", "Area trigger (ID:{}) does not exist in `AreaTrigger.dbc`.", Trigger_ID);
            continue;
        }

        MapEntry const* mapEntry = sMapStore.LookupEntry(at.target_mapId);
        if (!mapEntry)
        {
            LOG_ERROR("sql.sql", "Area trigger (ID:{}) target map (ID: {}) does not exist in `Map.dbc`.", Trigger_ID, at.target_mapId);
            continue;
        }

        if (at.target_X == 0 && at.target_Y == 0 && at.target_Z == 0)
        {
            LOG_ERROR("sql.sql", "Area trigger (ID:{}) target coordinates not provided.", Trigger_ID);
            continue;
        }

        _areaTriggerTeleportStore[Trigger_ID] = at;
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} Area Trigger Teleport Definitions in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

void ObjectMgr::LoadAccessRequirements()
{
    uint32 oldMSTime = getMSTime();

    if (!_accessRequirementStore.empty())
    {
        for (DungeonProgressionRequirementsContainer::iterator itr = _accessRequirementStore.begin(); itr != _accessRequirementStore.end(); ++itr)
        {
            std::unordered_map<uint8, DungeonProgressionRequirements*> difficulties = itr->second;
            for (auto difficultiesItr = difficulties.begin(); difficultiesItr != difficulties.end(); ++difficultiesItr)
            {
                for (auto questItr = difficultiesItr->second->quests.begin(); questItr != difficultiesItr->second->quests.end(); ++questItr)
                {
                    delete* questItr;
                }

                for (auto achievementItr = difficultiesItr->second->achievements.begin(); achievementItr != difficultiesItr->second->achievements.end(); ++achievementItr)
                {
                    delete* achievementItr;
                }

                for (auto itemsItr = difficultiesItr->second->items.begin(); itemsItr != difficultiesItr->second->items.end(); ++itemsItr)
                {
                    delete* itemsItr;
                }

                delete difficultiesItr->second;
            }
        }

        _accessRequirementStore.clear();                                  // need for reload case
    }
    //                                                               0       1            2           3          4            5
    QueryResult access_template_result = WorldDatabase.Query("SELECT id, map_id, difficulty, min_level, max_level, min_avg_item_level FROM dungeon_access_template");
    if (!access_template_result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 access requirement definitions. DB table `dungeon_access_template` is empty.");
        LOG_INFO("server.loading", " ");
        return;
    }

    uint32 count = 0;
    uint32 countProgressionRequirements = 0;

    do
    {
        Field* fields = access_template_result->Fetch();

        //Get the common variables for the access requirements
        uint8 dungeon_access_id = fields[0].Get<uint8>();
        uint32 mapid            = fields[1].Get<uint32>();
        uint8 difficulty        = fields[2].Get<uint8>();

        //Set up the access requirements
        DungeonProgressionRequirements* ar = new DungeonProgressionRequirements();
        ar->levelMin     = fields[3].Get<uint8>();
        ar->levelMax     = fields[4].Get<uint8>();
        ar->reqItemLevel = fields[5].Get<uint16>();

        //                                                                              0                 1               2                 3        4         6
        QueryResult progression_requirements_results = WorldDatabase.Query("SELECT requirement_type, requirement_id, requirement_note, faction, priority, leader_only FROM dungeon_access_requirements where dungeon_access_id = {}", dungeon_access_id);
        if (progression_requirements_results)
        {
            do
            {
                Field* progression_requirement_row = progression_requirements_results->Fetch();

                const uint8 requirement_type             = progression_requirement_row[0].Get<uint8>();
                const uint32 requirement_id              = progression_requirement_row[1].Get<uint32>();
                const std::string requirement_note       = progression_requirement_row[2].Get<std::string>();
                const uint8 requirement_faction          = progression_requirement_row[3].Get<uint8>();
                const uint8 requirement_priority         = progression_requirement_row[4].IsNull() ? UINT8_MAX : progression_requirement_row[4].Get<uint8>();
                const bool requirement_checkLeaderOnly   = progression_requirement_row[5].Get<bool>();

                ProgressionRequirement* progression_requirement = new ProgressionRequirement();
                progression_requirement->id              = requirement_id;
                progression_requirement->note            = requirement_note;
                progression_requirement->faction         = (TeamId)requirement_faction;
                progression_requirement->priority        = requirement_priority;
                progression_requirement->checkLeaderOnly = requirement_checkLeaderOnly;

                std::vector<ProgressionRequirement*>* currentRequirementsList = nullptr;

                switch (requirement_type)
                {
                case 0:
                {
                    //Achievement
                    if (!sAchievementStore.LookupEntry(progression_requirement->id))
                    {
                        LOG_ERROR("sql.sql", "Required achievement {} for faction {} does not exist for map {} difficulty {}, remove or fix this achievement requirement.", progression_requirement->id, requirement_faction, mapid, difficulty);
                        break;
                    }

                    currentRequirementsList = &ar->achievements;
                    break;
                }
                case 1:
                {
                    //Quest
                    if (!GetQuestTemplate(progression_requirement->id))
                    {
                        LOG_ERROR("sql.sql", "Required quest {} for faction {} does not exist for map {} difficulty {}, remove or fix this quest requirement.", progression_requirement->id, requirement_faction, mapid, difficulty);
                        break;
                    }

                    currentRequirementsList = &ar->quests;
                    break;
                }
                case 2:
                {
                    //Item
                    ItemTemplate const* pProto = GetItemTemplate(progression_requirement->id);
                    if (!pProto)
                    {
                        LOG_ERROR("sql.sql", "Required item {} for faction {} does not exist for map {} difficulty {}, remove or fix this item requirement.", progression_requirement->id, requirement_faction, mapid, difficulty);
                        break;
                    }

                    currentRequirementsList = &ar->items;
                    break;
                }
                default:
                    LOG_ERROR("sql.sql", "requirement_type of {} is not valid for map {} difficulty {}. Please use 0 for achievements, 1 for quest, 2 for items or remove this entry from the db.", requirement_type, mapid, difficulty);
                    break;
                }

                //Check if array is valid and delete the progression requirement
                if (!currentRequirementsList)
                {
                    delete progression_requirement;
                    continue;
                }

                //Insert into the array
                if (currentRequirementsList->size() > requirement_priority)
                {
                    currentRequirementsList->insert(currentRequirementsList->begin() + requirement_priority, progression_requirement);
                }
                else
                {
                    currentRequirementsList->push_back(progression_requirement);
                }

            } while (progression_requirements_results->NextRow());
        }

        //Sort all arrays for priority
        auto sortFunction = [](ProgressionRequirement const* const a, ProgressionRequirement const* const b) {return a->priority > b->priority; };
        std::sort(ar->achievements.begin(), ar->achievements.end(), sortFunction);
        std::sort(ar->quests.begin(), ar->quests.end(), sortFunction);
        std::sort(ar->items.begin(), ar->items.end(), sortFunction);

        countProgressionRequirements += ar->achievements.size();
        countProgressionRequirements += ar->quests.size();
        countProgressionRequirements += ar->items.size();
        count++;

        _accessRequirementStore[mapid][difficulty] = ar;
    } while (access_template_result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} Rows From dungeon_access_template And {} Rows From dungeon_access_requirements in {} ms", count, countProgressionRequirements, GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

/*
 * Searches for the areatrigger which teleports players out of the given map with instance_template.parent field support
 */
AreaTriggerTeleport const* ObjectMgr::GetGoBackTrigger(uint32 Map) const
{
    bool useParentDbValue = false;
    uint32 parentId = 0;
    MapEntry const* mapEntry = sMapStore.LookupEntry(Map);
    if (!mapEntry || mapEntry->CorpseMapID < 0)
        return nullptr;

    if (mapEntry->IsDungeon())
    {
        InstanceTemplate const* iTemplate = sObjectMgr->GetInstanceTemplate(Map);

        if (!iTemplate)
            return nullptr;

        parentId = iTemplate->Parent;
        useParentDbValue = true;
    }

    uint32 entrance_map = uint32(mapEntry->CorpseMapID);
    for (AreaTriggerTeleportContainer::const_iterator itr = _areaTriggerTeleportStore.begin(); itr != _areaTriggerTeleportStore.end(); ++itr)
        if ((!useParentDbValue && itr->second.target_mapId == entrance_map) || (useParentDbValue && itr->second.target_mapId == parentId))
        {
            AreaTrigger const* atEntry = GetAreaTrigger(itr->first);
            if (atEntry && atEntry->map == Map)
                return &itr->second;
        }
    return nullptr;
}

/**
 * Searches for the areatrigger which teleports players to the given map
 */
AreaTriggerTeleport const* ObjectMgr::GetMapEntranceTrigger(uint32 Map) const
{
    for (AreaTriggerTeleportContainer::const_iterator itr = _areaTriggerTeleportStore.begin(); itr != _areaTriggerTeleportStore.end(); ++itr)
    {
        if (itr->second.target_mapId == Map) // Id is used to determine correct Scarlet Monastery instance
        {
            // xinef: no need to check, already done at loading
            //AreaTriggerEntry const* atEntry = sAreaTriggerStore.LookupEntry(itr->first);
            //if (atEntry)
            return &itr->second;
        }
    }
    return nullptr;
}

void ObjectMgr::SetHighestGuids()
{
    // Refuse to start once the next guid would already trip ObjectGuidGenerator's overflow check. At the cap,
    // MAX(guid) + 1 wraps to 0 and the cleanup below deletes every item relation.
    auto setNextGuid = [](ObjectGuidGeneratorBase& generator, HighGuid high, char const* table)
    {
        QueryResult result = CharacterDatabase.Query("SELECT MAX(guid) FROM {}", table);
        if (!result)
            return;

        uint32 maxGuid = (*result)[0].Get<uint32>();
        if (maxGuid >= ObjectGuid::GetMaxCounter(high) - 2)
        {
            LOG_FATAL("server.loading", "{} guid space is exhausted (MAX(guid) in `{}` is {}). Renumber the guids before starting the server.",
                ObjectGuid::GetTypeName(high), table, maxGuid);
            exit(1);
        }

        generator.Set(maxGuid + 1);
    };

    setNextGuid(GetGuidSequenceGenerator<HighGuid::Player>(), HighGuid::Player, "characters");
    setNextGuid(GetGuidSequenceGenerator<HighGuid::Item>(), HighGuid::Item, "item_instance");

    // Cleanup other tables from not existed guids ( >= _hiItemGuid)
    CharacterDatabase.Execute("DELETE FROM character_inventory WHERE item >= '{}'", GetGuidSequenceGenerator<HighGuid::Item>().GetNextAfterMaxUsed());     // One-time query
    CharacterDatabase.Execute("DELETE FROM mail_items WHERE item_guid >= '{}'", GetGuidSequenceGenerator<HighGuid::Item>().GetNextAfterMaxUsed());         // One-time query
    CharacterDatabase.Execute("DELETE FROM auctionhouse WHERE itemguid >= '{}'", GetGuidSequenceGenerator<HighGuid::Item>().GetNextAfterMaxUsed());        // One-time query
    CharacterDatabase.Execute("DELETE FROM guild_bank_item WHERE item_guid >= '{}'", GetGuidSequenceGenerator<HighGuid::Item>().GetNextAfterMaxUsed());    // One-time query

    QueryResult result = WorldDatabase.Query("SELECT MAX(guid) FROM transports");
    if (result)
        GetGuidSequenceGenerator<HighGuid::Transport>().Set((*result)[0].Get<uint32>() + 1);

    result = CharacterDatabase.Query("SELECT MAX(id) FROM auctionhouse");
    if (result)
        _auctionId = (*result)[0].Get<uint32>() + 1;

    result = CharacterDatabase.Query("SELECT MAX(id) FROM mail");
    if (result)
        _mailId = (*result)[0].Get<uint32>() + 1;

    result = CharacterDatabase.Query("SELECT MAX(itemId) FROM character_void_storage");
    if (result)
        _voidItemId = (*result)[0].Get<uint64>() + 1;

    result = CharacterDatabase.Query("SELECT MAX(arenateamid) FROM arena_team");
    if (result)
        sArenaTeamMgr->SetNextArenaTeamId((*result)[0].Get<uint32>() + 1);

    result = CharacterDatabase.Query("SELECT MAX(fight_id) FROM log_arena_fights");
    if (result)
        sArenaTeamMgr->SetLastArenaLogId((*result)[0].Get<uint32>());

    result = CharacterDatabase.Query("SELECT MAX(setguid) FROM character_equipmentsets");
    if (result)
        _equipmentSetGuid = (*result)[0].Get<uint64>() + 1;

    result = CharacterDatabase.Query("SELECT MAX(guildId) FROM guild");
    if (result)
        sGuildMgr->SetNextGuildId((*result)[0].Get<uint32>() + 1);

    result = WorldDatabase.Query("SELECT MAX(guid) FROM creature");
    if (result)
        _creatureSpawnId = (*result)[0].Get<uint32>() + 1;

    result = WorldDatabase.Query("SELECT MAX(guid) FROM gameobject");
    if (result)
        _gameObjectSpawnId = (*result)[0].Get<uint32>() + 1;
}

uint32 ObjectMgr::GenerateAuctionID()
{
    if (_auctionId >= 0xFFFFFFFE)
    {
        LOG_ERROR("server.worldserver", "Auctions ids overflow!! Can't continue, shutting down server. ");
        World::StopNow(ERROR_EXIT_CODE);
    }
    return _auctionId++;
}

uint64 ObjectMgr::GenerateEquipmentSetGuid()
{
    if (_equipmentSetGuid >= uint64(0xFFFFFFFFFFFFFFFELL))
    {
        LOG_ERROR("server.worldserver", "EquipmentSet guid overflow!! Can't continue, shutting down server. ");
        World::StopNow(ERROR_EXIT_CODE);
    }
    return _equipmentSetGuid++;
}

uint32 ObjectMgr::GenerateMailID()
{
    if (_mailId >= 0xFFFFFFFE)
    {
        LOG_ERROR("server.worldserver", "Mail ids overflow!! Can't continue, shutting down server. ");
        World::StopNow(ERROR_EXIT_CODE);
    }
    std::lock_guard<std::mutex> guard(_mailIdMutex);
    return _mailId++;
}

uint64 ObjectMgr::GenerateVoidStorageItemId()
{
    std::lock_guard<std::mutex> guard(_voidItemIdMutex);
    return _voidItemId++;
}

ObjectGuid::LowType ObjectMgr::GenerateCreatureSpawnId()
{
    // 6.x guids carry a 40-bit counter
    if (_creatureSpawnId >= UI64LIT(0xFFFFFFFFFF))
    {
        LOG_ERROR("server.worldserver", "Creature spawn id overflow!! Can't continue, shutting down server. Search on forum for TCE00007 for more info.");
        World::StopNow(ERROR_EXIT_CODE);
    }
    return _creatureSpawnId++;
}

ObjectGuid::LowType ObjectMgr::GenerateGameObjectSpawnId()
{
    // 6.x guids carry a 40-bit counter
    if (_gameObjectSpawnId >= UI64LIT(0xFFFFFFFFFF))
    {
        LOG_ERROR("server.worldserver", "GameObject spawn id overflow!! Can't continue, shutting down server. Search on forum for TCE00007 for more info. ");
        World::StopNow(ERROR_EXIT_CODE);
    }
    return _gameObjectSpawnId++;
}

void ObjectMgr::LoadGameObjectLocales()
{
    uint32 oldMSTime = getMSTime();

    _gameObjectLocaleStore.clear(); // need for reload case

    //                                               0      1       2     3
    QueryResult result = WorldDatabase.Query("SELECT entry, locale, name, castBarCaption FROM gameobject_template_locale");
    if (!result)
        return;

    do
    {
        Field* fields = result->Fetch();

        uint32 ID = fields[0].Get<uint32>();

        LocaleConstant locale = GetLocaleByName(fields[1].Get<std::string>());
        if (locale == LOCALE_enUS)
            continue;

        GameObjectLocale& data = _gameObjectLocaleStore[ID];
        AddLocaleString(fields[2].Get<std::string>(), locale, data.Name);
        AddLocaleString(fields[3].Get<std::string>(), locale, data.CastBarCaption);
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} Gameobject Locale Strings in {} ms", (uint32)_gameObjectLocaleStore.size(), GetMSTimeDiffToNow(oldMSTime));
}

inline void CheckGOLockId(GameObjectTemplate const* goInfo, uint32 dataN, uint32 N)
{
    if (sLockStore.LookupEntry(dataN))
        return;

    LOG_ERROR("sql.sql", "Gameobject (Entry: {} GoType: {}) have data{}={} but lock (Id: {}) not found.",
                     goInfo->entry, goInfo->type, N, goInfo->door.lockId, goInfo->door.lockId);
}

inline void CheckGOLinkedTrapId(GameObjectTemplate const* goInfo, uint32 dataN, uint32 N)
{
    if (GameObjectTemplate const* trapInfo = sObjectMgr->GetGameObjectTemplate(dataN))
    {
        if (trapInfo->type != GAMEOBJECT_TYPE_TRAP)
            LOG_ERROR("sql.sql", "Gameobject (Entry: {} GoType: {}) have data{}={} but GO (Entry {}) have not GAMEOBJECT_TYPE_TRAP ({}) type.",
                             goInfo->entry, goInfo->type, N, dataN, dataN, GAMEOBJECT_TYPE_TRAP);
    }
}

inline void CheckGOSpellId(GameObjectTemplate const* goInfo, uint32 dataN, uint32 N)
{
    if (sSpellMgr->GetSpellInfo(dataN))
        return;

    LOG_ERROR("sql.sql", "Gameobject (Entry: {} GoType: {}) have data{}={} but Spell (Entry {}) not exist.",
                     goInfo->entry, goInfo->type, N, dataN, dataN);
}

inline void CheckAndFixGOChairHeightId(GameObjectTemplate const* goInfo, uint32 const& dataN, uint32 N)
{
    if (dataN <= (UNIT_STAND_STATE_SIT_HIGH_CHAIR - UNIT_STAND_STATE_SIT_LOW_CHAIR))
        return;

    LOG_ERROR("sql.sql", "Gameobject (Entry: {} GoType: {}) have data{}={} but correct chair height in range 0..{}.",
                     goInfo->entry, goInfo->type, N, dataN, UNIT_STAND_STATE_SIT_HIGH_CHAIR - UNIT_STAND_STATE_SIT_LOW_CHAIR);

    // prevent client and server unexpected work
    const_cast<uint32&>(dataN) = 0;
}

inline void CheckGONoDamageImmuneId(GameObjectTemplate* goTemplate, uint32 dataN, uint32 N)
{
    // 0/1 correct values
    if (dataN <= 1)
        return;

    LOG_ERROR("sql.sql", "Gameobject (Entry: {} GoType: {}) have data{}={} but expected boolean (0/1) noDamageImmune field value.", goTemplate->entry, goTemplate->type, N, dataN);
}

inline void CheckGOConsumable(GameObjectTemplate const* goInfo, uint32 dataN, uint32 N)
{
    // 0/1 correct values
    if (dataN <= 1)
        return;

    LOG_ERROR("sql.sql", "Gameobject (Entry: {} GoType: {}) have data{}={} but expected boolean (0/1) consumable field value.",
                     goInfo->entry, goInfo->type, N, dataN);
}

void ObjectMgr::LoadGameObjectTemplate()
{
    uint32 oldMSTime = getMSTime();

    //                                                 0      1      2        3       4             5          6      7
    QueryResult result = WorldDatabase.Query("SELECT entry, type, displayId, name, IconName, castBarCaption, unk1, size, "
                         //                                          8      9      10     11     12     13     14     15     16     17     18      19      20
                         "Data0, Data1, Data2, Data3, Data4, Data5, Data6, Data7, Data8, Data9, Data10, Data11, Data12, "
                         //                                          21      22      23      24      25      26      27      28      29      30      31      32        33
                         "Data13, Data14, Data15, Data16, Data17, Data18, Data19, Data20, Data21, Data22, Data23, AIName, ScriptName "
                         "FROM gameobject_template");

    if (!result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 gameobject definitions. DB table `gameobject_template` is empty.");
        LOG_INFO("server.loading", " ");
        return;
    }

    _gameObjectTemplateStore.rehash(result->GetRowCount());
    uint32 count = 0;
    do
    {
        Field* fields = result->Fetch();

        uint32 entry = fields[0].Get<uint32>();

        GameObjectTemplate& got = _gameObjectTemplateStore[entry];

        got.entry          = entry;
        got.type           = uint32(fields[1].Get<uint8>());
        got.displayId      = fields[2].Get<uint32>();
        got.name           = fields[3].Get<std::string>();
        got.IconName       = fields[4].Get<std::string>();
        got.castBarCaption = fields[5].Get<std::string>();
        got.unk1           = fields[6].Get<std::string>();
        got.size           = fields[7].Get<float>();

        for (uint8 i = 0; i < MAX_GAMEOBJECT_DATA; ++i)
            got.raw.data[i] = fields[8 + i].Get<int32>(); // data1 and data6 can be -1

        got.AIName = fields[32].Get<std::string>();
        got.ScriptId = GetScriptId(fields[33].Get<std::string>());
        got.IsForQuests = false;

        // Checks
        if (!got.AIName.empty() && !sGameObjectAIRegistry->HasItem(got.AIName))
        {
            LOG_ERROR("sql.sql", "GameObject (Entry: {}) has non-registered `AIName` '{}' set, removing", got.entry, got.AIName);
        }

        switch (got.type)
        {
            case GAMEOBJECT_TYPE_DOOR:                      //0
                {
                    if (got.door.lockId)
                        CheckGOLockId(&got, got.door.lockId, 1);
                    CheckGONoDamageImmuneId(&got, got.door.noDamageImmune, 3);
                    break;
                }
            case GAMEOBJECT_TYPE_BUTTON:                    //1
                {
                    if (got.button.lockId)
                        CheckGOLockId(&got, got.button.lockId, 1);
                    CheckGONoDamageImmuneId(&got, got.button.noDamageImmune, 4);
                    break;
                }
            case GAMEOBJECT_TYPE_QUESTGIVER:                //2
                {
                    if (got.questgiver.lockId)
                        CheckGOLockId(&got, got.questgiver.lockId, 0);
                    CheckGONoDamageImmuneId(&got, got.questgiver.noDamageImmune, 5);
                    break;
                }
            case GAMEOBJECT_TYPE_CHEST:                     //3
                {
                    if (got.chest.lockId)
                        CheckGOLockId(&got, got.chest.lockId, 0);

                    CheckGOConsumable(&got, got.chest.consumable, 3);

                    if (got.chest.linkedTrapId)              // linked trap
                        CheckGOLinkedTrapId(&got, got.chest.linkedTrapId, 7);
                    break;
                }
            case GAMEOBJECT_TYPE_TRAP:                      //6
                {
                    if (got.trap.lockId)
                        CheckGOLockId(&got, got.trap.lockId, 0);
                    break;
                }
            case GAMEOBJECT_TYPE_CHAIR:                     //7
                CheckAndFixGOChairHeightId(&got, got.chair.height, 1);
                break;
            case GAMEOBJECT_TYPE_SPELL_FOCUS:               //8
                {
                    if (got.spellFocus.focusId)
                    {
                        if (!sSpellFocusObjectStore.LookupEntry(got.spellFocus.focusId))
                            LOG_ERROR("sql.sql", "GameObject (Entry: {} GoType: {}) have data0={} but SpellFocus (Id: {}) not exist.",
                                             entry, got.type, got.spellFocus.focusId, got.spellFocus.focusId);
                    }

                    if (got.spellFocus.linkedTrapId)        // linked trap
                        CheckGOLinkedTrapId(&got, got.spellFocus.linkedTrapId, 2);
                    break;
                }
            case GAMEOBJECT_TYPE_GOOBER:                    //10
                {
                    if (got.goober.lockId)
                        CheckGOLockId(&got, got.goober.lockId, 0);

                    CheckGOConsumable(&got, got.goober.consumable, 3);

                    if (got.goober.pageId)                  // pageId
                    {
                        if (!GetPageText(got.goober.pageId))
                            LOG_ERROR("sql.sql", "GameObject (Entry: {} GoType: {}) have data7={} but PageText (Entry {}) not exist.",
                                             entry, got.type, got.goober.pageId, got.goober.pageId);
                    }
                    CheckGONoDamageImmuneId(&got, got.goober.noDamageImmune, 11);
                    if (got.goober.linkedTrapId)            // linked trap
                        CheckGOLinkedTrapId(&got, got.goober.linkedTrapId, 12);
                    break;
                }
            case GAMEOBJECT_TYPE_AREADAMAGE:                //12
                {
                    if (got.areadamage.lockId)
                        CheckGOLockId(&got, got.areadamage.lockId, 0);
                    break;
                }
            case GAMEOBJECT_TYPE_CAMERA:                    //13
                {
                    if (got.camera.lockId)
                        CheckGOLockId(&got, got.camera.lockId, 0);
                    break;
                }
            case GAMEOBJECT_TYPE_MAP_OBJ_TRANSPORT:              //15
                {
                    if (got.moTransport.taxiPathId)
                    {
                        if (got.moTransport.taxiPathId >= sTaxiPathNodesByPath.size() || sTaxiPathNodesByPath[got.moTransport.taxiPathId].empty())
                            LOG_ERROR("sql.sql", "GameObject (Entry: {} GoType: {}) have data0={} but TaxiPath (Id: {}) not exist.",
                                             entry, got.type, got.moTransport.taxiPathId, got.moTransport.taxiPathId);
                    }
                    if (uint32 transportMap = got.moTransport.mapID)
                        _transportMaps.insert(transportMap);
                    break;
                }
            case GAMEOBJECT_TYPE_RITUAL:          //18
                break;
            case GAMEOBJECT_TYPE_SPELLCASTER:               //22
                {
                    // always must have spell
                    CheckGOSpellId(&got, got.spellcaster.spellId, 0);
                    break;
                }
            case GAMEOBJECT_TYPE_FLAGSTAND:                 //24
                {
                    if (got.flagstand.lockId)
                        CheckGOLockId(&got, got.flagstand.lockId, 0);
                    CheckGONoDamageImmuneId(&got, got.flagstand.noDamageImmune, 5);
                    break;
                }
            case GAMEOBJECT_TYPE_FISHINGHOLE:               //25
                {
                    if (got.fishinghole.lockId)
                        CheckGOLockId(&got, got.fishinghole.lockId, 4);
                    break;
                }
            case GAMEOBJECT_TYPE_FLAGDROP:                  //26
                {
                    if (got.flagdrop.lockId)
                        CheckGOLockId(&got, got.flagdrop.lockId, 0);
                    CheckGONoDamageImmuneId(&got, got.flagdrop.noDamageImmune, 3);
                    break;
                }
            case GAMEOBJECT_TYPE_BARBER_CHAIR:              //32
                CheckAndFixGOChairHeightId(&got, got.barberChair.chairheight, 0);
                break;
        }

        ++count;
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} Game Object Templates in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

void ObjectMgr::LoadGameObjectTemplateAddons()
{
    uint32 oldMSTime = getMSTime();

    //                                                0       1       2      3        4       5        6        7        8
    QueryResult result = WorldDatabase.Query("SELECT entry, faction, flags, mingold, maxgold, artkit0, artkit1, artkit2, artkit3 FROM gameobject_template_addon");

    if (!result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 gameobject template addon definitions. DB table `gameobject_template_addon` is empty.");
        LOG_INFO("server.loading", " ");
        return;
    }

    uint32 count = 0;
    do
    {
        Field* fields = result->Fetch();

        uint32 entry = fields[0].Get<uint32>();

        GameObjectTemplate const* got = GetGameObjectTemplate(entry);
        if (!got)
        {
            LOG_ERROR("sql.sql",
                "GameObject template (Entry: {}) does not exist but has a record in `gameobject_template_addon`",
                entry);
            continue;
        }

        GameObjectTemplateAddon& gameObjectAddon = _gameObjectTemplateAddonStore[entry];
        gameObjectAddon.faction = uint32(fields[1].Get<uint16>());
        gameObjectAddon.flags   = fields[2].Get<uint32>();
        gameObjectAddon.mingold = fields[3].Get<uint32>();
        gameObjectAddon.maxgold = fields[4].Get<uint32>();

        for (uint32 i = 0; i < gameObjectAddon.artKits.size(); i++)
        {
            uint32 artKitID = fields[5 + i].Get<uint32>();
            if (!artKitID)
                continue;

            if (!sGameObjectArtKitStore.LookupEntry(artKitID))
            {
                LOG_ERROR("sql.sql", "GameObject (Entry: {}) has invalid `artkit{}` {} defined, set to zero instead.", entry, i, artKitID);
                continue;
            }

            gameObjectAddon.artKits[i] = artKitID;
        }

        // checks
        if (gameObjectAddon.faction && !sFactionTemplateStore.LookupEntry(gameObjectAddon.faction))
            LOG_ERROR("sql.sql",
                "GameObject (Entry: {}) has invalid faction ({}) defined in `gameobject_template_addon`.",
                entry, gameObjectAddon.faction);

        if (gameObjectAddon.maxgold > 0)
        {
            switch (got->type)
            {
                case GAMEOBJECT_TYPE_CHEST:
                case GAMEOBJECT_TYPE_FISHINGHOLE:
                    break;
                default:
                    LOG_ERROR("sql.sql",
                        "GameObject (Entry {} GoType: {}) cannot be looted but has maxgold set in `gameobject_template_addon`.",
                        entry, got->type);
                    break;
            }
        }

        ++count;
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} Game Object Template Addons in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

void ObjectMgr::LoadExplorationBaseXP()
{
    uint32 oldMSTime = getMSTime();

    QueryResult result = WorldDatabase.Query("SELECT level, basexp FROM exploration_basexp");

    if (!result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 BaseXP definitions. DB table `exploration_basexp` is empty.");
        LOG_INFO("server.loading", " ");
        return;
    }

    uint32 count = 0;

    do
    {
        Field* fields = result->Fetch();
        uint8 level  = fields[0].Get<uint8>();
        uint32 basexp = fields[1].Get<int32>();
        _baseXPTable[level] = basexp;
        ++count;
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} BaseXP Definitions in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

uint32 ObjectMgr::GetBaseXP(uint8 level)
{
    return _baseXPTable[level] ? _baseXPTable[level] : 0;
}

uint32 ObjectMgr::GetXPForLevel(uint8 level) const
{
    if (level < _playerXPperLevel.size())
        return _playerXPperLevel[level];
    return 0;
}

void ObjectMgr::LoadPetNames()
{
    uint32 oldMSTime = getMSTime();
    //                                                0     1      2
    QueryResult result = WorldDatabase.Query("SELECT word, entry, half FROM pet_name_generation");

    if (!result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 pet name parts. DB table `pet_name_generation` is empty!");
        LOG_INFO("server.loading", " ");
        return;
    }

    uint32 count = 0;

    do
    {
        Field* fields = result->Fetch();
        std::string word = fields[0].Get<std::string>();
        uint32 entry     = fields[1].Get<uint32>();
        bool   half      = fields[2].Get<bool>();
        if (half)
            _petHalfName1[entry].push_back(word);
        else
            _petHalfName0[entry].push_back(word);
        ++count;
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} Pet Name Parts in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

void ObjectMgr::LoadPetNumber()
{
    uint32 oldMSTime = getMSTime();

    QueryResult result = CharacterDatabase.Query("SELECT MAX(id) FROM character_pet");
    if (result)
    {
        Field* fields = result->Fetch();
        _hiPetNumber = fields[0].Get<uint32>() + 1;
    }

    LOG_INFO("server.loading", ">> Loaded The Max Pet Number: {} in {} ms", _hiPetNumber - 1, GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

std::string ObjectMgr::GeneratePetNameLocale(uint32 entry, LocaleConstant locale)
{
    std::vector<std::string>& list0 = _petHalfLocaleName0[std::make_pair(entry, locale)];
    std::vector<std::string>& list1 = _petHalfLocaleName1[std::make_pair(entry, locale)];

    if (list0.empty() || list1.empty())
    {
       return GeneratePetName(entry);
    }

    return *(list0.begin() + urand(0, list0.size() - 1)) + *(list1.begin() + urand(0, list1.size() - 1));
}

std::string ObjectMgr::GeneratePetName(uint32 entry)
{
    std::vector<std::string>& list0 = _petHalfName0[entry];
    std::vector<std::string>& list1 = _petHalfName1[entry];

    if (list0.empty() || list1.empty())
    {
        CreatureTemplate const* cinfo = GetCreatureTemplate(entry);
        char const* petname = GetPetName(cinfo->family, sWorld->GetDefaultDbcLocale());
        if (!petname)
            return cinfo->Name;

        return std::string(petname);
    }

    return *(list0.begin() + urand(0, list0.size() - 1)) + *(list1.begin() + urand(0, list1.size() - 1));
}

uint32 ObjectMgr::GeneratePetNumber()
{
    std::lock_guard<std::mutex> guard(_hiPetNumberMutex);
    return ++_hiPetNumber;
}

void ObjectMgr::LoadReputationRewardRate()
{
    uint32 oldMSTime = getMSTime();

    _repRewardRateStore.clear();                             // for reload case

    uint32 count = 0; //                                0          1             2                  3                  4                 5                      6             7
    QueryResult result = WorldDatabase.Query("SELECT faction, quest_rate, quest_daily_rate, quest_weekly_rate, quest_monthly_rate, quest_repeatable_rate, creature_rate, spell_rate FROM reputation_reward_rate");
    if (!result)
    {
        LOG_INFO("server.loading", ">> Loaded `reputation_reward_rate`, table is empty!");
        return;
    }

    do
    {
        Field* fields = result->Fetch();

        uint32 factionId            = fields[0].Get<uint32>();

        RepRewardRate repRate;

        repRate.questRate           = fields[1].Get<float>();
        repRate.questDailyRate      = fields[2].Get<float>();
        repRate.questWeeklyRate     = fields[3].Get<float>();
        repRate.questMonthlyRate    = fields[4].Get<float>();
        repRate.questRepeatableRate = fields[5].Get<float>();
        repRate.creatureRate        = fields[6].Get<float>();
        repRate.spellRate           = fields[7].Get<float>();

        FactionEntry const* factionEntry = sFactionStore.LookupEntry(factionId);
        if (!factionEntry)
        {
            LOG_ERROR("sql.sql", "Faction (faction.dbc) {} does not exist but is used in `reputation_reward_rate`", factionId);
            continue;
        }

        if (repRate.questRate < 0.0f)
        {
            LOG_ERROR("sql.sql", "Table reputation_reward_rate has quest_rate with invalid rate {}, skipping data for faction {}", repRate.questRate, factionId);
            continue;
        }

        if (repRate.questDailyRate < 0.0f)
        {
            LOG_ERROR("sql.sql", "Table reputation_reward_rate has quest_daily_rate with invalid rate {}, skipping data for faction {}", repRate.questDailyRate, factionId);
            continue;
        }

        if (repRate.questWeeklyRate < 0.0f)
        {
            LOG_ERROR("sql.sql", "Table reputation_reward_rate has quest_weekly_rate with invalid rate {}, skipping data for faction {}", repRate.questWeeklyRate, factionId);
            continue;
        }

        if (repRate.questMonthlyRate < 0.0f)
        {
            LOG_ERROR("sql.sql", "Table reputation_reward_rate has quest_monthly_rate with invalid rate {}, skipping data for faction {}", repRate.questMonthlyRate, factionId);
            continue;
        }

        if (repRate.questRepeatableRate < 0.0f)
        {
            LOG_ERROR("sql.sql", "Table reputation_reward_rate has quest_repeatable_rate with invalid rate {}, skipping data for faction {}", repRate.questRepeatableRate, factionId);
            continue;
        }

        if (repRate.creatureRate < 0.0f)
        {
            LOG_ERROR("sql.sql", "Table reputation_reward_rate has creature_rate with invalid rate {}, skipping data for faction {}", repRate.creatureRate, factionId);
            continue;
        }

        if (repRate.spellRate < 0.0f)
        {
            LOG_ERROR("sql.sql", "Table reputation_reward_rate has spell_rate with invalid rate {}, skipping data for faction {}", repRate.spellRate, factionId);
            continue;
        }

        _repRewardRateStore[factionId] = repRate;

        ++count;
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} Reputation Reward Rate in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

void ObjectMgr::LoadReputationOnKill()
{
    uint32 oldMSTime = getMSTime();

    // For reload case
    _repOnKillStore.clear();

    uint32 count = 0;

    //                                                0            1                     2
    QueryResult result = WorldDatabase.Query("SELECT creature_id, RewOnKillRepFaction1, RewOnKillRepFaction2, "
                         //   3             4             5                   6             7             8                   9
                         "IsTeamAward1, MaxStanding1, RewOnKillRepValue1, IsTeamAward2, MaxStanding2, RewOnKillRepValue2, TeamDependent "
                         "FROM creature_onkill_reputation");

    if (!result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 creature award reputation definitions. DB table `creature_onkill_reputation` is empty.");
        LOG_INFO("server.loading", " ");
        return;
    }

    do
    {
        Field* fields = result->Fetch();

        uint32 creature_id = fields[0].Get<uint32>();

        ReputationOnKillEntry repOnKill;
        repOnKill.RepFaction1          = fields[1].Get<int16>();
        repOnKill.RepFaction2          = fields[2].Get<int16>();
        repOnKill.IsTeamAward1        = fields[3].Get<bool>();
        repOnKill.ReputationMaxCap1  = fields[4].Get<uint8>();
        repOnKill.RepValue1            = fields[5].Get<float>();
        repOnKill.IsTeamAward2        = fields[6].Get<bool>();
        repOnKill.ReputationMaxCap2  = fields[7].Get<uint8>();
        repOnKill.RepValue2            = fields[8].Get<float>();
        repOnKill.TeamDependent       = fields[9].Get<uint8>();

        if (!GetCreatureTemplate(creature_id))
        {
            LOG_ERROR("sql.sql", "Table `creature_onkill_reputation` have data for not existed creature entry ({}), skipped", creature_id);
            continue;
        }

        if (repOnKill.RepFaction1)
        {
            FactionEntry const* factionEntry1 = sFactionStore.LookupEntry(repOnKill.RepFaction1);
            if (!factionEntry1)
            {
                LOG_ERROR("sql.sql", "Faction (faction.dbc) {} does not exist but is used in `creature_onkill_reputation`", repOnKill.RepFaction1);
                continue;
            }
        }

        if (repOnKill.RepFaction2)
        {
            FactionEntry const* factionEntry2 = sFactionStore.LookupEntry(repOnKill.RepFaction2);
            if (!factionEntry2)
            {
                LOG_ERROR("sql.sql", "Faction (faction.dbc) {} does not exist but is used in `creature_onkill_reputation`", repOnKill.RepFaction2);
                continue;
            }
        }

        _repOnKillStore[creature_id] = repOnKill;

        ++count;
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} Creature Award Reputation Definitions in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

void ObjectMgr::LoadReputationSpilloverTemplate()
{
    uint32 oldMSTime = getMSTime();

    _repSpilloverTemplateStore.clear();                      // for reload case

    uint32 count = 0; //                                0         1        2       3        4       5       6         7        8      9        10       11     12        13       14      15       16       17     18
    QueryResult result = WorldDatabase.Query("SELECT faction, faction1, rate_1, rank_1, faction2, rate_2, rank_2, faction3, rate_3, rank_3, faction4, rate_4, rank_4, faction5, rate_5, rank_5, faction6, rate_6, rank_6 FROM reputation_spillover_template");

    if (!result)
    {
        LOG_INFO("server.loading", ">> Loaded `reputation_spillover_template`, table is empty.");
        LOG_INFO("server.loading", " ");
        return;
    }

    do
    {
        Field* fields = result->Fetch();

        uint32 factionId                = fields[0].Get<uint16>();

        RepSpilloverTemplate repTemplate;

        repTemplate.faction[0]          = fields[1].Get<uint16>();
        repTemplate.faction_rate[0]     = fields[2].Get<float>();
        repTemplate.faction_rank[0]     = fields[3].Get<uint8>();
        repTemplate.faction[1]          = fields[4].Get<uint16>();
        repTemplate.faction_rate[1]     = fields[5].Get<float>();
        repTemplate.faction_rank[1]     = fields[6].Get<uint8>();
        repTemplate.faction[2]          = fields[7].Get<uint16>();
        repTemplate.faction_rate[2]     = fields[8].Get<float>();
        repTemplate.faction_rank[2]     = fields[9].Get<uint8>();
        repTemplate.faction[3]          = fields[10].Get<uint16>();
        repTemplate.faction_rate[3]     = fields[11].Get<float>();
        repTemplate.faction_rank[3]     = fields[12].Get<uint8>();
        repTemplate.faction[4]          = fields[13].Get<uint16>();
        repTemplate.faction_rate[4]     = fields[14].Get<float>();
        repTemplate.faction_rank[4]     = fields[15].Get<uint8>();
        repTemplate.faction[5]          = fields[16].Get<uint16>();
        repTemplate.faction_rate[5]     = fields[17].Get<float>();
        repTemplate.faction_rank[5]     = fields[18].Get<uint8>();

        FactionEntry const* factionEntry = sFactionStore.LookupEntry(factionId);

        if (!factionEntry)
        {
            LOG_ERROR("sql.sql", "Faction (faction.dbc) {} does not exist but is used in `reputation_spillover_template`", factionId);
            continue;
        }

        if (factionEntry->ParentFactionID == 0)
        {
            LOG_ERROR("sql.sql", "Faction (faction.dbc) {} in `reputation_spillover_template` does not belong to any team, skipping", factionId);
            continue;
        }

        for (uint32 i = 0; i < MAX_SPILLOVER_FACTIONS; ++i)
        {
            if (repTemplate.faction[i])
            {
                FactionEntry const* factionSpillover = sFactionStore.LookupEntry(repTemplate.faction[i]);

                if (!factionSpillover)
                {
                    LOG_ERROR("sql.sql", "Spillover faction (faction.dbc) {} does not exist but is used in `reputation_spillover_template` for faction {}, skipping", repTemplate.faction[i], factionId);
                    continue;
                }

                if (factionSpillover->ReputationIndex < 0)
                {
                    LOG_ERROR("sql.sql", "Spillover faction (faction.dbc) {} for faction {} in `reputation_spillover_template` can not be listed for client, and then useless, skipping", repTemplate.faction[i], factionId);
                    continue;
                }

                if (repTemplate.faction_rank[i] >= MAX_REPUTATION_RANK)
                {
                    LOG_ERROR("sql.sql", "Rank {} used in `reputation_spillover_template` for spillover faction {} is not valid, skipping", repTemplate.faction_rank[i], repTemplate.faction[i]);
                    continue;
                }
            }
        }

        FactionEntry const* factionEntry0 = sFactionStore.LookupEntry(repTemplate.faction[0]);
        if (repTemplate.faction[0] && !factionEntry0)
        {
            LOG_ERROR("sql.sql", "Faction (faction.dbc) {} does not exist but is used in `reputation_spillover_template`", repTemplate.faction[0]);
            continue;
        }
        FactionEntry const* factionEntry1 = sFactionStore.LookupEntry(repTemplate.faction[1]);
        if (repTemplate.faction[1] && !factionEntry1)
        {
            LOG_ERROR("sql.sql", "Faction (faction.dbc) {} does not exist but is used in `reputation_spillover_template`", repTemplate.faction[1]);
            continue;
        }
        FactionEntry const* factionEntry2 = sFactionStore.LookupEntry(repTemplate.faction[2]);
        if (repTemplate.faction[2] && !factionEntry2)
        {
            LOG_ERROR("sql.sql", "Faction (faction.dbc) {} does not exist but is used in `reputation_spillover_template`", repTemplate.faction[2]);
            continue;
        }
        FactionEntry const* factionEntry3 = sFactionStore.LookupEntry(repTemplate.faction[3]);
        if (repTemplate.faction[3] && !factionEntry3)
        {
            LOG_ERROR("sql.sql", "Faction (faction.dbc) {} does not exist but is used in `reputation_spillover_template`", repTemplate.faction[3]);
            continue;
        }

        _repSpilloverTemplateStore[factionId] = repTemplate;

        ++count;
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} Reputation Spillover Template in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

void ObjectMgr::LoadPointsOfInterest()
{
    uint32 oldMSTime = getMSTime();

    _pointsOfInterestStore.clear();                              // need for reload case

    uint32 count = 0;

    //                                               0       1          2        3     4      5    6
    QueryResult result = WorldDatabase.Query("SELECT ID, PositionX, PositionY, Icon, Flags, Importance, Name FROM points_of_interest");

    if (!result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 Points of Interest definitions. DB table `points_of_interest` is empty.");
        LOG_INFO("server.loading", " ");
        return;
    }

    do
    {
        Field* fields = result->Fetch();

        uint32 point_id = fields[0].Get<uint32>();

        PointOfInterest POI;
        POI.ID          = point_id;
        POI.PositionX   = fields[1].Get<float>();
        POI.PositionY   = fields[2].Get<float>();
        POI.Icon        = fields[3].Get<uint32>();
        POI.Flags       = fields[4].Get<uint32>();
        POI.Importance  = fields[5].Get<uint32>();
        POI.Name        = fields[6].Get<std::string>();

        if (!Acore::IsValidMapCoord(POI.PositionX, POI.PositionY))
        {
            LOG_ERROR("sql.sql", "Table `points_of_interest` (ID: {}) have invalid coordinates (X: {} Y: {}), ignored.", point_id, POI.PositionX, POI.PositionY);
            continue;
        }

        _pointsOfInterestStore[point_id] = POI;

        ++count;
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} Points of Interest Definitions in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

void ObjectMgr::LoadQuestPOI()
{
    if (!sWorld->getBoolConfig(CONFIG_QUEST_POI_ENABLED))
    {
        LOG_INFO("server.loading", ">> Loaded 0 quest POI definitions. Disabled by config.");
        LOG_INFO("server.loading", " ");
        return;
    }

    uint32 oldMSTime = getMSTime();

    _questPOIStore.clear();                              // need for reload case

    uint32 count = 0;

    //                                                 0        1        2            3           4                 5           6         7            8       9       10         11              12             13
    QueryResult result = WorldDatabase.Query("SELECT QuestID, BlobIndex, Idx1, ObjectiveIndex, QuestObjectiveID, QuestObjectID, MapID, WorldMapAreaId, Floor, Priority, Flags, WorldEffectID, PlayerConditionID, WoDUnk1 FROM quest_poi ORDER BY QuestID, Idx1");

    if (!result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 quest POI definitions. DB table `quest_poi` is empty.");
        LOG_INFO("server.loading", " ");
        return;
    }

    //                                                  0       1   2  3
    QueryResult points = WorldDatabase.Query("SELECT QuestID, Idx1, X, Y FROM quest_poi_points ORDER BY QuestID DESC, Idx1, Idx2");

    std::vector<std::vector<std::vector<QuestPOIPoint>>> POIs;

    if (points)
    {
        // The first result should have the highest questId
        Field* fields = points->Fetch();
        uint32 questIdMax = fields[0].Get<uint32>();
        POIs.resize(questIdMax + 1);

        do
        {
            fields = points->Fetch();

            uint32 questId = fields[0].Get<uint32>();
            uint32 idx1    = fields[1].Get<uint32>();
            int32 x        = fields[2].Get<int32>();
            int32 y        = fields[3].Get<int32>();

            if (POIs[questId].size() <= idx1 + 1)
                POIs[questId].resize(idx1 + 10);

            POIs[questId][idx1].emplace_back(x, y);
        } while (points->NextRow());
    }

    do
    {
        Field* fields = result->Fetch();

        uint32 questId            = fields[0].Get<uint32>();
        int32 blobIndex           = fields[1].Get<int32>();
        uint32 idx1               = fields[2].Get<uint32>();
        int32 objectiveIndex      = fields[3].Get<int32>();
        int32 questObjectiveID    = fields[4].Get<int32>();
        int32 questObjectID       = fields[5].Get<int32>();
        int32 mapID               = fields[6].Get<int32>();
        int32 worldMapAreaId      = fields[7].Get<int32>();
        int32 floor               = fields[8].Get<int32>();
        int32 priority            = fields[9].Get<int32>();
        int32 flags               = fields[10].Get<int32>();
        int32 worldEffectID       = fields[11].Get<int32>();
        int32 playerConditionID   = fields[12].Get<int32>();
        int32 wodUnk1             = fields[13].Get<int32>();

        if (!GetQuestTemplate(questId))
            LOG_ERROR("sql.sql", "`quest_poi` quest id ({}) Idx1 ({}) does not exist in `quest_template`", questId, idx1);

        QuestPOI POI(blobIndex, objectiveIndex, questObjectiveID, questObjectID, mapID, worldMapAreaId, floor, priority, flags, worldEffectID, playerConditionID, wodUnk1);
        if (questId < POIs.size() && idx1 < POIs[questId].size())
        {
            POI.points = POIs[questId][idx1];
            _questPOIStore[questId].push_back(POI);
        }
        else
            LOG_ERROR("sql.sql", "Table quest_poi references unknown quest points for quest {} POI id {}", questId, blobIndex);

        ++count;
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} Quest POI definitions in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

void ObjectMgr::LoadNPCSpellClickSpells()
{
    uint32 oldMSTime = getMSTime();

    _spellClickInfoStore.clear();
    //                                                0          1         2            3
    QueryResult result = WorldDatabase.Query("SELECT npc_entry, spell_id, cast_flags, user_type FROM npc_spellclick_spells");

    if (!result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 spellclick spells. DB table `npc_spellclick_spells` is empty.");
        LOG_INFO("server.loading", " ");
        return;
    }

    uint32 count = 0;

    do
    {
        Field* fields = result->Fetch();

        uint32 npc_entry = fields[0].Get<uint32>();
        CreatureTemplate const* cInfo = GetCreatureTemplate(npc_entry);
        if (!cInfo)
        {
            LOG_ERROR("sql.sql", "Table npc_spellclick_spells references unknown creature_template {}. Skipping entry.", npc_entry);
            continue;
        }

        uint32 spellid = fields[1].Get<uint32>();
        SpellInfo const* spellinfo = sSpellMgr->GetSpellInfo(spellid);
        if (!spellinfo)
        {
            LOG_ERROR("sql.sql", "Table npc_spellclick_spells references unknown spellid {}. Skipping entry.", spellid);
            continue;
        }

        uint8 userType = fields[3].Get<uint16>();
        if (userType >= SPELL_CLICK_USER_MAX)
            LOG_ERROR("sql.sql", "Table npc_spellclick_spells references unknown user type {}. Skipping entry.", uint32(userType));

        uint8 castFlags = fields[2].Get<uint8>();
        SpellClickInfo info;
        info.spellId = spellid;
        info.castFlags = castFlags;
        info.userType = SpellClickUserTypes(userType);
        _spellClickInfoStore.insert(SpellClickInfoContainer::value_type(npc_entry, info));

        ++count;
    } while (result->NextRow());

    // all spellclick data loaded, now we check if there are creatures with NPC_FLAG_SPELLCLICK but with no data
    // NOTE: It *CAN* be the other way around: no spellclick flag but with spellclick data, in case of creature-only vehicle accessories
    CreatureTemplateContainer const* ctc = GetCreatureTemplates();
    for (CreatureTemplateContainer::const_iterator itr = ctc->begin(); itr != ctc->end(); ++itr)
    {
        if ((itr->second.npcflag & UNIT_NPC_FLAG_SPELLCLICK) && _spellClickInfoStore.find(itr->second.Entry) == _spellClickInfoStore.end())
        {
            LOG_ERROR("sql.sql", "npc_spellclick_spells: Creature template {} has UNIT_NPC_FLAG_SPELLCLICK but no data in spellclick table! Removing flag", itr->second.Entry);
            const_cast<CreatureTemplate*>(&itr->second)->npcflag &= ~UNIT_NPC_FLAG_SPELLCLICK;
        }
    }

    LOG_INFO("server.loading", ">> Loaded {} Spellclick Definitions in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

void ObjectMgr::DeleteCreatureData(ObjectGuid::LowType guid)
{
    // remove mapid*cellid -> guid_set map
    CreatureData const* data = GetCreatureData(guid);
    if (data)
        RemoveCreatureFromGrid(guid, data);

    _creatureDataStore.erase(guid);
}

void ObjectMgr::DeleteGOData(ObjectGuid::LowType guid)
{
    // remove mapid*cellid -> guid_set map
    GameObjectData const* data = GetGameObjectData(guid);
    if (data)
        RemoveGameobjectFromGrid(guid, data);

    _gameObjectDataStore.erase(guid);
}

SpawnData const* ObjectMgr::GetSpawnData(SpawnObjectType type, ObjectGuid::LowType spawnId) const
{
    switch (type)
    {
        case SPAWN_TYPE_CREATURE:
            return GetCreatureData(spawnId);
        case SPAWN_TYPE_GAMEOBJECT:
            return GetGameObjectData(spawnId);
        default:
            return nullptr;
    }
}

void ObjectMgr::LoadSpawnGroupTemplates()
{
    uint32 oldMSTime = getMSTime();

    _spawnGroupDataStore.clear();

    //                                               0        1          2
    QueryResult result = WorldDatabase.Query("SELECT groupId, groupName, groupFlags FROM spawn_group_template");

    if (result)
    {
        do
        {
            Field* fields = result->Fetch();
            uint32 groupId = fields[0].Get<uint32>();
            SpawnGroupTemplateData& group = _spawnGroupDataStore[groupId];
            group.groupId = groupId;
            group.name = fields[1].Get<std::string>();
            group.mapId = SPAWNGROUP_MAP_UNSET;
            uint32 flags = fields[2].Get<uint32>();
            if (flags & ~uint32(SPAWNGROUP_FLAG_ALL))
            {
                flags &= uint32(SPAWNGROUP_FLAG_ALL);
                LOG_ERROR("sql.sql", "Invalid spawn group flag {} on group ID {} ({}), reduced to valid flags {}.",
                    fields[2].Get<uint32>(), groupId, group.name, flags);
            }
            if ((flags & SPAWNGROUP_FLAG_SYSTEM) && (flags & SPAWNGROUP_FLAG_MANUAL_SPAWN))
            {
                flags &= ~SPAWNGROUP_FLAG_MANUAL_SPAWN;
                LOG_ERROR("sql.sql", "System spawn group {} ({}) has invalid manual spawn flag. Ignored.", groupId, group.name);
            }
            group.flags = SpawnGroupFlags(flags);
        } while (result->NextRow());
    }

    if (_spawnGroupDataStore.find(0) == _spawnGroupDataStore.end())
    {
        LOG_ERROR("sql.sql", "Default spawn group (index 0) is missing from DB! Manually inserted.");
        SpawnGroupTemplateData& data = _spawnGroupDataStore[0];
        data.groupId = 0;
        data.name = "Default Group";
        data.mapId = SPAWNGROUP_MAP_UNSET;
        data.flags = SpawnGroupFlags(SPAWNGROUP_FLAG_SYSTEM);
    }
    if (_spawnGroupDataStore.find(1) == _spawnGroupDataStore.end())
    {
        LOG_ERROR("sql.sql", "Default legacy spawn group (index 1) is missing from DB! Manually inserted.");
        SpawnGroupTemplateData& data = _spawnGroupDataStore[1];
        data.groupId = 1;
        data.name = "Legacy Group";
        data.mapId = SPAWNGROUP_MAP_UNSET;
        data.flags = SpawnGroupFlags(SPAWNGROUP_FLAG_SYSTEM | SPAWNGROUP_FLAG_COMPATIBILITY_MODE);
    }

    LOG_INFO("server.loading", ">> Loaded {} spawn group templates in {} ms", _spawnGroupDataStore.size(), GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

void ObjectMgr::LoadSpawnGroups()
{
    uint32 oldMSTime = getMSTime();

    // Reset prior state for hot-reload support
    // Preserve the forced legacy group for spawns on transport maps (set in LoadCreatures/LoadGameobjects).
    _spawnGroupMapStore.clear();
    for (auto& [id, data] : _creatureDataStore)
        data.spawnGroupId = _transportMaps.count(data.mapid) ? 1 : 0;
    for (auto& [id, data] : _gameObjectDataStore)
        data.spawnGroupId = _transportMaps.count(data.mapid) ? 1 : 0;

    //                                               0        1          2
    QueryResult result = WorldDatabase.Query("SELECT groupId, spawnType, spawnId FROM spawn_group");

    if (!result)
    {
        LOG_INFO("server.loading", ">> Loaded 0 spawn group members. DB table `spawn_group` is empty.");
        LOG_INFO("server.loading", " ");
        return;
    }

    uint32 numMembers = 0;
    do
    {
        Field* fields = result->Fetch();
        uint32 groupId = fields[0].Get<uint32>();
        uint32 type = fields[1].Get<uint8>();
        if (type >= SPAWN_TYPE_MAX)
        {
            LOG_ERROR("sql.sql", "Spawn data with invalid type {} listed for spawn group {}. Skipped.", type, groupId);
            continue;
        }
        SpawnObjectType spawnType = SpawnObjectType(type);
        ObjectGuid::LowType spawnId = fields[2].Get<uint32>();

        SpawnData const* data = GetSpawnData(spawnType, spawnId);
        if (!data)
        {
            LOG_ERROR("sql.sql", "Spawn data with ID ({},{}) not found, but is listed as a member of spawn group {}!",
                uint32(spawnType), spawnId, groupId);
            continue;
        }
        if (data->spawnGroupId)
        {
            LOG_ERROR("sql.sql", "Spawn with ID ({},{}) is listed as a member of spawn group {}, but is already a member of spawn group {}. Skipping.",
                uint32(spawnType), spawnId, groupId, data->spawnGroupId);
            continue;
        }

        auto it = _spawnGroupDataStore.find(groupId);
        if (it == _spawnGroupDataStore.end())
        {
            LOG_ERROR("sql.sql", "Spawn group {} assigned to spawn ID ({},{}), but group is not found!", groupId, uint32(spawnType), spawnId);
            continue;
        }

        SpawnGroupTemplateData& groupTemplate = it->second;
        if (groupTemplate.mapId == SPAWNGROUP_MAP_UNSET)
            groupTemplate.mapId = data->mapid;
        else if (groupTemplate.mapId != data->mapid && !(groupTemplate.flags & SPAWNGROUP_FLAG_SYSTEM))
        {
            LOG_ERROR("sql.sql", "Spawn group {} has map ID {}, but spawn ({},{}) has map id {} - spawn NOT added to group!",
                groupId, groupTemplate.mapId, uint32(spawnType), spawnId, data->mapid);
            continue;
        }

        // Warn if spawn is also in a pool (non-system groups and pools are mutually exclusive)
        if (!(groupTemplate.flags & SPAWNGROUP_FLAG_SYSTEM))
        {
            uint32 poolId = 0;
            if (spawnType == SPAWN_TYPE_CREATURE)
                poolId = sPoolMgr->IsPartOfAPool<Creature>(spawnId);
            else if (spawnType == SPAWN_TYPE_GAMEOBJECT)
                poolId = sPoolMgr->IsPartOfAPool<GameObject>(spawnId);

            if (poolId)
                LOG_WARN("sql.sql", "Spawn ({},{}) is a member of spawn group {} and also part of pool {}. This may cause issues!",
                    uint32(spawnType), spawnId, groupId, poolId);
        }

        const_cast<SpawnData*>(data)->spawnGroupId = groupId;
        if (!(groupTemplate.flags & SPAWNGROUP_FLAG_SYSTEM))
            _spawnGroupMapStore.emplace(groupId, data);
        ++numMembers;
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} spawn group members in {} ms", numMembers, GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

void ObjectMgr::OnDeleteSpawnData(SpawnData const* data)
{
    auto templateIt = _spawnGroupDataStore.find(data->spawnGroupId);
    ASSERT(templateIt != _spawnGroupDataStore.end(), "Spawn data is being deleted and has invalid spawn group index {}!", data->spawnGroupId);
    if (templateIt->second.flags & SPAWNGROUP_FLAG_SYSTEM)
        return;

    auto pair = _spawnGroupMapStore.equal_range(data->spawnGroupId);
    for (auto it = pair.first; it != pair.second; ++it)
    {
        if (it->second != data)
            continue;
        _spawnGroupMapStore.erase(it);
        return;
    }
    ASSERT(false, "Spawn data being removed is member of spawn group {}, but not found in lookup table!", data->spawnGroupId);
}

void ObjectMgr::LoadQuestRelationsHelper(QuestRelations& map, std::string const& table, bool starter, bool go)
{
    uint32 oldMSTime = getMSTime();

    map.clear();                                            // need for reload case

    uint32 count = 0;

    QueryResult result = WorldDatabase.Query("SELECT id, quest, pool_entry FROM {} qr LEFT JOIN pool_quest pq ON qr.quest = pq.entry", table);

    if (!result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 quest relations from `{}`, table is empty.", table);
        LOG_INFO("server.loading", " ");
        return;
    }

    PooledQuestRelation* poolRelationMap = go ? &sPoolMgr->mQuestGORelation : &sPoolMgr->mQuestCreatureRelation;
    if (starter)
        poolRelationMap->clear();

    do
    {
        uint32 id     = result->Fetch()[0].Get<uint32>();
        uint32 quest  = result->Fetch()[1].Get<uint32>();
        uint32 poolId = result->Fetch()[2].Get<uint32>();

        if (_questTemplates.find(quest) == _questTemplates.end())
        {
            LOG_ERROR("sql.sql", "Table `{}`: Quest {} listed for entry {} does not exist.", table, quest, id);
            continue;
        }

        if (!poolId || !starter)
            map.insert(QuestRelations::value_type(id, quest));
        else if (starter)
            poolRelationMap->insert(PooledQuestRelation::value_type(quest, id));

        ++count;
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} Quest Relations From {} in {} ms", count, table, GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

void ObjectMgr::LoadGameobjectQuestStarters()
{
    LoadQuestRelationsHelper(_goQuestRelations, "gameobject_queststarter", true, true);

    for (QuestRelations::iterator itr = _goQuestRelations.begin(); itr != _goQuestRelations.end(); ++itr)
    {
        GameObjectTemplate const* goInfo = GetGameObjectTemplate(itr->first);
        if (!goInfo)
            LOG_ERROR("sql.sql", "Table `gameobject_queststarter` have data for not existed gameobject entry ({}) and existed quest {}", itr->first, itr->second);
        else if (goInfo->type != GAMEOBJECT_TYPE_QUESTGIVER)
            LOG_ERROR("sql.sql", "Table `gameobject_queststarter` have data gameobject entry ({}) for quest {}, but GO is not GAMEOBJECT_TYPE_QUESTGIVER", itr->first, itr->second);
    }
}

void ObjectMgr::LoadGameobjectQuestEnders()
{
    LoadQuestRelationsHelper(_goQuestInvolvedRelations, "gameobject_questender", false, true);

    for (QuestRelations::iterator itr = _goQuestInvolvedRelations.begin(); itr != _goQuestInvolvedRelations.end(); ++itr)
    {
        GameObjectTemplate const* goInfo = GetGameObjectTemplate(itr->first);
        if (!goInfo)
            LOG_ERROR("sql.sql", "Table `gameobject_questender` have data for not existed gameobject entry ({}) and existed quest {}", itr->first, itr->second);
        else if (goInfo->type != GAMEOBJECT_TYPE_QUESTGIVER)
            LOG_ERROR("sql.sql", "Table `gameobject_questender` have data gameobject entry ({}) for quest {}, but GO is not GAMEOBJECT_TYPE_QUESTGIVER", itr->first, itr->second);
    }
}

void ObjectMgr::LoadCreatureQuestStarters()
{
    LoadQuestRelationsHelper(_creatureQuestRelations, "creature_queststarter", true, false);

    for (QuestRelations::iterator itr = _creatureQuestRelations.begin(); itr != _creatureQuestRelations.end(); ++itr)
    {
        CreatureTemplate const* cInfo = GetCreatureTemplate(itr->first);
        if (!cInfo)
            LOG_ERROR("sql.sql", "Table `creature_queststarter` have data for not existed creature entry ({}) and existed quest {}", itr->first, itr->second);
        else if (!(cInfo->npcflag & UNIT_NPC_FLAG_QUESTGIVER))
            LOG_ERROR("sql.sql", "Table `creature_queststarter` has creature entry ({}) for quest {}, but npcflag does not include UNIT_NPC_FLAG_QUESTGIVER", itr->first, itr->second);
    }
}

void ObjectMgr::LoadCreatureQuestEnders()
{
    LoadQuestRelationsHelper(_creatureQuestInvolvedRelations, "creature_questender", false, false);

    for (QuestRelations::iterator itr = _creatureQuestInvolvedRelations.begin(); itr != _creatureQuestInvolvedRelations.end(); ++itr)
    {
        CreatureTemplate const* cInfo = GetCreatureTemplate(itr->first);
        if (!cInfo)
            LOG_ERROR("sql.sql", "Table `creature_questender` have data for not existed creature entry ({}) and existed quest {}", itr->first, itr->second);
        else if (!(cInfo->npcflag & UNIT_NPC_FLAG_QUESTGIVER))
            LOG_ERROR("sql.sql", "Table `creature_questender` has creature entry ({}) for quest {}, but npcflag does not include UNIT_NPC_FLAG_QUESTGIVER", itr->first, itr->second);
    }
}

void ObjectMgr::LoadReservedPlayerNamesDB()
{
    uint32 oldMSTime = getMSTime();

    _reservedNamesStore.clear();                                // need for reload case

    QueryResult result = CharacterDatabase.Query("SELECT name FROM reserved_name");

    if (!result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 reserved names. DB table `reserved_name` is empty!");
        return;
    }

    uint32 count = 0;

    Field* fields;
    do
    {
        fields = result->Fetch();
        std::string name = fields[0].Get<std::string>();

        std::wstring wstr;
        if (!Utf8toWStr (name, wstr))
        {
            LOG_ERROR("sql.sql", "Table `reserved_name` have invalid name: {}", name);
            continue;
        }

        wstrToLower(wstr);

        _reservedNamesStore.insert(wstr);
        ++count;
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} reserved names from DB in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
}

void ObjectMgr::LoadReservedPlayerNamesDBC()
{
    if (!sWorld->getBoolConfig(CONFIG_STRICT_NAMES_RESERVED))
    {
        LOG_WARN("server.loading", ">> Loaded 0 reserved names from DBC. Config option disabled.");
        return;
    }

    uint32 oldMSTime = getMSTime();

    uint32 count = 0;

    for (NamesReservedEntry const* reservedStore : sNamesReservedStore)
    {
        std::wstring wstr;

        Utf8toWStr(reservedStore->Name, wstr);

        // DBC does not have clean entries, remove the junk.
        boost::algorithm::replace_all(wstr, "\\<", "");
        boost::algorithm::replace_all(wstr, "\\>", "");

        _reservedNamesStore.insert(wstr);
        count++;
    }

    LOG_INFO("server.loading", ">> Loaded {} reserved names from DBC in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

bool ObjectMgr::IsReservedName(std::string_view name) const
{
    // pussywizard
    if (name.size() >= 2 && (name[name.size() - 2] == 'G' || name[name.size() - 2] == 'g') && (name[name.size() - 1] == 'M' || name[name.size() - 1] == 'm'))
        return true;

    std::wstring wstr;
    if (!Utf8toWStr (name, wstr))
        return false;

    wstrToLower(wstr);

    return _reservedNamesStore.find(wstr) != _reservedNamesStore.end();
}

void ObjectMgr::AddReservedPlayerName(std::string const& name)
{
    if (!IsReservedName(name))
    {
        std::wstring wstr;
        if (!Utf8toWStr(name, wstr))
        {
            LOG_ERROR("server", "Could not add invalid name to reserved player names: {}", name);
            return;
        }
        wstrToLower(wstr);

        _reservedNamesStore.insert(wstr);

        CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_INS_RESERVED_PLAYER_NAME);
        stmt->SetData(0, name);
        CharacterDatabase.Execute(stmt);
    }
}

void ObjectMgr::LoadProfanityNamesFromDB()
{
    uint32 oldMSTime = getMSTime();

    _profanityNamesStore.clear();                                // need for reload case

    QueryResult result = CharacterDatabase.Query("SELECT name FROM profanity_name");

    if (!result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 profanity names. DB table `profanity_name` is empty!");
        return;
    }

    uint32 count = 0;

    Field* fields;
    do
    {
        fields = result->Fetch();
        std::string name = fields[0].Get<std::string>();

        std::wstring wstr;
        if (!Utf8toWStr (name, wstr))
        {
            LOG_ERROR("sql.sql", "Table `profanity_name` have invalid name: {}", name);
            continue;
        }

        wstrToLower(wstr);

        _profanityNamesStore.insert(wstr);
        ++count;
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} profanity names from DB in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
}

void ObjectMgr::LoadProfanityNamesFromDBC()
{
    if (!sWorld->getBoolConfig(CONFIG_STRICT_NAMES_PROFANITY))
    {
        LOG_WARN("server.loading", ">> Loaded 0 profanity names from DBC. Config option disabled.");
        return;
    }

    uint32 oldMSTime = getMSTime();

    uint32 count = 0;

    for (NamesProfanityEntry const* profanityStore : sNamesProfanityStore)
    {
        std::wstring wstr;

        Utf8toWStr(profanityStore->Name, wstr);

        // DBC does not have clean entries, remove the junk.
        boost::algorithm::replace_all(wstr, "\\<", "");
        boost::algorithm::replace_all(wstr, "\\>", "");

        _profanityNamesStore.insert(wstr);
        count++;
    }

    LOG_INFO("server.loading", ">> Loaded {} profanity names from DBC in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

bool ObjectMgr::IsProfanityName(std::string_view name) const
{
    // pussywizard
    if (name.size() >= 2 && (name[name.size() - 2] == 'G' || name[name.size() - 2] == 'g') && (name[name.size() - 1] == 'M' || name[name.size() - 1] == 'm'))
        return true;

    std::wstring wstr;
    if (!Utf8toWStr (name, wstr))
        return false;

    wstrToLower(wstr);

    return _profanityNamesStore.find(wstr) != _profanityNamesStore.end();
}

void ObjectMgr::AddProfanityPlayerName(std::string const& name)
{
    if (!IsProfanityName(name))
    {
        std::wstring wstr;
        if (!Utf8toWStr(name, wstr))
        {
            LOG_ERROR("server", "Could not add invalid name to profanity player names: {}", name);
            return;
        }
        wstrToLower(wstr);

        _profanityNamesStore.insert(wstr);

        CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_INS_PROFANITY_PLAYER_NAME);
        stmt->SetData(0, name);
        CharacterDatabase.Execute(stmt);
    }
}

void ObjectMgr::LoadChatFilter()
{
    uint32 oldMSTime = getMSTime();

    _chatFilterAutomaton.reset();                               // need for reload case

    CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHAT_FILTER);
    PreparedQueryResult result = CharacterDatabase.Query(stmt);

    if (!result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 chat filter words. DB table `chat_filter` is empty!");
        LOG_INFO("server.loading", " ");
        return;
    }

    auto automaton = std::make_unique<Acore::AhoCorasick<wchar_t>>();
    uint32 count = 0;

    do
    {
        Field* fields = result->Fetch();
        std::string word = fields[1].Get<std::string>();

        if (word.empty())
            continue;

        std::wstring wstr;
        if (!Utf8toWStr(word, wstr))
        {
            LOG_ERROR("sql.sql", "Table `chat_filter` has invalid word: {}", word);
            continue;
        }

        wstrToLower(wstr);

        if (wstr.empty())
            continue;

        automaton->Insert(wstr);
        ++count;
    } while (result->NextRow());

    if (count > 0)
    {
        automaton->Build();
        _chatFilterAutomaton = std::move(automaton);
    }

    LOG_INFO("server.loading", ">> Loaded {} chat filter words in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

bool ObjectMgr::IsChatFiltered(std::string_view text) const
{
    if (!_chatFilterAutomaton || text.empty())
        return false;

    std::wstring wtext;
    if (!Utf8toWStr(text, wtext))
        return false;

    wstrToLower(wtext);

    return _chatFilterAutomaton->ContainsAny(wtext);
}

enum LanguageType
{
    LT_BASIC_LATIN    = 0x0000,
    LT_EXTENDEN_LATIN = 0x0001,
    LT_CYRILLIC       = 0x0002,
    LT_EAST_ASIA      = 0x0004,
    LT_ANY            = 0xFFFF
};

static LanguageType GetRealmLanguageType(bool create)
{
    switch (sWorld->getIntConfig(CONFIG_REALM_ZONE))
    {
        case REALM_ZONE_UNKNOWN:                            // any language
        case REALM_ZONE_DEVELOPMENT:
        case REALM_ZONE_TEST_SERVER:
        case REALM_ZONE_QA_SERVER:
            return LT_ANY;
        case REALM_ZONE_UNITED_STATES:                      // extended-Latin
        case REALM_ZONE_OCEANIC:
        case REALM_ZONE_LATIN_AMERICA:
        case REALM_ZONE_ENGLISH:
        case REALM_ZONE_GERMAN:
        case REALM_ZONE_FRENCH:
        case REALM_ZONE_SPANISH:
            return LT_EXTENDEN_LATIN;
        case REALM_ZONE_KOREA:                              // East-Asian
        case REALM_ZONE_TAIWAN:
        case REALM_ZONE_CHINA:
            return LT_EAST_ASIA;
        case REALM_ZONE_RUSSIAN:                            // Cyrillic
            return LT_CYRILLIC;
        default:
            return create ? LT_BASIC_LATIN : LT_ANY;        // basic-Latin at create, any at login
    }
}

bool isValidString(std::wstring wstr, uint32 strictMask, bool numericOrSpace, bool create = false)
{
    if (strictMask == 0)                                       // any language, ignore realm
    {
        if (isExtendedLatinString(wstr, numericOrSpace))
            return true;
        if (isCyrillicString(wstr, numericOrSpace))
            return true;
        if (isEastAsianString(wstr, numericOrSpace))
            return true;
        return false;
    }

    if (strictMask & 0x2)                                    // realm zone specific
    {
        LanguageType lt = GetRealmLanguageType(create);
        if (lt & LT_EXTENDEN_LATIN)
            if (isExtendedLatinString(wstr, numericOrSpace))
                return true;
        if (lt & LT_CYRILLIC)
            if (isCyrillicString(wstr, numericOrSpace))
                return true;
        if (lt & LT_EAST_ASIA)
            if (isEastAsianString(wstr, numericOrSpace))
                return true;
    }

    if (strictMask & 0x1)                                    // basic Latin
    {
        if (isBasicLatinString(wstr, numericOrSpace))
            return true;
    }

    return false;
}

uint8 ObjectMgr::CheckPlayerName(std::string_view name, bool create)
{
    std::wstring wname;

    // Check for invalid characters
    if (!Utf8toWStr(name, wname))
        return CHAR_NAME_INVALID_CHARACTER;

    // Check for too long name
    if (wname.size() > MAX_PLAYER_NAME)
        return CHAR_NAME_TOO_LONG;

    // Check for too short name
    uint32 minName = sWorld->getIntConfig(CONFIG_MIN_PLAYER_NAME);
    if (wname.size() < minName)
        return CHAR_NAME_TOO_SHORT;

    // Check for mixed languages
    uint32 strictMask = sWorld->getIntConfig(CONFIG_STRICT_PLAYER_NAMES);
    if (!isValidString(wname, strictMask, false, create))
        return CHAR_NAME_MIXED_LANGUAGES;

    // Check for three consecutive letters
    wstrToLower(wname);
    for (std::size_t i = 2; i < wname.size(); ++i)
        if (wname[i] == wname[i - 1] && wname[i] == wname[i - 2])
            return CHAR_NAME_THREE_CONSECUTIVE;

    // Check Reserved Name
    if (sObjectMgr->IsReservedName(name))
        return CHAR_NAME_RESERVED;

    // Check Profanity Name
    if (sObjectMgr->IsProfanityName(name))
        return CHAR_NAME_PROFANE;

    return CHAR_NAME_SUCCESS;
}

bool ObjectMgr::IsValidCharterName(std::string_view name)
{
    std::wstring wname;
    if (!Utf8toWStr(name, wname))
        return false;

    if (wname.size() > MAX_CHARTER_NAME)
        return false;

    uint32 minName = sWorld->getIntConfig(CONFIG_MIN_CHARTER_NAME);
    if (wname.size() < minName)
        return false;

    // Check Reserved Name
    if (sObjectMgr->IsReservedName(name))
        return false;

    // Check Profanity Name
    if (sObjectMgr->IsProfanityName(name))
        return false;

    uint32 strictMask = sWorld->getIntConfig(CONFIG_STRICT_CHARTER_NAMES);

    return isValidString(wname, strictMask, true);
}

bool ObjectMgr::IsValidChannelName(std::string const& name)
{
    std::wstring wname;
    if (!Utf8toWStr(name, wname))
        return false;

    if (wname.size() > MAX_CHANNEL_NAME)
        return false;

    uint32 strictMask = sWorld->getIntConfig(CONFIG_STRICT_CHANNEL_NAMES);

    return isValidString(wname, strictMask, true);
}

PetNameInvalidReason ObjectMgr::CheckPetName(std::string_view name)
{
    std::wstring wname;
    if (!Utf8toWStr(name, wname))
        return PET_NAME_INVALID;

    if (wname.size() > MAX_PET_NAME)
        return PET_NAME_TOO_LONG;

    uint32 minName = sWorld->getIntConfig(CONFIG_MIN_PET_NAME);
    if (wname.size() < minName)
        return PET_NAME_TOO_SHORT;

    uint32 strictMask = sWorld->getIntConfig(CONFIG_STRICT_PET_NAMES);
    if (!isValidString(wname, strictMask, false))
        return PET_NAME_MIXED_LANGUAGES;

    // Check Reserved Name
    if (sObjectMgr->IsReservedName(name))
        return PET_NAME_RESERVED;

    // Check Profanity Name
    if (sObjectMgr->IsProfanityName(name))
        return PET_NAME_PROFANE;

    return PET_NAME_SUCCESS;
}

void ObjectMgr::LoadGameObjectForQuests()
{
    uint32 oldMSTime = getMSTime();

    if (GetGameObjectTemplates()->empty())
    {
        LOG_WARN("server.loading", ">> Loaded 0 GameObjects for quests");
        LOG_INFO("server.loading", " ");
        return;
    }

    uint32 count = 0;

    // collect GO entries for GO that must activated
    GameObjectTemplateContainer* gotc = const_cast<GameObjectTemplateContainer*>(GetGameObjectTemplates());
    for (GameObjectTemplateContainer::iterator itr = gotc->begin(); itr != gotc->end(); ++itr)
    {
        itr->second.IsForQuests = false;
        switch (itr->second.type)
        {
            case GAMEOBJECT_TYPE_QUESTGIVER:
                itr->second.IsForQuests = true;
                ++count;
                break;
            case GAMEOBJECT_TYPE_CHEST:
                {
                    // scan GO chest with loot including quest items
                    uint32 loot_id = (itr->second.GetLootId());

                    // find quest loot for GO
                    if (itr->second.chest.questId || LootTemplates_Gameobject.HaveQuestLootFor(loot_id))
                    {
                        itr->second.IsForQuests = true;
                        ++count;
                    }
                    break;
                }
            case GAMEOBJECT_TYPE_GENERIC:
                {
                    if (itr->second._generic.questID > 0)            //quests objects
                    {
                        itr->second.IsForQuests = true;
                        ++count;
                    }
                    break;
                }
            case GAMEOBJECT_TYPE_SPELL_FOCUS:
                {
                    if (itr->second.spellFocus.questID > 0)          //quests objects
                    {
                        itr->second.IsForQuests = true;
                        ++count;
                    }
                    break;
                }
            case GAMEOBJECT_TYPE_GOOBER:
                {
                    if (itr->second.goober.questId > 0)              //quests objects
                    {
                        itr->second.IsForQuests = true;
                        ++count;
                    }
                    break;
                }
            default:
                break;
        }
    }

    LOG_INFO("server.loading", ">> Loaded {} GameObjects for quests in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

bool ObjectMgr::LoadModuleStrings()
{
    uint32 oldMSTime = getMSTime();

    _moduleStringStore.clear(); // for reload case
    QueryResult result = WorldDatabase.Query("SELECT module, id, string FROM module_string");
    if (!result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 module strings. DB table `module_string` is empty.");
        LOG_INFO("server.loading", " ");
        return false;
    }

    do
    {
        Field* fields = result->Fetch();

        std::string module = fields[0].Get<std::string>();
        uint32 id          = fields[1].Get<uint32>();

        std::pair<std::string, uint32> pairKey = std::make_pair(module, id);
        ModuleString& data = _moduleStringStore[pairKey];

        AddLocaleString(fields[2].Get<std::string>(), LOCALE_enUS, data.Content);
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} Module Strings in {} ms", _moduleStringStore.size(), GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");

    return true;
}

bool ObjectMgr::LoadModuleStringsLocale()
{
    uint32 oldMSTime = getMSTime();

    QueryResult result = WorldDatabase.Query("SELECT module, id, locale, string FROM module_string_locale");
    if (!result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 module strings locale. DB table `module_string_locale` is empty.");
        LOG_INFO("server.loading", " ");
        return false;
    }

    uint32 localeCount = 0;
    do
    {
        Field* fields = result->Fetch();

        std::string module = fields[0].Get<std::string>();
        uint32 id          = fields[1].Get<uint32>();

        std::pair<std::string, uint32> pairKey = std::make_pair(module, id);
        ModuleString& data = _moduleStringStore[pairKey];

        ModuleStringContainer::iterator ms = _moduleStringStore.find(pairKey);
        if (ms == _moduleStringStore.end())
        {
            LOG_ERROR("sql.sql", "ModuleString (Module: {} Id: {}) found in table `module_string_locale` but does not exist in `module_string`. Skipped!", module, id);
            continue;
        }

        LocaleConstant locale = GetLocaleByName(fields[2].Get<std::string>());
        if (locale == LOCALE_enUS)
            continue;

        AddLocaleString(fields[3].Get<std::string>(), locale, data.Content);
        localeCount++;
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} Module Strings Locales in {} ms", localeCount, GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");

    return true;
}

std::string const* ObjectMgr::GetModuleString(std::string module, uint32 id, LocaleConstant locale) const
{
    ModuleString const* ms = GetModuleString(module, id);
    if (ms && !ms->Content.empty())
    {
        if (ms->Content.size() > size_t(locale) && !ms->Content[locale].empty())
            return &ms->Content[locale];

        return &ms->Content[DEFAULT_LOCALE];
    }

    LOG_ERROR("sql.sql", "Module string module {} id {} not found in DB.", module, id);

    return (std::string*)"error";
}

bool ObjectMgr::LoadAcoreStrings()
{
    uint32 oldMSTime = getMSTime();

    _acoreStringStore.clear(); // for reload case

    //                                               0      1
    QueryResult result = WorldDatabase.Query("SELECT entry, content_default FROM acore_string");
    if (!result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 acore strings. DB table `acore_string` is empty.");
        LOG_INFO("server.loading", " ");
        return false;
    }

    do
    {
        Field* fields = result->Fetch();

        uint32 entry = fields[0].Get<uint32>();

        AcoreString& data = _acoreStringStore[entry];

        data.Content.resize(DEFAULT_LOCALE + 1);

        // content_default holds enUS
        AddLocaleString(fields[1].Get<std::string>(), DEFAULT_LOCALE, data.Content);
    } while (result->NextRow());

    // Texts in all other locales, including the ones registered by modules
    //                                     0      1       2
    result = WorldDatabase.Query("SELECT entry, locale, content FROM acore_string_locale");
    if (result)
    {
        do
        {
            Field* fields = result->Fetch();

            uint32 entry = fields[0].Get<uint32>();

            auto itr = _acoreStringStore.find(entry);
            if (itr == _acoreStringStore.end())
            {
                LOG_ERROR("sql.sql", "Table `acore_string_locale` has entry {} that is not in `acore_string`, skipped.",
                    entry);
                continue;
            }

            LocaleConstant locale = GetLocaleByName(fields[1].Get<std::string>());
            if (locale == LOCALE_enUS)
                continue;

            AddLocaleString(fields[2].Get<std::string>(), locale, itr->second.Content);
        } while (result->NextRow());
    }

    LOG_INFO("server.loading", ">> Loaded {} Acore Strings in {} ms", (uint32)_acoreStringStore.size(), GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");

    return true;
}

std::string ObjectMgr::GetAcoreString(uint32 entry, LocaleConstant locale) const
{
    AcoreString const* as = GetAcoreString(entry);
    if (as && !as->Content.empty())
    {
        if (as->Content.size() > std::size_t(locale) && !as->Content[locale].empty())
            return as->Content[locale];

        return as->Content[DEFAULT_LOCALE];
    }

    std::string msg = Acore::StringFormat("No entry for acore_string ({}) in DB.", entry);
    LOG_ERROR("sql.sql", msg);
    return msg;
}

void ObjectMgr::LoadFishingBaseSkillLevel()
{
    uint32 oldMSTime = getMSTime();

    _fishingBaseForAreaStore.clear();                            // for reload case

    QueryResult result = WorldDatabase.Query("SELECT entry, skill FROM skill_fishing_base_level");

    if (!result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 areas for fishing base skill level. DB table `skill_fishing_base_level` is empty.");
        LOG_INFO("server.loading", " ");
        return;
    }

    uint32 count = 0;

    do
    {
        Field* fields = result->Fetch();
        uint32 entry  = fields[0].Get<uint32>();
        int32 skill   = fields[1].Get<int16>();

        AreaTableEntry const* fArea = sAreaTableStore.LookupEntry(entry);
        if (!fArea)
        {
            LOG_ERROR("sql.sql", "AreaId {} defined in `skill_fishing_base_level` does not exist", entry);
            continue;
        }

        _fishingBaseForAreaStore[entry] = skill;
        ++count;
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} areas for fishing base skill level in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

void ObjectMgr::ChangeFishingBaseSkillLevel(uint32 entry, int32 skill)
{
    AreaTableEntry const* fArea = sAreaTableStore.LookupEntry(entry);
    if (!fArea)
    {
        LOG_ERROR("sql.sql", "AreaId {} defined in `skill_fishing_base_level` does not exist", entry);
        return;
    }

    _fishingBaseForAreaStore[entry] = skill;

    LOG_INFO("server.loading", ">> Fishing base skill level of area {} changed to {}", entry, skill);
    LOG_INFO("server.loading", " ");
}

bool ObjectMgr::CheckDeclinedNames(std::wstring w_ownname, DeclinedName const& names)
{
    // get main part of the name
    std::wstring mainpart = GetMainPartOfName(w_ownname, 0);
    // prepare flags
    bool x = true;
    bool y = true;

    // check declined names
    for (uint8 i = 0; i < MAX_DECLINED_NAME_CASES; ++i)
    {
        std::wstring wname;
        if (!Utf8toWStr(names.name[i], wname))
            return false;

        if (mainpart != GetMainPartOfName(wname, i + 1))
            x = false;

        if (w_ownname != wname)
            y = false;
    }
    return (x || y);
}

uint32 ObjectMgr::GetAreaTriggerScriptId(uint32 trigger_id)
{
    AreaTriggerScriptContainer::const_iterator i = _areaTriggerScriptStore.find(trigger_id);
    if (i != _areaTriggerScriptStore.end())
        return i->second;
    return 0;
}

SpellScriptsBounds ObjectMgr::GetSpellScriptsBounds(uint32 spell_id)
{
    return SpellScriptsBounds(_spellScriptsStore.lower_bound(spell_id), _spellScriptsStore.upper_bound(spell_id));
}

// this allows calculating base reputations to offline players, just by race and class
int32 ObjectMgr::GetBaseReputationOf(FactionEntry const* factionEntry, uint8 race, uint8 playerClass)
{
    if (!factionEntry)
        return 0;

    uint32 raceMask = (1 << (race - 1));
    uint32 classMask = (1 << (playerClass - 1));

    for (int i = 0; i < 4; i++)
    {
        if ((!factionEntry->ReputationClassMask[i] ||
                factionEntry->ReputationClassMask[i] & classMask) &&
                (!factionEntry->ReputationRaceMask[i] ||
                 factionEntry->ReputationRaceMask[i] & raceMask))
            return factionEntry->ReputationBase[i];
    }

    return 0;
}

void ObjectMgr::LoadSkillTiers()
{
    uint32 oldMSTime = getMSTime();

    _skillTiers.clear();

    PreparedQueryResult result = WorldDatabase.Query(WorldDatabase.GetPreparedStatement(WORLD_SEL_SKILL_TIERS));
    if (!result)
    {
        LOG_ERROR("server.loading", ">> Loaded 0 skill max values. DB table `skill_tiers` is empty.");
        return;
    }

    do
    {
        Field* fields = result->Fetch();
        uint32 id = fields[0].Get<uint32>();
        SkillTiersEntry& tier = _skillTiers[id];
        tier.ID = id;
        for (uint32 i = 0; i < MAX_SKILL_STEP; ++i)
            tier.Value[i] = fields[1 + i].Get<uint32>();
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} skill max values in {} ms", uint32(_skillTiers.size()), GetMSTimeDiffToNow(oldMSTime));
}

SkillRangeType GetSkillRangeType(SkillRaceClassInfoEntry const* rcEntry)
{
    SkillLineEntry const* skill = sSkillLineStore.LookupEntry(rcEntry->SkillID);
    if (!skill)
    {
        return SKILL_RANGE_NONE;
    }

    if (sObjectMgr->GetSkillTier(rcEntry->SkillTierID))
    {
        return SKILL_RANGE_RANK;
    }

    if (rcEntry->SkillID == SKILL_RUNEFORGING)
    {
        return SKILL_RANGE_MONO;
    }

    switch (skill->CategoryID)
    {
        case SKILL_CATEGORY_ARMOR:
            return SKILL_RANGE_MONO;
        case SKILL_CATEGORY_LANGUAGES:
            return SKILL_RANGE_LANGUAGE;
    }

    return SKILL_RANGE_LEVEL;
}

void ObjectMgr::LoadGameTele()
{
    uint32 oldMSTime = getMSTime();

    _gameTeleStore.clear();                                  // for reload case

    //                                                0       1           2           3           4        5     6
    QueryResult result = WorldDatabase.Query("SELECT id, position_x, position_y, position_z, orientation, map, name FROM game_tele");

    if (!result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 GameTeleports. DB table `game_tele` is empty!");
        LOG_INFO("server.loading", " ");
        return;
    }

    uint32 count = 0;

    do
    {
        Field* fields = result->Fetch();

        uint32 id         = fields[0].Get<uint32>();

        GameTele gt;

        gt.position_x     = fields[1].Get<float>();
        gt.position_y     = fields[2].Get<float>();
        gt.position_z     = fields[3].Get<float>();
        gt.orientation    = fields[4].Get<float>();
        gt.mapId          = fields[5].Get<uint16>();
        gt.name           = fields[6].Get<std::string>();

        if (!MapMgr::IsValidMapCoord(gt.mapId, gt.position_x, gt.position_y, gt.position_z, gt.orientation))
        {
            LOG_ERROR("sql.sql", "Wrong position for id {} (name: {}) in `game_tele` table, ignoring.", id, gt.name);
            continue;
        }

        if (!Utf8toWStr(gt.name, gt.wnameLow))
        {
            LOG_ERROR("sql.sql", "Wrong UTF8 name for id {} in `game_tele` table, ignoring.", id);
            continue;
        }

        wstrToLower(gt.wnameLow);

        _gameTeleStore[id] = gt;

        ++count;
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} GameTeleports in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

GameTele const* ObjectMgr::GetGameTele(std::string_view name, bool exactSearch) const
{
    // explicit name case
    std::wstring wname;
    if (!Utf8toWStr(name, wname))
        return nullptr;

    // converting string that we try to find to lower case
    wstrToLower(wname);

    // Alternative first GameTele what contains wnameLow as substring in case no GameTele location found
    GameTele const* alt = nullptr;
    for (GameTeleContainer::const_iterator itr = _gameTeleStore.begin(); itr != _gameTeleStore.end(); ++itr)
    {
        if (itr->second.wnameLow == wname)
            return &itr->second;
        else if (!exactSearch && !alt && itr->second.wnameLow.find(wname) != std::wstring::npos)
            alt = &itr->second;
    }

    return alt;
}

bool ObjectMgr::AddGameTele(GameTele& tele)
{
    // find max id
    uint32 new_id = 0;
    for (GameTeleContainer::const_iterator itr = _gameTeleStore.begin(); itr != _gameTeleStore.end(); ++itr)
        if (itr->first > new_id)
            new_id = itr->first;

    // use next
    ++new_id;

    if (!Utf8toWStr(tele.name, tele.wnameLow))
        return false;

    wstrToLower(tele.wnameLow);

    _gameTeleStore[new_id] = tele;

    WorldDatabasePreparedStatement* stmt = WorldDatabase.GetPreparedStatement(WORLD_INS_GAME_TELE);

    stmt->SetData(0, new_id);
    stmt->SetData(1, tele.position_x);
    stmt->SetData(2, tele.position_y);
    stmt->SetData(3, tele.position_z);
    stmt->SetData(4, tele.orientation);
    stmt->SetData(5, uint16(tele.mapId));
    stmt->SetData(6, tele.name);

    WorldDatabase.Execute(stmt);

    return true;
}

bool ObjectMgr::DeleteGameTele(std::string_view name)
{
    // explicit name case
    std::wstring wname;
    if (!Utf8toWStr(name, wname))
        return false;

    // converting string that we try to find to lower case
    wstrToLower(wname);

    for (GameTeleContainer::iterator itr = _gameTeleStore.begin(); itr != _gameTeleStore.end(); ++itr)
    {
        if (itr->second.wnameLow == wname)
        {
            WorldDatabasePreparedStatement* stmt = WorldDatabase.GetPreparedStatement(WORLD_DEL_GAME_TELE);

            stmt->SetData(0, itr->second.name);

            WorldDatabase.Execute(stmt);

            _gameTeleStore.erase(itr);
            return true;
        }
    }

    return false;
}

void ObjectMgr::LoadMailLevelRewards()
{
    uint32 oldMSTime = getMSTime();

    _mailLevelRewardStore.clear();                           // for reload case

    //                                                 0        1             2            3
    QueryResult result = WorldDatabase.Query("SELECT level, raceMask, mailTemplateId, senderEntry FROM mail_level_reward");

    if (!result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 level dependent mail rewards. DB table `mail_level_reward` is empty.");
        LOG_INFO("server.loading", " ");
        return;
    }

    uint32 count = 0;

    do
    {
        Field* fields = result->Fetch();

        uint8 level           = fields[0].Get<uint8>();
        uint32 raceMask       = fields[1].Get<uint32>();
        uint32 mailTemplateId = fields[2].Get<uint32>();
        uint32 senderEntry    = fields[3].Get<uint32>();

        if (level > MAX_LEVEL)
        {
            LOG_ERROR("sql.sql", "Table `mail_level_reward` have data for level {} that more supported by client ({}), ignoring.", level, MAX_LEVEL);
            continue;
        }

        if (!(raceMask & sRaceMgr->GetPlayableRaceMask()))
        {
            LOG_ERROR("sql.sql", "Table `mail_level_reward` have raceMask ({}) for level {} that not include any player races, ignoring.", raceMask, level);
            continue;
        }

        if (!sMailTemplateStore.LookupEntry(mailTemplateId))
        {
            LOG_ERROR("sql.sql", "Table `mail_level_reward` have invalid mailTemplateId ({}) for level {} that invalid not include any player races, ignoring.", mailTemplateId, level);
            continue;
        }

        if (!GetCreatureTemplate(senderEntry))
        {
            LOG_ERROR("sql.sql", "Table `mail_level_reward` have not existed sender creature entry ({}) for level {} that invalid not include any player races, ignoring.", senderEntry, level);
            continue;
        }

        _mailLevelRewardStore[level].push_back(MailLevelReward(raceMask, mailTemplateId, senderEntry));

        ++count;
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} Level Dependent Mail Rewards in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

void ObjectMgr::LoadTrainers()
{
    uint32 oldMSTime = getMSTime();

    // For reload case
    _trainers.clear();
    _classTrainers.clear();

    std::unordered_map<int32, std::vector<Trainer::Spell>> spellsByTrainer;
    if (QueryResult trainerSpellsResult = WorldDatabase.Query("SELECT TrainerId, SpellId, MoneyCost, ReqSkillLine, ReqSkillRank, ReqAbility1, ReqAbility2, ReqAbility3, ReqLevel FROM trainer_spell"))
    {
        do
        {
            Field* fields = trainerSpellsResult->Fetch();

            Trainer::Spell spell;
            uint32 trainerId = fields[0].Get<uint32>();
            spell.SpellId = fields[1].Get<uint32>();
            spell.MoneyCost = fields[2].Get<uint32>();
            spell.ReqSkillLine = fields[3].Get<uint32>();
            spell.ReqSkillRank = fields[4].Get<uint32>();
            spell.ReqAbility[0] = fields[5].Get<uint32>();
            spell.ReqAbility[1] = fields[6].Get<uint32>();
            spell.ReqAbility[2] = fields[7].Get<uint32>();
            spell.ReqLevel = fields[8].Get<uint8>();

            SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spell.SpellId);
            if (!spellInfo)
            {
                LOG_ERROR("sql.sql", "Table `trainer_spell` references non-existing spell (SpellId: {}) for TrainerId {}, ignoring", spell.SpellId, trainerId);
                continue;
            }

            if (GetTalentSpellCost(spell.SpellId))
            {
                LOG_ERROR("sql.sql", "Table `trainer_spell` references non-existing spell (SpellId: {}) which is a talent, for TrainerId {}, ignoring", spell.SpellId, trainerId);
                continue;
            }

            if (spell.ReqSkillLine && !sSkillLineStore.LookupEntry(spell.ReqSkillLine))
            {
                LOG_ERROR("sql.sql", "Table `trainer_spell` references non-existing skill (ReqSkillLine: {}) for TrainerId {} and SpellId {}, ignoring",
                    spell.ReqSkillLine, spell.SpellId, trainerId);
                continue;
            }

            bool allReqValid = true;
            for (std::size_t i = 0; i < spell.ReqAbility.size(); ++i)
            {
                uint32 requiredSpell = spell.ReqAbility[i];
                if (requiredSpell && !sSpellMgr->GetSpellInfo(requiredSpell))
                {
                    LOG_ERROR("sql.sql", "Table `trainer_spell` references non-existing spell (ReqAbility {} : {}) for TrainerId {} and SpellId {}, ignoring",
                        i + 1, requiredSpell, trainerId, spell.SpellId);
                    allReqValid = false;
                }
            }

            if (!allReqValid)
                continue;

            spellsByTrainer[trainerId].push_back(spell);
        } while (trainerSpellsResult->NextRow());
    }

    if (QueryResult trainersResult = WorldDatabase.Query("SELECT Id, Type, Requirement, Greeting FROM trainer"))
    {
        do
        {
            Field* fields = trainersResult->Fetch();

            uint32 trainerId = fields[0].Get<uint32>();
            Trainer::Type trainerType = Trainer::Type(fields[1].Get<uint8>());
            uint32 requirement = fields[2].Get<uint32>();
            std::string greeting = fields[3].Get<std::string>();
            std::vector<Trainer::Spell> spells;
            auto spellsItr = spellsByTrainer.find(trainerId);
            if (spellsItr != spellsByTrainer.end())
            {
                spells = std::move(spellsItr->second);
                spellsByTrainer.erase(spellsItr);
            }

            auto [it, isNew] = _trainers.emplace(std::piecewise_construct, std::forward_as_tuple(trainerId), std::forward_as_tuple(trainerId, trainerType, requirement, std::move(greeting), std::move(spells)));
            ASSERT(isNew);
            if (trainerType == Trainer::Type::Class)
            {
                if (!requirement || requirement >= MAX_CLASSES)
                    LOG_ERROR("sql.sql", "Table `trainer` has invalid class requirement for trainer {}, ignoring");
                else
                {
                    uint8 classId = static_cast<uint8>(requirement);
                    _classTrainers[classId].push_back(&it->second);
                }
            }
        } while (trainersResult->NextRow());
    }

    for (auto const& unusedSpells : spellsByTrainer)
    {
        for (Trainer::Spell const& unusedSpell : unusedSpells.second)
        {
            LOG_ERROR("sql.sql", "Table `trainer_spell` references non-existing trainer (TrainerId: {}) for SpellId {}, ignoring", unusedSpells.first, unusedSpell.SpellId);
        }
    }

    if (QueryResult trainerLocalesResult = WorldDatabase.Query("SELECT Id, locale, Greeting_lang FROM trainer_locale"))
    {
        do
        {
            Field* fields = trainerLocalesResult->Fetch();
            uint32 trainerId = fields[0].Get<uint32>();
            std::string localeName = fields[1].Get<std::string>();

            LocaleConstant locale = GetLocaleByName(localeName);
            if (locale == LOCALE_enUS)
                continue;

            if (Trainer::Trainer* trainer = Acore::Containers::MapGetValuePtr(_trainers, trainerId))
                trainer->AddGreetingLocale(locale, fields[2].Get<std::string>());
            else
                LOG_ERROR("sql.sql", "Table `trainer_locale` references non-existing trainer (TrainerId: {}) for locale %s, ignoring",
                    trainerId, localeName.c_str());
        } while (trainerLocalesResult->NextRow());
    }

    LOG_INFO("server.loading", ">> Loaded {} Trainers in {} ms", _trainers.size(), GetMSTimeDiffToNow(oldMSTime));
}

void ObjectMgr::LoadCreatureDefaultTrainers()
{
    uint32 oldMSTime = getMSTime();

    _creatureDefaultTrainers.clear();

    if (QueryResult result = WorldDatabase.Query("SELECT CreatureId, TrainerId FROM creature_default_trainer"))
    {
        do
        {
            Field* fields = result->Fetch();
            uint32 creatureId = fields[0].Get<uint32>();
            uint32 trainerId = fields[1].Get<uint32>();

            if (!GetCreatureTemplate(creatureId))
            {
                LOG_ERROR("sql.sql", "Table `creature_default_trainer` references non-existing creature template (CreatureId: %u), ignoring", creatureId);
                continue;
            }

            _creatureDefaultTrainers[creatureId] = trainerId;

        } while (result->NextRow());
    }

    LOG_INFO("server.loading", ">> Loaded {} default trainers in {} ms", _creatureDefaultTrainers.size(), GetMSTimeDiffToNow(oldMSTime));
}

Trainer::Trainer* ObjectMgr::GetTrainer(uint32 creatureId)
{
    auto itr = _creatureDefaultTrainers.find(creatureId);
    if (itr != _creatureDefaultTrainers.end())
        return Acore::Containers::MapGetValuePtr(_trainers, itr->second);

    return nullptr;
}

int ObjectMgr::LoadReferenceVendor(int32 vendor, int32 item, std::set<uint32>* skip_vendors)
{
    // find all items from the reference vendor
    WorldDatabasePreparedStatement* stmt = WorldDatabase.GetPreparedStatement(WORLD_SEL_NPC_VENDOR_REF);
    stmt->SetData(0, uint32(item));
    PreparedQueryResult result = WorldDatabase.Query(stmt);

    if (!result)
        return 0;

    uint32 count = 0;
    do
    {
        Field* fields = result->Fetch();

        int32 item_id = fields[0].Get<int32>();

        // if item is a negative, its a reference
        if (item_id < 0)
            count += LoadReferenceVendor(vendor, -item_id, skip_vendors);
        else
        {
            uint32  maxcount    = fields[1].Get<uint32>();
            uint32 incrtime     = fields[2].Get<uint32>();
            uint32 ExtendedCost = fields[3].Get<uint32>();

            if (!IsVendorItemValid(vendor, item_id, maxcount, incrtime, ExtendedCost, nullptr, skip_vendors))
                continue;

            VendorItemData& vList = _cacheVendorItemStore[vendor];

            vList.AddItem(item_id, maxcount, incrtime, ExtendedCost);
            ++count;
        }
    } while (result->NextRow());

    return count;
}

void ObjectMgr::LoadVendors()
{
    uint32 oldMSTime = getMSTime();

    // For reload case
    for (CacheVendorItemContainer::iterator itr = _cacheVendorItemStore.begin(); itr != _cacheVendorItemStore.end(); ++itr)
        itr->second.Clear();
    _cacheVendorItemStore.clear();

    std::set<uint32> skip_vendors;

    QueryResult result = WorldDatabase.Query("SELECT entry, item, maxcount, incrtime, ExtendedCost FROM npc_vendor ORDER BY entry, slot ASC, item, ExtendedCost");
    if (!result)
    {
        LOG_INFO("server.loading", " ");
        LOG_WARN("server.loading", ">> Loaded 0 Vendors. DB table `npc_vendor` is empty!");
        return;
    }

    uint32 count = 0;

    do
    {
        Field* fields = result->Fetch();

        uint32 entry        = fields[0].Get<uint32>();
        int32 item_id      = fields[1].Get<int32>();

        // if item is a negative, its a reference
        if (item_id < 0)
            count += LoadReferenceVendor(entry, -item_id, &skip_vendors);
        else
        {
            uint32 maxcount     = fields[2].Get<uint32>();
            uint32 incrtime     = fields[3].Get<uint32>();
            uint32 ExtendedCost = fields[4].Get<uint32>();

            if (!IsVendorItemValid(entry, item_id, maxcount, incrtime, ExtendedCost, nullptr, &skip_vendors))
                continue;

            VendorItemData& vList = _cacheVendorItemStore[entry];

            vList.AddItem(item_id, maxcount, incrtime, ExtendedCost);
            ++count;
        }
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} Vendors in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

void ObjectMgr::LoadGossipMenu()
{
    uint32 oldMSTime = getMSTime();

    _gossipMenusStore.clear();

    QueryResult result = WorldDatabase.Query("SELECT MenuID, TextID FROM gossip_menu");

    if (!result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 gossip_menu entries. DB table `gossip_menu` is empty!");
        LOG_INFO("server.loading", " ");
        return;
    }

    do
    {
        Field* fields = result->Fetch();

        GossipMenus gMenu;

        gMenu.MenuID        = fields[0].Get<uint32>();
        gMenu.TextID        = fields[1].Get<uint32>();

        if (!GetGossipText(gMenu.TextID))
        {
            LOG_ERROR("sql.sql", "Table gossip_menu entry {} are using non-existing TextID {}", gMenu.MenuID, gMenu.TextID);
            continue;
        }

        _gossipMenusStore.insert(GossipMenusContainer::value_type(gMenu.MenuID, gMenu));
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} gossip_menu entries in {} ms", (uint32)_gossipMenusStore.size(), GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

void ObjectMgr::LoadGossipMenuItems()
{
    uint32 oldMSTime = getMSTime();

    _gossipMenuItemsStore.clear();

    QueryResult result = WorldDatabase.Query(
                             //      0       1         2           3           4                      5           6              7             8            9         10        11       12
                             "SELECT MenuID, OptionID, OptionIcon, OptionText, OptionBroadcastTextID, OptionType, OptionNpcFlag, ActionMenuID, ActionPoiID, BoxCoded, BoxMoney, BoxText, BoxBroadcastTextID "
                             "FROM gossip_menu_option ORDER BY MenuID, OptionID");

    if (!result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 gossip_menu_option IDs. DB table `gossip_menu_option` is empty!");
        LOG_INFO("server.loading", " ");
        return;
    }

    do
    {
        Field* fields = result->Fetch();

        GossipMenuItems gMenuItem;

        gMenuItem.MenuID                    = fields[0].Get<uint32>();
        gMenuItem.OptionID                  = fields[1].Get<uint16>();
        gMenuItem.OptionIcon                = fields[2].Get<uint32>();
        gMenuItem.OptionText                = fields[3].Get<std::string>();
        gMenuItem.OptionBroadcastTextID     = fields[4].Get<uint32>();
        gMenuItem.OptionType                = fields[5].Get<uint8>();
        gMenuItem.OptionNpcFlag             = fields[6].Get<uint32>();
        gMenuItem.ActionMenuID              = fields[7].Get<uint32>();
        gMenuItem.ActionPoiID               = fields[8].Get<uint32>();
        gMenuItem.BoxCoded                  = fields[9].Get<bool>();
        gMenuItem.BoxMoney                  = fields[10].Get<uint32>();
        gMenuItem.BoxText                   = fields[11].Get<std::string>();
        gMenuItem.BoxBroadcastTextID        = fields[12].Get<uint32>();

        if (gMenuItem.OptionIcon >= GOSSIP_ICON_MAX)
        {
            LOG_ERROR("sql.sql", "Table `gossip_menu_option` for menu {}, id {} has unknown icon id {}. Replacing with GOSSIP_ICON_CHAT", gMenuItem.MenuID, gMenuItem.OptionID, gMenuItem.OptionIcon);
            gMenuItem.OptionIcon = GOSSIP_ICON_CHAT;
        }

        if (gMenuItem.OptionBroadcastTextID && !GetBroadcastText(gMenuItem.OptionBroadcastTextID))
        {
            LOG_ERROR("sql.sql", "Table `gossip_menu_option` for menu {}, id {} has non-existing or incompatible OptionBroadcastTextID {}, ignoring.", gMenuItem.MenuID, gMenuItem.OptionID, gMenuItem.OptionBroadcastTextID);
            gMenuItem.OptionBroadcastTextID = 0;
        }

        if (gMenuItem.OptionType >= GOSSIP_OPTION_MAX)
            LOG_ERROR("sql.sql", "Table `gossip_menu_option` for menu {}, id {} has unknown option id {}. Option will not be used", gMenuItem.MenuID, gMenuItem.OptionID, gMenuItem.OptionType);

        if (gMenuItem.ActionPoiID && !GetPointOfInterest(gMenuItem.ActionPoiID))
        {
            LOG_ERROR("sql.sql", "Table `gossip_menu_option` for menu {}, id {} use non-existing ActionPoiID {}, ignoring", gMenuItem.MenuID, gMenuItem.OptionID, gMenuItem.ActionPoiID);
            gMenuItem.ActionPoiID = 0;
        }

        if (gMenuItem.BoxBroadcastTextID && !GetBroadcastText(gMenuItem.BoxBroadcastTextID))
        {
            LOG_ERROR("sql.sql", "Table `gossip_menu_option` for menu {}, id {} has non-existing or incompatible BoxBroadcastTextID {}, ignoring.", gMenuItem.MenuID, gMenuItem.OptionID, gMenuItem.BoxBroadcastTextID);
            gMenuItem.BoxBroadcastTextID = 0;
        }

        _gossipMenuItemsStore.insert(GossipMenuItemsContainer::value_type(gMenuItem.MenuID, gMenuItem));
    } while (result->NextRow());

    // Warn if any trainer creature templates reference a GossipMenuId that has no gossip_menu_option entries
    // This will cause the gossip menu to fallback to MenuID 0 at runtime which will display: "I wish to unlearn my talents."
    std::set<uint32> checkedMenuIds;
    for (auto const& [entry, tmpl] : _creatureTemplateStore)
    {
        uint32 menuId = tmpl.GossipMenuId;
        if (!menuId)
            continue;

        if (!(tmpl.npcflag & UNIT_NPC_FLAG_TRAINER))
            continue;

        if (checkedMenuIds.contains(menuId))
            continue;

        checkedMenuIds.insert(menuId);

        auto [first, second] = _gossipMenuItemsStore.equal_range(menuId);
        if (first == second)
            LOG_WARN("server.loading", "Trainer creature template references GossipMenuId {} has no `gossip_menu_option` entries. This will fallback to MenuID 0.", menuId);
    }

    LOG_INFO("server.loading", ">> Loaded {} gossip_menu_option entries in {} ms", uint32(_gossipMenuItemsStore.size()), GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

void ObjectMgr::AddVendorItem(uint32 entry, uint32 item, uint32 maxcount, uint32 incrtime, uint32 extendedCost, bool persist /*= true*/)
{
    VendorItemData& vList = _cacheVendorItemStore[entry];
    vList.AddItem(item, maxcount, incrtime, extendedCost);

    if (persist)
    {
        WorldDatabasePreparedStatement* stmt = WorldDatabase.GetPreparedStatement(WORLD_INS_NPC_VENDOR);

        stmt->SetData(0, entry);
        stmt->SetData(1, item);
        stmt->SetData(2, maxcount);
        stmt->SetData(3, incrtime);
        stmt->SetData(4, extendedCost);

        WorldDatabase.Execute(stmt);
    }
}

bool ObjectMgr::RemoveVendorItem(uint32 entry, uint32 item, bool persist /*= true*/)
{
    CacheVendorItemContainer::iterator  iter = _cacheVendorItemStore.find(entry);
    if (iter == _cacheVendorItemStore.end())
        return false;

    if (!iter->second.RemoveItem(item))
        return false;

    if (persist)
    {
        WorldDatabasePreparedStatement* stmt = WorldDatabase.GetPreparedStatement(WORLD_DEL_NPC_VENDOR);

        stmt->SetData(0, entry);
        stmt->SetData(1, item);

        WorldDatabase.Execute(stmt);
    }

    return true;
}

bool ObjectMgr::IsVendorItemValid(uint32 vendor_entry, uint32 item_id, uint32 maxcount, uint32 incrtime, uint32 ExtendedCost, Player* player, std::set<uint32>* /*skip_vendors*/, uint32 /*ORnpcflag*/) const
{
    /*
    CreatureTemplate const* cInfo = GetCreatureTemplate(vendor_entry);
    if (!cInfo)
    {
        if (player)
            ChatHandler(player->GetSession()).SendSysMessage(LANG_COMMAND_VENDORSELECTION);
        else
            LOG_ERROR("sql.sql", "Table `(game_event_)npc_vendor` have data for not existed creature template (Entry: {}), ignore", vendor_entry);
        return false;
    }

    if (!((cInfo->npcflag | ORnpcflag) & UNIT_NPC_FLAG_VENDOR))
    {
        if (!skip_vendors || skip_vendors->count(vendor_entry) == 0)
        {
            if (player)
                ChatHandler(player->GetSession()).SendSysMessage(LANG_COMMAND_VENDORSELECTION);
            else
                LOG_ERROR("sql.sql", "Table `(game_event_)npc_vendor` have data for not creature template (Entry: {}) without vendor flag, ignore", vendor_entry);

            if (skip_vendors)
                skip_vendors->insert(vendor_entry);
        }
        return false;
    }
    */

    if (!sObjectMgr->GetItemTemplate(item_id))
    {
        if (player)
            ChatHandler(player->GetSession()).PSendSysMessage(LANG_ITEM_NOT_FOUND, item_id);
        else
            LOG_ERROR("sql.sql", "Table `(game_event_)npc_vendor` for Vendor (Entry: {}) have in item list non-existed item ({}), ignore", vendor_entry, item_id);
        return false;
    }

    if (ExtendedCost && !sItemExtendedCostStore.LookupEntry(ExtendedCost))
    {
        if (player)
            ChatHandler(player->GetSession()).PSendSysMessage(LANG_EXTENDED_COST_NOT_EXIST, ExtendedCost);
        else
            LOG_ERROR("sql.sql", "Table `(game_event_)npc_vendor` have Item (Entry: {}) with wrong ExtendedCost ({}) for vendor ({}), ignore", item_id, ExtendedCost, vendor_entry);
        return false;
    }

    if (maxcount > 0 && incrtime == 0)
    {
        if (player)
            ChatHandler(player->GetSession()).PSendSysMessage("MaxCount != 0 ({}) but IncrTime == 0", maxcount);
        else
            LOG_ERROR("sql.sql", "Table `(game_event_)npc_vendor` has `maxcount` ({}) for item {} of vendor (Entry: {}) but `incrtime`=0, ignore", maxcount, item_id, vendor_entry);
        return false;
    }
    else if (maxcount == 0 && incrtime > 0)
    {
        if (player)
            ChatHandler(player->GetSession()).PSendSysMessage("MaxCount == 0 but IncrTime<>= 0");
        else
            LOG_ERROR("sql.sql", "Table `(game_event_)npc_vendor` has `maxcount`=0 for item {} of vendor (Entry: {}) but `incrtime`<>0, ignore", item_id, vendor_entry);
        return false;
    }

    VendorItemData const* vItems = GetNpcVendorItemList(vendor_entry);
    if (!vItems)
        return true;                                        // later checks for non-empty lists

    if (vItems->FindItemCostPair(item_id, ExtendedCost))
    {
        if (player)
            ChatHandler(player->GetSession()).PSendSysMessage(LANG_ITEM_ALREADY_IN_LIST, item_id, ExtendedCost);
        else
            LOG_ERROR("sql.sql", "Table `npc_vendor` has duplicate items {} (with extended cost {}) for vendor (Entry: {}), ignoring", item_id, ExtendedCost, vendor_entry);
        return false;
    }

    return true;
}

void ObjectMgr::LoadScriptNames()
{
    uint32 oldMSTime = getMSTime();

    // We insert an empty placeholder here so we can use the
    // script id 0 as dummy for "no script found".
    _scriptNamesStore.emplace_back("");

    QueryResult result = WorldDatabase.Query(
                             "SELECT DISTINCT(ScriptName) FROM criteria_data WHERE ScriptName <> '' AND type = 11 "
                             "UNION "
                             "SELECT DISTINCT(ScriptName) FROM battleground_template WHERE ScriptName <> '' "
                             "UNION "
                             "SELECT DISTINCT(ScriptName) FROM creature WHERE ScriptName <> '' "
                             "UNION "
                             "SELECT DISTINCT(ScriptName) FROM creature_template WHERE ScriptName <> '' "
                             "UNION "
                             "SELECT DISTINCT(ScriptName) FROM gameobject WHERE ScriptName <> '' "
                             "UNION "
                             "SELECT DISTINCT(ScriptName) FROM gameobject_template WHERE ScriptName <> '' "
                             "UNION "
                             "SELECT DISTINCT(ScriptName) FROM item_template WHERE ScriptName <> '' "
                             "UNION "
                             "SELECT DISTINCT(ScriptName) FROM areatrigger_scripts WHERE ScriptName <> '' "
                             "UNION "
                             "SELECT DISTINCT(ScriptName) FROM spell_script_names WHERE ScriptName <> '' "
                             "UNION "
                             "SELECT DISTINCT(ScriptName) FROM transports WHERE ScriptName <> '' "
                             "UNION "
                             "SELECT DISTINCT(ScriptName) FROM game_weather WHERE ScriptName <> '' "
                             "UNION "
                             "SELECT DISTINCT(ScriptName) FROM conditions WHERE ScriptName <> '' "
                             "UNION "
                             "SELECT DISTINCT(ScriptName) FROM outdoorpvp_template WHERE ScriptName <> '' "
                             "UNION "
                             "SELECT DISTINCT(script) FROM instance_template WHERE script <> ''");

    if (!result)
    {
        LOG_INFO("server.loading", " ");
        LOG_ERROR("sql.sql", ">> Loaded empty set of Script Names!");
        return;
    }

    _scriptNamesStore.reserve(result->GetRowCount() + 1);

    do
    {
        _scriptNamesStore.push_back((*result)[0].Get<std::string>());
    } while (result->NextRow());

    std::sort(_scriptNamesStore.begin(), _scriptNamesStore.end());
    LOG_INFO("server.loading", ">> Loaded {} ScriptNames in {} ms", _scriptNamesStore.size(), GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

std::string const& ObjectMgr::GetScriptName(uint32 id) const
{
    static std::string const empty = "";
    return (id < _scriptNamesStore.size()) ? _scriptNamesStore[id] : empty;
}

uint32 ObjectMgr::GetScriptId(std::string const& name)
{
    // use binary search to find the script name in the sorted vector
    // assume "" is the first element
    if (name.empty())
        return 0;

    ScriptNameContainer::const_iterator itr = std::lower_bound(_scriptNamesStore.begin(), _scriptNamesStore.end(), name);
    if (itr == _scriptNamesStore.end() || (*itr != name))
        return 0;

    return uint32(itr - _scriptNamesStore.begin());
}

void ObjectMgr::LoadBroadcastTexts()
{
    uint32 oldMSTime = getMSTime();

    _broadcastTextStore.clear(); // for reload case

    //                                               0   1           2         3           4         5         6         7            8            9            10              11        12
    QueryResult result = WorldDatabase.Query("SELECT ID, LanguageID, MaleText, FemaleText, EmoteID1, EmoteID2, EmoteID3, EmoteDelay1, EmoteDelay2, EmoteDelay3, SoundEntriesID, EmotesID, Flags FROM broadcast_text");
    if (!result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 broadcast texts. DB table `broadcast_text` is empty.");
        LOG_INFO("server.loading", " ");
        return;
    }

    _broadcastTextStore.rehash(result->GetRowCount());

    do
    {
        Field* fields = result->Fetch();

        BroadcastText bct;

        bct.Id = fields[0].Get<uint32>();
        bct.LanguageID = fields[1].Get<uint32>();
        bct.MaleText[DEFAULT_LOCALE] = fields[2].Get<std::string>();
        bct.FemaleText[DEFAULT_LOCALE] = fields[3].Get<std::string>();
        bct.EmoteId1 = fields[4].Get<uint32>();
        bct.EmoteId2 = fields[5].Get<uint32>();
        bct.EmoteId3 = fields[6].Get<uint32>();
        bct.EmoteDelay1 = fields[7].Get<uint32>();
        bct.EmoteDelay2 = fields[8].Get<uint32>();
        bct.EmoteDelay3 = fields[9].Get<uint32>();
        bct.SoundEntriesId = fields[10].Get<uint32>();
        bct.EmotesID = fields[11].Get<uint32>();
        bct.Flags = fields[12].Get<uint32>();

        if (bct.SoundEntriesId)
        {
            if (!sSoundEntriesStore.LookupEntry(bct.SoundEntriesId))
            {
                LOG_DEBUG("misc", "BroadcastText (Id: {}) in table `broadcast_text` has SoundEntriesId {} but sound does not exist.", bct.Id, bct.SoundEntriesId);
                bct.SoundEntriesId = 0;
            }
        }

        if (!GetLanguageDescByID(bct.LanguageID))
        {
            LOG_DEBUG("misc", "BroadcastText (Id: {}) in table `broadcast_text` using Language {} but Language does not exist.", bct.Id, bct.LanguageID);
            bct.LanguageID = LANG_UNIVERSAL;
        }

        if (bct.EmoteId1)
        {
            if (!sEmotesStore.LookupEntry(bct.EmoteId1))
            {
                LOG_DEBUG("misc", "BroadcastText (Id: {}) in table `broadcast_text` has EmoteId1 {} but emote does not exist.", bct.Id, bct.EmoteId1);
                bct.EmoteId1 = 0;
            }
        }

        if (bct.EmoteId2)
        {
            if (!sEmotesStore.LookupEntry(bct.EmoteId2))
            {
                LOG_DEBUG("misc", "BroadcastText (Id: {}) in table `broadcast_text` has EmoteId2 {} but emote does not exist.", bct.Id, bct.EmoteId2);
                bct.EmoteId2 = 0;
            }
        }

        if (bct.EmoteId3)
        {
            if (!sEmotesStore.LookupEntry(bct.EmoteId3))
            {
                LOG_DEBUG("misc", "BroadcastText (Id: {}) in table `broadcast_text` has EmoteId3 {} but emote does not exist.", bct.Id, bct.EmoteId3);
                bct.EmoteId3 = 0;
            }
        }

        _broadcastTextStore[bct.Id] = bct;
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} Broadcast Texts in {} ms", _broadcastTextStore.size(), GetMSTimeDiffToNow(oldMSTime));
}

void ObjectMgr::LoadBroadcastTextLocales()
{
    uint32 oldMSTime = getMSTime();

    //                                               0   1       2         3
    QueryResult result = WorldDatabase.Query("SELECT ID, locale, MaleText, FemaleText FROM broadcast_text_locale");

    if (!result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 broadcast text locales. DB table `broadcast_text_locale` is empty.");
        LOG_INFO("server.loading", " ");
        return;
    }

    uint32 locales_count = 0;
    do
    {
        Field* fields = result->Fetch();

        uint32 id = fields[0].Get<uint32>();

        BroadcastTextContainer::iterator bct = _broadcastTextStore.find(id);
        if (bct == _broadcastTextStore.end())
        {
            LOG_ERROR("sql.sql", "BroadcastText (Id: {}) found in table `broadcast_text_locale` but does not exist in `broadcast_text`. Skipped!", id);
            continue;
        }

        LocaleConstant locale = GetLocaleByName(fields[1].Get<std::string>());
        if (locale == LOCALE_enUS)
            continue;

        AddLocaleString(fields[2].Get<std::string>(), locale, bct->second.MaleText);
        AddLocaleString(fields[3].Get<std::string>(), locale, bct->second.FemaleText);
        locales_count++;
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} Broadcast Text Locales in {} ms", locales_count, GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

CreatureBaseStats const* ObjectMgr::GetCreatureBaseStats(uint8 level, uint8 unitClass)
{
    CreatureBaseStatsContainer::const_iterator it = _creatureBaseStatsStore.find(MAKE_PAIR16(level, unitClass));

    if (it != _creatureBaseStatsStore.end())
        return &(it->second);

    struct DefaultCreatureBaseStats : public CreatureBaseStats
    {
        DefaultCreatureBaseStats()
        {
            BaseArmor = 1;
            for (uint8 j = 0; j < MAX_CREATURE_EXPANSIONS; ++j)
            {
                BaseHealth[j] = 1;
                BaseDamage[j] = 0.0f;
            }
            BaseMana = 0;
            AttackPower = 0;
            RangedAttackPower = 0;
            Strength = 0;
            Agility = 0;
            Stamina = 0;
            Intellect = 0;
            Spirit = 0;
        }
    };
    static const DefaultCreatureBaseStats defStats;
    return &defStats;
}

void ObjectMgr::LoadCreatureClassLevelStats()
{
    uint32 oldMSTime = getMSTime();

    QueryResult result = WorldDatabase.Query("SELECT level, class, basehp0, basehp1, basehp2, basehp3, basemana, basearmor, attackpower, rangedattackpower, damage_base, damage_exp1, damage_exp2, damage_exp3, Strength, Agility, Stamina, Intellect, Spirit FROM creature_classlevelstats");

    if (!result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 creature base stats. DB table `creature_classlevelstats` is empty.");
        LOG_INFO("server.loading", " ");
        return;
    }

    uint32 count = 0;
    do
    {
        Field* fields = result->Fetch();

        uint8 Level = fields[0].Get<uint8>();
        uint8 Class = fields[1].Get<uint8>();

        if (!Class || ((1 << (Class - 1)) & CLASSMASK_ALL_CREATURES) == 0)
            LOG_ERROR("sql.sql", "Creature base stats for level {} has invalid class {}", Level, Class);

        CreatureBaseStats stats;

        for (uint8 i = 0; i < MAX_CREATURE_EXPANSIONS; ++i)
        {
            stats.BaseHealth[i] = fields[2 + i].Get<uint32>();

            if (stats.BaseHealth[i] == 0)
            {
                LOG_ERROR("sql.sql", "Creature base stats for class {}, level {} has invalid zero base HP[{}] - set to 1", Class, Level, i);
                stats.BaseHealth[i] = 1;
            }

            // xinef: if no data is available, get them from lower expansions
            if (stats.BaseHealth[i] <= 1)
            {
                for (uint8 j = i; j > 0;)
                {
                    --j;
                    if (stats.BaseHealth[j] > 1)
                    {
                        stats.BaseHealth[i] = stats.BaseHealth[j];
                        break;
                    }
                }
            }

            stats.BaseDamage[i] = fields[10 + i].Get<float>();
            if (stats.BaseDamage[i] < 0.0f)
            {
                LOG_ERROR("sql.sql", "Creature base stats for class {}, level {} has invalid negative base damage[{}] - set to 0.0", Class, Level, i);
                stats.BaseDamage[i] = 0.0f;
            }

            // TDB 434 only fills the Cataclysm damage column from level 80 on; keep the lower
            // expansion's value elsewhere so a Cataclysm creature never swings for zero.
            if (stats.BaseDamage[i] == 0.0f)
            {
                for (uint8 j = i; j > 0;)
                {
                    --j;
                    if (stats.BaseDamage[j] > 0.0f)
                    {
                        stats.BaseDamage[i] = stats.BaseDamage[j];
                        break;
                    }
                }
            }
        }

        stats.BaseMana = fields[6].Get<uint32>();
        stats.BaseArmor = fields[7].Get<uint32>();

        stats.AttackPower = fields[8].Get<uint32>();
        stats.RangedAttackPower = fields[9].Get<uint32>();

        stats.Strength = fields[14].Get<uint32>();
        stats.Agility = fields[15].Get<uint32>();
        stats.Stamina = fields[16].Get<uint32>();
        stats.Intellect = fields[17].Get<uint32>();
        stats.Spirit = fields[18].Get<uint32>();

        if (!stats.Strength || !stats.Agility || !stats.Stamina || !stats.Intellect || !stats.Spirit)
        {
            // Once these attributes are implemented, this should probably be uncommented.
            // LOG_WARN("server.loading", "Creature base attributes for class {}, level {} are missing!", Class, Level);
        }

        _creatureBaseStatsStore[MAKE_PAIR16(Level, Class)] = stats;

        ++count;
    } while (result->NextRow());

    CreatureTemplateContainer const* ctc = GetCreatureTemplates();
    for (CreatureTemplateContainer::const_iterator itr = ctc->begin(); itr != ctc->end(); ++itr)
    {
        for (uint16 lvl = itr->second.minlevel; lvl <= itr->second.maxlevel; ++lvl)
        {
            if (_creatureBaseStatsStore.find(MAKE_PAIR16(lvl, itr->second.unit_class)) == _creatureBaseStatsStore.end())
                LOG_ERROR("sql.sql", "Missing base stats for creature class {} level {}", itr->second.unit_class, lvl);
        }
    }

    LOG_INFO("server.loading", ">> Loaded {} Creature Base Stats in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

void ObjectMgr::LoadFactionChangeAchievements()
{
    uint32 oldMSTime = getMSTime();

    QueryResult result = WorldDatabase.Query("SELECT alliance_id, horde_id FROM player_factionchange_achievement");

    if (!result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 faction change achievement pairs. DB table `player_factionchange_achievement` is empty.");
        LOG_INFO("server.loading", " ");
        return;
    }

    uint32 count = 0;

    do
    {
        Field* fields = result->Fetch();

        uint32 alliance = fields[0].Get<uint32>();
        uint32 horde = fields[1].Get<uint32>();

        if (!sAchievementStore.LookupEntry(alliance))
            LOG_ERROR("sql.sql", "Achievement {} (alliance_id) referenced in `player_factionchange_achievement` does not exist, pair skipped!", alliance);
        else if (!sAchievementStore.LookupEntry(horde))
            LOG_ERROR("sql.sql", "Achievement {} (horde_id) referenced in `player_factionchange_achievement` does not exist, pair skipped!", horde);
        else
            FactionChangeAchievements[alliance] = horde;

        ++count;
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} faction change achievement pairs in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

void ObjectMgr::LoadFactionChangeItems()
{
    uint32 oldMSTime = getMSTime();

    QueryResult result = WorldDatabase.Query("SELECT alliance_id, horde_id FROM player_factionchange_items");

    if (!result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 faction change item pairs. DB table `player_factionchange_items` is empty.");
        LOG_INFO("server.loading", " ");
        return;
    }

    uint32 count = 0;

    do
    {
        Field* fields = result->Fetch();

        uint32 alliance = fields[0].Get<uint32>();
        uint32 horde = fields[1].Get<uint32>();

        if (!GetItemTemplate(alliance))
            LOG_ERROR("sql.sql", "Item {} (alliance_id) referenced in `player_factionchange_items` does not exist, pair skipped!", alliance);
        else if (!GetItemTemplate(horde))
            LOG_ERROR("sql.sql", "Item {} (horde_id) referenced in `player_factionchange_items` does not exist, pair skipped!", horde);
        else
            FactionChangeItems[alliance] = horde;

        ++count;
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} faction change item pairs in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

void ObjectMgr::LoadFactionChangeQuests()
{
    uint32 oldMSTime = getMSTime();

    QueryResult result = WorldDatabase.Query("SELECT alliance_id, horde_id FROM player_factionchange_quests");

    if (!result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 faction change quest pairs. DB table `player_factionchange_quests` is empty.");
        LOG_INFO("server.loading", " ");
        return;
    }

    uint32 count = 0;

    do
    {
        Field* fields = result->Fetch();

        uint32 alliance = fields[0].Get<uint32>();
        uint32 horde = fields[1].Get<uint32>();

        if (!GetQuestTemplate(alliance))
            LOG_ERROR("sql.sql", "Quest {} (alliance_id) referenced in `player_factionchange_quests` does not exist, pair skipped!", alliance);
        else if (!GetQuestTemplate(horde))
            LOG_ERROR("sql.sql", "Quest {} (horde_id) referenced in `player_factionchange_quests` does not exist, pair skipped!", horde);
        else
            FactionChangeQuests[alliance] = horde;

        ++count;
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} faction change quest pairs in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

void ObjectMgr::LoadFactionChangeReputations()
{
    uint32 oldMSTime = getMSTime();

    QueryResult result = WorldDatabase.Query("SELECT alliance_id, horde_id FROM player_factionchange_reputations");

    if (!result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 faction change reputation pairs. DB table `player_factionchange_reputations` is empty.");
        LOG_INFO("server.loading", " ");
        return;
    }

    uint32 count = 0;

    do
    {
        Field* fields = result->Fetch();

        uint32 alliance = fields[0].Get<uint32>();
        uint32 horde = fields[1].Get<uint32>();

        if (!sFactionStore.LookupEntry(alliance))
            LOG_ERROR("sql.sql", "Reputation {} (alliance_id) referenced in `player_factionchange_reputations` does not exist, pair skipped!", alliance);
        else if (!sFactionStore.LookupEntry(horde))
            LOG_ERROR("sql.sql", "Reputation {} (horde_id) referenced in `player_factionchange_reputations` does not exist, pair skipped!", horde);
        else
            FactionChangeReputation[alliance] = horde;

        ++count;
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} faction change reputation pairs in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

void ObjectMgr::LoadFactionChangeSpells()
{
    uint32 oldMSTime = getMSTime();

    QueryResult result = WorldDatabase.Query("SELECT alliance_id, horde_id FROM player_factionchange_spells");

    if (!result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 faction change spell pairs. DB table `player_factionchange_spells` is empty.");
        LOG_INFO("server.loading", " ");
        return;
    }

    uint32 count = 0;

    do
    {
        Field* fields = result->Fetch();

        uint32 alliance = fields[0].Get<uint32>();
        uint32 horde = fields[1].Get<uint32>();

        if (!sSpellMgr->GetSpellInfo(alliance))
            LOG_ERROR("sql.sql", "Spell {} (alliance_id) referenced in `player_factionchange_spells` does not exist, pair skipped!", alliance);
        else if (!sSpellMgr->GetSpellInfo(horde))
            LOG_ERROR("sql.sql", "Spell {} (horde_id) referenced in `player_factionchange_spells` does not exist, pair skipped!", horde);
        else
            FactionChangeSpells[alliance] = horde;

        ++count;
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} faction change spell pairs in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

void ObjectMgr::LoadFactionChangeTitles()
{
    uint32 oldMSTime = getMSTime();

    QueryResult result = WorldDatabase.Query("SELECT alliance_id, horde_id FROM player_factionchange_titles");

    if (!result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 faction change title pairs. DB table `player_factionchange_title` is empty.");
        return;
    }

    uint32 count = 0;

    do
    {
        Field* fields = result->Fetch();

        uint32 alliance = fields[0].Get<uint32>();
        uint32 horde = fields[1].Get<uint32>();

        if (!sCharTitlesStore.LookupEntry(alliance))
            LOG_ERROR("sql.sql", "Title {} (alliance_id) referenced in `player_factionchange_title` does not exist, pair skipped!", alliance);
        else if (!sCharTitlesStore.LookupEntry(horde))
            LOG_ERROR("sql.sql", "Title {} (horde_id) referenced in `player_factionchange_title` does not exist, pair skipped!", horde);
        else
            FactionChangeTitles[alliance] = horde;

        ++count;
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} faction change title pairs in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

GameObjectTemplate const* ObjectMgr::GetGameObjectTemplate(uint32 entry)
{
    GameObjectTemplateContainer::const_iterator itr = _gameObjectTemplateStore.find(entry);
    if (itr != _gameObjectTemplateStore.end())
        return &(itr->second);

    return nullptr;
}

bool ObjectMgr::IsGameObjectStaticTransport(uint32 entry)
{
    GameObjectTemplate const* goinfo = GetGameObjectTemplate(entry);
    return goinfo && goinfo->type == GAMEOBJECT_TYPE_TRANSPORT;
}

GameObjectTemplateAddon const* ObjectMgr::GetGameObjectTemplateAddon(uint32 entry) const
{
    auto itr = _gameObjectTemplateAddonStore.find(entry);
    if (itr != _gameObjectTemplateAddonStore.end())
        return &itr->second;

    return nullptr;
}

CreatureTemplate const* ObjectMgr::GetCreatureTemplate(uint32 entry)
{
    return entry < _creatureTemplateStoreFast.size() ? _creatureTemplateStoreFast[entry] : nullptr;
}

VehicleAccessoryList const* ObjectMgr::GetVehicleAccessoryList(Vehicle* veh) const
{
    if (Creature* cre = veh->GetBase()->ToCreature())
    {
        // Give preference to GUID-based accessories
        VehicleAccessoryContainer::const_iterator itr = _vehicleAccessoryStore.find(cre->GetSpawnId());
        if (itr != _vehicleAccessoryStore.end())
            return &itr->second;
    }

    // Otherwise return entry-based
    VehicleAccessoryContainer::const_iterator itr = _vehicleTemplateAccessoryStore.find(veh->GetCreatureEntry());
    if (itr != _vehicleTemplateAccessoryStore.end())
        return &itr->second;
    return nullptr;
}

PlayerInfo const* ObjectMgr::GetPlayerInfo(uint32 race, uint32 class_) const
{
    if (race >= sRaceMgr->GetMaxRaces())
        return nullptr;
    if (class_ >= MAX_CLASSES)
        return nullptr;
    PlayerInfo const* info = _playerInfo[race][class_];
    if (!info)
        return nullptr;
    return info;
}

void ObjectMgr::LoadGameObjectQuestItems()
{
    uint32 oldMSTime = getMSTime();

    //                                               0                1        2
    QueryResult result = WorldDatabase.Query("SELECT GameObjectEntry, ItemId, Idx FROM gameobject_questitem ORDER BY Idx ASC");

    if (!result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 gameobject quest items. DB table `gameobject_questitem` is empty.");
        return;
    }

    uint32 count = 0;
    do
    {
        Field* fields = result->Fetch();

        uint32 entry = fields[0].Get<uint32>();
        uint32 item = fields[1].Get<uint32>();
        uint32 idx = fields[2].Get<uint32>();

        GameObjectTemplate const* goInfo = GetGameObjectTemplate(entry);
        if (!goInfo)
        {
            LOG_ERROR("sql.sql", "Table `gameobject_questitem` has data for nonexistent gameobject (entry: {}, idx: {}), skipped", entry, idx);
            continue;
        };

        ItemEntry const* dbcData = sItemStore.LookupEntry(item);
        if (!dbcData)
        {
            LOG_ERROR("sql.sql", "Table `gameobject_questitem` has nonexistent item (ID: {}) in gameobject (entry: {}, idx: {}), skipped", item, entry, idx);
            continue;
        };

        _gameObjectQuestItemStore[entry].push_back(item);

        ++count;
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} Gameobject Quest Items in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

void ObjectMgr::LoadCreatureQuestItems()
{
    uint32 oldMSTime = getMSTime();

    //                                               0              1        2
    QueryResult result = WorldDatabase.Query("SELECT CreatureEntry, ItemId, Idx FROM creature_questitem ORDER BY Idx ASC");

    if (!result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 creature quest items. DB table `creature_questitem` is empty.");
        return;
    }

    uint32 count = 0;
    do
    {
        Field* fields = result->Fetch();

        uint32 entry = fields[0].Get<uint32>();
        uint32 item = fields[1].Get<uint32>();
        uint32 idx = fields[2].Get<uint32>();

        CreatureTemplate const* creatureInfo = GetCreatureTemplate(entry);
        if (!creatureInfo)
        {
            LOG_ERROR("sql.sql", "Table `creature_questitem` has data for nonexistent creature (entry: {}, idx: {}), skipped", entry, idx);
            continue;
        };

        ItemEntry const* dbcData = sItemStore.LookupEntry(item);
        if (!dbcData)
        {
            LOG_ERROR("sql.sql", "Table `creature_questitem` has nonexistent item (ID: {}) in creature (entry: {}, idx: {}), skipped", item, entry, idx);
            continue;
        };

        _creatureQuestItemStore[entry].push_back(item);

        ++count;
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} Creature Quest Items in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

void ObjectMgr::LoadQuestMoneyRewards()
{
    uint32 oldMSTime = getMSTime();

    _questMoneyRewards.clear();

    //                                                0       1       2       3       4       5       6       7       8       9       10
    QueryResult result = WorldDatabase.Query("SELECT `Level`, Money0, Money1, Money2, Money3, Money4, Money5, Money6, Money7, Money8, Money9 FROM `quest_money_reward` ORDER BY `Level`");
    if (!result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 quest money rewards. DB table `quest_money_reward` is empty.");
        return;
    }

    uint32 count = 0;
    do
    {
        Field* fields = result->Fetch();
        uint32 Level = fields[0].Get<uint32>();

        QuestMoneyRewardArray& questMoneyReward = _questMoneyRewards[Level];
        questMoneyReward.fill(0);

        for (uint8 i = 0; i < MAX_QUEST_MONEY_REWARDS; ++i)
        {
            questMoneyReward[i] = fields[1 + i].Get<uint32>();
            ++count;
        }
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} Quest Money Rewards in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

uint32 ObjectMgr::GetQuestMoneyReward(uint8 level, uint32 questMoneyDifficulty) const
{
    if (questMoneyDifficulty < MAX_QUEST_MONEY_REWARDS)
    {
        auto const& itr = _questMoneyRewards.find(level);
        if (itr != _questMoneyRewards.end())
        {
            return itr->second.at(questMoneyDifficulty);
        }
    }

    return 0;
}
