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
        buffer << uint32(gossip ? gossip->Options[i].BroadcastTextID : 0);

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
