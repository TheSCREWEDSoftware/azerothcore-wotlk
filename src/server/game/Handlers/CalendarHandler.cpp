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

/*
----- Opcodes Not Used yet -----

SMSG_CALENDAR_EVENT_INVITE_NOTES        [ packguid(Invitee), uint64(inviteId), string(Text), Boolean(Unk) ]
?CMSG_CALENDAR_EVENT_INVITE_NOTES       [ uint32(unk1), uint32(unk2), uint32(unk3), uint32(unk4), uint32(unk5) ]
SMSG_CALENDAR_EVENT_INVITE_NOTES_ALERT  [ uint64(inviteId), string(Text) ]
SMSG_CALENDAR_EVENT_INVITE_STATUS_ALERT [ uint64(eventId), uint32(eventTime), uint32(unkFlag), uint8(deletePending) ]

----- TODO -----

Finish complains' handling - what to do with received complains and how to respond?
Find out what to do with all "not used yet" opcodes
Correct errors sending (event/invite not found, invites exceeded, event already passed, permissions etc.)
Fix locked events to be displayed properly and response time shouldn't be shown for people that haven't respond yet
Copied events should probably have a new owner

*/

#include "ArenaTeamMgr.h"
#include "CalendarMgr.h"
#include "CalendarPackets.h"
#include "DatabaseEnv.h"
#include "DisableMgr.h"
#include "GameEventMgr.h"
#include "GameTime.h"
#include "GuildMgr.h"
#include "InstanceSaveMgr.h"
#include "Log.h"
#include "ObjectAccessor.h"
#include "Opcodes.h"
#include "Player.h"
#include "SocialMgr.h"
#include "WorldSession.h"
#include <utf8.h>

// 5.4.8 18414, from Project SkyFire
void WorldSession::HandleCalendarGetCalendar(WorldPacket& /*recvData*/)
{
    ObjectGuid guid = _player->GetGUID();
    LOG_DEBUG("network", "CMSG_CALENDAR_GET_CALENDAR [{}]", guid.ToString());

    time_t currTime = GameTime::GetGameTime().count();

    CalendarInviteStore invites = sCalendarMgr->GetPlayerInvites(guid);
    CalendarEventStore playerEvents = sCalendarMgr->GetPlayerEvents(guid);

    ByteBuffer lockoutInfoBuffer;
    ByteBuffer invitesInfoBuffer;
    ByteBuffer eventsInfoBuffer;

    WorldPacket data(SMSG_CALENDAR_SEND_CALENDAR, 1000); // Average size if no instance
    std::size_t resetPos = data.bitwpos();
    data.WriteBits(0, 20);                                 // raid reset count, filled in below
    data.WriteBits(0, 16);                                 // holiday count, the client reads Holidays.dbc
    std::size_t lockoutPos = data.bitwpos();
    data.WriteBits(0, 20);                                 // lockout count, filled in below

    uint32 lockoutCount = 0;
    for (uint8 i = 0; i < MAX_DIFFICULTY; ++i)
    {
        BoundInstancesMap const& boundInstances = sInstanceSaveMgr->PlayerGetBoundInstances(guid, Difficulty(i));
        for (BoundInstancesMap::const_iterator itr = boundInstances.begin(); itr != boundInstances.end(); ++itr)
        {
            if (!itr->second.perm)
                continue;

            InstanceSave const* save = itr->second.save;
            time_t resetTime = itr->second.extended ? save->GetExtendedResetTime() : save->GetResetTime();
            ObjectGuid instanceGuid = ObjectGuid::Create<HighGuid::Instance>(save->GetInstanceId());

            data.WriteGuidMask(instanceGuid, { 6, 7, 2, 1, 5, 4, 0, 3 });

            lockoutInfoBuffer << uint32(save->GetDifficulty());
            lockoutInfoBuffer.WriteGuidBytes(instanceGuid, { 3, 0, 1, 5 });
            lockoutInfoBuffer << uint32(resetTime >= currTime ? resetTime - currTime : 0);
            lockoutInfoBuffer << uint32(save->GetMapId());
            lockoutInfoBuffer.WriteGuidBytes(instanceGuid, { 2, 7, 6, 4 });

            ++lockoutCount;
        }
    }

    data.WriteBits(invites.size(), 19);
    for (CalendarInviteStore::const_iterator itr = invites.begin(); itr != invites.end(); ++itr)
    {
        CalendarEvent* calendarEvent = sCalendarMgr->GetEvent((*itr)->GetEventId());
        ObjectGuid senderGuid = calendarEvent ? calendarEvent->GetCreatorGUID() : (*itr)->GetSenderGUID();

        data.WriteGuidMask(senderGuid, { 1, 2, 6, 7, 3, 0, 4, 5 });

        invitesInfoBuffer.WriteGuidBytes(senderGuid, { 2 });
        invitesInfoBuffer << uint64((*itr)->GetInviteId());
        invitesInfoBuffer << uint8((*itr)->GetStatus());
        invitesInfoBuffer.WriteGuidBytes(senderGuid, { 6, 3, 4, 1, 0 });
        invitesInfoBuffer << uint64((*itr)->GetEventId());
        invitesInfoBuffer.WriteGuidBytes(senderGuid, { 7, 5 });
        invitesInfoBuffer << uint8((*itr)->GetRank());
        invitesInfoBuffer << uint8(calendarEvent && calendarEvent->IsGuildEvent() && calendarEvent->GetGuildId() == _player->GetGuildId());
    }

    data.WriteBits(playerEvents.size(), 19);
    for (CalendarEventStore::const_iterator itr = playerEvents.begin(); itr != playerEvents.end(); ++itr)
    {
        CalendarEvent* calendarEvent = *itr;
        Guild* guild = sGuildMgr->GetGuildById(calendarEvent->GetGuildId());
        ObjectGuid guildGuid = guild ? guild->GetGUID() : ObjectGuid::Empty;
        ObjectGuid creatorGuid = calendarEvent->GetCreatorGUID();

        data.WriteGuidMask(creatorGuid, { 2 });
        data.WriteGuidMask(guildGuid, { 1, 7 });
        data.WriteGuidMask(creatorGuid, { 4 });
        data.WriteGuidMask(guildGuid, { 5, 6, 3, 4 });
        data.WriteGuidMask(creatorGuid, { 7 });
        data.WriteBits(calendarEvent->GetTitle().size(), 8);
        data.WriteGuidMask(creatorGuid, { 1 });
        data.WriteGuidMask(guildGuid, { 2, 0 });
        data.WriteGuidMask(creatorGuid, { 0, 3, 6, 5 });

        eventsInfoBuffer.WriteGuidBytes(creatorGuid, { 5 });
        eventsInfoBuffer.WriteGuidBytes(guildGuid, { 3 });
        eventsInfoBuffer.WriteString(calendarEvent->GetTitle());
        eventsInfoBuffer.WriteGuidBytes(guildGuid, { 7 });
        eventsInfoBuffer << int32(calendarEvent->GetDungeonId());
        eventsInfoBuffer.WriteGuidBytes(creatorGuid, { 0, 4 });
        eventsInfoBuffer.WriteGuidBytes(guildGuid, { 2 });
        eventsInfoBuffer.WriteGuidBytes(creatorGuid, { 7, 2 });
        eventsInfoBuffer.AppendPackedTime(calendarEvent->GetEventTime());
        eventsInfoBuffer.WriteGuidBytes(creatorGuid, { 3, 1 });
        eventsInfoBuffer.WriteGuidBytes(guildGuid, { 6, 1 });
        eventsInfoBuffer.WriteGuidBytes(creatorGuid, { 6 });
        eventsInfoBuffer << uint32(calendarEvent->GetFlags());
        eventsInfoBuffer.WriteGuidBytes(guildGuid, { 4, 5, 0 });
        eventsInfoBuffer << uint64(calendarEvent->GetEventId());
        eventsInfoBuffer << uint8(calendarEvent->GetType());
    }

    data.FlushBits();
    data.PutBits(lockoutPos, lockoutCount, 20);
    data.append(eventsInfoBuffer);
    data.append(lockoutInfoBuffer);
    data.append(invitesInfoBuffer);
    data.AppendPackedTime(currTime);                       // zone time

    uint32 resetCount = 0;
    std::set<uint32> sentMaps;
    ResetTimeByMapDifficultyMap const& resets = sInstanceSaveMgr->GetResetTimeMap();
    for (ResetTimeByMapDifficultyMap::const_iterator itr = resets.begin(); itr != resets.end(); ++itr)
    {
        uint32 mapId = PAIR32_LOPART(itr->first);
        if (sentMaps.find(mapId) != sentMaps.end())
            continue;

        MapEntry const* mapEntry = sMapStore.LookupEntry(mapId);
        if (!mapEntry || !mapEntry->IsRaid())
            continue;

        sentMaps.insert(mapId);

        time_t period = sInstanceSaveMgr->GetExtendedResetTimeFor(mapId, Difficulty(PAIR32_HIPART(itr->first))) - itr->second;
        data << int32(mapId);
        data << int32(period);                             // reset time period
        data << int32(0);                                  // reset time offset, needed for periods not aligned with the relation time
        ++resetCount;
    }

    data.PutBits(resetPos, resetCount, 20);

    // set point in time (default 29.12.2005) + X hours
    uint32 relationTime = sWorld->getIntConfig(CONFIG_INSTANCE_RESET_TIME_RELATIVE_TIMESTAMP) + sWorld->getIntConfig(CONFIG_INSTANCE_RESET_TIME_HOUR) * HOUR;
    data << uint32(relationTime);
    data << uint32(currTime);                              // server time

    SendPacket(&data);
}

void WorldSession::HandleCalendarGetEvent(WorldPackets::Calendar::GetEvent& packet)
{
    LOG_DEBUG("network", "CMSG_CALENDAR_GET_EVENT. Player [{}] Event [{}]", _player->GetGUID().ToString(), packet.EventId);

    if (CalendarEvent* calendarEvent = sCalendarMgr->GetEvent(packet.EventId))
        sCalendarMgr->SendCalendarEvent(_player->GetGUID(), *calendarEvent, CALENDAR_SENDTYPE_GET);
    else
        sCalendarMgr->SendCalendarCommandResult(_player->GetGUID(), CALENDAR_ERROR_EVENT_INVALID);
}

void WorldSession::HandleCalendarGuildFilter(WorldPackets::Calendar::GuildFilter& packet)
{
    LOG_DEBUG("network", "CMSG_CALENDAR_GUILD_FILTER [{}]", _player->GetGUID().ToString());

    if (Guild* guild = sGuildMgr->GetGuildById(_player->GetGuildId()))
        guild->MassInviteToEvent(this, packet.MinimumLevel, packet.MaximumLevel, packet.MinimumRank);

    LOG_DEBUG("network", "CMSG_CALENDAR_GUILD_FILTER: Min level [{}], Max level [{}], Min rank [{}]", packet.MinimumLevel, packet.MaximumLevel, packet.MinimumRank);
}

void WorldSession::HandleCalendarArenaTeam(WorldPackets::Calendar::ArenaTeam& packet)
{
    LOG_DEBUG("network", "CMSG_CALENDAR_ARENA_TEAM [{}]", _player->GetGUID().ToString());

    if (ArenaTeam* team = sArenaTeamMgr->GetArenaTeamById(packet.ArenaTeamId))
        team->MassInviteToEvent(this);
}

bool validUtf8String(WorldPacket& recvData, std::string& s, std::string action, ObjectGuid playerGUID)
{
    if (!utf8::is_valid(s.begin(), s.end()))
    {
        LOG_INFO("network.opcode", "CalendarHandler: Player ({}) attempt to {} an event with invalid name or description (packet modification)",
            playerGUID.ToString(), action);
        recvData.rfinish();
        return false;
    }
    return true;
}

// 5.4.8 18414, from Project SkyFire
void WorldSession::HandleCalendarAddEvent(WorldPacket& recvData)
{
    ObjectGuid guid = _player->GetGUID();

    uint32 maxInvites;
    uint32 flags;
    int32 dungeonId;
    uint32 eventPackedTime;
    uint8 type;

    recvData >> maxInvites >> flags >> dungeonId;
    recvData.ReadPackedTime(eventPackedTime);
    recvData >> type;

    uint32 inviteCount = recvData.ReadBits(22);
    uint32 descriptionLength = recvData.ReadBits(11);
    if (inviteCount > CALENDAR_MAX_INVITES)
    {
        recvData.rfinish();
        sCalendarMgr->SendCalendarCommandResult(guid, CALENDAR_ERROR_INVITES_EXCEEDED);
        return;
    }

    std::vector<ObjectGuid> invitees(inviteCount);
    std::vector<uint8> statuses(inviteCount, 0);
    std::vector<uint8> ranks(inviteCount, 0);

    for (uint32 i = 0; i < inviteCount; ++i)
        recvData.ReadGuidMask(invitees[i], { 7, 2, 6, 3, 5, 1, 0, 4 });

    uint32 titleLength = recvData.ReadBits(8);

    for (uint32 i = 0; i < inviteCount; ++i)
    {
        recvData.ReadGuidBytes(invitees[i], { 4, 2, 3, 1, 0, 6, 7 });
        recvData >> statuses[i];
        recvData.ReadGuidBytes(invitees[i], { 5 });
        recvData >> ranks[i];
    }

    std::string title = recvData.ReadString(titleLength);
    std::string description = recvData.ReadString(descriptionLength);

    // prevent attacks with non-utf8 chars -> with multiple packets it will hang up the db due to errors.
    if (!validUtf8String(recvData, title, "create", guid) || title.size() > 31 || !validUtf8String(recvData, description, "create", guid) || description.size() > 255)
        return;

    // prevent events in the past
    // To Do: properly handle timezones and remove the "- time_t(86400L)" hack
    if (time_t(eventPackedTime) < (GameTime::GetGameTime().count() - time_t(86400L)))
    {
        sCalendarMgr->SendCalendarCommandResult(guid, CALENDAR_ERROR_EVENT_PASSED);
        return;
    }

    // If the event is a guild event, check if the player is in a guild
    if (CalendarEvent::IsGuildEvent(flags) || CalendarEvent::IsGuildAnnouncement(flags))
    {
        if (!_player->GetGuildId())
        {
            sCalendarMgr->SendCalendarCommandResult(guid, CALENDAR_ERROR_GUILD_PLAYER_NOT_IN_GUILD);
            return;
        }

        // Check if the guild reached the max number of events allowed
        if (sCalendarMgr->GetGuildEvents(_player->GetGuildId()).size() >= CALENDAR_MAX_GUILD_EVENTS)
        {
            sCalendarMgr->SendCalendarCommandResult(guid, CALENDAR_ERROR_GUILD_EVENTS_EXCEEDED);
            return;
        }
    }
    else if (sCalendarMgr->GetEventsCreatedBy(guid).size() >= CALENDAR_MAX_EVENTS)
    {
        sCalendarMgr->SendCalendarCommandResult(guid, CALENDAR_ERROR_EVENTS_EXCEEDED);
        return;
    }

    if (GetCalendarEventCreationCooldown() > GameTime::GetGameTime().count())
    {
        sCalendarMgr->SendCalendarCommandResult(guid, CALENDAR_ERROR_INTERNAL);
        return;
    }
    SetCalendarEventCreationCooldown(GameTime::GetGameTime().count() + CALENDAR_CREATE_EVENT_COOLDOWN);

    CalendarEvent* calendarEvent = new CalendarEvent(sCalendarMgr->GetFreeEventId(), guid, 0, CalendarEventType(type), dungeonId,
            time_t(eventPackedTime), flags, time_t(0), title, description);

    if (calendarEvent->IsGuildEvent() || calendarEvent->IsGuildAnnouncement())
        calendarEvent->SetGuildId(_player->GetGuildId());

    if (calendarEvent->IsGuildAnnouncement())
    {
        // 946684800 is 01/01/2000 00:00:00 - default response time
        CalendarInvite* invite = new CalendarInvite(0, calendarEvent->GetEventId(), ObjectGuid::Empty, guid, 946684800, CALENDAR_STATUS_NOT_SIGNED_UP, CALENDAR_RANK_PLAYER, "");
        sCalendarMgr->AddInvite(calendarEvent, invite);
    }
    else
    {
        CharacterDatabaseTransaction trans;
        if (inviteCount > 1)
            trans = CharacterDatabase.BeginTransaction();

        for (uint32 i = 0; i < inviteCount; ++i)
        {
            // 946684800 is 01/01/2000 00:00:00 - default response time
            CalendarInvite* invite = new CalendarInvite(sCalendarMgr->GetFreeInviteId(), calendarEvent->GetEventId(), invitees[i], guid, 946684800, CalendarInviteStatus(statuses[i]), CalendarModerationRank(ranks[i]), "");
            sCalendarMgr->AddInvite(calendarEvent, invite, trans);
        }

        if (inviteCount > 1)
            CharacterDatabase.CommitTransaction(trans);
    }

    sCalendarMgr->AddEvent(calendarEvent, CALENDAR_SENDTYPE_ADD);
}

// 5.4.8 18414, from Project SkyFire
void WorldSession::HandleCalendarUpdateEvent(WorldPacket& recvData)
{
    ObjectGuid guid = _player->GetGUID();

    ObjectGuid eventId;
    ObjectGuid inviteId;
    uint32 maxInvites;
    int32 dungeonId;
    uint32 eventPackedTime;
    uint32 flags;
    uint8 type;

    recvData >> maxInvites >> dungeonId;
    recvData.ReadPackedTime(eventPackedTime);
    recvData >> flags >> type;

    recvData.ReadGuidMask(eventId, { 4, 5, 2 });
    recvData.ReadGuidMask(inviteId, { 4 });
    recvData.ReadGuidMask(eventId, { 7, 0 });
    recvData.ReadGuidMask(inviteId, { 5, 3 });
    recvData.ReadGuidMask(eventId, { 6, 1 });
    recvData.ReadGuidMask(inviteId, { 6, 2, 7, 1, 0 });
    uint32 descriptionLength = recvData.ReadBits(11);
    uint32 titleLength = recvData.ReadBits(8);
    recvData.ReadGuidMask(eventId, { 3 });

    recvData.ReadGuidBytes(inviteId, { 6 });
    recvData.ReadGuidBytes(eventId, { 0 });
    recvData.ReadGuidBytes(inviteId, { 7, 3 });
    recvData.ReadGuidBytes(eventId, { 6 });
    recvData.ReadGuidBytes(inviteId, { 1 });
    recvData.ReadGuidBytes(eventId, { 2 });
    std::string title = recvData.ReadString(titleLength);
    recvData.ReadGuidBytes(inviteId, { 5, 4 });
    recvData.ReadGuidBytes(eventId, { 5, 3 });
    recvData.ReadGuidBytes(inviteId, { 0 });
    recvData.ReadGuidBytes(eventId, { 4 });
    std::string description = recvData.ReadString(descriptionLength);
    recvData.ReadGuidBytes(eventId, { 1 });
    recvData.ReadGuidBytes(inviteId, { 2 });
    recvData.ReadGuidBytes(eventId, { 7 });

    // prevent attacks with non-utf8 chars -> with multiple packets it will hang up the db due to errors.
    if (!validUtf8String(recvData, title, "update", guid) || title.size() > 31 || !validUtf8String(recvData, description, "update", guid) || description.size() > 255)
        return;

    // prevent events in the past
    // To Do: properly handle timezones and remove the "- time_t(86400L)" hack
    if (time_t(eventPackedTime) < (GameTime::GetGameTime().count() - time_t(86400L)))
        return;

    LOG_DEBUG("network", "CMSG_CALENDAR_UPDATE_EVENT [{}] EventId [{}], InviteId [{}] Title {}, Description {}, type {} "
        "MaxInvites {}, Dungeon ID {}, Time {}, Flags {}",
        guid.ToString(), eventId.GetRawValue(), inviteId.GetRawValue(), title, description, type, maxInvites, dungeonId, eventPackedTime, flags);

    if (CalendarEvent* calendarEvent = sCalendarMgr->GetEvent(eventId.GetRawValue()))
    {
        time_t oldEventTime = calendarEvent->GetEventTime();

        calendarEvent->SetType(CalendarEventType(type));
        calendarEvent->SetFlags(flags);
        calendarEvent->SetEventTime(time_t(eventPackedTime));
        calendarEvent->SetDungeonId(dungeonId);
        calendarEvent->SetTitle(title);
        calendarEvent->SetDescription(description);

        sCalendarMgr->UpdateEvent(calendarEvent);
        sCalendarMgr->SendCalendarEventUpdateAlert(*calendarEvent, oldEventTime);
    }
    else
        sCalendarMgr->SendCalendarCommandResult(guid, CALENDAR_ERROR_EVENT_INVALID);
}

void WorldSession::HandleCalendarRemoveEvent(WorldPacket& recvData)
{
    ObjectGuid guid = _player->GetGUID();
    uint64 eventId;

    recvData.read_skip<uint64>(); // invite id
    recvData >> eventId;
    recvData.rfinish(); // Skip flags, we don't use them

    sCalendarMgr->RemoveEvent(eventId, guid);
}

void WorldSession::HandleCalendarCopyEvent(WorldPacket& recvData)
{
    ObjectGuid guid = _player->GetGUID();
    uint64 eventId;
    uint64 inviteId;
    uint32 eventTime;

    recvData >> eventId >> inviteId;
    recvData.ReadPackedTime(eventTime);
    LOG_DEBUG("network", "CMSG_CALENDAR_COPY_EVENT [{}], EventId [{}] inviteId [{}] Time: {}", guid.ToString(), eventId, inviteId, eventTime);

    // prevent events in the past
    // To Do: properly handle timezones and remove the "- time_t(86400L)" hack
    if (time_t(eventTime) < (GameTime::GetGameTime().count() - time_t(86400L)))
    {
        recvData.rfinish();
        sCalendarMgr->SendCalendarCommandResult(guid, CALENDAR_ERROR_EVENT_PASSED);
        return;
    }

    if (CalendarEvent* oldEvent = sCalendarMgr->GetEvent(eventId))
    {
        // Ensure that the player has access to the event
        if (oldEvent->IsGuildEvent() || oldEvent->IsGuildAnnouncement())
        {
            if (oldEvent->GetGuildId() != _player->GetGuildId())
            {
                sCalendarMgr->SendCalendarCommandResult(guid, CALENDAR_ERROR_EVENT_INVALID);
                return;
            }
        }
        else
        {
            if (oldEvent->GetCreatorGUID() != guid)
            {
                sCalendarMgr->SendCalendarCommandResult(guid, CALENDAR_ERROR_EVENT_INVALID);
                return;
            }
        }

        // Check if the player reached the max number of events allowed to create
        if (oldEvent->IsGuildEvent() || oldEvent->IsGuildAnnouncement())
        {
            if (sCalendarMgr->GetGuildEvents(_player->GetGuildId()).size() >= CALENDAR_MAX_GUILD_EVENTS)
            {
                sCalendarMgr->SendCalendarCommandResult(guid, CALENDAR_ERROR_GUILD_EVENTS_EXCEEDED);
                return;
            }
        }
        else
        {
            if (sCalendarMgr->GetEventsCreatedBy(guid).size() >= CALENDAR_MAX_EVENTS)
            {
                sCalendarMgr->SendCalendarCommandResult(guid, CALENDAR_ERROR_EVENTS_EXCEEDED);
                return;
            }
        }

        if (GetCalendarEventCreationCooldown() > GameTime::GetGameTime().count())
        {
            sCalendarMgr->SendCalendarCommandResult(guid, CALENDAR_ERROR_INTERNAL);
            return;
        }
        SetCalendarEventCreationCooldown(GameTime::GetGameTime().count() + CALENDAR_CREATE_EVENT_COOLDOWN);

        CalendarEvent* newEvent = new CalendarEvent(*oldEvent, sCalendarMgr->GetFreeEventId());
        newEvent->SetEventTime(time_t(eventTime));
        sCalendarMgr->AddEvent(newEvent, CALENDAR_SENDTYPE_COPY);

        CalendarInviteStore invites = sCalendarMgr->GetEventInvites(eventId);
        CharacterDatabaseTransaction trans;
        if (invites.size() > 1)
            trans = CharacterDatabase.BeginTransaction();

        for (CalendarInviteStore::const_iterator itr = invites.begin(); itr != invites.end(); ++itr)
            sCalendarMgr->AddInvite(newEvent, new CalendarInvite(**itr, sCalendarMgr->GetFreeInviteId(), newEvent->GetEventId()), trans);

        if (invites.size() > 1)
            CharacterDatabase.CommitTransaction(trans);
        // should we change owner when somebody makes a copy of event owned by another person?
    }
    else
        sCalendarMgr->SendCalendarCommandResult(guid, CALENDAR_ERROR_EVENT_INVALID);
}

void WorldSession::HandleCalendarEventInvite(WorldPacket& recvData)
{
    LOG_DEBUG("network", "CMSG_CALENDAR_EVENT_INVITE");

    ObjectGuid playerGuid = _player->GetGUID();

    uint64 eventId;
    uint64 inviteId;
    std::string name;
    bool isPreInvite;
    bool isGuildEvent;

    ObjectGuid inviteeGuid;
    uint32 inviteeTeamId = TEAM_NEUTRAL;
    uint32 inviteeGuildId = 0;

    recvData >> eventId >> inviteId;
    isPreInvite = recvData.ReadBit();
    uint32 nameLength = recvData.ReadBits(9);
    isGuildEvent = recvData.ReadBit();
    name = recvData.ReadString(nameLength);

    if (Player* player = ObjectAccessor::FindPlayerByName(name.c_str(), false))
    {
        // Invitee is online
        inviteeGuid = player->GetGUID();
        inviteeTeamId = player->GetTeamId();
        inviteeGuildId = player->GetGuildId();
    }
    else
    {
        // xinef: Get Data From global storage
        if (ObjectGuid guid = sCharacterCache->GetCharacterGuidByName(name))
        {
            if (CharacterCacheEntry const* playerData = sCharacterCache->GetCharacterCacheByGuid(guid))
            {
                inviteeGuid = guid;
                inviteeTeamId = Player::TeamIdForRace(playerData->Race);
                inviteeGuildId = playerData->GuildId;
            }
        }
    }

    if (!inviteeGuid)
    {
        sCalendarMgr->SendCalendarCommandResult(playerGuid, CALENDAR_ERROR_PLAYER_NOT_FOUND);
        return;
    }

    if (_player->GetTeamId() != inviteeTeamId && !sWorld->getBoolConfig(CONFIG_ALLOW_TWO_SIDE_INTERACTION_CALENDAR))
    {
        sCalendarMgr->SendCalendarCommandResult(playerGuid, CALENDAR_ERROR_NOT_ALLIED);
        return;
    }

    // xinef: sync query
    if (QueryResult result = CharacterDatabase.Query("SELECT flags FROM character_social WHERE guid = {} AND friend = {}", inviteeGuid.GetCounter(), playerGuid.GetCounter()))
    {
        Field* fields = result->Fetch();
        if (fields[0].Get<uint8>() & SOCIAL_FLAG_IGNORED)
        {
            sCalendarMgr->SendCalendarCommandResult(playerGuid, CALENDAR_ERROR_IGNORING_YOU_S, name.c_str());
            return;
        }
    }

    if (!isPreInvite)
    {
        if (CalendarEvent* calendarEvent = sCalendarMgr->GetEvent(eventId))
        {
            if (calendarEvent->IsGuildEvent() && calendarEvent->GetGuildId() == inviteeGuildId)
            {
                // we can't invite guild members to guild events
                sCalendarMgr->SendCalendarCommandResult(playerGuid, CALENDAR_ERROR_NO_GUILD_INVITES);
                return;
            }

            // 946684800 is 01/01/2000 00:00:00 - default response time
            CalendarInvite* invite = new CalendarInvite(sCalendarMgr->GetFreeInviteId(), eventId, inviteeGuid, playerGuid, 946684800, CALENDAR_STATUS_INVITED, CALENDAR_RANK_PLAYER, "");
            sCalendarMgr->AddInvite(calendarEvent, invite);
        }
        else
            sCalendarMgr->SendCalendarCommandResult(playerGuid, CALENDAR_ERROR_EVENT_INVALID);
    }
    else
    {
        if (isGuildEvent && inviteeGuildId == _player->GetGuildId())
        {
            sCalendarMgr->SendCalendarCommandResult(playerGuid, CALENDAR_ERROR_NO_GUILD_INVITES);
            return;
        }

        // 946684800 is 01/01/2000 00:00:00 - default response time
        CalendarInvite* invite = new CalendarInvite(inviteId, 0, inviteeGuid, playerGuid, 946684800, CALENDAR_STATUS_INVITED, CALENDAR_RANK_PLAYER, "");
        sCalendarMgr->SendCalendarEventInvite(*invite);
    }
}

void WorldSession::HandleCalendarEventSignup(WorldPacket& recvData)
{
    ObjectGuid guid = _player->GetGUID();
    uint64 eventId;
    bool tentative;

    recvData >> eventId;
    tentative = recvData.ReadBit();
    LOG_DEBUG("network", "CMSG_CALENDAR_EVENT_SIGNUP [{}] EventId [{}] Tentative {}", guid.ToString(), eventId, tentative);

    if (CalendarEvent* calendarEvent = sCalendarMgr->GetEvent(eventId))
    {
        if (calendarEvent->IsGuildEvent() && calendarEvent->GetGuildId() != _player->GetGuildId())
        {
            sCalendarMgr->SendCalendarCommandResult(guid, CALENDAR_ERROR_GUILD_PLAYER_NOT_IN_GUILD);
            return;
        }

        CalendarInviteStatus status = tentative ? CALENDAR_STATUS_TENTATIVE : CALENDAR_STATUS_SIGNED_UP;
        CalendarInvite* invite = new CalendarInvite(sCalendarMgr->GetFreeInviteId(), eventId, guid, guid, GameTime::GetGameTime().count(), status, CALENDAR_RANK_PLAYER, "");
        sCalendarMgr->AddInvite(calendarEvent, invite);
        sCalendarMgr->SendCalendarClearPendingAction(guid);
    }
    else
        sCalendarMgr->SendCalendarCommandResult(guid, CALENDAR_ERROR_EVENT_INVALID);
}

void WorldSession::HandleCalendarEventRsvp(WorldPacket& recvData)
{
    ObjectGuid guid = _player->GetGUID();
    uint64 eventId;
    uint64 inviteId;
    uint8 status;

    recvData >> eventId >> inviteId >> status;
    LOG_DEBUG("network", "CMSG_CALENDAR_EVENT_RSVP [{}] EventId [{}], InviteId [{}], status {}",
        guid.ToString(), eventId, inviteId, status);

    if (CalendarEvent* calendarEvent = sCalendarMgr->GetEvent(eventId))
    {
        // i think we still should be able to remove self from locked events
        if (status != CALENDAR_STATUS_REMOVED && calendarEvent->GetFlags() & CALENDAR_FLAG_INVITES_LOCKED)
        {
            sCalendarMgr->SendCalendarCommandResult(guid, CALENDAR_ERROR_EVENT_LOCKED);
            return;
        }

        if (CalendarInvite* invite = sCalendarMgr->GetInvite(inviteId))
        {
            invite->SetStatus(CalendarInviteStatus(status));
            invite->SetStatusTime(GameTime::GetGameTime().count());

            sCalendarMgr->UpdateInvite(invite);
            sCalendarMgr->SendCalendarEventStatus(*calendarEvent, *invite);
            sCalendarMgr->SendCalendarEventStatusAlert(*calendarEvent, *invite);
            sCalendarMgr->SendCalendarClearPendingAction(guid);
        }
        else
            sCalendarMgr->SendCalendarCommandResult(guid, CALENDAR_ERROR_NO_INVITE); // correct?
    }
    else
        sCalendarMgr->SendCalendarCommandResult(guid, CALENDAR_ERROR_EVENT_INVALID);
}

void WorldSession::HandleCalendarEventRemoveInvite(WorldPacket& recvData)
{
    ObjectGuid guid = _player->GetGUID();
    ObjectGuid invitee;
    uint64 eventId;
    uint64 ownerInviteId; // isn't it sender's inviteId?
    uint64 inviteId;

    recvData >> inviteId >> ownerInviteId >> eventId;
    recvData.ReadGuidMask(invitee, { 6, 3, 2, 4, 5, 7, 0, 1 });
    recvData.ReadGuidBytes(invitee, { 0, 4, 7, 3, 5, 1, 2, 6 });

    LOG_DEBUG("network", "CMSG_CALENDAR_EVENT_REMOVE_INVITE [{}] EventId [{}], ownerInviteId [{}], Invitee ([{}] id: [{}])",
                   guid.ToString(), eventId, ownerInviteId, invitee.ToString(), inviteId);

    if (CalendarEvent* calendarEvent = sCalendarMgr->GetEvent(eventId))
    {
        if (calendarEvent->GetCreatorGUID() == invitee)
        {
            sCalendarMgr->SendCalendarCommandResult(guid, CALENDAR_ERROR_DELETE_CREATOR_FAILED);
            return;
        }

        sCalendarMgr->RemoveInvite(inviteId, eventId, guid);
    }
    else
        sCalendarMgr->SendCalendarCommandResult(guid, CALENDAR_ERROR_NO_INVITE);
}

void WorldSession::HandleCalendarEventStatus(WorldPacket& recvData)
{
    ObjectGuid guid = _player->GetGUID();
    ObjectGuid invitee;
    uint64 eventId;
    uint64 inviteId;
    uint64 ownerInviteId; // isn't it sender's inviteId?
    uint8 status;

    recvData >> ownerInviteId >> eventId >> inviteId >> status;
    recvData.ReadGuidMask(invitee, { 4, 3, 7, 6, 2, 0, 5, 1 });
    recvData.ReadGuidBytes(invitee, { 7, 5, 2, 1, 4, 6, 0, 3 });
    LOG_DEBUG("network", "CMSG_CALENDAR_EVENT_STATUS [{}] EventId [{}] ownerInviteId [{}], Invitee ({}) id: [{}], status {}",
        guid.ToString(), eventId, ownerInviteId, invitee.ToString(), inviteId, status);

    if (CalendarEvent* calendarEvent = sCalendarMgr->GetEvent(eventId))
    {
        if (CalendarInvite* invite = sCalendarMgr->GetInvite(inviteId))
        {
            invite->SetStatus((CalendarInviteStatus)status);
            invite->SetStatusTime(GameTime::GetGameTime().count());

            sCalendarMgr->UpdateInvite(invite);
            sCalendarMgr->SendCalendarEventStatus(*calendarEvent, *invite);
            sCalendarMgr->SendCalendarEventStatusAlert(*calendarEvent, *invite);
            sCalendarMgr->SendCalendarClearPendingAction(invitee);
        }
        else
            sCalendarMgr->SendCalendarCommandResult(guid, CALENDAR_ERROR_NO_INVITE); // correct?
    }
    else
        sCalendarMgr->SendCalendarCommandResult(guid, CALENDAR_ERROR_EVENT_INVALID);
}

void WorldSession::HandleCalendarEventModeratorStatus(WorldPacket& recvData)
{
    ObjectGuid guid = _player->GetGUID();
    ObjectGuid invitee;
    uint64 eventId;
    uint64 inviteId;
    uint64 ownerInviteId; // isn't it sender's inviteId?
    uint8 rank;

    recvData >> rank >> eventId >> ownerInviteId >> inviteId;
    recvData.ReadGuidMask(invitee, { 6, 5, 1, 3, 4, 7, 0, 2 });
    recvData.ReadGuidBytes(invitee, { 7, 5, 0, 4, 1, 3, 2, 6 });
    LOG_DEBUG("network", "CMSG_CALENDAR_EVENT_MODERATOR_STATUS [{}] EventId [{}] ownerInviteId [{}], Invitee ([{}] id: [{}], rank {}",
        guid.ToString(), eventId, ownerInviteId, invitee.ToString(), inviteId, rank);

    if (CalendarEvent* calendarEvent = sCalendarMgr->GetEvent(eventId))
    {
        if (calendarEvent->IsGuildEvent())
        {
            sCalendarMgr->SendCalendarCommandResult(guid, CALENDAR_ERROR_NO_MODERATOR);
            return;
        }

        if (CalendarInvite* invite = sCalendarMgr->GetInvite(inviteId))
        {
            invite->SetRank(CalendarModerationRank(rank));
            sCalendarMgr->UpdateInvite(invite);
            sCalendarMgr->SendCalendarEventModeratorStatus(*calendarEvent, *invite);
        }
        else
            sCalendarMgr->SendCalendarCommandResult(guid, CALENDAR_ERROR_NO_INVITE); // correct?
    }
    else
        sCalendarMgr->SendCalendarCommandResult(guid, CALENDAR_ERROR_EVENT_INVALID);
}

void WorldSession::HandleCalendarComplain(WorldPackets::Calendar::CalendarComplain& packet)
{
    if (sWorld->getBoolConfig(CONFIG_LOGSPAMREPORTS))
    {
        CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_INS_SPAM_REPORT);

        stmt->SetData(0, 2); // SpamType 2 = Calendar
        stmt->SetData(1, packet.ComplainGuid.GetCounter());
        stmt->SetData(2, 0);
        stmt->SetData(3, 0);
        stmt->SetData(4, 0);
        stmt->SetData(5, 0);
        stmt->SetData(6, "EventId: " + std::to_string(packet.EventId));
        stmt->SetData(7, GameTime::GetGameTime().count());

        CharacterDatabase.Execute(stmt);
    }
}

void WorldSession::HandleCalendarGetNumPending(WorldPacket& /*recvData*/)
{
    ObjectGuid guid = _player->GetGUID();
    uint32 pending = sCalendarMgr->GetPlayerNumPending(guid);

    LOG_DEBUG("network", "CMSG_CALENDAR_GET_NUM_PENDING: [{}] Pending: {}", guid.ToString(), pending);

    WorldPacket data(SMSG_CALENDAR_SEND_NUM_PENDING, 4);
    data << uint32(pending);
    SendPacket(&data);
}

void WorldSession::HandleSetSavedInstanceExtend(WorldPacket& recvData)
{
    uint32 mapId, difficulty;
    uint8 toggleExtendOn;
    recvData >> mapId >> difficulty >> toggleExtendOn;

    MapEntry const* entry = sMapStore.LookupEntry(mapId);
    if (!entry || !entry->IsRaid())
        return;

    InstancePlayerBind* instanceBind = sInstanceSaveMgr->PlayerGetBoundInstance(GetPlayer()->GetGUID(), mapId, Difficulty(difficulty));
    if (!instanceBind || !instanceBind->perm || (bool)toggleExtendOn == instanceBind->extended)
        return;

    instanceBind->extended = (bool)toggleExtendOn;

    // update in db
    CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_CHAR_INSTANCE_EXTENDED);
    stmt->SetData(0, toggleExtendOn ? 1 : 0);
    stmt->SetData(1, GetPlayer()->GetGUID().GetCounter());
    stmt->SetData(2, instanceBind->save->GetInstanceId());
    CharacterDatabase.Execute(stmt);

    SendCalendarRaidLockoutUpdated(instanceBind->save, (bool)toggleExtendOn);
}

// ----------------------------------- SEND ------------------------------------

// 5.4.8 18414, from Project SkyFire
void WorldSession::SendCalendarRaidLockout(InstanceSave const* save, bool add)
{
    LOG_DEBUG("network", "{}", add ? "SMSG_CALENDAR_RAID_LOCKOUT_ADDED" : "SMSG_CALENDAR_RAID_LOCKOUT_REMOVED");
    time_t currTime = GameTime::GetGameTime().count();
    ObjectGuid instanceGuid = ObjectGuid::Create<HighGuid::Instance>(save->GetInstanceId());

    if (add)
    {
        WorldPacket data(SMSG_CALENDAR_RAID_LOCKOUT_ADDED, 4 + 4 + 4 + 4 + 8 + 1);
        data.WriteGuidMask(instanceGuid, { 3, 1, 2, 0, 4, 7, 5, 6 });
        data.FlushBits();

        data.WriteGuidBytes(instanceGuid, { 6 });
        data << uint32(save->GetDifficulty());
        data.WriteGuidBytes(instanceGuid, { 0, 4, 5, 3, 2 });
        data << uint32(save->GetMapId());
        data.WriteGuidBytes(instanceGuid, { 1 });
        data.AppendPackedTime(currTime);
        data << uint32(save->GetResetTime() >= currTime ? save->GetResetTime() - currTime : 0);
        data.WriteGuidBytes(instanceGuid, { 7 });
        SendPacket(&data);
    }
    else
    {
        WorldPacket data(SMSG_CALENDAR_RAID_LOCKOUT_REMOVED, 4 + 4 + 8 + 1);
        data << uint32(save->GetDifficulty());
        data << uint32(save->GetMapId());
        data.WriteGuidMask(instanceGuid, { 2, 0, 4, 6, 5, 7, 3, 1 });
        data.FlushBits();

        data.WriteGuidBytes(instanceGuid, { 6, 1, 7, 3, 4, 5, 0, 2 });
        SendPacket(&data);
    }
}

// 5.4.8 18414, from Project SkyFire
void WorldSession::SendCalendarRaidLockoutUpdated(InstanceSave const* save, bool isExtended)
{
    time_t currTime = GameTime::GetGameTime().count();
    time_t resetTime = isExtended ? save->GetExtendedResetTime() : save->GetResetTime();
    time_t resetTimeOp = isExtended ? save->GetResetTime() : save->GetExtendedResetTime();
    WorldPacket data(SMSG_CALENDAR_RAID_LOCKOUT_UPDATED, 4 + 4 + 4 + 4 + 4);
    data << uint32(resetTime >= currTime ? resetTime - currTime : 0);           // new time in secs to reset
    data << uint32(save->GetMapId());
    data << uint32(save->GetDifficulty());
    data.AppendPackedTime(currTime);
    data << uint32(resetTimeOp >= currTime ? resetTimeOp - currTime : resetTimeOp); // old time in secs to reset
    SendPacket(&data);
}
