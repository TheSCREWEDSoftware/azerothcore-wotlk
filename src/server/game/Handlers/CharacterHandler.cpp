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

#include "AccountMgr.h"
#include "AreaDefines.h"
#include "RBAC.h"
#include "ArenaTeamMgr.h"
#include "AuctionHouseMgr.h"
#include "Battleground.h"
#include "CalendarMgr.h"
#include "CharacterCache.h"
#include "AuthenticationPackets.h"
#include "CharacterPackets.h"
#include "EquipmentSetPackets.h"
#include "Chat.h"
#include "Common.h"
#include "DatabaseEnv.h"
#include "GameTime.h"
#include "GitRevision.h"
#include "Group.h"
#include "Guild.h"
#include "GuildFinderMgr.h"
#include "GuildMgr.h"
#include "InstanceSaveMgr.h"
#include "Language.h"
#include "Log.h"
#include "MapMgr.h"
#include "Metric.h"
#include "MotdMgr.h"
#include "ObjectAccessor.h"
#include "ObjectMgr.h"
#include "Opcodes.h"
#include "Pet.h"
#include "Player.h"
#include "PlayerDump.h"
#include "QueryHolder.h"
#include "QueryPackets.h"
#include "RaceMgr.h"
#include "Realm.h"
#include "ReputationMgr.h"
#include "ScriptMgr.h"
#include "SharedDefines.h"
#include "SocialMgr.h"
#include "SpellAuraEffects.h"
#include "SpellAuras.h"
#include "StringConvert.h"
#include "MiscPackets.h"
#include "MovementPackets.h"
#include "ReputationPackets.h"
#include "SystemPackets.h"
#include "TC9Sidecar.h"
#include "Tokenize.h"
#include "Transport.h"
#include "Util.h"
#include "World.h"
#include "WorldPacket.h"
#include "WorldSession.h"
#include "WorldSessionMgr.h"

LoginQueryHolder::LoginQueryHolder(uint32 accountId, ObjectGuid guid)
    : _accountId(accountId), _guid(guid) { }

bool LoginQueryHolder::Initialize()
{
    SetSize(MAX_PLAYER_LOGIN_QUERY);

    bool res = true;
    ObjectGuid::LowType lowGuid = _guid.GetCounter();

    CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER);
    stmt->SetData(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_FROM, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_AURAS);
    stmt->SetData(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_AURAS, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_AURA_EFFECTS);
    stmt->SetData(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_AURA_EFFECTS, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_SPELL);
    stmt->SetData(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_SPELLS, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_QUESTSTATUS);
    stmt->SetData(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_QUEST_STATUS, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_QUESTSTATUS_OBJECTIVES);
    stmt->SetData(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_QUEST_STATUS_OBJECTIVES, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_DAILYQUESTSTATUS);
    stmt->SetData(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_DAILY_QUEST_STATUS, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_WEEKLYQUESTSTATUS);
    stmt->SetData(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_WEEKLY_QUEST_STATUS, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_MONTHLYQUESTSTATUS);
    stmt->SetData(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_MONTHLY_QUEST_STATUS, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_SEASONALQUESTSTATUS);
    stmt->SetData(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_SEASONAL_QUEST_STATUS, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_REPUTATION);
    stmt->SetData(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_REPUTATION, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_CURRENCIES);
    stmt->SetData(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_CURRENCY, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHAR_VOID_STORAGE);
    stmt->SetData(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_VOID_STORAGE, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHAR_ARCHAEOLOGY_SITES);
    stmt->SetData(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_ARCHAEOLOGY_SITES, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHAR_ARCHAEOLOGY_COMPLETED);
    stmt->SetData(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_ARCHAEOLOGY_COMPLETED, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHAR_ARCHAEOLOGY_PROJECTS);
    stmt->SetData(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_ARCHAEOLOGY_PROJECTS, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHAR_CUF_PROFILES);
    stmt->SetData(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_CUF_PROFILES, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_INVENTORY);
    stmt->SetData(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_INVENTORY, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_ACTIONS);
    stmt->SetData(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_ACTIONS, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_MAIL);
    stmt->SetData(0, lowGuid);

    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_MAILS, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_MAILITEMS);
    stmt->SetData(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_MAIL_ITEMS, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_SOCIALLIST);
    stmt->SetData(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_SOCIAL_LIST, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_HOMEBIND);
    stmt->SetData(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_HOME_BIND, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_SPELLCOOLDOWNS);
    stmt->SetData(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_SPELL_COOLDOWNS, stmt);

    if (sWorld->getBoolConfig(CONFIG_DECLINED_NAMES_USED))
    {
        stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_DECLINEDNAMES);
        stmt->SetData(0, lowGuid);
        res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_DECLINED_NAMES, stmt);
    }

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_ACHIEVEMENTS);
    stmt->SetData(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_ACHIEVEMENTS, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_CRITERIAPROGRESS);
    stmt->SetData(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_CRITERIA_PROGRESS, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_EQUIPMENTSETS);
    stmt->SetData(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_EQUIPMENT_SETS, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_ENTRY_POINT);
    stmt->SetData(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_ENTRY_POINT, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_GLYPHS);
    stmt->SetData(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_GLYPHS, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_TALENTS);
    stmt->SetData(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_TALENTS, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_PLAYER_ACCOUNT_DATA);
    stmt->SetData(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_ACCOUNT_DATA, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_SKILLS);
    stmt->SetData(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_SKILLS, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_RANDOMBG);
    stmt->SetData(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_RANDOM_BG, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_BANNED);
    stmt->SetData(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_BANNED, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_QUESTSTATUSREW);
    stmt->SetData(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_QUEST_STATUS_REW, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_BREW_OF_THE_MONTH);
    stmt->SetData(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_BREW_OF_THE_MONTH, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_ACCOUNT_INSTANCELOCKTIMES);
    stmt->SetData(0, _accountId);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_INSTANCE_LOCK_TIMES, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CORPSE_LOCATION);
    stmt->SetData(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_CORPSE_LOCATION, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHAR_SETTINGS);
    stmt->SetData(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_CHARACTER_SETTINGS, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHAR_PETS);
    stmt->SetData(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_PET_SLOTS, stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHAR_ACHIEVEMENT_OFFLINE_UPDATES);
    stmt->SetData(0, lowGuid);
    res &= SetPreparedQuery(PLAYER_LOGIN_QUERY_LOAD_OFFLINE_ACHIEVEMENTS_UPDATES, stmt);

    return res;
}

// fills a character list entry from CHAR_SEL_ENUM / CHAR_SEL_UNDELETE_ENUM, false when the character can't be listed
static bool FillEnumCharacterInfo(Field* fields, WorldPackets::Character::EnumCharactersResult::CharacterInfo& charInfo)
{
    //             0               1                2                3                 4                  5                 6               7
    //    "SELECT characters.guid, characters.name, characters.race, characters.class, characters.gender, characters.skin, characters.face, characters.hairStyle,
    //     8                     9                       10              11               12              13                     14                     15
    //    characters.hairColor, characters.facialStyle, character.level, characters.zone, characters.map, characters.position_x, characters.position_y, characters.position_z,
    //    16                    17                      18                   19                   20                     21                   22               23
    //    guild_member.guildid, characters.playerFlags, characters.at_login, character_pet.entry, character_pet.modelid, character_pet.level, characters.equipmentCache, character_banned.guid,
    //    24                             25                     26
    //    COALESCE(characters.order, 0), characters.logout_time, character_declinedname.genitive
    charInfo.Guid               = ObjectGuid::Create<HighGuid::Player>(fields[0].Get<uint32>());
    charInfo.Name               = fields[1].Get<std::string>();
    charInfo.Race               = fields[2].Get<uint8>();
    charInfo.Class              = fields[3].Get<uint8>();
    charInfo.Sex                = fields[4].Get<uint8>();
    charInfo.Skin               = fields[5].Get<uint8>();
    charInfo.Face               = fields[6].Get<uint8>();
    charInfo.HairStyle          = fields[7].Get<uint8>();
    charInfo.HairColor          = fields[8].Get<uint8>();
    charInfo.FacialHair         = fields[9].Get<uint8>();
    charInfo.Level              = fields[10].Get<uint8>();
    charInfo.MapId              = int32(fields[12].Get<uint16>());
    charInfo.PreLoadPosition    = G3D::Vector3(fields[13].Get<float>(), fields[14].Get<float>(), fields[15].Get<float>());

    if (ObjectGuid::LowType guildId = fields[16].Get<uint32>())
        charInfo.GuildGuid = ObjectGuid::Create<HighGuid::Guild>(guildId);

    PlayerInfo const* info = sObjectMgr->GetPlayerInfo(charInfo.Race, charInfo.Class);
    if (!info)
    {
        LOG_ERROR("entities.player", "Player {} has incorrect race/class pair. Don't build enum.", charInfo.Guid.ToString());
        return false;
    }
    else if (charInfo.Sex > GENDER_FEMALE)
    {
        LOG_ERROR("entities.player", "Player ({}) has incorrect gender ({}), Don't build enum.", charInfo.Guid.ToString(), charInfo.Sex);
        return false;
    }

    uint32 playerFlags = fields[17].Get<uint32>();
    uint32 atLoginFlags = fields[18].Get<uint16>();

    if (playerFlags & PLAYER_FLAGS_RESTING)
        charInfo.Flags |= CHARACTER_FLAG_RESTING;

    /*if (atLoginFlags & AT_LOGIN_RESET_TALENTS)
        charInfo->Flags |= CHARACTER_FLAG_RESET_TALENTS_ON_LOGIN;*/

    if (atLoginFlags & AT_LOGIN_RESURRECT)
        playerFlags &= ~PLAYER_FLAGS_GHOST;

    if (playerFlags & PLAYER_FLAGS_GHOST)
        charInfo.Flags |= CHARACTER_FLAG_GHOST;

    if (atLoginFlags & AT_LOGIN_RENAME)
        charInfo.Flags |= CHARACTER_FLAG_RENAME;

    if (fields[23].Get<uint32>())
        charInfo.Flags |= CHARACTER_FLAG_LOCKED_BY_BILLING;

    if (sWorld->getBoolConfig(CONFIG_DECLINED_NAMES_USED) && !fields[26].Get<std::string>().empty())
        charInfo.Flags |= CHARACTER_FLAG_DECLINED;

    if (atLoginFlags & AT_LOGIN_CUSTOMIZE)
        charInfo.CustomizationFlag = CHAR_CUSTOMIZE_FLAG_CUSTOMIZE;
    else if (atLoginFlags & AT_LOGIN_CHANGE_FACTION)
        charInfo.CustomizationFlag = CHAR_CUSTOMIZE_FLAG_FACTION;
    else if (atLoginFlags & AT_LOGIN_CHANGE_RACE)
        charInfo.CustomizationFlag = CHAR_CUSTOMIZE_FLAG_RACE;
    else
        charInfo.CustomizationFlag = CHAR_CUSTOMIZE_FLAG_NONE;

    /*if (playerFlags & PLAYER_FLAGS_NO_XP_GAIN)
        charInfo->Flags2 |= CHARACTER_FLAG_2_NO_XP_GAIN;

    if (playerFlags & PLAYER_FLAGS_LOW_LEVEL_RAID_ENABLED)
        charInfo->Flags2 |= CHARACTER_FLAG_2_LOW_LEVEL_RAID_ENABLED;

    if (playerFlags & PLAYER_FLAGS_AUTO_DECLINE_GUILD)
        charInfo->Flags2 |= CHARACTER_FLAG_2_AUTO_DECLINE_GUILD;*/

    charInfo.FirstLogin         = (atLoginFlags & AT_LOGIN_FIRST) != 0;
    charInfo.ZoneId             = charInfo.FirstLogin ? 0 : int32(fields[11].Get<uint16>()); // if first login do not show the zone

    if (playerFlags & PLAYER_FLAGS_HIDE_HELM)
        charInfo.Flags |= CHARACTER_FLAG_HIDE_HELM;

    if (playerFlags & PLAYER_FLAGS_HIDE_CLOAK)
        charInfo.Flags |= CHARACTER_FLAG_HIDE_CLOAK;

    // show pet at selection character in character list only for non-ghost character
    if (!(playerFlags & PLAYER_FLAGS_GHOST) && (charInfo.Class == CLASS_WARLOCK || charInfo.Class == CLASS_HUNTER || charInfo.Class == CLASS_DEATH_KNIGHT))
    {
        if (CreatureTemplate const* creatureInfo = sObjectMgr->GetCreatureTemplate(fields[19].Get<uint32>()))
        {
            charInfo.Pet.CreatureDisplayId = fields[20].Get<uint32>();
            charInfo.Pet.Level = fields[21].Get<uint16>();
            charInfo.Pet.CreatureFamily = creatureInfo->family;
        }
    }

    std::vector<std::string_view> equipment = Acore::Tokenize(fields[22].Get<std::string_view>(), ' ', false);
    charInfo.ListPosition       = fields[24].Get<uint8>();
    charInfo.LastPlayedTime     = fields[25].Get<uint32>();

    for (uint8 slot = 0; slot < INVENTORY_SLOT_BAG_END; ++slot)
    {
        uint32 const visualBase = slot * 2;
        Optional<uint32> itemId;
        if (visualBase < equipment.size())
            itemId = Acore::StringTo<uint32>(equipment[visualBase]);

        ItemTemplate const* proto = nullptr;
        if (itemId)
            proto = sObjectMgr->GetItemTemplate(*itemId);

        if (!proto)
        {
            if (!itemId || *itemId)
            {
                LOG_WARN("entities.player.loading", "Player %u has invalid equipment '%s' in `equipmentcache` at index %u. Skipped.",
                    charInfo.Guid.GetCounter(), (visualBase < equipment.size()) ? std::string(equipment[visualBase]).c_str() : "<none>", visualBase);
            }
            return false;
        }

        SpellItemEnchantmentEntry const* enchant = nullptr;

        Optional<uint32> enchants;
        if ((visualBase+1) < equipment.size())
            enchants = Acore::StringTo<uint32>(equipment[visualBase + 1]);
        if (!enchants)
        {
            LOG_WARN("entities.player.loading", "Player %u has invalid enchantment info '%s' in `equipmentcache` at index %u. Skipped.",
                charInfo.Guid.GetCounter(), ((visualBase+1) < equipment.size()) ? std::string(equipment[visualBase + 1]).c_str() : "<none>", visualBase + 1);
            enchants = 0;
        }
        for (uint8 enchantSlot = PERM_ENCHANTMENT_SLOT; enchantSlot <= TEMP_ENCHANTMENT_SLOT; ++enchantSlot)
        {
            // values stored in 2 uint16
            uint32 enchantId = 0x0000FFFF & ((*enchants) >> enchantSlot * 16);
            if (!enchantId)
                return false;

            enchant = sSpellItemEnchantmentStore.LookupEntry(enchantId);
            if (enchant)
                break;
        }

        charInfo.VisualItems[slot].InventoryType = proto->InventoryType;
        charInfo.VisualItems[slot].DisplayId = sDB2Manager.GetItemDisplayId(proto->ItemId, 0);
        charInfo.VisualItems[slot].DisplayEnchantId = enchant ? enchant->ItemVisual : 0;
    }

    return true;
}

void WorldSession::HandleCharEnum(PreparedQueryResult result)
{
    WorldPackets::Character::EnumCharactersResult charEnum;
    charEnum.Success = true;
    _legitCharacters.clear();

    if (result)
    {
        do
        {
            WorldPackets::Character::EnumCharactersResult::CharacterInfo charInfo;
            if (!FillEnumCharacterInfo(result->Fetch(), charInfo))
                continue;

            _legitCharacters.insert(charInfo.Guid);
            LOG_INFO("network.opcode", "Enumerated account {} character {} at list position {}.",
                GetAccountId(), charInfo.Guid.ToString(), charInfo.ListPosition);
            charEnum.Characters.push_back(std::move(charInfo));
        } while (result->NextRow());
    }

    LOG_INFO("network.opcode", "Account {} has {} character(s).", GetAccountId(), charEnum.Characters.size());
    SendPacket(charEnum.Write());
}

void WorldSession::HandleCharEnumOpcode(WorldPackets::Character::EnumCharacters& /*packet*/)
{
    CharacterDatabasePreparedStatement* stmt = nullptr;

    /// get all the data necessary for loading all characters (along with their pets) on the account

    if (sWorld->getBoolConfig(CONFIG_DECLINED_NAMES_USED))
        stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_ENUM_DECLINED_NAME);
    else
        stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_ENUM);

    stmt->SetData(0, PET_SAVE_AS_CURRENT);
    stmt->SetData(1, GetAccountId());

    _queryProcessor.AddCallback(CharacterDatabase.AsyncQuery(stmt).WithPreparedCallback(std::bind(&WorldSession::HandleCharEnum, this, std::placeholders::_1)));
}

void WorldSession::HandleCharUndeleteEnum(PreparedQueryResult result)
{
    WorldPackets::Character::EnumCharactersResult charEnum;
    charEnum.Success = true;
    charEnum.IsDeletedCharacters = true;

    if (result)
    {
        do
        {
            WorldPackets::Character::EnumCharactersResult::CharacterInfo charInfo;
            if (!FillEnumCharacterInfo(result->Fetch(), charInfo))
                continue;

            LOG_INFO("network.opcode", "Enumerated account {} deleted character {}.", GetAccountId(), charInfo.Guid.ToString());
            charEnum.Characters.push_back(std::move(charInfo));
        } while (result->NextRow());
    }

    SendPacket(charEnum.Write());
}

void WorldSession::HandleCharUndeleteEnumOpcode(WorldPackets::Character::EnumCharacters& /*enumCharacters*/)
{
    // all the data needed to list the account's deleted characters (along with their pets), TC 6.x
    CharacterDatabasePreparedStatement* stmt = nullptr;
    if (sWorld->getBoolConfig(CONFIG_DECLINED_NAMES_USED))
        stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_UNDELETE_ENUM_DECLINED_NAME);
    else
        stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_UNDELETE_ENUM);

    stmt->SetData(0, PET_SAVE_AS_CURRENT);
    stmt->SetData(1, GetAccountId());

    _queryProcessor.AddCallback(CharacterDatabase.AsyncQuery(stmt).WithPreparedCallback(std::bind(&WorldSession::HandleCharUndeleteEnum, this, std::placeholders::_1)));
}

void WorldSession::HandleGetUndeleteCooldownStatus(WorldPackets::Character::GetUndeleteCharacterCooldownStatus& /*getCooldown*/)
{
    LoginDatabasePreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_SEL_LAST_CHAR_UNDELETE);
    stmt->SetData(0, GetAccountId());

    _queryProcessor.AddCallback(LoginDatabase.AsyncQuery(stmt).WithPreparedCallback(std::bind(&WorldSession::HandleUndeleteCooldownStatusCallback, this, std::placeholders::_1)));
}

void WorldSession::HandleUndeleteCooldownStatusCallback(PreparedQueryResult result)
{
    uint32 cooldown = 0;
    uint32 maxCooldown = sWorld->getIntConfig(CONFIG_FEATURE_SYSTEM_CHARACTER_UNDELETE_COOLDOWN);
    if (result)
    {
        uint32 lastUndelete = result->Fetch()[0].Get<uint32>();
        uint32 now = uint32(GameTime::GetGameTime().count());
        if (lastUndelete + maxCooldown > now)
            cooldown = lastUndelete + maxCooldown - now;
    }

    SendUndeleteCooldownStatusResponse(cooldown, maxCooldown);
}

void WorldSession::HandleCharUndeleteOpcode(WorldPackets::Character::UndeleteCharacter& undeleteInfo)
{
    std::shared_ptr<WorldPackets::Character::CharacterUndeleteInfo> info = undeleteInfo.UndeleteInfo;

    if (!sWorld->getBoolConfig(CONFIG_FEATURE_SYSTEM_CHARACTER_UNDELETE_ENABLED))
    {
        SendUndeleteCharacterResponse(CHARACTER_UNDELETE_RESULT_ERROR_DISABLED, info.get());
        return;
    }

    LoginDatabasePreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_SEL_LAST_CHAR_UNDELETE);
    stmt->SetData(0, GetAccountId());

    // TC 6.x: cooldown, deleted character of this account, free name, free character slot, then restore
    _queryProcessor.AddCallback(LoginDatabase.AsyncQuery(stmt)
    .WithChainingPreparedCallback([this, info](QueryCallback& queryCallback, PreparedQueryResult result)
    {
        if (result)
        {
            uint32 lastUndelete = result->Fetch()[0].Get<uint32>();
            uint32 maxCooldown = sWorld->getIntConfig(CONFIG_FEATURE_SYSTEM_CHARACTER_UNDELETE_COOLDOWN);
            if (lastUndelete && (lastUndelete + maxCooldown > uint32(GameTime::GetGameTime().count())))
            {
                SendUndeleteCharacterResponse(CHARACTER_UNDELETE_RESULT_ERROR_COOLDOWN, info.get());
                return;
            }
        }

        CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHAR_DEL_INFO_BY_GUID);
        stmt->SetData(0, info->CharacterGuid.GetCounter());
        queryCallback.SetNextQuery(CharacterDatabase.AsyncQuery(stmt));
    })
    .WithChainingPreparedCallback([this, info](QueryCallback& queryCallback, PreparedQueryResult result)
    {
        if (!result)
        {
            SendUndeleteCharacterResponse(CHARACTER_UNDELETE_RESULT_ERROR_CHAR_CREATE, info.get());
            return;
        }

        Field* fields = result->Fetch();
        info->Name = fields[1].Get<std::string>();
        uint32 account = fields[2].Get<uint32>();
        if (account != GetAccountId())
        {
            SendUndeleteCharacterResponse(CHARACTER_UNDELETE_RESULT_ERROR_UNKNOWN, info.get());
            return;
        }

        CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHECK_NAME);
        stmt->SetData(0, info->Name);
        queryCallback.SetNextQuery(CharacterDatabase.AsyncQuery(stmt));
    })
    .WithChainingPreparedCallback([this, info](QueryCallback& queryCallback, PreparedQueryResult result)
    {
        if (result)
        {
            SendUndeleteCharacterResponse(CHARACTER_UNDELETE_RESULT_ERROR_NAME_TAKEN_BY_THIS_ACCOUNT, info.get());
            return;
        }

        CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_SUM_CHARS);
        stmt->SetData(0, GetAccountId());
        queryCallback.SetNextQuery(CharacterDatabase.AsyncQuery(stmt));
    })
    .WithChainingPreparedCallback([this, info](QueryCallback& queryCallback, PreparedQueryResult result)
    {
        if (result && result->Fetch()[0].Get<uint64>() >= sWorld->getIntConfig(CONFIG_CHARACTERS_PER_REALM))
        {
            SendUndeleteCharacterResponse(CHARACTER_UNDELETE_RESULT_ERROR_CHAR_CREATE, info.get());
            return;
        }

        CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_UDP_RESTORE_DELETE_INFO);
        stmt->SetData(0, info->Name);
        stmt->SetData(1, GetAccountId());
        stmt->SetData(2, info->CharacterGuid.GetCounter());
        CharacterDatabase.Execute(stmt);

        LoginDatabasePreparedStatement* loginStmt = LoginDatabase.GetPreparedStatement(LOGIN_UPD_LAST_CHAR_UNDELETE);
        loginStmt->SetData(0, GetAccountId());
        LoginDatabase.Execute(loginStmt);

        // the cache needs race, class, gender and level of the restored character
        stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_NAME_DATA);
        stmt->SetData(0, info->CharacterGuid.GetCounter());
        queryCallback.SetNextQuery(CharacterDatabase.AsyncQuery(stmt));
    })
    .WithPreparedCallback([this, info](PreparedQueryResult result)
    {
        if (result)
            sCharacterCache->AddCharacterCacheEntry(info->CharacterGuid, GetAccountId(), info->Name, (*result)[2].Get<uint8>(), (*result)[0].Get<uint8>(), (*result)[1].Get<uint8>(), (*result)[3].Get<uint8>());

        SendUndeleteCharacterResponse(CHARACTER_UNDELETE_RESULT_OK, info.get());
    }));
}

void WorldSession::SendUndeleteCooldownStatusResponse(uint32 currentCooldown, uint32 maxCooldown)
{
    WorldPackets::Character::UndeleteCooldownStatusResponse response;
    response.OnCooldown = (currentCooldown > 0);
    response.MaxCooldown = maxCooldown;
    response.CurrentCooldown = currentCooldown;
    SendPacket(response.Write());
}

void WorldSession::SendUndeleteCharacterResponse(CharacterUndeleteResult result, WorldPackets::Character::CharacterUndeleteInfo const* undeleteInfo)
{
    WorldPackets::Character::UndeleteCharacterResponse response;
    response.UndeleteInfo = undeleteInfo;
    response.Result = result;
    SendPacket(response.Write());
}

void WorldSession::HandleOpeningCinematic(WorldPackets::Misc::OpeningCinematic& /*packet*/)
{
    // Only players that has not yet gained any experience can use this
    if (_player->GetUInt32Value(PLAYER_XP))
        return;

    if (ChrClassesEntry const* classEntry = sChrClassesStore.LookupEntry(_player->getClass()))
    {
        if (classEntry->CinematicSequenceID)
            _player->SendCinematicStart(classEntry->CinematicSequenceID);
        else if (ChrRacesEntry const* raceEntry = sChrRacesStore.LookupEntry(_player->getRace()))
            _player->SendCinematicStart(raceEntry->CinematicSequenceID);
    }
}

void WorldSession::SendFeatureSystemStatus()
{
    WorldPackets::System::FeatureSystemStatus features;

    // TC 6.x values
    features.ComplaintStatus = 0;
    features.ScrollOfResurrectionRequestsRemaining = 1;
    features.ScrollOfResurrectionMaxRequestsPerDay = 1;
    features.TwitterPostThrottleLimit = 60;
    features.TwitterPostThrottleCooldown = 20;
    features.CfgRealmID = realm.Id.Realm;
    features.CfgRealmRecID = 0;
    features.TokenPollTimeSeconds = 300;
    features.TokenRedeemIndex = 0;
    features.VoiceEnabled = false;
    features.BrowserEnabled = false; // Has to be false, otherwise client will crash if "Customer Support" is opened
    features.TutorialsEnabled = true;
    features.NPETutorialsEnabled = true;

    features.EuropaTicketSystemStatus.emplace();
    features.EuropaTicketSystemStatus->ThrottleState.MaxTries = 10;
    features.EuropaTicketSystemStatus->ThrottleState.PerMilliseconds = 60000;
    features.EuropaTicketSystemStatus->ThrottleState.TryCount = 1;
    features.EuropaTicketSystemStatus->ThrottleState.LastResetTimeBeforeNow = 111111;
    features.EuropaTicketSystemStatus->TicketsEnabled = sWorld->getBoolConfig(CONFIG_SUPPORT_TICKETS_ENABLED);
    features.EuropaTicketSystemStatus->BugsEnabled = sWorld->getBoolConfig(CONFIG_SUPPORT_BUGS_ENABLED);
    features.EuropaTicketSystemStatus->ComplaintsEnabled = sWorld->getBoolConfig(CONFIG_SUPPORT_COMPLAINTS_ENABLED);
    features.EuropaTicketSystemStatus->SuggestionsEnabled = sWorld->getBoolConfig(CONFIG_SUPPORT_SUGGESTIONS_ENABLED);

    features.CharUndeleteEnabled = sWorld->getBoolConfig(CONFIG_FEATURE_SYSTEM_CHARACTER_UNDELETE_ENABLED);
    features.BpayStoreEnabled = sWorld->getBoolConfig(CONFIG_FEATURE_SYSTEM_BPAY_STORE_ENABLED);

    SendPacket(features.Write());
}

void WorldSession::HandleCharCreateOpcode(WorldPackets::Character::CreateCharacter& charCreate)
{
    std::shared_ptr<CharacterCreateInfo> createInfo = std::make_shared<CharacterCreateInfo>();
    createInfo->Name = charCreate.CreateInfo->Name;
    createInfo->Race = charCreate.CreateInfo->Race;
    createInfo->Class = charCreate.CreateInfo->Class;
    createInfo->Gender = charCreate.CreateInfo->Sex;
    createInfo->Skin = charCreate.CreateInfo->Skin;
    createInfo->Face = charCreate.CreateInfo->Face;
    createInfo->HairStyle = charCreate.CreateInfo->HairStyle;
    createInfo->HairColor = charCreate.CreateInfo->HairColor;
    createInfo->FacialHair = charCreate.CreateInfo->FacialHairStyle;
    createInfo->OutfitId = charCreate.CreateInfo->OutfitId;

    // 6.x character templates (boosts) are not supported, the character is created as usual
    if (charCreate.CreateInfo->TemplateSet)
        LOG_DEBUG("network", "Account {} created a character from template set {}, which is not supported", GetAccountId(), *charCreate.CreateInfo->TemplateSet);

    if (!HasPermission(rbac::RBAC_PERM_SKIP_CHECK_CHARACTER_CREATION_TEAMMASK))
    {
        if (uint32 mask = sWorld->getIntConfig(CONFIG_CHARACTER_CREATING_DISABLED))
        {
            if (mask & (1 << Player::TeamIdForRace(createInfo->Race)))
            {
                SendCharCreate(CHAR_CREATE_DISABLED);
                return;
            }
        }
    }

    ChrClassesEntry const* classEntry = sChrClassesStore.LookupEntry(createInfo->Class);
    if (!classEntry)
    {
        SendCharCreate(CHAR_CREATE_FAILED);
        LOG_ERROR("network.opcode", "Class ({}) not found in DBC while creating new char for account (ID: {}): wrong DBC files or cheater?", createInfo->Class, GetAccountId());
        return;
    }

    ChrRacesEntry const* raceEntry = sChrRacesStore.LookupEntry(createInfo->Race);
    if (!raceEntry)
    {
        SendCharCreate(CHAR_CREATE_FAILED);
        LOG_ERROR("network.opcode", "Race ({}) not found in DBC while creating new char for account (ID: {}): wrong DBC files or cheater?", createInfo->Race, GetAccountId());
        return;
    }

    // prevent character creating Expansion race without Expansion account
    if (GetRaceExpansion(createInfo->Race) > Expansion())
    {
        SendCharCreate(CHAR_CREATE_EXPANSION);
        LOG_ERROR("network.opcode", "Expansion {} account:[{}] tried to Create character with expansion {} race ({})", Expansion(), GetAccountId(), GetRaceExpansion(createInfo->Race), createInfo->Race);
        return;
    }

    // prevent character creating Expansion class without Expansion account
    if (GetClassExpansion(createInfo->Class) > Expansion())
    {
        SendCharCreate(CHAR_CREATE_EXPANSION_CLASS);
        LOG_ERROR("network.opcode", "Expansion {} account:[{}] tried to Create character with expansion {} class ({})", Expansion(), GetAccountId(), GetClassExpansion(createInfo->Class), createInfo->Class);
        return;
    }

    if (!HasPermission(rbac::RBAC_PERM_SKIP_CHECK_CHARACTER_CREATION_RACEMASK))
    {
        uint32 raceMaskDisabled = sWorld->getIntConfig(CONFIG_CHARACTER_CREATING_DISABLED_RACEMASK);
        if ((1 << (createInfo->Race - 1)) & raceMaskDisabled)
        {
            SendCharCreate(CHAR_CREATE_DISABLED);
            return;
        }
    }

    if (!HasPermission(rbac::RBAC_PERM_SKIP_CHECK_CHARACTER_CREATION_CLASSMASK))
    {
        uint32 classMaskDisabled = sWorld->getIntConfig(CONFIG_CHARACTER_CREATING_DISABLED_CLASSMASK);
        if ((1 << (createInfo->Class - 1)) & classMaskDisabled)
        {
            SendCharCreate(CHAR_CREATE_DISABLED);
            return;
        }
    }

    // prevent character creating with invalid name
    if (!normalizePlayerName(createInfo->Name))
    {
        SendCharCreate(CHAR_NAME_NO_NAME);
        LOG_ERROR("network.opcode", "Account:[{}] but tried to Create character with empty [name] ", GetAccountId());
        return;
    }

    // check name limitations
    uint8 res = ObjectMgr::CheckPlayerName(createInfo->Name, true);
    if (res != CHAR_NAME_SUCCESS)
    {
        if (res != CHAR_NAME_RESERVED || !HasPermission(rbac::RBAC_PERM_SKIP_CHECK_CHARACTER_CREATION_RESERVEDNAME))
        {
            SendCharCreate(ResponseCodes(res));
            return;
        }
    }

    // speedup check for heroic class disabled case
    uint32 heroic_free_slots = sWorld->getIntConfig(CONFIG_HEROIC_CHARACTERS_PER_REALM);
    if (heroic_free_slots == 0 && !HasPermission(rbac::RBAC_PERM_SKIP_CHECK_CHARACTER_CREATION_DEATH_KNIGHT) && createInfo->Class == CLASS_DEATH_KNIGHT)
    {
        SendCharCreate(CHAR_CREATE_UNIQUE_CLASS_LIMIT);
        return;
    }

    // speedup check for heroic class disabled case
    uint32 req_level_for_heroic = sWorld->getIntConfig(CONFIG_CHARACTER_CREATING_MIN_LEVEL_FOR_HEROIC_CHARACTER);
    if (!HasPermission(rbac::RBAC_PERM_SKIP_CHECK_CHARACTER_CREATION_DEATH_KNIGHT) && createInfo->Class == CLASS_DEATH_KNIGHT
        && req_level_for_heroic > sWorld->getIntConfig(CONFIG_MAX_PLAYER_LEVEL) && !HasAccountFlag(ACCOUNT_FLAG_DEATH_KNIGHT_OK))
    {
        SendCharCreate(CHAR_CREATE_LEVEL_REQUIREMENT);
        return;
    }

    CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHECK_NAME);
    stmt->SetData(0, createInfo->Name);

    _queryProcessor.AddCallback(CharacterDatabase.AsyncQuery(stmt)
    .WithChainingPreparedCallback([this](QueryCallback& queryCallback, PreparedQueryResult result)
    {
        if (result)
        {
            SendCharCreate(CHAR_CREATE_NAME_IN_USE);
            return;
        }

        LoginDatabasePreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_SEL_SUM_REALM_CHARACTERS);
        stmt->SetData(0, GetAccountId());
        queryCallback.SetNextQuery(LoginDatabase.AsyncQuery(stmt));
    })
    .WithChainingPreparedCallback([this](QueryCallback& queryCallback, PreparedQueryResult result)
    {
        uint64 acctCharCount = 0;
        if (result)
        {
            Field* fields = result->Fetch();
            acctCharCount = uint64(fields[0].Get<double>());
        }

        if (acctCharCount >= static_cast<uint64>(sWorld->getIntConfig(CONFIG_CHARACTERS_PER_ACCOUNT)))
        {
            SendCharCreate(CHAR_CREATE_ACCOUNT_LIMIT);
            return;
        }

        CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_SUM_CHARS);
        stmt->SetData(0, GetAccountId());
        queryCallback.SetNextQuery(CharacterDatabase.AsyncQuery(stmt));
    })
    .WithChainingPreparedCallback([this, createInfo](QueryCallback& queryCallback, PreparedQueryResult result)
    {
        if (result)
        {
            Field* fields = result->Fetch();
            createInfo->CharCount = uint8(fields[0].Get<uint64>()); // SQL's COUNT() returns uint64 but it will always be less than uint8.Max

            if (createInfo->CharCount >= sWorld->getIntConfig(CONFIG_CHARACTERS_PER_REALM))
            {
                SendCharCreate(CHAR_CREATE_SERVER_LIMIT);
                return;
            }
        }

        bool allowTwoSideAccounts = !sWorld->IsPvPRealm() || sWorld->getBoolConfig(CONFIG_ALLOW_TWO_SIDE_ACCOUNTS) || HasPermission(rbac::RBAC_PERM_TWO_SIDE_CHARACTER_CREATION);
        uint32 skipCinematics = sWorld->getIntConfig(CONFIG_SKIP_CINEMATICS);

        std::function<void(PreparedQueryResult)> finalizeCharacterCreation = [this, createInfo](PreparedQueryResult result)
        {
            if (!sScriptMgr->CanAccountCreateCharacter(GetAccountId(), createInfo->Race, createInfo->Class))
            {
                SendCharCreate(CHAR_CREATE_DISABLED);
                return;
            }
            bool haveSameRace = false;
            uint32 heroicReqLevel = sWorld->getIntConfig(CONFIG_CHARACTER_CREATING_MIN_LEVEL_FOR_HEROIC_CHARACTER);
            bool const deathKnightFlagAlreadySet = HasAccountFlag(ACCOUNT_FLAG_DEATH_KNIGHT_OK); // Account flag is superior to level requirement.
            bool hasHeroicReqLevel = (heroicReqLevel == 0) || deathKnightFlagAlreadySet;
            bool allowTwoSideAccounts = !sWorld->IsPvPRealm() || sWorld->getBoolConfig(CONFIG_ALLOW_TWO_SIDE_ACCOUNTS) || HasPermission(rbac::RBAC_PERM_TWO_SIDE_CHARACTER_CREATION);
            uint32 skipCinematics = sWorld->getIntConfig(CONFIG_SKIP_CINEMATICS);
            bool checkDeathKnightReqs = !HasPermission(rbac::RBAC_PERM_SKIP_CHECK_CHARACTER_CREATION_DEATH_KNIGHT) && createInfo->Class == CLASS_DEATH_KNIGHT;

            if (result)
            {
                TeamId teamId = Player::TeamIdForRace(createInfo->Race);
                uint32 freeDeathKnightSlots = sWorld->getIntConfig(CONFIG_HEROIC_CHARACTERS_PER_REALM);

                Field* field = result->Fetch();
                uint8 accRace = field[1].Get<uint8>();

                if (checkDeathKnightReqs)
                {
                    uint8 accClass = field[2].Get<uint8>();
                    if (accClass == CLASS_DEATH_KNIGHT)
                    {
                        if (freeDeathKnightSlots > 0)
                            --freeDeathKnightSlots;

                        if (freeDeathKnightSlots == 0)
                        {
                            SendCharCreate(CHAR_CREATE_UNIQUE_CLASS_LIMIT);
                            return;
                        }
                    }

                    if (!hasHeroicReqLevel)
                    {
                        uint8 accLevel = field[0].Get<uint8>();
                        if (accLevel >= heroicReqLevel)
                            hasHeroicReqLevel = true;
                    }
                }

                // need to check team only for first character
                /// @todo what to if account already has characters of both races?
                if (!allowTwoSideAccounts)
                {
                    uint32 accTeam = 0;
                    if (accRace > 0)
                        accTeam = Player::TeamIdForRace(accRace);

                    if (accTeam != teamId)
                    {
                        SendCharCreate(CHAR_CREATE_PVP_TEAMS_VIOLATION);
                        return;
                    }
                }

                // search same race for cinematic or same class if need
                /// @todo check if cinematic already shown? (already logged in?; cinematic field)
                while ((skipCinematics == 1 && !haveSameRace) || createInfo->Class == CLASS_DEATH_KNIGHT)
                {
                    if (!result->NextRow())
                        break;

                    field = result->Fetch();
                    accRace = field[1].Get<uint8>();

                    if (!haveSameRace)
                        haveSameRace = createInfo->Race == accRace;

                    if (checkDeathKnightReqs)
                    {
                        uint8 acc_class = field[2].Get<uint8>();
                        if (acc_class == CLASS_DEATH_KNIGHT)
                        {
                            if (freeDeathKnightSlots > 0)
                                --freeDeathKnightSlots;

                            if (freeDeathKnightSlots == 0)
                            {
                                SendCharCreate(CHAR_CREATE_UNIQUE_CLASS_LIMIT);
                                return;
                            }
                        }

                        if (!hasHeroicReqLevel)
                        {
                            uint8 acc_level = field[0].Get<uint8>();
                            if (acc_level >= heroicReqLevel)
                                hasHeroicReqLevel = true;
                        }
                    }
                }
            }

            if (checkDeathKnightReqs)
            {
                if (!deathKnightFlagAlreadySet && hasHeroicReqLevel)
                    UpdateAccountFlag(ACCOUNT_FLAG_DEATH_KNIGHT_OK);

                if (!hasHeroicReqLevel)
                {
                    SendCharCreate(CHAR_CREATE_LEVEL_REQUIREMENT);
                    return;
                }
            }

            // Check name uniqueness in the same step as saving to database
            if (sCharacterCache->GetCharacterGuidByName(createInfo->Name))
            {
                SendCharCreate(CHAR_CREATE_NAME_IN_USE);
                return;
            }

            std::shared_ptr<Player> newChar(new Player(this), [](Player* ptr)
            {
                // Only when player is created correctly do clean
                if (ptr->HasAtLoginFlag(AT_LOGIN_FIRST))
                {
                    ptr->CleanupsBeforeDelete();
                }
                delete ptr;
            });

            newChar->GetMotionMaster()->Initialize();
            if (!newChar->Create(sObjectMgr->GetGenerator<HighGuid::Player>().Generate(), createInfo.get()))
            {
                // Player not create (race/class/etc problem?)
                SendCharCreate(CHAR_CREATE_ERROR);
                return;
            }

            if ((haveSameRace && skipCinematics == 1) || skipCinematics == 2)
                newChar->setCinematic(1);                         // not show intro

            newChar->SetAtLoginFlag(AT_LOGIN_FIRST);              // First login

            CharacterDatabaseTransaction characterTransaction = CharacterDatabase.BeginTransaction();
            LoginDatabaseTransaction trans = LoginDatabase.BeginTransaction();

            // Player created, save it now
            newChar->SaveToDB(characterTransaction, true, false);
            createInfo->CharCount++;

            LoginDatabasePreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_REP_REALM_CHARACTERS);
            stmt->SetData(0, createInfo->CharCount);
            stmt->SetData(1, GetAccountId());
            stmt->SetData(2, realm.Id.Realm);
            trans->Append(stmt);

            LoginDatabase.CommitTransaction(trans);

            AddTransactionCallback(CharacterDatabase.AsyncCommitTransaction(characterTransaction)).AfterComplete([this, newChar = std::move(newChar)](bool success)
            {
                if (success)
                {
                    LOG_INFO("entities.player.character", "Account: {} (IP: {}) Create Character: {} {}", GetAccountId(), GetRemoteAddress(), newChar->GetName(), newChar->GetGUID().ToString());
                    sScriptMgr->OnPlayerCreate(newChar.get());
                    sCharacterCache->AddCharacterCacheEntry(newChar->GetGUID(), GetAccountId(), newChar->GetName(), newChar->getGender(), newChar->getRace(), newChar->getClass(), newChar->GetLevel());
                    SendCharCreate(CHAR_CREATE_SUCCESS);
                }
                else
                    SendCharCreate(CHAR_CREATE_ERROR);
            });
        };

        if (allowTwoSideAccounts && !skipCinematics && createInfo->Class != CLASS_DEATH_KNIGHT)
        {
            finalizeCharacterCreation(PreparedQueryResult(nullptr));
            return;
        }

        CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHAR_CREATE_INFO);
        stmt->SetData(0, GetAccountId());
        stmt->SetData(1, (skipCinematics == 1 || createInfo->Class == CLASS_DEATH_KNIGHT) ? 10 : 1);
        queryCallback.WithPreparedCallback(std::move(finalizeCharacterCreation)).SetNextQuery(CharacterDatabase.AsyncQuery(stmt));
    }));
}

void WorldSession::HandleCharDeleteOpcode(WorldPackets::Character::CharDelete& charDelete)
{
    ObjectGuid guid = charDelete.Guid;

    // Initiating
    uint32 initAccountId = GetAccountId();

    // can't delete loaded character
    if (ObjectAccessor::FindConnectedPlayer(guid) || sWorldSessionMgr->FindOfflineSessionForCharacterGUID(guid.GetCounter()))
    {
        sScriptMgr->OnPlayerFailedDelete(guid, initAccountId);
        return;
    }

    uint32 accountId = 0;
    uint8 level = 0;
    std::string name;

    // is guild leader
    if (sGuildMgr->GetGuildByLeader(guid))
    {
        sScriptMgr->OnPlayerFailedDelete(guid, initAccountId);
        SendCharDelete(CHAR_DELETE_FAILED_GUILD_LEADER);
        return;
    }

    // is arena team captain
    if (sArenaTeamMgr->GetArenaTeamByCaptain(guid))
    {
        sScriptMgr->OnPlayerFailedDelete(guid, initAccountId);
        SendCharDelete(CHAR_DELETE_FAILED_ARENA_CAPTAIN);
        return;
    }

    if (CharacterCacheEntry const* playerData = sCharacterCache->GetCharacterCacheByGuid(guid))
    {
        accountId = playerData->AccountId;
        name = playerData->Name;
        level = playerData->Level;
    }

    // prevent deleting other players' characters using cheating tools
    if (accountId != initAccountId)
    {
        sScriptMgr->OnPlayerFailedDelete(guid, initAccountId);
        return;
    }

    LOG_INFO("entities.player.character", "Account: {}, IP: {} deleted character: {}, {}, Level: {}", accountId, GetRemoteAddress(), name, guid.ToString(), level);

    // To prevent hook failure, place hook before removing reference from DB
    sScriptMgr->OnPlayerDelete(guid, initAccountId); // To prevent race conditioning, but as it also makes sense, we hand the accountId over for successful delete.
    sCalendarMgr->RemoveAllPlayerEventsAndInvites(guid);
    sGuildFinderMgr->RemoveAllMembershipRequestsFromPlayer(guid.GetCounter());
    Player::DeleteFromDB(guid.GetCounter(), GetAccountId(), true, false);

    sWorld->UpdateRealmCharCount(GetAccountId());

    SendCharDelete(CHAR_DELETE_SUCCESS);
}

void WorldSession::HandlePlayerLoginOpcode(WorldPackets::Character::PlayerLogin& packet)
{
    LOG_INFO("network.opcode", "Received CMSG_PLAYER_LOGIN for account {}.", GetAccountId());
    if (!sWorld->getBoolConfig(CONFIG_REALM_LOGIN_ENABLED))
    {
        SendCharLoginFailed(LoginFailureReason::NoWorld);
        return;
    }

    if (PlayerLoading() || GetPlayer() != nullptr)
    {
        LOG_ERROR("network", "Player tried to login again, AccountId = {}", GetAccountId());
        KickPlayer("WorldSession::HandlePlayerLoginOpcode Another client logging in");
        return;
    }

    LOG_INFO("network", "Account ({}) is trying to login.", GetAccountId());
    ObjectGuid playerGuid = packet.Guid;
    LOG_INFO("network", "Account ({}) is trying to login with character ({}).", GetAccountId(), playerGuid.ToString());

    // The ownership check is delegated to the gateway in cluster mode, but a
    // non-player GUID is invalid regardless of who authenticated the session.
    if (!playerGuid.IsPlayer() || (!sToCloud9Sidecar->ClusterModeEnabled() && !IsLegitCharacterForAccount(playerGuid)))
    {
        LOG_ERROR("network", "Account ({}) can't login with that character ({}).", GetAccountId(), playerGuid.ToString());
        KickPlayer("Account can't login with this character");
        return;
    }

    if (WorldSession* sess = sWorldSessionMgr->FindOfflineSessionForCharacterGUID(playerGuid.GetCounter()))
    {
        if (sess->GetAccountId() != GetAccountId())
        {
            SendCharLoginFailed(LoginFailureReason::DuplicateCharacter);
            return;
        }
    }

    // 6.x: the character loads once the client has opened the instance connection (HandleContinuePlayerLogin)
    m_playerLoading = true;
    _playerLoadingGuid = playerGuid;
    SendConnectToInstance(WorldPackets::Auth::ConnectToSerial::WorldAttempt1);
}

void WorldSession::HandleContinuePlayerLogin()
{
    if (!PlayerLoading() || GetPlayer())
    {
        KickPlayer("WorldSession::HandleContinuePlayerLogin unexpected instance connection");
        return;
    }

    ObjectGuid playerGuid = _playerLoadingGuid;

    SendPacket(WorldPackets::Auth::ResumeComms(CONNECTION_TYPE_INSTANCE).Write());

    if (WorldSession* sess = sWorldSessionMgr->FindOfflineSession(GetAccountId()))
    {
        Player* p = sess->GetPlayer();
        if (!p || sess->IsKicked())
        {
            AbortLogin(LoginFailureReason::DuplicateCharacter);
            return;
        }

        if (p->GetGUID() != playerGuid)
            sess->KickPlayer("No return, go to normal loading"); // no return, go to normal loading
        else
        {
            // pussywizard: players stay ingame no matter what (prevent abuse), but allow to turn it off to stop crashing
            if (!sWorld->getBoolConfig(CONFIG_ENABLE_LOGIN_AFTER_DC))
            {
                AbortLogin(LoginFailureReason::DuplicateCharacter);
                return;
            }

            uint8 limitA = 10, limitB = 10, limitC = 10; // pussywizard: this somehow froze (probably, ahh crash logs ...), and while (far) have never frozen in LogoutPlayer o_O maybe it's the combination of while(far); while(near);
            while (sess->GetPlayer() && (sess->GetPlayer()->IsBeingTeleportedFar() || (sess->GetPlayer()->IsInWorld() && sess->GetPlayer()->IsBeingTeleportedNear())))
            {
                if (limitA == 0 || --limitA == 0)
                {
                    LOG_INFO("misc", "HandlePlayerLoginOpcode A");
                    break;
                }
                while (sess->GetPlayer() && sess->GetPlayer()->IsBeingTeleportedFar())
                {
                    if (limitB == 0 || --limitB == 0)
                    {
                        LOG_INFO("misc", "HandlePlayerLoginOpcode B");
                        break;
                    }
                    sess->HandleMoveWorldportAck();
                }
                while (sess->GetPlayer() && sess->GetPlayer()->IsInWorld() && sess->GetPlayer()->IsBeingTeleportedNear())
                {
                    if (limitC == 0 || --limitC == 0)
                    {
                        LOG_INFO("misc", "HandlePlayerLoginOpcode C");
                        break;
                    }

                    Player* plMover = sess->GetPlayer()->m_mover->ToPlayer();
                    if (!plMover)
                        break;

                    WorldPacket pkt(CMSG_MOVE_TELEPORT_ACK, 16 + 4 + 4);
                    pkt << plMover->GetGUID();
                    pkt << int32(0);                        // ack index
                    pkt << int32(0);                        // move time
                    WorldPackets::Movement::MoveTeleportAck teleportAck(std::move(pkt));
                    teleportAck.Read();
                    sess->HandleMoveTeleportAck(teleportAck);
                }
            }
            if (!p->FindMap() || !p->IsInWorld() || sess->IsKicked())
            {
                AbortLogin(LoginFailureReason::DuplicateCharacter);
                return;
            }

            sess->SetPlayer(nullptr);
            SetPlayer(p);
            p->SetSession(this);
            delete p->PlayerTalkClass;
            p->PlayerTalkClass = new PlayerMenu(p->GetSession());
            m_playerLoading = false;
            HandlePlayerLoginToCharInWorld(p);
            return;
        }
    }

    std::shared_ptr<LoginQueryHolder> holder = std::make_shared<LoginQueryHolder>(GetAccountId(), playerGuid);
    if (!holder->Initialize())
    {
        m_playerLoading = false;
        return;
    }

    AddQueryHolderCallback(CharacterDatabase.DelayQueryHolder(holder)).AfterComplete([this](SQLQueryHolderBase const& holder)
    {
        HandlePlayerLoginFromDB(static_cast<LoginQueryHolder const&>(holder));
    });
}

void WorldSession::AbortLogin(LoginFailureReason reason)
{
    if (!PlayerLoading() || GetPlayer())
    {
        KickPlayer("WorldSession::AbortLogin no login in progress");
        return;
    }

    m_playerLoading = false;
    SendCharLoginFailed(reason);
}

void WorldSession::HandlePlayerLoginFromDB(LoginQueryHolder const& holder)
{
    m_playerLoading = true; // Set here for direct callers.
    LOG_INFO("network.opcode", "Player login callback for account {} character {}.",
        GetAccountId(), holder.GetGuid().ToString());
    ObjectGuid playerGuid = holder.GetGuid();

    Player* pCurrChar = new Player(this);
    // for send server info and strings (config)
    ChatHandler chH = ChatHandler(pCurrChar->GetSession());

    // "GetAccountId() == db stored account id" checked in LoadFromDB (prevent login not own character using cheating tools)
    if (!pCurrChar->LoadFromDB(playerGuid, holder))
    {
        SetPlayer(nullptr);
        KickPlayer("WorldSession::HandlePlayerLogin Player::LoadFromDB failed"); // disconnect client, player no set to session and it will not deleted or saved at kick
        delete pCurrChar; // delete it manually
        m_playerLoading = false;
        return;
    }

    pCurrChar->GetMotionMaster()->Initialize();
    pCurrChar->SendDungeonDifficulty(false);

    LOG_INFO("network", "Account ({}) logged in with character ({}).", GetAccountId(), playerGuid.ToString());
    LOG_INFO("network", "Character in zone {}, map {}, x {}, y {}, z {}.", pCurrChar->GetZoneId(), pCurrChar->GetMapId(), pCurrChar->GetPositionX(), pCurrChar->GetPositionY(), pCurrChar->GetPositionZ());

    // load player specific part before send times
    LoadAccountData(holder.GetPreparedResult(PLAYER_LOGIN_QUERY_LOAD_ACCOUNT_DATA), PER_CHARACTER_CACHE_MASK);
    SendAccountDataTimes(PER_CHARACTER_CACHE_MASK);

    if (!sToCloud9Sidecar->ClusterModeEnabled())
    {
        if (uint32 guildId = sCharacterCache->GetCharacterGuildIdByGuid(pCurrChar->GetGUID()))
        {
            Guild* guild = sGuildMgr->GetGuildById(guildId);
            Guild::Member const* member = guild ? guild->GetMember(pCurrChar->GetGUID()) : nullptr;
            if (member)
            {
                pCurrChar->SetInGuild(guildId);
                pCurrChar->SetRank(member->GetRankId());
                pCurrChar->SetGuildLevel(guild->GetLevel());
                guild->SendLoginInfo(this);
            }
            else
            {
                LOG_ERROR("network.opcode", "Player {} ({}) marked as member of not existing guild (id: {}), removing guild membership for player.",
                    pCurrChar->GetName(), pCurrChar->GetGUID().ToString(), guildId);
                pCurrChar->SetInGuild(0);
                pCurrChar->SetRank(0);
                pCurrChar->SetGuildLevel(0);
            }
        }
        else
        {
            pCurrChar->SetInGuild(0);
            pCurrChar->SetRank(0);
            pCurrChar->SetGuildLevel(0);
        }
    }
    else
    {
        pCurrChar->SetInGuild(0);
        pCurrChar->SetRank(0);
        pCurrChar->SetGuildLevel(0);
    }

    SendPacket(WorldPackets::Query::HotfixNotifyBlob().Write());
    pCurrChar->SendInitialPacketsBeforeAddToMap(true);

    SendPacket(sMotdMgr->GetMotdPacket(pCurrChar->GetSession()->GetSessionDbLocaleIndex()));
    if (sWorld->getIntConfig(CONFIG_ENABLE_SINFO_LOGIN) == 1)
        chH.PSendSysMessage("{}", GitRevision::GetFullVersion());

    SendFeatureSystemStatus();

    LOG_INFO("network", "Finished sending initial packets before going to map");
    //Show cinematic at the first time that player login
    if (!pCurrChar->getCinematic())
    {
        pCurrChar->setCinematic(1);

        if (ChrClassesEntry const* cEntry = sChrClassesStore.LookupEntry(pCurrChar->getClass()))
        {
            if (cEntry->CinematicSequenceID)
                pCurrChar->GetCinematicMgr().StartCinematic(cEntry->CinematicSequenceID);
            else if (ChrRacesEntry const* rEntry = sChrRacesStore.LookupEntry(pCurrChar->getRace()))
                pCurrChar->GetCinematicMgr().StartCinematic(rEntry->CinematicSequenceID);

            // send new char string if not empty
            std::string_view newCharString = sWorld->getStringConfig(CONFIG_NEW_CHAR_STRING);
            if (!newCharString.empty())
                chH.PSendSysMessage("{}", newCharString);
        }
    }
    // The client must know which map and position it is entering before it receives any
    // object updates for that map. TrinityCore appears to send SMSG_LOGIN_VERIFY_WORLD
    // after AddPlayerToMap, but there object updates only accumulate and are flushed at
    // end of tick, so verify-world still reaches the wire first. AzerothCore flushes them
    // inline during AddPlayerToMap, which inverted the order. A confirmed-working 4.3.4
    // capture shows LOGIN_VERIFY_WORLD immediately before the first SMSG_UPDATE_OBJECT.
    // The instance-login failure path below teleports, which sends its own verify-world
    // for the corrected destination.
    WorldPackets::Character::LoginVerifyWorld loginVerifyWorld;
    loginVerifyWorld.MapID = pCurrChar->GetMapId();
    loginVerifyWorld.Pos = pCurrChar->GetPosition();
    SendPacket(loginVerifyWorld.Write());

    LOG_INFO("network", "Adding to map");
    // Xinef: moved this from below
    ObjectAccessor::AddObject(pCurrChar);

    if (!pCurrChar->GetMap()->AddPlayerToMap(pCurrChar) || !pCurrChar->CheckInstanceLoginValid())
    {
        AreaTriggerTeleport const* at = sObjectMgr->GetGoBackTrigger(pCurrChar->GetMapId());
        if (at)
        {
            LOG_INFO("network", "Teleport to mapId:{}, x:{}, y:{}, z:{}.", at->target_mapId, at->target_X, at->target_Y, at->target_Z);
            pCurrChar->TeleportTo(at->target_mapId, at->target_X, at->target_Y, at->target_Z, pCurrChar->GetOrientation());
        }
        else
        {
            LOG_INFO("network", "Teleport to homebind location, mapId:{}, x:{}, y:{}, z:{}.", pCurrChar->m_homebindMapId, pCurrChar->m_homebindX, pCurrChar->m_homebindY, pCurrChar->m_homebindZ);
            pCurrChar->TeleportTo(pCurrChar->m_homebindMapId, pCurrChar->m_homebindX, pCurrChar->m_homebindY, pCurrChar->m_homebindZ, pCurrChar->GetOrientation());
        }

        // Probably a hackfix, but currently the best workaround to prevent character names showing as Unknown after teleport out from instances at login.
        pCurrChar->GetSession()->SendNameQueryOpcode(pCurrChar->GetGUID());
    }

    LOG_INFO("network", "Finished sending initial packets after going to map");
    pCurrChar->SendInitialPacketsAfterAddToMap();
    LOG_INFO("network", "Finished object update bootstrap after adding to map");

    if (!sToCloud9Sidecar->ClusterModeEnabled())
    {
        CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_CHAR_ONLINE);
        stmt->SetData(0, pCurrChar->GetGUID().GetCounter());
        CharacterDatabase.Execute(stmt);
    }

    LoginDatabasePreparedStatement* loginStmt = LoginDatabase.GetPreparedStatement(LOGIN_UPD_ACCOUNT_ONLINE);
    loginStmt->SetData(0, realm.Id.Realm);
    loginStmt->SetData(1, GetAccountId());
    LoginDatabase.Execute(loginStmt);

    pCurrChar->SetInGameTime(GameTime::GetGameTimeMS().count());

    // announce group about member online (must be after add to player list to receive announce to self)
    if (Group* group = pCurrChar->GetGroup())
    {
        group->SendUpdate();
        group->ResetMaxEnchantingLevel();
        group->SendRaidMarkersChanged(pCurrChar);
    }

    // pussywizard: send instance welcome message as when entering the instance through a portal
    if (MapDifficultyEntry const* mapDiff = GetMapDifficultyData(pCurrChar->GetMap()->GetId(), pCurrChar->GetMap()->GetDifficulty()))
        if (mapDiff->RaidDuration)
            if (time_t timeReset = sInstanceSaveMgr->GetResetTimeFor(pCurrChar->GetMap()->GetId(), pCurrChar->GetMap()->GetDifficulty()))
            {
                uint32 timeleft = uint32(timeReset - GameTime::GetGameTime().count());
                pCurrChar->SendInstanceResetWarning(pCurrChar->GetMap()->GetId(), pCurrChar->GetMap()->GetDifficulty(), timeleft, true);
            }

    // pussywizard: ensure that we end up on map with our loaded transport:
    if (Transport* t = pCurrChar->GetTransport())
        if (!t->IsInMap(pCurrChar))
        {
            t->RemovePassenger(pCurrChar);
            pCurrChar->m_transport = nullptr;
            pCurrChar->m_movementInfo.transport.Reset();
            pCurrChar->m_movementInfo.RemoveMovementFlag(MOVEMENTFLAG_ONTRANSPORT);
        }

    // friend status
    sSocialMgr->SendFriendStatus(pCurrChar, FRIEND_ONLINE, pCurrChar->GetGUID(), true);

    // Place character in world (and load zone) before some object loading
    pCurrChar->LoadCorpse(holder.GetPreparedResult(PLAYER_LOGIN_QUERY_LOAD_CORPSE_LOCATION));

    // Set FFA PvP for non GM in non-rest mode
    if (sWorld->IsFFAPvPRealm() && !pCurrChar->IsGameMaster() && !pCurrChar->HasPlayerFlag(PLAYER_FLAGS_RESTING))
        if (!pCurrChar->HasByteFlag(UNIT_FIELD_BYTES_2, 1, UNIT_BYTE2_FLAG_FFA_PVP))
        {
            sScriptMgr->OnPlayerFfaPvpStateUpdate(pCurrChar,true);
            pCurrChar->SetByteFlag(UNIT_FIELD_BYTES_2, 1, UNIT_BYTE2_FLAG_FFA_PVP);
        }

    if (pCurrChar->HasPlayerFlag(PLAYER_FLAGS_CONTESTED_PVP))
    {
        pCurrChar->SetContestedPvP(nullptr, false);
    }

    // Apply at_login requests
    if (pCurrChar->HasAtLoginFlag(AT_LOGIN_RESET_SPELLS))
    {
        pCurrChar->resetSpells();
        ChatHandler(this).SendNotification(LANG_RESET_SPELLS);
    }

    if (pCurrChar->HasAtLoginFlag(AT_LOGIN_RESET_TALENTS))
    {
        pCurrChar->resetTalents(true);
        pCurrChar->SendTalentsInfoData(false);              // original talents send already in to SendInitialPacketsBeforeAddToMap, resend reset state
        ChatHandler(this).SendNotification(LANG_RESET_TALENTS);
    }

    if (pCurrChar->HasAtLoginFlag(AT_LOGIN_CHECK_ACHIEVS))
    {
        // If we process the check while players are loading they won't be notified of the changes.
        pCurrChar->m_Events.AddEventAtOffset([pCurrChar]
        {
            pCurrChar->RemoveAtLoginFlag(AT_LOGIN_CHECK_ACHIEVS, true);
            pCurrChar->CheckAllAchievementCriteria();
        }, 1s);
    }

    bool firstLogin = pCurrChar->HasAtLoginFlag(AT_LOGIN_FIRST);
    if (firstLogin)
    {
        PlayerInfo const* info = sObjectMgr->GetPlayerInfo(pCurrChar->getRace(), pCurrChar->getClass());
        for (uint32 spellId : info->castSpells)
        {
            pCurrChar->CastSpell(pCurrChar, spellId, true);
        }

        // start with every map explored
        if (sWorld->getBoolConfig(CONFIG_START_ALL_EXPLORED))
        {
            for (uint8 i = 0; i < PLAYER_EXPLORED_ZONES_SIZE; i++)
            {
                pCurrChar->SetFlag(PLAYER_EXPLORED_ZONES_1 + i, 0xFFFFFFFF);
            }
        }

        // Reputations if "StartAllReputation" is enabled, -- TODO: Fix this in a better way
        if (sWorld->getBoolConfig(CONFIG_START_ALL_REP))
        {
            ReputationMgr& repMgr = pCurrChar->GetReputationMgr();

            auto SendFullReputation = [&repMgr](std::initializer_list<uint32> factionsList)
            {
                for (auto const& itr : factionsList)
                {
                    repMgr.SetOneFactionReputation(sFactionStore.LookupEntry(itr), 42999.f, false);
                }
            };

            SendFullReputation({ 942, 935, 936, 1011, 970, 967, 989, 932, 934, 1038, 1077, 1106, 1104, 1090, 1098, 1156, 1073, 1105, 1119, 1091 });

            switch (pCurrChar->GetFaction())
            {
                case ALLIANCE:
                    SendFullReputation({ 72, 47, 69, 930, 730, 978, 54, 946, 1037, 1068, 1126, 1094, 1050 });
                    break;
                case HORDE:
                    SendFullReputation({ 76, 68, 81, 911, 729, 941, 530, 947, 1052, 1067, 1124, 1064, 1085 });
                    break;
                default:
                    break;
            }

            repMgr.SendStates();
        }
    }

    // show time before shutdown if shutdown planned.
    if (sWorld->IsShuttingDown())
        sWorld->ShutdownMsg(true, pCurrChar);

    if (sWorld->getBoolConfig(CONFIG_ALL_TAXI_PATHS))
        pCurrChar->SetTaxiCheater(true);

    if (pCurrChar->IsGameMaster())
        ChatHandler(this).SendNotification(LANG_GM_ON);

    std::string IP_str = GetRemoteAddress();
    LOG_INFO("entities.player", "Account: {} (IP: {}) Login Character:[{}] ({}) Level: {}",
                  GetAccountId(), IP_str, pCurrChar->GetName(), pCurrChar->GetGUID().ToString(), pCurrChar->GetLevel());

    if (!pCurrChar->IsStandState() && !pCurrChar->HasUnitState(UNIT_STATE_STUNNED))
        pCurrChar->SetStandState(UNIT_STAND_STATE_STAND);

    m_playerLoading = false;

    // Handle Login-Achievements (should be handled after loading)
    _player->UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_ON_LOGIN, 1);

    // Xinef: fix vendors falling of player vehicle, due to isBeingLoaded checks
    if (pCurrChar->IsInWorld())
    {
        if (pCurrChar->GetMountBlockId() && !pCurrChar->HasMountedAura())
        {
            pCurrChar->CastSpell(pCurrChar, pCurrChar->GetMountBlockId(), true);
            pCurrChar->SetMountBlockId(0);

            // Xinef: refresh this in case mount aura changes anything (eg no fly zone)
            pCurrChar->UpdateAreaDependentAuras(pCurrChar->GetAreaId());
            pCurrChar->UpdateZoneDependentAuras(pCurrChar->GetZoneId());
        }
    }

    // pussywizard: pvp mode
    pCurrChar->RemovePlayerFlag(PLAYER_FLAGS_PVP_TIMER);
    if (pCurrChar->HasPlayerFlag(PLAYER_FLAGS_IN_PVP))
        pCurrChar->UpdatePvP(true, true);

    // pussywizard: on login it's not possible to go back to arena as a spectator, HandleMoveWorldportAckOpcode is not sent, so call it here
    pCurrChar->SetIsSpectator(false);

    // xinef: do this after everything is loaded
    pCurrChar->ContinueTaxiFlight();

    // reset for all pets before pet loading
    if (pCurrChar->HasAtLoginFlag(AT_LOGIN_RESET_PET_TALENTS))
        Pet::resetTalentsForAllPetsOf(pCurrChar);

    // Load pet if any (if player not alive and in taxi flight or another then pet will remember as temporary unsummoned)
    pCurrChar->LoadPet();

    if (pCurrChar->GetSession()->GetRecruiterId() != 0 || pCurrChar->GetSession()->IsARecruiter())
    {
        bool isReferrer = pCurrChar->GetSession()->IsARecruiter();

        for (auto const& [accID, session] : sWorldSessionMgr->GetAllSessions())
        {
            if (!session->GetRecruiterId() && !session->IsARecruiter())
                continue;

            if ((isReferrer && pCurrChar->GetSession()->GetAccountId() == session->GetRecruiterId()) || (!isReferrer && pCurrChar->GetSession()->GetRecruiterId() == session->GetAccountId()))
            {
                Player* rf = session->GetPlayer();
                if (rf)
                {
                    pCurrChar->SendUpdateToPlayer(rf);
                    rf->SendUpdateToPlayer(pCurrChar);
                }
            }
        }
    }

    LOG_INFO("network", "Finished loading character, now calling scripts and sending updates");
    sScriptMgr->OnPlayerLogin(pCurrChar);

    if (pCurrChar->HasAtLoginFlag(AT_LOGIN_FIRST))
    {
        pCurrChar->RemoveAtLoginFlag(AT_LOGIN_FIRST);
        sScriptMgr->OnPlayerFirstLogin(pCurrChar);
    }

    METRIC_EVENT("player_events", "Login", pCurrChar->GetName());
}

void WorldSession::HandleLoadScreenOpcode(WorldPackets::Character::LoadingScreenNotify& /*loadingScreenNotify*/)
{
    // TODO: Do something with this packet
}

void WorldSession::HandlePlayerLoginToCharInWorld(Player* pCurrChar)
{
    ChatHandler chH = ChatHandler(this);
    m_playerLoading = true;

    // Exit vehicle on reconnect - the client has fully reset so
    // the player can no longer control the vehicle. Without this
    // the player is stuck: server-side still seated, but the
    // client has no vehicle UI or movement control.
    if (pCurrChar->GetVehicle())
        pCurrChar->ExitVehicle();

    pCurrChar->SendDungeonDifficulty(false);

    WorldPackets::Character::LoginVerifyWorld loginVerifyWorld;
    loginVerifyWorld.MapID = pCurrChar->GetMapId();
    loginVerifyWorld.Pos = pCurrChar->GetPosition();
    SendPacket(loginVerifyWorld.Write());

    SendAccountDataTimes(PER_CHARACTER_CACHE_MASK);

    SendFeatureSystemStatus();

    // Send MOTD
    {
        SendPacket(sMotdMgr->GetMotdPacket(pCurrChar->GetSession()->GetSessionDbLocaleIndex()));

        // send server info
        if (sWorld->getIntConfig(CONFIG_ENABLE_SINFO_LOGIN) == 1)
            chH.PSendSysMessage("{}", GitRevision::GetFullVersion());

        LOG_DEBUG("network.opcode", "WORLD: Sent server info");
    }

    // Xinef: fix possible problem with flag UNIT_FLAG_STUNNED added during logout
    if (pCurrChar->HasUnitState(UNIT_STATE_LOGOUT_TIMER))
    {
        pCurrChar->SetRooted(false, true, true);
        pCurrChar->RemoveUnitFlag(UNIT_FLAG_STUNNED);
    }

    if (pCurrChar->GetPendingFlightChange() <= pCurrChar->GetMapChangeOrderCounter())
    {
        if (!pCurrChar->HasIncreaseMountedFlightSpeedAura() && !pCurrChar->HasFlyAura())
            pCurrChar->m_movementInfo.RemoveMovementFlag(MOVEMENTFLAG_CAN_FLY);
    }

    pCurrChar->SendInitialPacketsBeforeAddToMap();

    // necessary actions from AddPlayerToMap:
    pCurrChar->GetMap()->SendInitTransports(pCurrChar);
    pCurrChar->GetMap()->SendInitSelf(pCurrChar);

    // If we are logging into an existing player, simply clear visibility references
    // so player will receive a fresh list of new objects on the next vis update.
    pCurrChar->GetObjectVisibilityContainer().CleanVisibilityReferences();
    pCurrChar->UpdateObjectVisibility(false);

    pCurrChar->CleanupChannels();
    pCurrChar->SendInitialPacketsAfterAddToMap();
    uint32 currZone, currArea;
    pCurrChar->GetZoneAndAreaId(currZone, currArea);
    pCurrChar->SendInitWorldStates(currZone, currArea);
    pCurrChar->SetInGameTime(GameTime::GetGameTimeMS().count());

    // Xinef: we need to resend all spell mods
    pCurrChar->SendSpellModifiers();

    if (Group* group = pCurrChar->GetGroup())
        group->SendUpdate();

    // pussywizard: send instance welcome message as when entering the instance through a portal
    if (MapDifficultyEntry const* mapDiff = GetMapDifficultyData(pCurrChar->GetMap()->GetId(), pCurrChar->GetMap()->GetDifficulty()))
        if (mapDiff->RaidDuration)
            if (time_t timeReset = sInstanceSaveMgr->GetResetTimeFor(pCurrChar->GetMap()->GetId(), pCurrChar->GetMap()->GetDifficulty()))
            {
                uint32 timeleft = uint32(timeReset - GameTime::GetGameTime().count());
                GetPlayer()->SendInstanceResetWarning(pCurrChar->GetMap()->GetId(), pCurrChar->GetMap()->GetDifficulty(), timeleft, true);
            }

    // this shouldn't do anything, becaues offline can't be on taxi, but just in case
    pCurrChar->ContinueTaxiFlight();

    // send pet data, action bar, talents, etc.
    pCurrChar->PetSpellInitialize();
    pCurrChar->SendTalentsInfoData(true);

    // show time before shutdown if shutdown planned.
    if (sWorld->IsShuttingDown())
        sWorld->ShutdownMsg(true, pCurrChar);

    if (pCurrChar->IsGameMaster())
        ChatHandler(pCurrChar->GetSession()).SendNotification(LANG_GM_ON);

    m_playerLoading = false;
}

void WorldSession::HandlePlayerLoginToCharOutOfWorld(Player* /*pCurrChar*/)
{
    ABORT();
}

void WorldSession::HandleSetFactionAtWar(WorldPackets::Character::SetFactionAtWar& packet)
{
    GetPlayer()->GetReputationMgr().SetAtWar(packet.FactionIndex, true);
}

void WorldSession::HandleSetFactionNotAtWar(WorldPackets::Character::SetFactionNotAtWar& packet)
{
    GetPlayer()->GetReputationMgr().SetAtWar(packet.FactionIndex, false);
}

//I think this function is never used :/ I dunno, but i guess this opcode not exists
void WorldSession::HandleSetFactionCheat(WorldPacket& /*recvData*/)
{
    LOG_ERROR("network.opcode", "WORLD SESSION: HandleSetFactionCheat, not expected call, please report.");
    GetPlayer()->GetReputationMgr().SendStates();
}

void WorldSession::HandleTutorialFlag(WorldPackets::Misc::TutorialSetFlag& packet)
{
    switch (packet.Action)
    {
        case TUTORIAL_ACTION_UPDATE:
        {
            uint8 index = uint8(packet.TutorialBit >> 5);
            if (index >= MAX_ACCOUNT_TUTORIAL_VALUES)
            {
                LOG_ERROR("network", "CMSG_TUTORIAL_FLAG received bad TutorialBit {}.", packet.TutorialBit);
                return;
            }
            uint32 flag = GetTutorialInt(index);
            flag |= (1 << (packet.TutorialBit & 0x1F));
            SetTutorialInt(index, flag);
            break;
        }
        case TUTORIAL_ACTION_CLEAR:
            for (uint8 i = 0; i < MAX_ACCOUNT_TUTORIAL_VALUES; ++i)
                SetTutorialInt(i, 0xFFFFFFFF);
            break;
        case TUTORIAL_ACTION_RESET:
            for (uint8 i = 0; i < MAX_ACCOUNT_TUTORIAL_VALUES; ++i)
                SetTutorialInt(i, 0x00000000);
            break;
        default:
            LOG_ERROR("network", "CMSG_TUTORIAL_FLAG received unknown TutorialAction {}.", packet.Action);
            return;
    }
}

void WorldSession::HandleSetWatchedFactionOpcode(WorldPackets::Character::SetWatchedFaction& packet)
{
    GetPlayer()->SetUInt32Value(PLAYER_FIELD_WATCHED_FACTION_INDEX, packet.FactionIndex);
}

void WorldSession::HandleSetFactionInactiveOpcode(WorldPackets::Character::SetFactionInactive& packet)
{
    _player->GetReputationMgr().SetInactive(packet.Index, packet.State);
}

void WorldSession::HandleRequestForcedReactionsOpcode(WorldPackets::Reputation::RequestForcedReactions& /*requestForcedReactions*/)
{
    _player->GetReputationMgr().SendForceReactions();
}

void WorldSession::HandleShowingHelmOpcode(WorldPackets::Character::ShowingHelm& packet)
{
    if (packet.ShowHelm)
        _player->RemovePlayerFlag(PLAYER_FLAGS_HIDE_HELM);
    else
        _player->SetPlayerFlag(PLAYER_FLAGS_HIDE_HELM);
}

void WorldSession::HandleShowingCloakOpcode(WorldPackets::Character::ShowingCloak& packet)
{
    if (packet.ShowCloak)
        _player->RemovePlayerFlag(PLAYER_FLAGS_HIDE_CLOAK);
    else
        _player->SetPlayerFlag(PLAYER_FLAGS_HIDE_CLOAK);
}

void WorldSession::HandleCharRenameOpcode(WorldPackets::Character::CharacterRenameRequest& request)
{
    std::shared_ptr<CharacterRenameInfo> renameInfo = std::make_shared<CharacterRenameInfo>();
    renameInfo->Guid = request.RenameInfo->Guid;
    renameInfo->Name = request.RenameInfo->NewName;

    // prevent character rename to invalid name
    if (!normalizePlayerName(renameInfo->Name))
    {
        SendCharRename(CHAR_NAME_NO_NAME, renameInfo.get());
        return;
    }

    uint8 res = ObjectMgr::CheckPlayerName(renameInfo->Name, true);
    if (res != CHAR_NAME_SUCCESS)
    {
        if (res != CHAR_NAME_RESERVED || !HasPermission(rbac::RBAC_PERM_SKIP_CHECK_CHARACTER_CREATION_RESERVEDNAME))
        {
            SendCharRename(ResponseCodes(res), renameInfo.get());
            return;
        }
    }

    // Ensure that the character belongs to the current account, that rename at login is enabled
    // and that there is no character with the desired new name
    CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_FREE_NAME);

    stmt->SetData(0, renameInfo->Guid.GetCounter());
    stmt->SetData(1, GetAccountId());
    stmt->SetData(2, renameInfo->Name);

    _queryProcessor.AddCallback(CharacterDatabase.AsyncQuery(stmt)
        .WithPreparedCallback(std::bind(&WorldSession::HandleCharRenameCallBack, this, renameInfo, std::placeholders::_1)));
}

void WorldSession::HandleCharRenameCallBack(std::shared_ptr<CharacterRenameInfo> renameInfo, PreparedQueryResult result)
{
    if (!result)
    {
        SendCharRename(CHAR_CREATE_ERROR, renameInfo.get());
        return;
    }

    Field* fields = result->Fetch();

    ObjectGuid::LowType guidLow = fields[0].Get<uint32>();
    std::string oldName = fields[1].Get<std::string>();
    uint16 atLoginFlags = fields[2].Get<uint16>();

    if (!(atLoginFlags & AT_LOGIN_RENAME))
    {
        SendCharRename(CHAR_CREATE_ERROR, renameInfo.get());
        return;
    }

    atLoginFlags &= ~AT_LOGIN_RENAME;

    // pussywizard:
    if (ObjectAccessor::FindConnectedPlayer(ObjectGuid::Create<HighGuid::Player>(guidLow)) || sWorldSessionMgr->FindOfflineSessionForCharacterGUID(guidLow))
    {
        SendCharRename(CHAR_CREATE_ERROR, renameInfo.get());
        return;
    }

    // Update name and at_login flag in the db
    CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_CHAR_NAME_AT_LOGIN);
    stmt->SetData(0, renameInfo->Name);
    stmt->SetData(1, atLoginFlags);
    stmt->SetData(2, guidLow);
    CharacterDatabase.Execute(stmt);

    // Removed declined name from db
    if (sWorld->getBoolConfig(CONFIG_DECLINED_NAMES_USED))
    {
        stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_DECLINED_NAME);
        stmt->SetData(0, guidLow);
        CharacterDatabase.Execute(stmt);
    }

    LOG_INFO("entities.player.character", "Account: {} (IP: {}), Character [{}] (guid: {}) Changed name to: {}", GetAccountId(), GetRemoteAddress(), oldName, guidLow, renameInfo->Name);

    SendCharRename(RESPONSE_SUCCESS, renameInfo.get());

    // xinef: update global data
    sCharacterCache->UpdateCharacterData(renameInfo->Guid, renameInfo->Name);
}

void WorldSession::HandleSetPlayerDeclinedNames(WorldPackets::Character::SetPlayerDeclinedNames& packet)
{
    // pussywizard:
    if (!sWorld->getBoolConfig(CONFIG_DECLINED_NAMES_USED))
        return;

    ObjectGuid guid = packet.Player;
    DeclinedName declinedname = packet.DeclinedNames;

    // not accept declined names for unsupported languages
    std::string name;
    if (!sCharacterCache->GetCharacterNameByGuid(guid, name))
    {
        SendSetPlayerDeclinedNamesResult(DECLINED_NAMES_RESULT_ERROR, guid);
        return;
    }

    std::wstring wname;
    if (!Utf8toWStr(name, wname))
    {
        SendSetPlayerDeclinedNamesResult(DECLINED_NAMES_RESULT_ERROR, guid);
        return;
    }

    if (!isCyrillicCharacter(wname[0]))                      // name already stored as only single alphabet using
    {
        SendSetPlayerDeclinedNamesResult(DECLINED_NAMES_RESULT_ERROR, guid);
        return;
    }

    for (int i = 0; i < MAX_DECLINED_NAME_CASES; ++i)
    {
        if (!normalizePlayerName(declinedname.name[i]))
        {
            SendSetPlayerDeclinedNamesResult(DECLINED_NAMES_RESULT_ERROR, guid);
            return;
        }
    }

    if (!ObjectMgr::CheckDeclinedNames(wname, declinedname))
    {
        SendSetPlayerDeclinedNamesResult(DECLINED_NAMES_RESULT_ERROR, guid);
        return;
    }

    for (int i = 0; i < MAX_DECLINED_NAME_CASES; ++i)
        CharacterDatabase.EscapeString(declinedname.name[i]);

    CharacterDatabaseTransaction trans = CharacterDatabase.BeginTransaction();

    CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_CHAR_DECLINED_NAME);
    stmt->SetData(0, guid.GetCounter());
    trans->Append(stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_INS_CHAR_DECLINED_NAME);
    stmt->SetData(0, guid.GetCounter());

    for (uint8 i = 0; i < 5; i++)
        stmt->SetData(i + 1, declinedname.name[i]);

    trans->Append(stmt);

    CharacterDatabase.CommitTransaction(trans);

    SendSetPlayerDeclinedNamesResult(DECLINED_NAMES_RESULT_SUCCESS, guid);
}

void WorldSession::HandleAlterAppearance(WorldPackets::Character::AlterApperance& packet)
{
    uint8 const race = _player->getRace();
    uint8 const gender = _player->getGender();

    BarberShopStyleEntry const* bs_hair = sBarberShopStyleStore.LookupEntry(packet.NewHairStyle);
    if (!bs_hair || bs_hair->Type != 0 || bs_hair->Race != race || bs_hair->Sex != gender)
        return;

    BarberShopStyleEntry const* bs_facialHair = sBarberShopStyleStore.LookupEntry(packet.NewFacialHair);
    if (!bs_facialHair || bs_facialHair->Type != 2 || bs_facialHair->Race != race || bs_facialHair->Sex != gender)
        return;

    BarberShopStyleEntry const* bs_skinColor = sBarberShopStyleStore.LookupEntry(packet.NewSkinColor);
    if (bs_skinColor && (bs_skinColor->Type != 3 || bs_skinColor->Race != race || bs_skinColor->Sex != gender))
        return;

    // 6.x barbers can change the face too (TC 6.x)
    BarberShopStyleEntry const* bs_face = sBarberShopStyleStore.LookupEntry(packet.NewFace);
    if (bs_face && (bs_face->Type != 4 || bs_face->Race != race || bs_face->Sex != gender))
        return;

    GameObject* go = _player->FindNearestGameObjectOfType(GAMEOBJECT_TYPE_BARBER_CHAIR, 5.0f);
    if (!go)
    {
        SendBarberShopResult(BARBER_SHOP_RESULT_NOT_ON_CHAIR);
        return;
    }

    if (_player->getStandState() != UNIT_STAND_STATE_SIT_LOW_CHAIR + go->GetGOInfo()->barberChair.chairheight)
    {
        SendBarberShopResult(BARBER_SHOP_RESULT_NOT_ON_CHAIR);
        return;
    }

    uint32 cost = _player->GetBarberShopCost(bs_hair, uint8(packet.NewHairColor), bs_facialHair, bs_skinColor, bs_face);
    if (!_player->HasEnoughMoney(cost))
    {
        SendBarberShopResult(BARBER_SHOP_RESULT_NO_MONEY);
        return;
    }

    SendBarberShopResult(BARBER_SHOP_RESULT_SUCCESS);

    _player->ModifyMoney(-int32(cost));                     // it isn't free
    _player->UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_GOLD_SPENT_AT_BARBER, cost);

    _player->SetByteValue(PLAYER_BYTES, 2, uint8(bs_hair->Data));
    _player->SetByteValue(PLAYER_BYTES, 3, uint8(packet.NewHairColor));
    _player->SetByteValue(PLAYER_BYTES_2, 0, uint8(bs_facialHair->Data));
    if (bs_skinColor)
        _player->SetByteValue(PLAYER_BYTES, 0, uint8(bs_skinColor->Data));
    if (bs_face)
        _player->SetByteValue(PLAYER_BYTES, 1, uint8(bs_face->Data));

    _player->UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_VISIT_BARBER_SHOP, 1);

    _player->SetStandState(UNIT_STAND_STATE_STAND);         // stand up
}

void WorldSession::SendBarberShopResult(BarberShopResult result)
{
    WorldPackets::Character::BarberShopResultServer packet;
    packet.Result = result;
    SendPacket(packet.Write());
}

void WorldSession::HandleRemoveGlyph(WorldPacket& recvData)
{
    uint32 slot;
    recvData >> slot;

    if (slot >= MAX_GLYPH_SLOT_INDEX)
    {
        LOG_DEBUG("network", "Client sent wrong glyph slot number in opcode CMSG_REMOVE_GLYPH {}", slot);
        return;
    }

    if (uint32 glyph = _player->GetGlyph(slot))
    {
        if (GlyphPropertiesEntry const* glyphEntry = sGlyphPropertiesStore.LookupEntry(glyph))
        {
            _player->RemoveAurasDueToSpell(glyphEntry->SpellID);

            // Removed any triggered auras
            Unit::AuraMap& ownedAuras = _player->GetOwnedAuras();
            for (Unit::AuraMap::iterator iter = ownedAuras.begin(); iter != ownedAuras.end();)
            {
                Aura* aura = iter->second;
                if (SpellInfo const* triggeredByAuraSpellInfo = aura->GetTriggeredByAuraSpellInfo())
                {
                    if (triggeredByAuraSpellInfo->Id == glyphEntry->SpellID)
                    {
                        _player->RemoveOwnedAura(iter);
                        continue;
                    }
                }
                ++iter;
            }

            _player->SendLearnPacket(glyphEntry->SpellID, false); // Send packet to properly handle client-side spell tooltips
            _player->SetGlyph(slot, 0, true);
            _player->SendTalentsInfoData(false);
        }
    }
}

void WorldSession::HandleCharCustomizeOpcode(WorldPackets::Character::CharCustomize& packet)
{
    std::shared_ptr<CharacterCustomizeInfo> customizeInfo = std::make_shared<CharacterCustomizeInfo>();
    customizeInfo->Guid = packet.CustomizeInfo->CharGUID;
    customizeInfo->Name = packet.CustomizeInfo->CharName;
    customizeInfo->Gender = packet.CustomizeInfo->SexID;
    customizeInfo->Skin = packet.CustomizeInfo->SkinID;
    customizeInfo->Face = packet.CustomizeInfo->FaceID;
    customizeInfo->HairStyle = packet.CustomizeInfo->HairStyleID;
    customizeInfo->HairColor = packet.CustomizeInfo->HairColorID;
    customizeInfo->FacialHair = packet.CustomizeInfo->FacialHairStyleID;

    if (!IsLegitCharacterForAccount(customizeInfo->Guid))
    {
        LOG_ERROR("entities.player.cheat", "Account {}, IP: {} tried to customise {}, but it does not belong to their account!",
            GetAccountId(), GetRemoteAddress(), customizeInfo->Guid.ToString());
        KickPlayer("WorldSession::HandleCharCustomize Trying to customise character of another account");
        return;
    }

    // pussywizard:
    if (ObjectAccessor::FindConnectedPlayer(customizeInfo->Guid) || sWorldSessionMgr->FindOfflineSessionForCharacterGUID(customizeInfo->Guid.GetCounter()))
    {
        SendCharCustomize(CHAR_CREATE_ERROR, customizeInfo.get());
        return;
    }

    CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHAR_CUSTOMIZE_INFO);
    stmt->SetData(0, customizeInfo->Guid.GetCounter());

    _queryProcessor.AddCallback(CharacterDatabase.AsyncQuery(stmt)
        .WithPreparedCallback(std::bind(&WorldSession::HandleCharCustomizeCallback, this, customizeInfo, std::placeholders::_1)));
}

void WorldSession::HandleCharCustomizeCallback(std::shared_ptr<CharacterCustomizeInfo> customizeInfo, PreparedQueryResult result)
{
    if (!result)
    {
        SendCharCustomize(CHAR_CREATE_ERROR, customizeInfo.get());
        return;
    }

    // get the players old (at this moment current) race
    CharacterCacheEntry const* playerData = sCharacterCache->GetCharacterCacheByGuid(customizeInfo->Guid);
    if (!playerData)
    {
        SendCharCustomize(CHAR_CREATE_ERROR, customizeInfo.get());
        return;
    }

    Field* fields = result->Fetch();
    std::string oldName = fields[0].Get<std::string>();
    //uint8 plrRace = fields[1].Get<uint8>();
    //uint8 plrClass = fields[2].Get<uint8>();
    //uint8 plrGender = fields[3].Get<uint8>();
    uint32 atLoginFlags = fields[4].Get<uint16>();

    if (!(atLoginFlags & AT_LOGIN_CUSTOMIZE))
    {
        SendCharCustomize(CHAR_CREATE_ERROR, customizeInfo.get());
        return;
    }

    atLoginFlags &= ~AT_LOGIN_CUSTOMIZE;

    // prevent character rename to invalid name
    if (!normalizePlayerName(customizeInfo->Name))
    {
        SendCharCustomize(CHAR_NAME_NO_NAME, customizeInfo.get());
        return;
    }

    ResponseCodes res = static_cast<ResponseCodes>(ObjectMgr::CheckPlayerName(customizeInfo->Name, true));
    if (res != CHAR_NAME_SUCCESS)
    {
        if (res != CHAR_NAME_RESERVED || !HasPermission(rbac::RBAC_PERM_SKIP_CHECK_CHARACTER_CREATION_RESERVEDNAME))
        {
            SendCharCustomize(res, customizeInfo.get());
            return;
        }
    }

    // character with this name already exist
    if (ObjectGuid newguid = sCharacterCache->GetCharacterGuidByName(customizeInfo->Name))
    {
        if (newguid != customizeInfo->Guid)
        {
            SendCharCustomize(CHAR_CREATE_NAME_IN_USE, customizeInfo.get());
            return;
        }
    }

    CharacterDatabasePreparedStatement* stmt = nullptr;
    CharacterDatabaseTransaction trans = CharacterDatabase.BeginTransaction();

    ObjectGuid::LowType lowGuid = customizeInfo->Guid.GetCounter();

    /// Customize
    Player::Customize(customizeInfo.get(), trans);

    /// Name Change and update atLogin flags
    {
        stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_CHAR_NAME_AT_LOGIN);
        stmt->SetData(0, customizeInfo->Name);
        stmt->SetData(1, atLoginFlags);
        stmt->SetData(2, lowGuid);

        trans->Append(stmt);

        if (sWorld->getBoolConfig(CONFIG_DECLINED_NAMES_USED))
        {
            stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_DECLINED_NAME);
            stmt->SetData(0, lowGuid);

            trans->Append(stmt);
        }
    }

    CharacterDatabase.CommitTransaction(trans);

    sCharacterCache->UpdateCharacterData(customizeInfo->Guid, customizeInfo->Name, customizeInfo->Gender);

    SendCharCustomize(RESPONSE_SUCCESS, customizeInfo.get());

    LOG_INFO("entities.player.character", "Account: {} (IP: {}), Character[{}] ({}) Customized to: {}",
        GetAccountId(), GetRemoteAddress(), oldName, customizeInfo->Guid.ToString(), customizeInfo->Name);
}

void WorldSession::HandleEquipmentSetSave(WorldPackets::EquipmentSet::SaveEquipmentSet& saveEquipmentSet)
{
    if (saveEquipmentSet.Set.SetID >= MAX_EQUIPMENT_SET_INDEX) // client set slots amount
        return;

    if (saveEquipmentSet.Set.SetName.length() > 16) // Client limitation
    {
        LOG_ERROR("entities.player.cheat", "Character GUID {} tried to create equipment set {} with too long a name!", _player->GetGUID().ToString(), saveEquipmentSet.Set.Guid);
        return;
    }

    if (saveEquipmentSet.Set.SetIcon.length() > 100) // DB limitation
    {
        LOG_ERROR("entities.player.cheat", "Character GUID {} tried to create equipment set {} with too long an icon name!", _player->GetGUID().ToString(), saveEquipmentSet.Set.Guid);
        return;
    }

    for (uint8 i = 0; i < EQUIPMENT_SLOT_END; ++i)
    {
        if (!(saveEquipmentSet.Set.IgnoreMask & (1 << i)))
        {
            ObjectGuid const& itemGuid = saveEquipmentSet.Set.Pieces[i];

            Item* item = _player->GetItemByPos(INVENTORY_SLOT_BAG_0, i);

            /// cheating check 1 (item equipped but sent empty guid)
            if (!item && !itemGuid.IsEmpty())
                return;

            /// cheating check 2 (sent guid does not match equipped item)
            if (item && item->GetGUID() != itemGuid)
                return;
        }
        else
            saveEquipmentSet.Set.Pieces[i].Clear();
    }

    saveEquipmentSet.Set.IgnoreMask &= 0x7FFFF; /// clear invalid bits (i > EQUIPMENT_SLOT_END)

    _player->SetEquipmentSet(std::move(saveEquipmentSet.Set));
}

void WorldSession::HandleDeleteEquipmentSet(WorldPackets::EquipmentSet::DeleteEquipmentSet& deleteEquipmentSet)
{
    _player->DeleteEquipmentSet(deleteEquipmentSet.ID);
}

void WorldSession::HandleUseEquipmentSet(WorldPackets::EquipmentSet::UseEquipmentSet& useEquipmentSet)
{
    ObjectGuid ignoredItemGuid;
    ignoredItemGuid.SetRawValue(0, 1);

    std::vector<std::unique_ptr<SavedItem>> savedItems;
    uint8 errorId = 0;
    for (uint32 i = 0; i < EQUIPMENT_SLOT_END; ++i)
    {
        ObjectGuid const& itemGuid = useEquipmentSet.Items[i].Item;

        LOG_DEBUG("entities.player.items", "Item {}: srcbag {}, srcslot {}", itemGuid.ToString(), useEquipmentSet.Items[i].ContainerSlot, useEquipmentSet.Items[i].Slot);

        // check if item slot is set to "ignored" (raw value == 1), must not be unequipped then
        if (itemGuid == ignoredItemGuid)
            continue;

        // Only equip weapons in combat
        if (_player->IsInCombat() && i != EQUIPMENT_SLOT_MAINHAND && i != EQUIPMENT_SLOT_OFFHAND && i != EQUIPMENT_SLOT_RANGED)
            continue;

        Item* item = nullptr;
        if (itemGuid)
            item = _player->GetItemByGuid(itemGuid);

        uint16 dstpos = i | (INVENTORY_SLOT_BAG_0 << 8);

        InventoryResult msg;
        Item* uItem = _player->GetItemByPos(INVENTORY_SLOT_BAG_0, i);
        if (uItem)
        {
            if (uItem->IsEquipped())
            {
                msg = _player->CanUnequipItem(dstpos, true);
                if (msg != EQUIP_ERR_OK)
                {
                    _player->SendEquipError(msg, uItem, nullptr);
                    continue;
                }
            }

            if (!item)
            {
                ItemPosCountVec sDest;
                msg = _player->CanStoreItem(NULL_BAG, NULL_SLOT, sDest, uItem, false);
                if (msg == EQUIP_ERR_OK)
                {
                    savedItems.emplace_back(std::make_unique<SavedItem>(uItem, dstpos));
                    _player->RemoveItem(INVENTORY_SLOT_BAG_0, i, true);
                    _player->StoreItem(sDest, uItem, true);
                }
                else
                {
                    errorId = 4;
                    for (uint8_t j = 0; j < savedItems.size(); ++j)
                    {
                        _player->SwapItem(savedItems[j].get()->item->GetPos(), savedItems[j].get()->dstpos);
                    }
                    break;
                }

                continue;
            }
        }

        if (item)
        {
            if (item->GetPos() == dstpos)
                continue;

            if (!item->IsEquipped())
            {
                uint16 _candidatePos;
                msg = _player->CanEquipItem(NULL_SLOT, _candidatePos, item, true);
                if (msg != EQUIP_ERR_OK)
                {
                    _player->SendEquipError(msg, item, nullptr);
                    continue;
                }
            }

            _player->SwapItem(item->GetPos(), dstpos);
        }
    }

    WorldPackets::EquipmentSet::UseEquipmentSetResult result;
    result.Reason = errorId;                                // 4 - equipment swap failed - inventory is full
    SendPacket(result.Write());
}

void WorldSession::HandleCharRaceOrFactionChangeOpcode(WorldPackets::Character::CharRaceOrFactionChange& packet)
{
    std::shared_ptr<CharacterFactionChangeInfo> factionChangeInfo = std::make_shared<CharacterFactionChangeInfo>();
    WorldPackets::Character::CharRaceOrFactionChangeInfo const& info = *packet.RaceOrFactionChangeInfo;
    factionChangeInfo->Guid = info.Guid;
    factionChangeInfo->Name = info.Name;
    factionChangeInfo->Race = info.RaceID;
    factionChangeInfo->Gender = info.SexID;
    factionChangeInfo->FactionChange = info.FactionChange;
    // the appearance fields are optional
    factionChangeInfo->HairColor = info.HairColorID.value_or(0);
    factionChangeInfo->HairStyle = info.HairStyleID.value_or(0);
    factionChangeInfo->Skin = info.SkinID.value_or(0);
    factionChangeInfo->Face = info.FaceID.value_or(0);
    factionChangeInfo->FacialHair = info.FacialHairStyleID.value_or(0);

    if (!IsLegitCharacterForAccount(factionChangeInfo->Guid))
    {
        LOG_ERROR("entities.player.cheat", "Account {}, IP: {} tried to factionchange character {}, but it does not belong to their account!",
            GetAccountId(), GetRemoteAddress(), factionChangeInfo->Guid.ToString());
        KickPlayer("WorldSession::HandleCharFactionOrRaceChange Trying to change faction of character of another account");
        return;
    }

    // pussywizard:
    if (ObjectAccessor::FindConnectedPlayer(factionChangeInfo->Guid) || sWorldSessionMgr->FindOfflineSessionForCharacterGUID(factionChangeInfo->Guid.GetCounter()))
    {
        SendCharFactionChange(CHAR_CREATE_ERROR, factionChangeInfo.get());
        return;
    }

    CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHAR_RACE_OR_FACTION_CHANGE_INFOS);
    stmt->SetData(0, factionChangeInfo->Guid.GetCounter());

    _queryProcessor.AddCallback(CharacterDatabase.AsyncQuery(stmt)
        .WithPreparedCallback(std::bind(&WorldSession::HandleCharFactionOrRaceChangeCallback, this, factionChangeInfo, std::placeholders::_1)));
}

void WorldSession::HandleCharFactionOrRaceChangeCallback(std::shared_ptr<CharacterFactionChangeInfo> factionChangeInfo, PreparedQueryResult result)
{
    if (!result)
    {
        SendCharFactionChange(CHAR_CREATE_ERROR, factionChangeInfo.get());
        return;
    }

    ObjectGuid::LowType lowGuid = factionChangeInfo->Guid.GetCounter();

    // get the players old (at this moment current) race
    CharacterCacheEntry const* playerData = sCharacterCache->GetCharacterCacheByGuid(factionChangeInfo->Guid);
    if (!playerData)
    {
        SendCharFactionChange(CHAR_CREATE_ERROR, factionChangeInfo.get());
        return;
    }

    uint8 oldRace = playerData->Race;
    uint8 playerClass = playerData->Class;
    uint8 level = playerData->Level;

    if (!sObjectMgr->GetPlayerInfo(factionChangeInfo->Race, playerClass))
    {
        SendCharFactionChange(CHAR_CREATE_ERROR, factionChangeInfo.get());
        return;
    }

    Field* fields = result->Fetch();
    uint32 atLoginFlags = fields[0].Get<uint16>();
    std::string knownTitlesStr = fields[1].Get<std::string>();
    uint64 money = fields[2].Get<uint64>();

    uint32 usedLoginFlag = (factionChangeInfo->FactionChange ? AT_LOGIN_CHANGE_FACTION : AT_LOGIN_CHANGE_RACE);
    if (!(atLoginFlags & usedLoginFlag))
    {
        SendCharFactionChange(CHAR_CREATE_ERROR, factionChangeInfo.get());
        return;
    }

    // xinef: add some safety checks
    if (factionChangeInfo->FactionChange)
    {
        // if player is in a guild
        if (playerData->GuildId && !sWorld->getBoolConfig(CONFIG_ALLOW_TWO_SIDE_INTERACTION_GUILD))
        {
            SendCharFactionChange(CHAR_CREATE_CHARACTER_IN_GUILD, factionChangeInfo.get());
            return;
        }

        // is arena team captain
        if (!sWorld->getBoolConfig(CONFIG_ALLOW_TWO_SIDE_INTERACTION_ARENA) && sArenaTeamMgr->GetArenaTeamByCaptain(factionChangeInfo->Guid))
        {
            SendCharFactionChange(CHAR_CREATE_CHARACTER_ARENA_LEADER, factionChangeInfo.get());
            return;
        }

        // check mailbox
        if (playerData->MailCount)
        {
            SendCharFactionChange(CHAR_CREATE_CHARACTER_DELETE_MAIL, factionChangeInfo.get());
            return;
        }

        // check auctions, current packet is processed single-threaded way, so not a problem
        bool has_auctions = false;

        for (uint8 i = 0; i < 2; ++i) // check both neutral and faction-specific AH
        {
            AuctionHouseObject* auctionHouse = sAuctionMgr->GetAuctionsMap(i == 0 ? 0 : (((1 << (playerData->Race - 1)) & sRaceMgr->GetAllianceRaceMask()) ? 12 : 29));

            for (auto const& [auID, Aentry] : auctionHouse->GetAuctions())
            {
                if (Aentry && (Aentry->owner == factionChangeInfo->Guid || Aentry->bidder == factionChangeInfo->Guid))
                {
                    has_auctions = true;
                    break;
                }
            }

            if (has_auctions)
                break;
        }

        if (has_auctions)
        {
            SendCharFactionChange(CHAR_CREATE_ERROR, factionChangeInfo.get());
            return;
        }
    }

    TeamId newTeam = Player::TeamIdForRace(factionChangeInfo->Race);
    if (factionChangeInfo->FactionChange == (Player::TeamIdForRace(oldRace) == newTeam))
    {
        SendCharFactionChange(factionChangeInfo->FactionChange ? CHAR_CREATE_CHARACTER_SWAP_FACTION : CHAR_CREATE_CHARACTER_RACE_ONLY, factionChangeInfo.get());
        return;
    }

    uint32 maxMoney = sWorld->getIntConfig(CONFIG_CHANGE_FACTION_MAX_MONEY);
    if (maxMoney && money > maxMoney)
    {
        SendCharFactionChange(CHAR_CREATE_CHARACTER_GOLD_LIMIT, factionChangeInfo.get());
        return;
    }

    // pussywizard: check titles here to prevent return while building queries
    const uint32 ktcount = KNOWN_TITLES_SIZE * 2;
    std::vector<std::string_view> tokens = Acore::Tokenize(knownTitlesStr, ' ', false);

    if (factionChangeInfo->FactionChange && tokens.size() != ktcount)
    {
        SendCharFactionChange(CHAR_CREATE_ERROR, factionChangeInfo.get());
        return;
    }

    if (!HasPermission(rbac::RBAC_PERM_SKIP_CHECK_CHARACTER_CREATION_RACEMASK))
    {
        uint32 raceMaskDisabled = sWorld->getIntConfig(CONFIG_CHARACTER_CREATING_DISABLED_RACEMASK);
        if ((1 << (factionChangeInfo->Race - 1)) & raceMaskDisabled)
        {
            SendCharFactionChange(CHAR_CREATE_ERROR, factionChangeInfo.get());
            return;
        }
    }

    // prevent character rename to invalid name
    if (!normalizePlayerName(factionChangeInfo->Name))
    {
        SendCharFactionChange(CHAR_NAME_NO_NAME, factionChangeInfo.get());
        return;
    }

    ResponseCodes res = static_cast<ResponseCodes>(ObjectMgr::CheckPlayerName(factionChangeInfo->Name, true));
    if (res != CHAR_NAME_SUCCESS)
    {
        if (res != CHAR_NAME_RESERVED || !HasPermission(rbac::RBAC_PERM_SKIP_CHECK_CHARACTER_CREATION_RESERVEDNAME))
        {
            SendCharFactionChange(res, factionChangeInfo.get());
            return;
        }
    }

    // character with this name already exist
    if (ObjectGuid newguid = sCharacterCache->GetCharacterGuidByName(factionChangeInfo->Name))
    {
        if (newguid != factionChangeInfo->Guid)
        {
            SendCharFactionChange(CHAR_CREATE_NAME_IN_USE, factionChangeInfo.get());
            return;
        }
    }

    CharacterDatabasePreparedStatement* stmt = nullptr;
    CharacterDatabaseTransaction trans = CharacterDatabase.BeginTransaction();

    // resurrect the character in case he's dead
    Player::OfflineResurrect(factionChangeInfo->Guid, trans);

    // Name Change and update atLogin flags
    {
        CharacterDatabase.EscapeString(factionChangeInfo->Name);

        stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_CHAR_NAME_AT_LOGIN);
        stmt->SetData(0, factionChangeInfo->Name);
        stmt->SetData(1, uint16((atLoginFlags | AT_LOGIN_RESURRECT) & ~usedLoginFlag));
        stmt->SetData(2, lowGuid);
        trans->Append(stmt);

        stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_CHAR_DECLINED_NAME);
        stmt->SetData(0, lowGuid);
        trans->Append(stmt);
    }

    // Customize
    Player::Customize(factionChangeInfo.get(), trans);

    // Race Change
    {
        stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_CHAR_RACE);
        stmt->SetData(0, factionChangeInfo->Race);
        stmt->SetData(1, lowGuid);
        trans->Append(stmt);
    }

    LOG_INFO("entities.player.character", "Account: {} (IP: {}), Character [{}] (guid: {}) Changed Race/Faction to: {}",
        GetAccountId(), GetRemoteAddress(), playerData->Name, lowGuid, factionChangeInfo->Name);

    // xinef: update global data
    sCharacterCache->UpdateCharacterData(factionChangeInfo->Guid, factionChangeInfo->Name, factionChangeInfo->Gender, factionChangeInfo->Race);

    if (oldRace != factionChangeInfo->Race)
    {
        // Switch Languages
        // delete all languages first
        stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_CHAR_SKILL_LANGUAGES);
        stmt->SetData(0, lowGuid);
        trans->Append(stmt);

        // Now add them back
        stmt = CharacterDatabase.GetPreparedStatement(CHAR_INS_CHAR_SKILL_LANGUAGE);
        stmt->SetData(0, lowGuid);

        // Faction specific languages
        if (newTeam == TEAM_HORDE)
            stmt->SetData(1, 109);
        else
            stmt->SetData(1, 98);

        trans->Append(stmt);

        // Race specific languages
        if (factionChangeInfo->Race != RACE_ORC && factionChangeInfo->Race != RACE_HUMAN)
        {
            stmt = CharacterDatabase.GetPreparedStatement(CHAR_INS_CHAR_SKILL_LANGUAGE);
            stmt->SetData(0, lowGuid);

            switch (factionChangeInfo->Race)
            {
            case RACE_DWARF:
                stmt->SetData(1, 111);
                break;
            case RACE_DRAENEI:
                stmt->SetData(1, 759);
                break;
            case RACE_GNOME:
                stmt->SetData(1, 313);
                break;
            case RACE_NIGHTELF:
                stmt->SetData(1, 113);
                break;
            case RACE_UNDEAD_PLAYER:
                stmt->SetData(1, 673);
                break;
            case RACE_TAUREN:
                stmt->SetData(1, 115);
                break;
            case RACE_TROLL:
                stmt->SetData(1, 315);
                break;
            case RACE_BLOODELF:
                stmt->SetData(1, 137);
                break;
            }

            trans->Append(stmt);
        }

        if (factionChangeInfo->FactionChange)
        {
            {
                // Delete all Flypaths
                stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_CHAR_TAXI_PATH);
                stmt->SetData(0, lowGuid);
                trans->Append(stmt);

                // Update Taxi path: the new faction's nodes (TC 6.x), death knights keep Acherus
                std::ostringstream taximaskstream;
                if (level > 7)
                {
                    TaxiMask const& factionMask = newTeam == TEAM_HORDE ? sHordeTaxiNodesMask : sAllianceTaxiNodesMask;
                    for (uint8 i = 0; i < TaxiMaskSize; ++i)
                    {
                        // node 315: i = (315 - 1) / 8 = 39, m = 1 << ((315 - 1) % 8) = 4
                        uint8 deathKnightExtraNode = playerClass != CLASS_DEATH_KNIGHT || i != 39 ? 0 : 4;
                        taximaskstream << uint32(factionMask[i] | deathKnightExtraNode) << ' ';
                    }
                }
                else
                {
                    for (uint8 i = 0; i < TaxiMaskSize; ++i)
                        taximaskstream << "0 ";
                }

                stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_CHAR_TAXIMASK);
                stmt->SetData(0, taximaskstream.str());
                stmt->SetData(1, lowGuid);
                trans->Append(stmt);
            }

            // Reset guild
            if (!sWorld->getBoolConfig(CONFIG_ALLOW_TWO_SIDE_INTERACTION_GUILD))
            {
                if (uint32 guildId = playerData->GuildId)
                    if (Guild* guild = sGuildMgr->GetGuildById(guildId))
                        guild->DeleteMember(factionChangeInfo->Guid, false, false, true);
            }

            if (!HasPermission(rbac::RBAC_PERM_TWO_SIDE_ADD_FRIEND))
            {
                // Delete Friend List
                stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_CHAR_SOCIAL_BY_GUID);
                stmt->SetData(0, lowGuid);
                trans->Append(stmt);

                stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_CHAR_SOCIAL_BY_FRIEND);
                stmt->SetData(0, lowGuid);
                trans->Append(stmt);
            }

            // Leave Arena Teams
            if (!sWorld->getBoolConfig(CONFIG_ALLOW_TWO_SIDE_INTERACTION_ARENA))
                Player::LeaveAllArenaTeams(factionChangeInfo->Guid);

            // Reset homebind and position
            stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_PLAYER_HOMEBIND);
            stmt->SetData(0, lowGuid);
            trans->Append(stmt);

            stmt = CharacterDatabase.GetPreparedStatement(CHAR_INS_PLAYER_HOMEBIND);
            stmt->SetData(0, lowGuid);

            WorldLocation loc;
            uint16 zoneId = 0;

            if (newTeam == TEAM_ALLIANCE)
            {
                loc.WorldRelocate(MAP_EASTERN_KINGDOMS, -8867.68f, 673.373f, 97.9034f, 0.0f);
                zoneId = AREA_STORMWIND_CITY;
            }
            else
            {
                loc.WorldRelocate(MAP_KALIMDOR, 1633.33f, -4439.11f, 15.7588f, 0.0f);
                zoneId = AREA_ORGRIMMAR;
            }

            stmt->SetData(1, loc.GetMapId());
            stmt->SetData(2, zoneId);
            stmt->SetData(3, loc.GetPositionX());
            stmt->SetData(4, loc.GetPositionY());
            stmt->SetData(5, loc.GetPositionZ());
            trans->Append(stmt);

            Player::SavePositionInDB(loc, zoneId, factionChangeInfo->Guid, trans);

            // Achievement conversion
            for (auto const& [achiev_alliance, achiev_horde] : sObjectMgr->FactionChangeAchievements)
            {
                stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_CHAR_ACHIEVEMENT_BY_ACHIEVEMENT);
                stmt->SetData(0, uint16(newTeam == TEAM_ALLIANCE ? achiev_alliance : achiev_horde));
                stmt->SetData(1, lowGuid);
                trans->Append(stmt);

                stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_CHAR_ACHIEVEMENT);
                stmt->SetData(0, uint16(newTeam == TEAM_ALLIANCE ? achiev_alliance : achiev_horde));
                stmt->SetData(1, uint16(newTeam == TEAM_ALLIANCE ? achiev_horde : achiev_alliance));
                stmt->SetData(2, lowGuid);
                trans->Append(stmt);
            }

            // Item conversion
            for (auto const& [item_alliance, item_horde] : sObjectMgr->FactionChangeItems)
            {
                uint32 new_entry = (newTeam == TEAM_ALLIANCE ? item_alliance : item_horde);
                uint32 old_entry = (newTeam == TEAM_ALLIANCE ? item_horde : item_alliance);

                if (old_entry == 45978 /*Solid Gold Coin*/ || old_entry == 2589 /*Linen Cloth*/ || old_entry == 5976 /*Guild Tabard*/)
                    continue;

                stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_CHAR_INVENTORY_FACTION_CHANGE);
                stmt->SetData(0, new_entry);
                stmt->SetData(1, old_entry);
                stmt->SetData(2, lowGuid);
                trans->Append(stmt);
            }

            // Delete all current quests
            stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_CHAR_QUESTSTATUS);
            stmt->SetData(0, lowGuid);
            trans->Append(stmt);

            stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_CHAR_QUESTSTATUS_OBJECTIVES);
            stmt->SetData(0, lowGuid);
            trans->Append(stmt);

            // Quest conversion
            for (auto const& [quest_alliance, quest_horde] : sObjectMgr->FactionChangeQuests)
            {
                stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_CHAR_QUESTSTATUS_REWARDED_BY_QUEST);
                stmt->SetData(0, lowGuid);
                stmt->SetData(1, (newTeam == TEAM_ALLIANCE ? quest_alliance : quest_horde));
                trans->Append(stmt);

                stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_CHAR_QUESTSTATUS_REWARDED_FACTION_CHANGE);
                stmt->SetData(0, (newTeam == TEAM_ALLIANCE ? quest_alliance : quest_horde));
                stmt->SetData(1, (newTeam == TEAM_ALLIANCE ? quest_horde : quest_alliance));
                stmt->SetData(2, lowGuid);
                trans->Append(stmt);
            }

            // Mark all rewarded quests as "active" (will count for completed quests achievements)
            stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_CHAR_QUESTSTATUS_REWARDED_ACTIVE);
            stmt->SetData(0, lowGuid);
            trans->Append(stmt);

            // Disable all old-faction specific quests
            for (auto const& [questID, quest] : sObjectMgr->GetQuestTemplates())
            {
                uint32 newRaceMask = (newTeam == TEAM_ALLIANCE) ? sRaceMgr->GetAllianceRaceMask() : sRaceMgr->GetHordeRaceMask();

                if (quest->GetAllowableRaces() && !(quest->GetAllowableRaces() & newRaceMask))
                {
                    stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_CHAR_QUESTSTATUS_REWARDED_ACTIVE_BY_QUEST);
                    stmt->SetData(0, quest->GetQuestId());
                    stmt->SetData(1, lowGuid);
                    trans->Append(stmt);
                }
            }

            // Spell conversion
            for (auto const& [spell_alliance, spell_horde] : sObjectMgr->FactionChangeSpells)
            {
                stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_CHAR_SPELL_BY_SPELL);
                stmt->SetData(0, lowGuid);
                stmt->SetData(1, (newTeam == TEAM_ALLIANCE ? spell_alliance : spell_horde));
                trans->Append(stmt);

                stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_CHAR_SPELL_FACTION_CHANGE);
                stmt->SetData(0, (newTeam == TEAM_ALLIANCE ? spell_alliance : spell_horde));
                stmt->SetData(1, (newTeam == TEAM_ALLIANCE ? spell_horde : spell_alliance));
                stmt->SetData(2, lowGuid);
                trans->Append(stmt);
            }

            // Reputation conversion
            for (auto const& [reputation_alliance, reputation_horde] : sObjectMgr->FactionChangeReputation)
            {
                uint32 newReputation = (newTeam == TEAM_ALLIANCE) ? reputation_alliance : reputation_horde;
                uint32 oldReputation = (newTeam == TEAM_ALLIANCE) ? reputation_horde : reputation_alliance;

                // select old standing set in db
                stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHAR_REP_BY_FACTION);
                stmt->SetData(0, oldReputation);
                stmt->SetData(1, lowGuid);

                PreparedQueryResult result = CharacterDatabase.Query(stmt);
                if (!result)
                    continue;

                fields = result->Fetch();
                int32 oldDBRep = fields[0].Get<int32>();
                FactionEntry const* factionEntry = sFactionStore.LookupEntry(oldReputation);

                // old base reputation
                int32 oldBaseRep = sObjectMgr->GetBaseReputationOf(factionEntry, oldRace, playerClass);

                // new base reputation
                int32 newBaseRep = sObjectMgr->GetBaseReputationOf(sFactionStore.LookupEntry(newReputation), factionChangeInfo->Race, playerClass);

                // final reputation shouldnt change
                int32 FinalRep = oldDBRep + oldBaseRep;
                int32 newDBRep = FinalRep - newBaseRep;

                stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_CHAR_REP_BY_FACTION);
                stmt->SetData(0, newReputation);
                stmt->SetData(1, lowGuid);
                trans->Append(stmt);

                stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_CHAR_REP_FACTION_CHANGE);
                stmt->SetData(0, uint16(newReputation));
                stmt->SetData(1, newDBRep);
                stmt->SetData(2, uint16(oldReputation));
                stmt->SetData(3, lowGuid);
                trans->Append(stmt);
            }

            // Title conversion
            if (!knownTitlesStr.empty())
            {
                std::array<uint32, KNOWN_TITLES_SIZE * 2> knownTitles;

                for (uint32 index = 0; index < knownTitles.size(); ++index)
                {
                    Optional<uint32> thisMask;
                    if (index < tokens.size())
                        thisMask = Acore::StringTo<uint32>(tokens[index]);

                    if (thisMask)
                        knownTitles[index] = *thisMask;
                    else
                    {
                        LOG_WARN("entities.player", "{} has invalid title data '{}' at index {} - skipped, this may result in titles being lost",
                            GetPlayerInfo(), (index < tokens.size()) ? std::string(tokens[index]) : "<none>", index);

                        knownTitles[index] = 0;
                    }
                }

                for (auto const& [title_alliance, title_horde] : sObjectMgr->FactionChangeTitles)
                {
                    CharTitlesEntry const* atitleInfo = sCharTitlesStore.LookupEntry(title_alliance);
                    CharTitlesEntry const* htitleInfo = sCharTitlesStore.LookupEntry(title_horde);

                    // new team
                    if (newTeam == TEAM_ALLIANCE)
                    {
                        uint32 bitIndex = htitleInfo->MaskID;
                        uint32 index = bitIndex / 32;
                        uint32 old_flag = 1 << (bitIndex % 32);
                        uint32 new_flag = 1 << (atitleInfo->MaskID % 32);

                        if (knownTitles[index] & old_flag)
                        {
                            knownTitles[index] &= ~old_flag;
                            // use index of the new title
                            knownTitles[atitleInfo->MaskID / 32] |= new_flag;
                        }
                    }
                    else
                    {
                        uint32 bitIndex = atitleInfo->MaskID;
                        uint32 index = bitIndex / 32;
                        uint32 old_flag = 1 << (bitIndex % 32);
                        uint32 new_flag = 1 << (htitleInfo->MaskID % 32);

                        if (knownTitles[index] & old_flag)
                        {
                            knownTitles[index] &= ~old_flag;
                            // use index of the new title
                            knownTitles[htitleInfo->MaskID / 32] |= new_flag;
                        }
                    }

                    std::ostringstream ss;
                    for (uint32 mask : knownTitles)
                        ss << mask << ' ';

                    stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_CHAR_TITLES_FACTION_CHANGE);
                    stmt->SetData(0, ss.str().c_str());
                    stmt->SetData(1, lowGuid);
                    trans->Append(stmt);

                    // unset any currently chosen title
                    stmt = CharacterDatabase.GetPreparedStatement(CHAR_RES_CHAR_TITLES_FACTION_CHANGE);
                    stmt->SetData(0, lowGuid);
                    trans->Append(stmt);
                }
            }
        }
    }

    // Re-check all achievement criterias
    stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_ADD_AT_LOGIN_FLAG);
    stmt->SetData(0, uint16(AT_LOGIN_CHECK_ACHIEVS));
    stmt->SetData(1, lowGuid);
    trans->Append(stmt);

    CharacterDatabase.CommitTransaction(trans);

    LOG_DEBUG("entities.player", "{} (IP: {}) changed race from {} to {}", GetPlayerInfo(), GetRemoteAddress(), oldRace, factionChangeInfo->Race);

    SendCharFactionChange(RESPONSE_SUCCESS, factionChangeInfo.get());
}

void WorldSession::SendCharCreate(ResponseCodes result)
{
    WorldPackets::Character::CreateChar response;
    response.Code = result;
    SendPacket(response.Write());
}

void WorldSession::SendCharDelete(ResponseCodes result)
{
    WorldPackets::Character::DeleteChar response;
    response.Code = result;
    SendPacket(response.Write());
}

void WorldSession::SendCharLoginFailed(LoginFailureReason reason)
{
    WorldPackets::Character::CharacterLoginFailed loginFailed(reason);
    SendPacket(loginFailed.Write());
}

void WorldSession::SendCharRename(ResponseCodes result, CharacterRenameInfo const* renameInfo)
{
    WorldPackets::Character::CharacterRenameResult packet;
    packet.Result = result;
    packet.Name = renameInfo->Name;
    if (result == RESPONSE_SUCCESS)
        packet.Guid = renameInfo->Guid;

    SendPacket(packet.Write());
}

void WorldSession::SendCharFactionChange(ResponseCodes result, CharacterFactionChangeInfo const* factionChangeInfo)
{
    WorldPackets::Character::CharFactionChangeResult packet;
    packet.Result = result;
    packet.Guid = factionChangeInfo->Guid;

    if (result == RESPONSE_SUCCESS)
    {
        packet.Display.emplace();
        packet.Display->Name = factionChangeInfo->Name;
        packet.Display->SexID = factionChangeInfo->Gender;
        packet.Display->SkinID = factionChangeInfo->Skin;
        packet.Display->HairColorID = factionChangeInfo->HairColor;
        packet.Display->HairStyleID = factionChangeInfo->HairStyle;
        packet.Display->FacialHairStyleID = factionChangeInfo->FacialHair;
        packet.Display->FaceID = factionChangeInfo->Face;
        packet.Display->RaceID = factionChangeInfo->Race;
    }

    SendPacket(packet.Write());
}

void WorldSession::SendCharCustomize(ResponseCodes result, CharacterCustomizeInfo const* customizeInfo)
{
    if (result == RESPONSE_SUCCESS)
    {
        WorldPackets::Character::CharCustomizeResponse response;
        response.CharGUID = customizeInfo->Guid;
        response.CharName = customizeInfo->Name;
        response.SexID = customizeInfo->Gender;
        response.SkinID = customizeInfo->Skin;
        response.HairColorID = customizeInfo->HairColor;
        response.HairStyleID = customizeInfo->HairStyle;
        response.FacialHairStyleID = customizeInfo->FacialHair;
        response.FaceID = customizeInfo->Face;
        SendPacket(response.Write());
    }
    else
    {
        WorldPackets::Character::CharCustomizeFailed failed;
        failed.Result = uint8(result);
        failed.CharGUID = customizeInfo->Guid;
        SendPacket(failed.Write());
    }
}

void WorldSession::SendSetPlayerDeclinedNamesResult(DeclinedNameResult result, ObjectGuid guid)
{
    WorldPackets::Character::SetPlayerDeclinedNamesResult packet;
    packet.ResultCode = result;
    packet.Player = guid;
    SendPacket(packet.Write());
}

void WorldSession::HandleReorderCharacters(WorldPackets::Character::ReorderCharacters& reorderChars)
{
    CharacterDatabaseTransaction trans = CharacterDatabase.BeginTransaction();
    for (WorldPackets::Character::ReorderCharacters::ReorderInfo const& reorderInfo : reorderChars.Entries)
    {
        // the account check keeps a client from reordering characters of another account
        CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_CHAR_LIST_ORDER);
        stmt->SetData(0, reorderInfo.NewPosition);
        stmt->SetData(1, reorderInfo.PlayerGUID.GetCounter());
        stmt->SetData(2, GetAccountId());
        trans->Append(stmt);
    }

    CharacterDatabase.CommitTransaction(trans);
}

void WorldSession::HandleRandomizeCharNameOpcode(WorldPackets::Character::GenerateRandomCharacterName& packet)
{
    if (!sChrRacesStore.LookupEntry(packet.Race))
    {
        LOG_ERROR("network", "Invalid race ({}) sent by accountId: {}", packet.Race, GetAccountId());
        return;
    }

    if (!Player::IsValidGender(packet.Sex))
    {
        LOG_ERROR("network", "Invalid gender ({}) sent by accountId: {}", packet.Sex, GetAccountId());
        return;
    }

    WorldPackets::Character::GenerateRandomCharacterNameResult result;
    result.Name = GetRandomCharacterName(packet.Race, packet.Sex);
    result.Success = !result.Name.empty();
    SendPacket(result.Write());
}
