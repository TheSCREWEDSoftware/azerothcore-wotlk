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

#include "Common.h"
#include "Corpse.h"
#include "GameTime.h"
#include "Log.h"
#include "MapMgr.h"
#include "NPCHandler.h"
#include "ObjectMgr.h"
#include "Opcodes.h"
#include "Pet.h"
#include "Player.h"
#include "QueryPackets.h"
#include "Realm.h"
#include "World.h"
#include "WorldPacket.h"
#include "WorldSession.h"

void WorldSession::SendNameQueryOpcode(ObjectGuid guid)
{
    CharacterCacheEntry const* playerData = sCharacterCache->GetCharacterCacheByGuid(guid);

    WorldPackets::Query::NameQueryResponse nameQueryResponse;
    nameQueryResponse.Guid = guid;
    if (!playerData)
    {
        nameQueryResponse.NameUnknown = true;
        SendPacket(nameQueryResponse.Write());
        return;
    }

    Player* player = ObjectAccessor::FindConnectedPlayer(guid);

    nameQueryResponse.NameUnknown = false;
    nameQueryResponse.Name = playerData->Name;
    nameQueryResponse.VirtualRealmAddress = realm.Id.Realm;
    nameQueryResponse.AccountID = playerData->AccountId;
    nameQueryResponse.Race = player ? player->getRace() : playerData->Race;
    nameQueryResponse.Sex = player ? player->getGender() : playerData->Sex;
    nameQueryResponse.Class = player ? player->getClass() : playerData->Class;
    nameQueryResponse.Level = player ? player->GetLevel() : playerData->Level;

    if (DeclinedName const* names = (player ? player->GetDeclinedNames() : nullptr))
    {
        nameQueryResponse.Declined = true;
        nameQueryResponse.DeclinedNames = *names;
    }
    else
        nameQueryResponse.Declined = false;

    SendPacket(nameQueryResponse.Write());
}

void WorldSession::HandleNameQueryOpcode(WorldPackets::Query::NameQuery& packet)
{
    // This is disable by default to prevent lots of console spam
    // LOG_INFO("network.opcode", "HandleNameQueryOpcode {}", guid);

    SendNameQueryOpcode(packet.Guid);
}

void WorldSession::HandleTimeQueryOpcode(WorldPackets::Query::TimeQuery& /*packet*/)
{
    SendTimeQueryResponse();
}

// 5.4.8 18414, from Project SkyFire (WorldSession::SendRealmNameQueryOpcode)
void WorldSession::HandleRealmNameQueryOpcode(WorldPacket& recvData)
{
    uint32 realmId;
    recvData >> realmId;

    bool const found = realmId == realm.Id.Realm;

    WorldPacket data(SMSG_REALM_NAME_QUERY_RESPONSE, 1 + 4 + 2 + realm.Name.size() * 2);
    data << uint8(!found);
    data << uint32(realmId);

    if (found)
    {
        data.WriteBits(realm.Name.size(), 8);
        data.WriteBit(true);                                // this is the player's realm
        data.WriteBits(realm.Name.size(), 8);
        data.FlushBits();
        data.WriteString(realm.Name);
        data.WriteString(realm.Name);
    }

    SendPacket(&data);
}

void WorldSession::SendTimeQueryResponse()
{
    auto timeResponse = sWorld->GetNextDailyQuestsResetTime() - GameTime::GetGameTime();

    WorldPackets::Query::TimeQueryResponse timeQueryResponse;
    timeQueryResponse.ServerTime = GameTime::GetGameTime().count();
    timeQueryResponse.TimeResponse = timeResponse.count();
    SendPacket(timeQueryResponse.Write());
}

/// Only _static_ data is sent in this packet !!!
void WorldSession::HandleCreatureQueryOpcode(WorldPacket& recvData)
{
    // 5.4.8 sends only the entry
    uint32 entry;
    recvData >> entry;

    CreatureTemplate const* ci = sObjectMgr->GetCreatureTemplate(entry);
    WorldPackets::Query::QueryCreatureResponse response;
    response.CreatureID = entry;

    if (ci)
    {
        std::string Name, Title;
        Name = ci->Name;
        Title = ci->SubName;

        LocaleConstant loc_idx = GetSessionDbLocaleIndex();
        if (loc_idx >= 0)
        {
            if (CreatureLocale const* cl = sObjectMgr->GetCreatureLocale(entry))
            {
                ObjectMgr::GetLocaleString(cl->Name, loc_idx, Name);
                ObjectMgr::GetLocaleString(cl->Title, loc_idx, Title);
            }
        }

        response.Allow = true;
        response.Stats.Name[0] = Name;
        // NameAlt[0..3] left empty: this fork does not yet port a per-gender/locale alternate name column
        response.Stats.Title = Title;
        response.Stats.CursorName = ci->IconName;                    // "Directions" for guard, string for Icons 2.3.0
        response.Stats.Flags[0] = ci->type_flags;
        // Flags[1] left zero: this fork does not yet port a second creature type-flags column
        response.Stats.CreatureType = ci->type;                      // CreatureType.dbc
        response.Stats.CreatureFamily = ci->family;                  // CreatureFamily.dbc
        response.Stats.Classification = ci->rank;                    // Creature Rank (elite, boss, etc)
        response.Stats.ProxyCreatureID[0] = ci->KillCredit[0];
        response.Stats.ProxyCreatureID[1] = ci->KillCredit[1];

        for (std::size_t i = 0; i < response.Stats.CreatureDisplayID.size(); ++i)
        {
            if (CreatureModel const* model = ci->GetModelByIdx(i))
                response.Stats.CreatureDisplayID[i] = model->CreatureDisplayID;
        }

        response.Stats.HpMulti = ci->ModHealth;                      // dmg/hp modifier
        response.Stats.EnergyMulti = ci->ModMana;                    // dmg/mana modifier
        response.Stats.Leader = ci->RacialLeader;

        if (CreatureQuestItemList const* items = sObjectMgr->GetCreatureQuestItemList(entry))
            for (std::size_t i = 0; i < MAX_CREATURE_QUEST_ITEMS && i < items->size(); ++i)
                response.Stats.QuestItems[i] = (*items)[i];

        response.Stats.CreatureMovementInfoID = ci->movementId;      // CreatureMovementInfo.dbc
        response.Stats.RequiredExpansion = ci->expansion;
        SendPacket(response.Write());
    }
    else
    {
        LOG_DEBUG("network", "WORLD: CMSG_CREATURE_QUERY - NO CREATURE INFO! (entry {})", entry);
        SendPacket(response.Write());
        LOG_DEBUG("network", "WORLD: Sent SMSG_CREATURE_QUERY_RESPONSE");
    }
}

/// Only _static_ data is sent in this packet !!!
void WorldSession::HandleGameObjectQueryOpcode(WorldPacket& recvData)
{
    // 5.4.8 18414, from Project SkyFire (WorldSession::HandleGameObjectQueryOpcode)
    uint32 entry;
    ObjectGuid guid;
    recvData >> entry;
    for (uint8 index : { 5, 3, 6, 2, 7, 1, 0, 4 })
        guid[index] = recvData.ReadBit();
    for (uint8 index : { 1, 5, 3, 4, 6, 2, 7, 0 })
        recvData.ReadByteSeq(guid[index]);

    GameObjectTemplate const* info = sObjectMgr->GetGameObjectTemplate(entry);

    WorldPacket data(SMSG_GAMEOBJECT_QUERY_RESPONSE, 150);
    data.WriteBit(info != nullptr);
    data.FlushBits();
    data << uint32(entry);

    size_t const sizePos = data.wpos();
    data << uint32(0);

    if (info)
    {
        std::string Name = info->name;
        std::string IconName = info->IconName;
        std::string CastBarCaption = info->castBarCaption;

        LocaleConstant localeConstant = GetSessionDbLocaleIndex();
        if (localeConstant >= LOCALE_enUS)
            if (GameObjectLocale const* gameObjectLocale = sObjectMgr->GetGameObjectLocale(entry))
            {
                ObjectMgr::GetLocaleString(gameObjectLocale->Name, localeConstant, Name);
                ObjectMgr::GetLocaleString(gameObjectLocale->CastBarCaption, localeConstant, CastBarCaption);
            }

        LOG_DEBUG("network", "WORLD: CMSG_GAMEOBJECT_QUERY '{}' - Entry: {}. ", info->name, entry);
        data << uint32(info->type);
        data << uint32(info->displayId);
        data << Name;
        data << uint8(0) << uint8(0) << uint8(0);           // name2, name3, name4
        data << IconName;                                   // Icon name to use instead of default icon for go's (ex: "Attack" makes sword)
        data << CastBarCaption;                             // Text will appear in Cast Bar when using GO (ex: "Collecting")
        data << info->unk1;
        data.append(info->raw.data, MAX_GAMEOBJECT_DATA);
        data << float(info->size);                          // go size

        data << uint8(MAX_GAMEOBJECT_QUEST_ITEMS);
        GameObjectQuestItemList const* items = sObjectMgr->GetGameObjectQuestItemList(entry);
        for (std::size_t i = 0; i < MAX_GAMEOBJECT_QUEST_ITEMS; ++i)
            data << uint32(items && i < items->size() ? (*items)[i] : 0);

        data << int32(0);                                   // RequiredLevel, not imported yet
        data.put<uint32>(sizePos, uint32(data.wpos() - (sizePos + 4)));
    }
    else
        LOG_DEBUG("network", "WORLD: CMSG_GAMEOBJECT_QUERY - Missing gameobject info for ({}, entry {})", guid.ToString(), entry);

    SendPacket(&data);
}

void WorldSession::HandleCorpseQueryOpcode(WorldPacket& /*recvData*/)
{
    // 5.4.8 18414, from Project SkyFire (WorldSession::HandleCorpseQueryOpcode)
    Corpse* corpse = _player->GetCorpse();
    if (!corpse)
    {
        WorldPacket data(SMSG_CORPSE_QUERY, 1 + 5 * 4);
        data.WriteBits(0, 9);                               // not found, no guid bytes
        data.FlushBits();
        for (uint8 i = 0; i < 5; ++i)
            data << uint32(0);
        SendPacket(&data);
        return;
    }

    WorldLocation corpseLocation = _player->GetCorpseLocation();
    uint32 corpseMapID = corpseLocation.GetMapId();
    uint32 mapID = corpseLocation.GetMapId();
    float x = corpseLocation.GetPositionX();
    float y = corpseLocation.GetPositionY();
    float z = corpseLocation.GetPositionZ();

    // if corpse at different map
    if (mapID != _player->GetMapId())
    {
        // search entrance map for proper show entrance
        if (MapEntry const* corpseMapEntry = sMapStore.LookupEntry(mapID))
        {
            if (corpseMapEntry->IsDungeon() && corpseMapEntry->entrance_map >= 0)
            {
                // if corpse map have entrance
                if (Map const* entranceMap = sMapMgr->CreateBaseMap(corpseMapEntry->entrance_map))
                {
                    mapID = corpseMapEntry->entrance_map;
                    x = corpseMapEntry->entrance_x;
                    y = corpseMapEntry->entrance_y;
                    z = entranceMap->GetHeight(GetPlayer()->GetPhaseMask(), x, y, MAX_HEIGHT);
                }
            }
        }
    }

    ObjectGuid corpseGuid = corpse->GetGUID();

    WorldPacket data(SMSG_CORPSE_QUERY, 9 + 1 + 4 * 5);
    for (uint8 index : { 0, 3, 2 })
        data.WriteBit(corpseGuid[index]);
    data.WriteBit(true);                                    // corpse found
    for (uint8 index : { 5, 4, 1, 7, 6 })
        data.WriteBit(corpseGuid[index]);
    data.FlushBits();

    data.WriteByteSeq(corpseGuid[5]);
    data << float(z);
    data.WriteByteSeq(corpseGuid[1]);
    data << uint32(corpseMapID);
    data.WriteByteSeq(corpseGuid[6]);
    data.WriteByteSeq(corpseGuid[4]);
    data << float(x);
    data.WriteByteSeq(corpseGuid[3]);
    data.WriteByteSeq(corpseGuid[7]);
    data.WriteByteSeq(corpseGuid[2]);
    data.WriteByteSeq(corpseGuid[0]);
    data << int32(mapID);
    data << float(y);
    SendPacket(&data);
}

void WorldSession::HandleNpcTextQueryOpcode(WorldPacket& recvData)
{
    // 5.4.8 18414: the text itself comes from BroadcastText.db2, the server sends the
    // probability and broadcast text id of each option.
    uint32 textID;
    ObjectGuid guid;
    recvData >> textID;
    for (uint8 index : { 4, 5, 1, 7, 0, 2, 6, 3 })
        guid[index] = recvData.ReadBit();
    for (uint8 index : { 4, 0, 2, 5, 1, 7, 3, 6 })
        recvData.ReadByteSeq(guid[index]);

    LOG_DEBUG("network", "WORLD: CMSG_NPC_TEXT_QUERY TextId: {}", textID);

    GossipText const* gossip = sObjectMgr->GetGossipText(textID);

    ByteBuffer buffer(MAX_GOSSIP_TEXT_OPTIONS * 8);
    for (uint8 i = 0; i < MAX_GOSSIP_TEXT_OPTIONS; ++i)
        buffer << float(gossip ? gossip->Options[i].Probability : (i == 0 ? 1.0f : 0.0f));
    for (uint8 i = 0; i < MAX_GOSSIP_TEXT_OPTIONS; ++i)
    {
        uint32 broadcastTextId = 0;
        if (gossip)
        {
            broadcastTextId = gossip->Options[i].BroadcastTextID;
            if (!broadcastTextId && (!gossip->Options[i].Text_0.empty() || !gossip->Options[i].Text_1.empty()))
                broadcastTextId = MakeGossipBroadcastTextId(textID, i);
        }
        buffer << uint32(broadcastTextId);
    }

    WorldPacket data(SMSG_NPC_TEXT_UPDATE, 4 + 4 + buffer.size() + 1);
    data << uint32(textID);
    data << uint32(buffer.size());
    data.append(buffer);
    data.WriteBit(gossip != nullptr);                      // allow caching
    data.FlushBits();
    SendPacket(&data);

    LOG_DEBUG("network", "WORLD: Sent SMSG_NPC_TEXT_UPDATE");
}

/// Only _static_ data is sent in this packet !!!
void WorldSession::HandlePageTextQueryOpcode(WorldPacket& recvData)
{
    // 5.4.8 18414, from Project SkyFire (WorldSession::HandlePageTextQueryOpcode)
    uint32 pageID;
    ObjectGuid guid;
    recvData >> pageID;
    for (uint8 index : { 2, 1, 3, 7, 6, 4, 0, 5 })
        guid[index] = recvData.ReadBit();
    for (uint8 index : { 1, 6, 3, 5, 0, 7, 4, 2 })
        recvData.ReadByteSeq(guid[index]);

    while (pageID)
    {
        PageText const* pageText = sObjectMgr->GetPageText(pageID);

        WorldPacket data(SMSG_PAGE_TEXT_QUERY_RESPONSE, 50);
        data.WriteBit(pageText != nullptr);

        if (!pageText)
        {
            data.FlushBits();
            data << uint32(pageID);
            pageID = 0;
        }
        else
        {
            std::string Text = pageText->Text;

            int loc_idx = GetSessionDbLocaleIndex();
            if (loc_idx >= 0)
                if (PageTextLocale const* player = sObjectMgr->GetPageTextLocale(pageID))
                    ObjectMgr::GetLocaleString(player->Text, loc_idx, Text);

            data.WriteBits(Text.size(), 12);
            data.FlushBits();
            data << uint32(pageText->NextPage);
            data << uint32(pageID);
            data.WriteString(Text);
            data << uint32(pageID);
            pageID = pageText->NextPage;
        }

        SendPacket(&data);
        LOG_DEBUG("network", "WORLD: Sent SMSG_PAGE_TEXT_QUERY_RESPONSE");
    }
}

void WorldSession::HandleCorpseMapPositionQuery(WorldPackets::Query::CorpseMapPositionQuery& /*packet*/)
{
    LOG_DEBUG("network", "WORLD: Recv CMSG_CORPSE_MAP_POSITION_QUERY");

    WorldPacket data(SMSG_CORPSE_MAP_POSITION_QUERY_RESPONSE, 4 + 4 + 4 + 4);
    data << float(0);
    data << float(0);
    data << float(0);
    data << float(0);
    SendPacket(&data);
}

void WorldSession::HandleQuestNPCQuery(WorldPacket& recvData)
{
    // 5.4.8 18414: a fixed array of 50 quest ids followed by the used count
    std::array<uint32, 50> questList;
    for (uint32& questId : questList)
        recvData >> questId;

    uint32 count;
    recvData >> count;
    count = std::min<uint32>(count, questList.size());

    WorldPackets::Query::QuestNPCQueryResponse response;
    std::unordered_set<uint32> questIds;
    for (uint32 i = 0; i < count; ++i)
    {
        uint32 questId = questList[i];
        if (!questIds.insert(questId).second || !sObjectMgr->GetQuestTemplate(questId))
            continue;

        // Reuse the live relation maps so reloads cannot leave a reverse cache stale.
        for (auto const& [entry, quest] : *sObjectMgr->GetCreatureQuestInvolvedRelationMap())
            if (quest == questId)
                response.Quests[questId].push_back(entry);
        for (auto const& [entry, quest] : *sObjectMgr->GetGOQuestInvolvedRelationMap())
            if (quest == questId)
                response.Quests[questId].push_back(entry | 0x80000000);
    }
    SendPacket(response.Write());
}

void WorldSession::HandleQuestPOIQuery(WorldPacket& recvData)
{
    // 5.4.8 18414, from Project SkyFire (WorldSession::HandleQuestPOIQuery)
    uint32 count = recvData.ReadBits(22);
    if (count > MAX_QUEST_LOG_SIZE)
    {
        recvData.rfinish();
        return;
    }

    std::vector<uint32> questIds(count);
    for (uint32& questId : questIds)
        recvData >> questId;

    WorldPacket data(SMSG_QUEST_POI_QUERY_RESPONSE, 4 + (4 + 4) * count);
    ByteBuffer poiData;
    data.WriteBits(count, 20);

    for (uint32 questId : questIds)
    {
        bool questOk = false;
        uint16 questSlot = _player->FindQuestSlot(questId);
        if (questSlot != MAX_QUEST_LOG_SIZE)
            questOk = _player->GetQuestSlotQuestId(questSlot) == questId;

        QuestPOIVector const* POI = questOk ? sObjectMgr->GetQuestPOIVector(questId) : nullptr;
        if (!POI)
        {
            data.WriteBits(0, 18);
            poiData << uint32(questId);
            poiData << uint32(0);
            continue;
        }

        data.WriteBits(POI->size(), 18);
        for (QuestPOI const& poi : *POI)
        {
            data.WriteBits(poi.points.size(), 21);

            poiData << uint32(poi.FloorId);
            for (QuestPOIPoint const& point : poi.points)
            {
                poiData << int32(point.x);
                poiData << int32(point.y);
            }

            poiData << int32(poi.ObjectiveIndex);
            poiData << uint32(poi.Id);
            poiData << uint32(0);                           // unknown, new in 5.x
            poiData << uint32(0);                           // unknown, new in 5.x
            poiData << uint32(poi.MapId);
            poiData << uint32(poi.points.size());
            poiData << uint32(poi.AreaId);
            poiData << uint32(0);                           // unknown, new in 5.x
            poiData << uint32(poi.Unk4);
            poiData << uint32(poi.Unk3);
        }

        poiData << uint32(questId);
        poiData << uint32(POI->size());
    }

    poiData << uint32(count);

    data.FlushBits();
    data.append(poiData);
    SendPacket(&data);
}

namespace
{
    // DB2 table hashes, from Project SkyFire (WorldSession.h)
    constexpr uint32 DB2_REPLY_BROADCAST_TEXT = 35137211;
    constexpr uint32 DB2_REPLY_ITEM = 1344507586;
    constexpr uint32 DB2_REPLY_ITEM_SPARSE = 2442913102;

    void WriteDB2String(ByteBuffer& buffer, std::string const& text)
    {
        buffer << uint16(text.size());
        if (!text.empty())
            buffer.WriteString(text);
    }

    // Item.db2 record: id, class, subclass, sound override subclass, material, display id, inventory type, sheath
    void WriteItemRecord(ItemTemplate const* proto, ByteBuffer& buffer)
    {
        buffer << uint32(proto->ItemId);
        buffer << uint32(proto->Class);
        buffer << uint32(proto->SubClass);
        buffer << int32(proto->SoundOverrideSubclass);
        buffer << int32(proto->Material);
        buffer << uint32(proto->DisplayInfoID);
        buffer << uint32(proto->InventoryType);
        buffer << uint32(proto->Sheath);
    }

    // Item-sparse.db2 record, field order from Project SkyFire (ItemSparseEntry)
    void WriteItemSparseRecord(ItemTemplate const* proto, LocaleConstant locale, ByteBuffer& buffer)
    {
        std::string name = proto->Name1;
        std::string description = proto->Description;
        if (locale != DEFAULT_LOCALE)
            if (ItemLocale const* itemLocale = sObjectMgr->GetItemLocale(proto->ItemId))
            {
                ObjectMgr::GetLocaleString(itemLocale->Name, locale, name);
                ObjectMgr::GetLocaleString(itemLocale->Description, locale, description);
            }

        buffer << uint32(proto->ItemId);
        buffer << uint32(proto->Quality);
        buffer << uint32(proto->Flags);
        buffer << uint32(proto->Flags2);
        buffer << uint32(0);                                // Flags3
        buffer << float(proto->PriceRandomValue);
        buffer << float(proto->PriceVariance);
        buffer << uint32(proto->BuyCount);
        buffer << int32(proto->BuyPrice);
        buffer << uint32(proto->SellPrice);
        buffer << uint32(proto->InventoryType);
        buffer << int32(proto->AllowableClass);
        buffer << int32(proto->AllowableRace);
        buffer << uint32(proto->ItemLevel);
        buffer << int32(proto->RequiredLevel);
        buffer << uint32(proto->RequiredSkill);
        buffer << uint32(proto->RequiredSkillRank);
        buffer << uint32(proto->RequiredSpell);
        buffer << uint32(proto->RequiredHonorRank);
        buffer << uint32(proto->RequiredCityRank);
        buffer << uint32(proto->RequiredReputationFaction);
        buffer << uint32(proto->RequiredReputationRank);
        buffer << int32(proto->MaxCount);
        buffer << int32(proto->Stackable);
        buffer << uint32(proto->ContainerSlots);

        for (_ItemStat const& stat : proto->ItemStat)
            buffer << int32(stat.ItemStatType);
        for (_ItemStat const& stat : proto->ItemStat)
            buffer << int32(stat.ItemStatValue);
        for (uint8 i = 0; i < MAX_ITEM_PROTO_STATS; ++i)
            buffer << int32(0);                             // stat socket cost multiplier
        for (uint8 i = 0; i < MAX_ITEM_PROTO_STATS; ++i)
            buffer << int32(0);                             // stat percent of socket

        buffer << uint32(proto->ScalingStatDistribution);
        buffer << uint32(proto->Damage[0].DamageType);
        buffer << uint32(proto->Delay);
        buffer << float(proto->RangedModRange);

        for (_Spell const& spell : proto->Spells)
            buffer << int32(spell.SpellId);
        for (_Spell const& spell : proto->Spells)
            buffer << int32(spell.SpellTrigger);
        for (_Spell const& spell : proto->Spells)
            buffer << int32(spell.SpellCharges);
        for (_Spell const& spell : proto->Spells)
            buffer << int32(spell.SpellCooldown);
        for (_Spell const& spell : proto->Spells)
            buffer << int32(spell.SpellCategory);
        for (_Spell const& spell : proto->Spells)
            buffer << int32(spell.SpellCategoryCooldown);

        buffer << uint32(proto->Bonding);
        WriteDB2String(buffer, name);
        for (uint8 i = 0; i < 3; ++i)
            buffer << uint16(0);                            // Name2, Name3, Name4
        WriteDB2String(buffer, description);
        buffer << uint32(proto->PageText);
        buffer << uint32(proto->LanguageID);
        buffer << uint32(proto->PageMaterial);
        buffer << uint32(proto->StartQuest);
        buffer << uint32(proto->LockID);
        buffer << int32(proto->Material);
        buffer << uint32(proto->Sheath);
        buffer << int32(proto->RandomProperty);
        buffer << int32(proto->RandomSuffix);
        buffer << uint32(proto->ItemSet);
        buffer << uint32(proto->Area);
        buffer << uint32(proto->Map);
        buffer << uint32(proto->BagFamily);
        buffer << uint32(proto->TotemCategory);

        for (_Socket const& socket : proto->Socket)
            buffer << uint32(socket.Color);
        for (_Socket const& socket : proto->Socket)
            buffer << uint32(socket.Content);

        buffer << uint32(proto->socketBonus);
        buffer << uint32(proto->GemProperties);
        buffer << float(proto->ArmorDamageModifier);
        buffer << uint32(proto->Duration);
        buffer << uint32(proto->ItemLimitCategory);
        buffer << uint32(proto->HolidayId);
        buffer << float(0.0f);                              // StatScalingFactor
        buffer << int32(0);                                 // CurrencySubstitutionId
        buffer << int32(0);                                 // CurrencySubstitutionCount
    }

    // BroadcastText.db2 record: id, language, male text, female text, 3 emotes, 3 emote delays, sound, end emote, type
    void WriteBroadcastTextRecord(uint32 id, uint32 language, std::string const& maleText, std::string const& femaleText,
        BroadcastText const* broadcastText, ByteBuffer& buffer)
    {
        buffer << uint32(id);
        buffer << uint32(language);
        WriteDB2String(buffer, maleText);
        WriteDB2String(buffer, femaleText);
        buffer << uint32(broadcastText ? broadcastText->EmoteId1 : 0);
        buffer << uint32(broadcastText ? broadcastText->EmoteId2 : 0);
        buffer << uint32(broadcastText ? broadcastText->EmoteId3 : 0);
        buffer << uint32(broadcastText ? broadcastText->EmoteDelay1 : 0);
        buffer << uint32(broadcastText ? broadcastText->EmoteDelay2 : 0);
        buffer << uint32(broadcastText ? broadcastText->EmoteDelay3 : 0);
        buffer << uint32(broadcastText ? broadcastText->SoundEntriesId : 0);
        buffer << uint32(broadcastText ? broadcastText->EmotesID : 0);
        buffer << uint32(broadcastText ? broadcastText->Flags : 1);
    }

    bool WriteBroadcastText(uint32 entry, LocaleConstant locale, ByteBuffer& buffer)
    {
        if (BroadcastText const* broadcastText = sObjectMgr->GetBroadcastText(entry))
        {
            WriteBroadcastTextRecord(entry, broadcastText->LanguageID, broadcastText->GetText(locale, GENDER_MALE, true),
                broadcastText->GetText(locale, GENDER_FEMALE, true), broadcastText, buffer);
            return true;
        }

        if (!(entry & GOSSIP_BROADCAST_TEXT_FLAG))
            return false;

        uint32 textId = (entry & ~GOSSIP_BROADCAST_TEXT_FLAG) >> 3;
        uint8 option = entry & 7;
        GossipText const* gossip = sObjectMgr->GetGossipText(textId);
        if (!gossip)
            return false;

        std::string maleText = gossip->Options[option].Text_0;
        std::string femaleText = gossip->Options[option].Text_1;
        if (locale != DEFAULT_LOCALE)
            if (NpcTextLocale const* npcTextLocale = sObjectMgr->GetNpcTextLocale(textId))
            {
                ObjectMgr::GetLocaleString(npcTextLocale->Text_0[option], locale, maleText);
                ObjectMgr::GetLocaleString(npcTextLocale->Text_1[option], locale, femaleText);
            }

        if (maleText.empty())
            maleText = femaleText;
        if (femaleText.empty())
            femaleText = maleText;

        WriteBroadcastTextRecord(entry, gossip->Options[option].Language, maleText, femaleText, nullptr, buffer);
        return true;
    }
}

// 5.4.8 18414, request from Project SkyFire (WorldSession::HandleRequestHotfix)
void WorldSession::HandleRequestHotfix(WorldPacket& recvData)
{
    uint32 type;
    recvData >> type;

    uint32 count = recvData.ReadBits(21);
    if (count > 2000)
    {
        recvData.rfinish();
        return;
    }

    std::vector<ObjectGuid> guids(count);
    for (ObjectGuid& guid : guids)
        for (uint8 index : { 6, 3, 0, 1, 4, 5, 7, 2 })
            guid[index] = recvData.ReadBit();

    std::vector<uint32> entries(count);
    for (uint32 i = 0; i < count; ++i)
    {
        recvData.ReadByteSeq(guids[i][1]);
        recvData >> entries[i];
        for (uint8 index : { 0, 5, 6, 4, 7, 2, 3 })
            recvData.ReadByteSeq(guids[i][index]);
    }

    LocaleConstant const locale = GetSessionDbLocaleIndex();
    for (uint32 entry : entries)
    {
        ByteBuffer record;
        bool found = false;
        switch (type)
        {
            case DB2_REPLY_ITEM:
                if (ItemTemplate const* proto = sObjectMgr->GetItemTemplate(entry))
                {
                    WriteItemRecord(proto, record);
                    found = true;
                }
                break;
            case DB2_REPLY_ITEM_SPARSE:
                if (ItemTemplate const* proto = sObjectMgr->GetItemTemplate(entry))
                {
                    WriteItemSparseRecord(proto, locale, record);
                    found = true;
                }
                break;
            case DB2_REPLY_BROADCAST_TEXT:
                found = WriteBroadcastText(entry, locale, record);
                break;
            default:
                LOG_DEBUG("network", "CMSG_REQUEST_HOTFIX: no server data for table {:#x}, entry {}", type, entry);
                break;
        }

        // An empty record tells the client the row does not exist
        WorldPacket data(SMSG_DB_REPLY, 4 * 4 + record.size());
        data << uint32(entry);
        data << uint32(found ? GameTime::GetGameTime().count() : 0);
        data << uint32(type);
        data << uint32(record.size());
        data.append(record);
        SendPacket(&data);
    }
}
