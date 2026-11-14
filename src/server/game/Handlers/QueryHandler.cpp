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

#include "CharacterCache.h"
#include "Common.h"
#include "Corpse.h"
#include "DB2Stores.h"
#include "GameTime.h"
#include "Log.h"
#include "MapMgr.h"
#include "NPCHandler.h"
#include "ObjectMgr.h"
#include "Opcodes.h"
#include "Player.h"
#include "QueryPackets.h"
#include "Realm.h"
#include "World.h"
#include "WorldPacket.h"
#include "WorldSession.h"

void WorldSession::SendNameQueryOpcode(ObjectGuid guid)
{
    Player* player = ObjectAccessor::FindConnectedPlayer(guid);

    WorldPackets::Query::QueryPlayerNameResponse response;
    response.Player = guid;

    if (response.Data.Initialize(guid, player))
        response.Result = RESPONSE_SUCCESS; // name known
    else
        response.Result = RESPONSE_FAILURE; // name unknown

    SendPacket(response.Write());
}

void WorldSession::HandleNameQueryOpcode(WorldPackets::Query::QueryPlayerName& packet)
{
    SendNameQueryOpcode(packet.Player);
}

void WorldSession::HandleQueryTimeOpcode(WorldPackets::Query::QueryTime& /*queryTime*/)
{
    SendTimeQueryResponse();
}

void WorldSession::SendTimeQueryResponse()
{
    WorldPackets::Query::QueryTimeResponse queryTimeResponse;
    queryTimeResponse.CurrentTime = GameTime::GetGameTime().count();
    queryTimeResponse.TimeOutRequest = int32((sWorld->GetNextDailyQuestsResetTime() - GameTime::GetGameTime()).count());
    SendPacket(queryTimeResponse.Write());
}

void WorldSession::HandleQueryRealmName(WorldPackets::Query::QueryRealmName& queryRealmName)
{
    WorldPackets::Query::RealmQueryResponse response;
    response.VirtualRealmAddress = queryRealmName.VirtualRealmAddress;

    // the virtual realm address keeps the realm id in its low 16 bits
    uint32 realmId = queryRealmName.VirtualRealmAddress & 0xFFFF;
    std::string realmName = sObjectMgr->GetRealmName(realmId);
    if (realmName.empty())
        response.LookupState = 1;
    else
    {
        response.LookupState = 0;
        response.NameInfo.IsLocal = queryRealmName.VirtualRealmAddress == GetVirtualRealmAddress();
        response.NameInfo.RealmNameActual = realmName;
        response.NameInfo.RealmNameNormalized = sObjectMgr->GetNormalizedRealmName(realmId);
    }

    SendPacket(response.Write());
}

/// Only _static_ data is sent in this packet !!!
void WorldSession::HandleCreatureQuery(WorldPackets::Query::QueryCreature& packet)
{
    WorldPackets::Query::QueryCreatureResponse response;
    response.CreatureID = packet.CreatureID;

    if (CreatureTemplate const* creatureInfo = sObjectMgr->GetCreatureTemplate(packet.CreatureID))
    {
        response.Allow = true;

        WorldPackets::Query::CreatureStats& stats = response.Stats;

        stats.Leader = creatureInfo->RacialLeader;

        stats.Name[0] = creatureInfo->Name;
        stats.NameAlt[0] = creatureInfo->FemaleName;

        stats.Flags[0] = creatureInfo->type_flags;
        stats.Flags[1] = creatureInfo->type_flags2;

        stats.CreatureType = creatureInfo->type;
        stats.CreatureFamily = creatureInfo->family;
        stats.Classification = creatureInfo->rank;

        for (uint32 i = 0; i < MAX_KILL_CREDIT; ++i)
            stats.ProxyCreatureID[i] = creatureInfo->KillCredit[i];

        for (uint32 i = 0; i < MAX_CREATURE_MODELS; ++i)
        {
            stats.CreatureDisplayID[i] = 0;
            if (CreatureModel const* model = creatureInfo->GetModelByIdx(i))
                stats.CreatureDisplayID[i] = model->CreatureDisplayID;
        }

        stats.HpMulti = creatureInfo->ModHealth;
        stats.EnergyMulti = creatureInfo->ModMana;

        stats.CreatureMovementInfoID = creatureInfo->movementId;
        stats.RequiredExpansion = creatureInfo->expansion;

        stats.Title = creatureInfo->SubName;
        stats.CursorName = creatureInfo->IconName;

        if (CreatureQuestItemList const* items = sObjectMgr->GetCreatureQuestItemList(packet.CreatureID))
            for (uint32 item : *items)
                stats.QuestItems.push_back(int32(item));

        LocaleConstant localeConstant = GetSessionDbLocaleIndex();
        if (localeConstant >= LOCALE_enUS)
            if (CreatureLocale const* creatureLocale = sObjectMgr->GetCreatureLocale(packet.CreatureID))
            {
                ObjectMgr::GetLocaleString(creatureLocale->Name, localeConstant, stats.Name[0]);
                ObjectMgr::GetLocaleString(creatureLocale->Title, localeConstant, stats.Title);
            }
    }
    else
        LOG_DEBUG("network", "WORLD: CMSG_QUERY_CREATURE - NO CREATURE INFO! (entry {})", packet.CreatureID);

    SendPacket(response.Write());
}

/// Only _static_ data is sent in this packet !!!
void WorldSession::HandleGameObjectQueryOpcode(WorldPackets::Query::QueryGameObject& packet)
{
    WorldPackets::Query::QueryGameObjectResponse response;
    response.GameObjectID = packet.GameObjectID;

    if (GameObjectTemplate const* gameObjectInfo = sObjectMgr->GetGameObjectTemplate(packet.GameObjectID))
    {
        response.Allow = true;
        WorldPackets::Query::GameObjectStats& stats = response.Stats;

        stats.Type = gameObjectInfo->type;
        stats.DisplayID = gameObjectInfo->displayId;

        stats.Name[0] = gameObjectInfo->name;
        stats.IconName = gameObjectInfo->IconName;
        stats.CastBarCaption = gameObjectInfo->castBarCaption;
        stats.UnkString = gameObjectInfo->unk1;

        LocaleConstant localeConstant = GetSessionDbLocaleIndex();
        if (localeConstant >= LOCALE_enUS)
            if (GameObjectLocale const* gameObjectLocale = sObjectMgr->GetGameObjectLocale(packet.GameObjectID))
            {
                ObjectMgr::GetLocaleString(gameObjectLocale->Name, localeConstant, stats.Name[0]);
                ObjectMgr::GetLocaleString(gameObjectLocale->CastBarCaption, localeConstant, stats.CastBarCaption);
            }

        stats.Size = gameObjectInfo->size;

        if (GameObjectQuestItemList const* items = sObjectMgr->GetGameObjectQuestItemList(packet.GameObjectID))
            for (uint32 item : *items)
                stats.QuestItems.push_back(int32(item));

        for (uint32 i = 0; i < MAX_GAMEOBJECT_DATA; ++i)
            stats.Data[i] = gameObjectInfo->raw.data[i];
    }
    else
        LOG_DEBUG("network", "WORLD: CMSG_QUERY_GAME_OBJECT - Missing gameobject info for ({}, entry {})", packet.Guid.ToString(), packet.GameObjectID);

    SendPacket(response.Write());
}

void WorldSession::HandleQueryCorpseLocation(WorldPackets::Query::QueryCorpseLocationFromClient& /*packet*/)
{
    if (!_player->HasCorpse())
    {
        WorldPackets::Query::CorpseLocation packet;
        packet.Valid = false;                               // corpse not found
        SendPacket(packet.Write());
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
            if (corpseMapEntry->IsDungeon() && corpseMapEntry->CorpseMapID >= 0)
            {
                // if corpse map have entrance
                if (Map const* entranceMap = sMapMgr->CreateBaseMap(corpseMapEntry->CorpseMapID))
                {
                    mapID = corpseMapEntry->CorpseMapID;
                    x = corpseMapEntry->CorpsePos.X;
                    y = corpseMapEntry->CorpsePos.Y;
                    z = entranceMap->GetHeight(GetPlayer()->GetPhaseMask(), x, y, MAX_HEIGHT);
                }
            }
        }
    }

    WorldPackets::Query::CorpseLocation packet;
    packet.Valid = true;
    packet.MapID = corpseMapID;
    packet.ActualMapID = mapID;
    packet.Position = G3D::Vector3(x, y, z);
    packet.Transport = ObjectGuid::Empty;
    SendPacket(packet.Write());
}

void WorldSession::HandleQueryCorpseTransport(WorldPackets::Query::QueryCorpseTransport& queryCorpseTransport)
{
    Corpse* corpse = _player->GetCorpse();

    WorldPackets::Query::CorpseTransportQuery response;
    if (!corpse || corpse->GetTransGUID().IsEmpty() || corpse->GetTransGUID() != queryCorpseTransport.Transport)
    {
        response.Position = G3D::Vector3(0.0f, 0.0f, 0.0f);
        response.Facing = 0.0f;
    }
    else
    {
        response.Position = G3D::Vector3(corpse->GetTransOffsetX(), corpse->GetTransOffsetY(), corpse->GetTransOffsetZ());
        response.Facing = corpse->GetTransOffsetO();
    }

    SendPacket(response.Write());
}

void WorldSession::HandleNpcTextQueryOpcode(WorldPackets::Query::QueryNPCText& packet)
{
    LOG_DEBUG("network", "WORLD: CMSG_QUERY_NPC_TEXT TextId: {}", packet.TextID);

    // 6.x clients read the gossip text from BroadcastText.db2. npc_text rows without a broadcast text id
    // get a server side id that HandleDBQueryBulk answers from the npc_text row.
    GossipText const* gossip = sObjectMgr->GetGossipText(packet.TextID);

    WorldPackets::Query::QueryNPCTextResponse response;
    response.TextID = packet.TextID;

    for (uint8 i = 0; i < MAX_NPC_TEXT_OPTIONS; ++i)
    {
        response.Probabilities[i] = 0.0f;
        response.BroadcastTextID[i] = 0;

        if (!gossip)
            continue;

        uint32 broadcastTextId = gossip->Options[i].BroadcastTextID;
        if (!broadcastTextId && (!gossip->Options[i].Text_0.empty() || !gossip->Options[i].Text_1.empty()))
            broadcastTextId = MakeGossipBroadcastTextId(packet.TextID, i);

        response.Probabilities[i] = gossip->Options[i].Probability;
        response.BroadcastTextID[i] = broadcastTextId;
        if (broadcastTextId)
            response.Allow = true;
    }

    if (!response.Allow)
        LOG_ERROR("sql.sql", "HandleNpcTextQueryOpcode: no BroadcastTextID or text found for text {} in `npc_text`", packet.TextID);

    SendPacket(response.Write());
}

/// Only _static_ data is sent in this packet !!!
void WorldSession::HandleQueryPageText(WorldPackets::Query::QueryPageText& packet)
{
    uint32 pageID = packet.PageTextID;

    while (pageID)
    {
        PageText const* pageText = sObjectMgr->GetPageText(pageID);

        WorldPackets::Query::QueryPageTextResponse response;
        response.PageTextID = pageID;

        if (!pageText)
        {
            response.Allow = false;
            pageID = 0;
        }
        else
        {
            response.Allow = true;
            response.Info.ID = pageID;
            response.Info.NextPageID = pageText->NextPage;
            response.Info.Text = pageText->Text;

            LocaleConstant localeConstant = GetSessionDbLocaleIndex();
            if (localeConstant >= LOCALE_enUS)
                if (PageTextLocale const* pageTextLocale = sObjectMgr->GetPageTextLocale(pageID))
                    ObjectMgr::GetLocaleString(pageTextLocale->Text, localeConstant, response.Info.Text);

            pageID = pageText->NextPage;
        }

        SendPacket(response.Write());
    }
}

void WorldSession::HandleQueryQuestCompletionNPCs(WorldPackets::Query::QueryQuestCompletionNPCs& queryQuestCompletionNPCs)
{
    WorldPackets::Query::QuestCompletionNPCResponse response;

    std::unordered_set<int32> questIds;
    for (int32 questID : queryQuestCompletionNPCs.QuestCompletionNPCs)
    {
        if (!questIds.insert(questID).second)
            continue;

        if (!sObjectMgr->GetQuestTemplate(questID))
        {
            LOG_DEBUG("network", "WORLD: Unknown quest {} in CMSG_QUERY_QUEST_COMPLETION_NPCS by {}", questID, _player->GetGUID().ToString());
            continue;
        }

        WorldPackets::Query::QuestCompletionNPC questCompletionNPC;
        questCompletionNPC.QuestID = questID;

        // Reuse the live relation maps so reloads cannot leave a reverse cache stale.
        for (auto const& [entry, quest] : *sObjectMgr->GetCreatureQuestInvolvedRelationMap())
            if (quest == uint32(questID))
                questCompletionNPC.NPCs.push_back(int32(entry));

        for (auto const& [entry, quest] : *sObjectMgr->GetGOQuestInvolvedRelationMap())
            if (quest == uint32(questID))
                questCompletionNPC.NPCs.push_back(int32(entry | 0x80000000)); // GO mask

        response.QuestCompletionNPCs.push_back(questCompletionNPC);
    }

    SendPacket(response.Write());
}

void WorldSession::HandleQuestPOIQuery(WorldPackets::Query::QuestPOIQuery& questPoiQuery)
{
    if (questPoiQuery.MissingQuestCount > MAX_QUEST_LOG_SIZE)
        return;

    // Read quest ids and add the in a unordered_set so we don't send POIs for the same quest multiple times
    std::unordered_set<int32> questIds;
    for (int32 i = 0; i < questPoiQuery.MissingQuestCount; ++i)
        questIds.insert(questPoiQuery.MissingQuestPOIs[i]); // QuestID

    WorldPackets::Query::QuestPOIQueryResponse response;

    for (int32 questID : questIds)
    {
        bool questOk = false;

        uint16 questSlot = _player->FindQuestSlot(uint32(questID));
        if (questSlot != MAX_QUEST_LOG_SIZE)
            questOk = _player->GetQuestSlotQuestId(questSlot) == uint32(questID);

        if (!questOk)
            continue;

        QuestPOIVector const* poiData = sObjectMgr->GetQuestPOIVector(questID);
        if (!poiData)
            continue;

        WorldPackets::Query::QuestPOIData questPOIData;
        questPOIData.QuestID = questID;

        for (QuestPOI const& data : *poiData)
        {
            WorldPackets::Query::QuestPOIBlobData questPOIBlobData;

            questPOIBlobData.BlobIndex          = data.BlobIndex;
            questPOIBlobData.ObjectiveIndex     = data.ObjectiveIndex;
            questPOIBlobData.QuestObjectiveID   = data.QuestObjectiveID;
            questPOIBlobData.QuestObjectID      = data.QuestObjectID;
            questPOIBlobData.MapID              = data.MapID;
            questPOIBlobData.WorldMapAreaID     = data.WorldMapAreaID;
            questPOIBlobData.Floor              = data.Floor;
            questPOIBlobData.Priority           = data.Priority;
            questPOIBlobData.Flags              = data.Flags;
            questPOIBlobData.WorldEffectID      = data.WorldEffectID;
            questPOIBlobData.PlayerConditionID  = data.PlayerConditionID;
            questPOIBlobData.UnkWoD1            = data.UnkWoD1;

            for (QuestPOIPoint const& point : data.points)
            {
                WorldPackets::Query::QuestPOIBlobPoint questPOIBlobPoint;
                questPOIBlobPoint.X = point.X;
                questPOIBlobPoint.Y = point.Y;
                questPOIBlobData.QuestPOIBlobPointStats.push_back(questPOIBlobPoint);
            }

            questPOIData.QuestPOIBlobDataStats.push_back(questPOIBlobData);
        }

        response.QuestPOIDataStats.push_back(questPOIData);
    }

    SendPacket(response.Write());
}

namespace
{
    // 6.x DB2 string layout: uint16 length including the terminator, then the string and its terminator
    void WriteDB2String(ByteBuffer& buffer, std::string const& text)
    {
        buffer << uint16(text.empty() ? 0 : text.size() + 1);
        if (!text.empty())
        {
            buffer.append(text.c_str(), text.size());
            buffer << uint8(0);
        }
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

void WorldSession::HandleDBQueryBulk(WorldPackets::Query::DBQueryBulk& packet)
{
    DB2StorageBase const* store = sDB2Manager.GetStorage(packet.TableHash);
    if (!store)
    {
        LOG_ERROR("network", "CMSG_DB_QUERY_BULK: {} requested unsupported unknown hotfix type: {}", GetPlayerInfo(), packet.TableHash);
        return;
    }

    LocaleConstant const locale = GetSessionDbcLocale();
    bool const isBroadcastText = packet.TableHash == sBroadcastTextStore.GetHash();

    for (WorldPackets::Query::DBQueryBulk::DBQueryRecord const& rec : packet.Queries)
    {
        WorldPackets::Query::DBReply response;
        response.TableHash = packet.TableHash;
        response.RecordID = rec.RecordID;

        if (store->HasRecord(rec.RecordID))
        {
            response.Allow = true;
            response.Timestamp = sDB2Manager.GetHotfixDate(rec.RecordID, packet.TableHash);
            store->WriteRecord(rec.RecordID, locale, response.Data);
        }
        // AC keeps world broadcast_text rows and npc_text texts the client files do not have
        else if (isBroadcastText && WriteBroadcastText(rec.RecordID, GetSessionDbLocaleIndex(), response.Data))
        {
            response.Allow = true;
            response.Timestamp = uint32(GameTime::GetGameTime().count());
        }
        else
        {
            LOG_TRACE("network", "CMSG_DB_QUERY_BULK: {} requested non-existing entry {} in datastore: {}", GetPlayerInfo(), rec.RecordID, packet.TableHash);
            response.Timestamp = uint32(GameTime::GetGameTime().count());
        }

        SendPacket(response.Write());
    }
}
