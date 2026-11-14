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

#include "CalendarMgr.h"
#include "CalendarPackets.h"
#include "CharacterCache.h"
#include "DatabaseEnv.h"
#include "GameTime.h"
#include "GuildMgr.h"
#include "InstanceSaveMgr.h"
#include "Log.h"
#include "ObjectAccessor.h"
#include "Opcodes.h"
#include "Player.h"
#include "SocialMgr.h"
#include "World.h"
#include "WorldSession.h"
#include <utf8.h>

// 6.x 19342: handlers ported from TrinityCore 6.x (Handlers/CalendarHandler.cpp), AC event limits and checks kept

namespace
{
    bool IsValidCalendarText(std::string const& title, std::string const& description, char const* action, ObjectGuid playerGUID)
    {
        // prevent attacks with non-utf8 chars -> with multiple packets it will hang up the db due to errors.
        if (!utf8::is_valid(title.begin(), title.end()) || !utf8::is_valid(description.begin(), description.end()))
        {
            LOG_INFO("network.opcode", "CalendarHandler: Player ({}) attempt to {} an event with invalid name or description (packet modification)",
                playerGUID.ToString(), action);
            return false;
        }

        return title.size() <= 31 && description.size() <= 255;
    }
}

void WorldSession::HandleCalendarGetCalendar(WorldPackets::Calendar::CalendarGetCalendar& /*calendarGetCalendar*/)
{
    ObjectGuid guid = _player->GetGUID();
    time_t currTime = GameTime::GetGameTime().count();

    WorldPackets::Calendar::CalendarSendCalendar packet;
    packet.ServerNow = currTime;
    // set point in time (default 29.12.2005) + X hours
    packet.RaidOrigin = sWorld->getIntConfig(CONFIG_INSTANCE_RESET_TIME_RELATIVE_TIMESTAMP) + sWorld->getIntConfig(CONFIG_INSTANCE_RESET_TIME_HOUR) * HOUR;
    packet.ServerTime = currTime;

    for (CalendarInvite const* invite : sCalendarMgr->GetPlayerInvites(guid))
    {
        CalendarEvent* calendarEvent = sCalendarMgr->GetEvent(invite->GetEventId());

        WorldPackets::Calendar::CalendarSendCalendarInviteInfo inviteInfo;
        inviteInfo.EventID = invite->GetEventId();
        inviteInfo.InviteID = invite->GetInviteId();
        inviteInfo.InviterGuid = calendarEvent ? calendarEvent->GetCreatorGUID() : invite->GetSenderGUID();
        inviteInfo.Status = invite->GetStatus();
        inviteInfo.Moderator = invite->GetRank();
        inviteInfo.InviteType = calendarEvent && calendarEvent->IsGuildEvent() && calendarEvent->GetGuildId() == _player->GetGuildId();
        packet.Invites.push_back(inviteInfo);
    }

    for (CalendarEvent const* calendarEvent : sCalendarMgr->GetPlayerEvents(guid))
    {
        Guild* guild = sGuildMgr->GetGuildById(calendarEvent->GetGuildId());

        WorldPackets::Calendar::CalendarSendCalendarEventInfo eventInfo;
        eventInfo.EventID = calendarEvent->GetEventId();
        eventInfo.Date = calendarEvent->GetEventTime();
        eventInfo.EventGuildID = guild ? guild->GetGUID() : ObjectGuid::Empty;
        eventInfo.EventName = calendarEvent->GetTitle();
        eventInfo.EventType = calendarEvent->GetType();
        eventInfo.Flags = calendarEvent->GetFlags();
        eventInfo.OwnerGuid = calendarEvent->GetCreatorGUID();
        eventInfo.TextureID = calendarEvent->GetDungeonId();
        packet.Events.push_back(eventInfo);
    }

    for (uint8 i = 0; i < MAX_DIFFICULTY; ++i)
    {
        for (auto const& [mapId, bind] : sInstanceSaveMgr->PlayerGetBoundInstances(guid, Difficulty(i)))
        {
            if (!bind.perm)
                continue;

            InstanceSave const* save = bind.save;
            time_t resetTime = bind.extended ? save->GetExtendedResetTime() : save->GetResetTime();

            WorldPackets::Calendar::CalendarSendCalendarRaidLockoutInfo lockoutInfo;
            lockoutInfo.MapID = save->GetMapId();
            lockoutInfo.DifficultyID = save->GetDifficulty();
            lockoutInfo.ExpireTime = resetTime >= currTime ? resetTime - currTime : 0;
            lockoutInfo.InstanceID = save->GetInstanceId(); // instance save id as unique instance copy id
            packet.RaidLockouts.push_back(lockoutInfo);
        }
    }

    std::set<uint32> sentMaps;
    for (auto const& [mapDifficulty, resetTime] : sInstanceSaveMgr->GetResetTimeMap())
    {
        uint32 mapId = PAIR32_LOPART(mapDifficulty);
        if (sentMaps.find(mapId) != sentMaps.end())
            continue;

        MapEntry const* mapEntry = sMapStore.LookupEntry(mapId);
        if (!mapEntry || !mapEntry->IsRaid())
            continue;

        sentMaps.insert(mapId);

        WorldPackets::Calendar::CalendarSendCalendarRaidResetInfo resetInfo;
        resetInfo.MapID = mapId;
        resetInfo.Duration = uint32(sInstanceSaveMgr->GetExtendedResetTimeFor(mapId, Difficulty(PAIR32_HIPART(mapDifficulty))) - resetTime);
        resetInfo.Offset = 0; // needed for periods not aligned with the relation time
        packet.RaidResets.push_back(resetInfo);
    }

    SendPacket(packet.Write());
}

void WorldSession::HandleCalendarGetEvent(WorldPackets::Calendar::CalendarGetEvent& calendarGetEvent)
{
    if (CalendarEvent* calendarEvent = sCalendarMgr->GetEvent(calendarGetEvent.EventID))
        sCalendarMgr->SendCalendarEvent(_player->GetGUID(), *calendarEvent, CALENDAR_SENDTYPE_GET);
    else
        sCalendarMgr->SendCalendarCommandResult(_player->GetGUID(), CALENDAR_ERROR_EVENT_INVALID);
}

void WorldSession::HandleCalendarGuildFilter(WorldPackets::Calendar::CalendarGuildFilter& calendarGuildFilter)
{
    if (Guild* guild = sGuildMgr->GetGuildById(_player->GetGuildId()))
        guild->MassInviteToEvent(this, calendarGuildFilter.MinLevel, calendarGuildFilter.MaxLevel, calendarGuildFilter.MaxRankOrder);
}

void WorldSession::HandleCalendarAddEvent(WorldPackets::Calendar::CalendarAddEvent& calendarAddEvent)
{
    ObjectGuid guid = _player->GetGUID();
    WorldPackets::Calendar::CalendarAddEventInfo const& eventInfo = calendarAddEvent.EventInfo;

    if (!IsValidCalendarText(eventInfo.Title, eventInfo.Description, "create", guid))
        return;

    // prevent events in the past
    // To Do: properly handle timezones and remove the "- time_t(86400L)" hack
    if (eventInfo.Time < (GameTime::GetGameTime().count() - time_t(86400L)))
    {
        sCalendarMgr->SendCalendarCommandResult(guid, CALENDAR_ERROR_EVENT_PASSED);
        return;
    }

    // If the event is a guild event, check if the player is in a guild
    if (CalendarEvent::IsGuildEvent(eventInfo.Flags) || CalendarEvent::IsGuildAnnouncement(eventInfo.Flags))
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

    CalendarEvent* calendarEvent = new CalendarEvent(sCalendarMgr->GetFreeEventId(), guid, 0, CalendarEventType(eventInfo.EventType), eventInfo.TextureID,
        eventInfo.Time, eventInfo.Flags, time_t(0), eventInfo.Title, eventInfo.Description);

    if (calendarEvent->IsGuildEvent() || calendarEvent->IsGuildAnnouncement())
        calendarEvent->SetGuildId(_player->GetGuildId());

    if (calendarEvent->IsGuildAnnouncement())
    {
        CalendarInvite* invite = new CalendarInvite(0, calendarEvent->GetEventId(), ObjectGuid::Empty, guid, CALENDAR_DEFAULT_RESPONSE_TIME, CALENDAR_STATUS_NOT_SIGNED_UP, CALENDAR_RANK_PLAYER, "");
        sCalendarMgr->AddInvite(calendarEvent, invite);
    }
    else
    {
        CharacterDatabaseTransaction trans;
        if (eventInfo.Invites.size() > 1)
            trans = CharacterDatabase.BeginTransaction();

        for (WorldPackets::Calendar::CalendarAddEventInviteInfo const& inviteInfo : eventInfo.Invites)
        {
            CalendarInvite* invite = new CalendarInvite(sCalendarMgr->GetFreeInviteId(), calendarEvent->GetEventId(), inviteInfo.Guid, guid, CALENDAR_DEFAULT_RESPONSE_TIME,
                CalendarInviteStatus(inviteInfo.Status), CalendarModerationRank(inviteInfo.Moderator), "");
            sCalendarMgr->AddInvite(calendarEvent, invite, trans);
        }

        if (eventInfo.Invites.size() > 1)
            CharacterDatabase.CommitTransaction(trans);
    }

    sCalendarMgr->AddEvent(calendarEvent, CALENDAR_SENDTYPE_ADD);
}

void WorldSession::HandleCalendarUpdateEvent(WorldPackets::Calendar::CalendarUpdateEvent& calendarUpdateEvent)
{
    ObjectGuid guid = _player->GetGUID();
    WorldPackets::Calendar::CalendarUpdateEventInfo const& eventInfo = calendarUpdateEvent.EventInfo;

    if (!IsValidCalendarText(eventInfo.Title, eventInfo.Description, "update", guid))
        return;

    // prevent events in the past
    // To Do: properly handle timezones and remove the "- time_t(86400L)" hack
    if (eventInfo.Time < (GameTime::GetGameTime().count() - time_t(86400L)))
        return;

    if (CalendarEvent* calendarEvent = sCalendarMgr->GetEvent(eventInfo.EventID))
    {
        time_t oldEventTime = calendarEvent->GetEventTime();

        calendarEvent->SetType(CalendarEventType(eventInfo.EventType));
        calendarEvent->SetFlags(eventInfo.Flags);
        calendarEvent->SetEventTime(eventInfo.Time);
        calendarEvent->SetDungeonId(int32(eventInfo.TextureID));
        calendarEvent->SetTitle(eventInfo.Title);
        calendarEvent->SetDescription(eventInfo.Description);

        sCalendarMgr->UpdateEvent(calendarEvent);
        sCalendarMgr->SendCalendarEventUpdateAlert(*calendarEvent, oldEventTime);
    }
    else
        sCalendarMgr->SendCalendarCommandResult(guid, CALENDAR_ERROR_EVENT_INVALID);
}

void WorldSession::HandleCalendarRemoveEvent(WorldPackets::Calendar::CalendarRemoveEvent& calendarRemoveEvent)
{
    sCalendarMgr->RemoveEvent(calendarRemoveEvent.EventID, _player->GetGUID());
}

void WorldSession::HandleCalendarCopyEvent(WorldPackets::Calendar::CalendarCopyEvent& calendarCopyEvent)
{
    ObjectGuid guid = _player->GetGUID();

    // prevent events in the past
    // To Do: properly handle timezones and remove the "- time_t(86400L)" hack
    if (calendarCopyEvent.Date < (GameTime::GetGameTime().count() - time_t(86400L)))
    {
        sCalendarMgr->SendCalendarCommandResult(guid, CALENDAR_ERROR_EVENT_PASSED);
        return;
    }

    CalendarEvent* oldEvent = sCalendarMgr->GetEvent(calendarCopyEvent.EventID);
    if (!oldEvent)
    {
        sCalendarMgr->SendCalendarCommandResult(guid, CALENDAR_ERROR_EVENT_INVALID);
        return;
    }

    bool const isGuildEvent = oldEvent->IsGuildEvent() || oldEvent->IsGuildAnnouncement();

    // Ensure that the player has access to the event
    if (isGuildEvent ? oldEvent->GetGuildId() != _player->GetGuildId() : oldEvent->GetCreatorGUID() != guid)
    {
        sCalendarMgr->SendCalendarCommandResult(guid, CALENDAR_ERROR_EVENT_INVALID);
        return;
    }

    // Check if the player reached the max number of events allowed to create
    if (isGuildEvent)
    {
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

    CalendarEvent* newEvent = new CalendarEvent(*oldEvent, sCalendarMgr->GetFreeEventId());
    newEvent->SetEventTime(calendarCopyEvent.Date);
    sCalendarMgr->AddEvent(newEvent, CALENDAR_SENDTYPE_COPY);

    CalendarInviteStore invites = sCalendarMgr->GetEventInvites(calendarCopyEvent.EventID);
    CharacterDatabaseTransaction trans;
    if (invites.size() > 1)
        trans = CharacterDatabase.BeginTransaction();

    for (CalendarInvite const* invite : invites)
        sCalendarMgr->AddInvite(newEvent, new CalendarInvite(*invite, sCalendarMgr->GetFreeInviteId(), newEvent->GetEventId()), trans);

    if (invites.size() > 1)
        CharacterDatabase.CommitTransaction(trans);
    // should we change owner when somebody makes a copy of event owned by another person?
}

void WorldSession::HandleCalendarEventInvite(WorldPackets::Calendar::CalendarEventInvite& calendarEventInvite)
{
    ObjectGuid playerGuid = _player->GetGUID();

    // 6.x clients send "Name-Realm" for cross-realm names; only the local realm is supported
    std::string name = calendarEventInvite.Name;
    if (std::size_t realmSeparator = name.find('-'); realmSeparator != std::string::npos)
        name.resize(realmSeparator);

    ObjectGuid inviteeGuid;
    uint32 inviteeTeamId = TEAM_NEUTRAL;
    uint32 inviteeGuildId = 0;

    Player* invitee = ObjectAccessor::FindPlayerByName(name, false);
    if (Player* player = invitee)
    {
        // Invitee is online
        inviteeGuid = player->GetGUID();
        inviteeTeamId = player->GetTeamId();
        inviteeGuildId = player->GetGuildId();
    }
    else if (ObjectGuid guid = sCharacterCache->GetCharacterGuidByName(name))
    {
        if (CharacterCacheEntry const* playerData = sCharacterCache->GetCharacterCacheByGuid(guid))
        {
            inviteeGuid = guid;
            inviteeTeamId = Player::TeamIdForRace(playerData->Race);
            inviteeGuildId = playerData->GuildId;
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

    if (invitee)
    {
        if (invitee->GetSocial()->HasIgnore(playerGuid))
        {
            sCalendarMgr->SendCalendarCommandResult(playerGuid, CALENDAR_ERROR_IGNORING_YOU_S, name.c_str());
            return;
        }
    }
    else if (QueryResult result = CharacterDatabase.Query("SELECT flags FROM character_social WHERE guid = {} AND friend = {}", inviteeGuid.GetCounter(), playerGuid.GetCounter()))
    {
        Field* fields = result->Fetch();
        if (fields[0].Get<uint8>() & SOCIAL_FLAG_IGNORED)
        {
            sCalendarMgr->SendCalendarCommandResult(playerGuid, CALENDAR_ERROR_IGNORING_YOU_S, name.c_str());
            return;
        }
    }

    if (!calendarEventInvite.Creating)
    {
        if (CalendarEvent* calendarEvent = sCalendarMgr->GetEvent(calendarEventInvite.EventID))
        {
            if (calendarEvent->IsGuildEvent() && calendarEvent->GetGuildId() == inviteeGuildId)
            {
                // we can't invite guild members to guild events
                sCalendarMgr->SendCalendarCommandResult(playerGuid, CALENDAR_ERROR_NO_GUILD_INVITES);
                return;
            }

            CalendarInvite* invite = new CalendarInvite(sCalendarMgr->GetFreeInviteId(), calendarEventInvite.EventID, inviteeGuid, playerGuid, CALENDAR_DEFAULT_RESPONSE_TIME, CALENDAR_STATUS_INVITED, CALENDAR_RANK_PLAYER, "");
            sCalendarMgr->AddInvite(calendarEvent, invite);
        }
        else
            sCalendarMgr->SendCalendarCommandResult(playerGuid, CALENDAR_ERROR_EVENT_INVALID);
    }
    else
    {
        if (calendarEventInvite.IsSignUp && inviteeGuildId == _player->GetGuildId())
        {
            sCalendarMgr->SendCalendarCommandResult(playerGuid, CALENDAR_ERROR_NO_GUILD_INVITES);
            return;
        }

        CalendarInvite invite(sCalendarMgr->GetFreeInviteId(), 0, inviteeGuid, playerGuid, CALENDAR_DEFAULT_RESPONSE_TIME, CALENDAR_STATUS_INVITED, CALENDAR_RANK_PLAYER, "");
        sCalendarMgr->SendCalendarEventInvite(invite);
    }
}

void WorldSession::HandleCalendarEventSignup(WorldPackets::Calendar::CalendarEventSignUp& calendarEventSignUp)
{
    ObjectGuid guid = _player->GetGUID();

    if (CalendarEvent* calendarEvent = sCalendarMgr->GetEvent(calendarEventSignUp.EventID))
    {
        if (calendarEvent->IsGuildEvent() && calendarEvent->GetGuildId() != _player->GetGuildId())
        {
            sCalendarMgr->SendCalendarCommandResult(guid, CALENDAR_ERROR_GUILD_PLAYER_NOT_IN_GUILD);
            return;
        }

        CalendarInviteStatus status = calendarEventSignUp.Tentative ? CALENDAR_STATUS_TENTATIVE : CALENDAR_STATUS_SIGNED_UP;
        CalendarInvite* invite = new CalendarInvite(sCalendarMgr->GetFreeInviteId(), calendarEventSignUp.EventID, guid, guid, GameTime::GetGameTime().count(), status, CALENDAR_RANK_PLAYER, "");
        sCalendarMgr->AddInvite(calendarEvent, invite);
        sCalendarMgr->SendCalendarClearPendingAction(guid);
    }
    else
        sCalendarMgr->SendCalendarCommandResult(guid, CALENDAR_ERROR_EVENT_INVALID);
}

void WorldSession::HandleCalendarEventRsvp(WorldPackets::Calendar::CalendarEventRSVP& calendarEventRSVP)
{
    ObjectGuid guid = _player->GetGUID();

    if (CalendarEvent* calendarEvent = sCalendarMgr->GetEvent(calendarEventRSVP.EventID))
    {
        // i think we still should be able to remove self from locked events
        if (calendarEventRSVP.Status != CALENDAR_STATUS_REMOVED && calendarEvent->GetFlags() & CALENDAR_FLAG_INVITES_LOCKED)
        {
            sCalendarMgr->SendCalendarCommandResult(guid, CALENDAR_ERROR_EVENT_LOCKED);
            return;
        }

        if (CalendarInvite* invite = sCalendarMgr->GetInvite(calendarEventRSVP.InviteID))
        {
            invite->SetStatus(CalendarInviteStatus(calendarEventRSVP.Status));
            invite->SetStatusTime(GameTime::GetGameTime().count());

            sCalendarMgr->UpdateInvite(invite);
            sCalendarMgr->SendCalendarEventStatus(*calendarEvent, *invite);
            sCalendarMgr->SendCalendarEventStatusAlert(*calendarEvent, *invite);
            sCalendarMgr->SendCalendarClearPendingAction(guid);
        }
        else
            sCalendarMgr->SendCalendarCommandResult(guid, CALENDAR_ERROR_NO_INVITE);
    }
    else
        sCalendarMgr->SendCalendarCommandResult(guid, CALENDAR_ERROR_EVENT_INVALID);
}

void WorldSession::HandleCalendarEventRemoveInvite(WorldPackets::Calendar::CalendarRemoveInvite& calendarRemoveInvite)
{
    ObjectGuid guid = _player->GetGUID();

    if (CalendarEvent* calendarEvent = sCalendarMgr->GetEvent(calendarRemoveInvite.EventID))
    {
        if (calendarEvent->GetCreatorGUID() == calendarRemoveInvite.Guid)
        {
            sCalendarMgr->SendCalendarCommandResult(guid, CALENDAR_ERROR_DELETE_CREATOR_FAILED);
            return;
        }

        sCalendarMgr->RemoveInvite(calendarRemoveInvite.InviteID, calendarRemoveInvite.EventID, guid);
    }
    else
        sCalendarMgr->SendCalendarCommandResult(guid, CALENDAR_ERROR_NO_INVITE);
}

void WorldSession::HandleCalendarEventStatus(WorldPackets::Calendar::CalendarEventStatus& calendarEventStatus)
{
    ObjectGuid guid = _player->GetGUID();

    if (CalendarEvent* calendarEvent = sCalendarMgr->GetEvent(calendarEventStatus.EventID))
    {
        if (CalendarInvite* invite = sCalendarMgr->GetInvite(calendarEventStatus.InviteID))
        {
            invite->SetStatus(CalendarInviteStatus(calendarEventStatus.Status));
            invite->SetStatusTime(GameTime::GetGameTime().count());

            sCalendarMgr->UpdateInvite(invite);
            sCalendarMgr->SendCalendarEventStatus(*calendarEvent, *invite);
            sCalendarMgr->SendCalendarEventStatusAlert(*calendarEvent, *invite);
            sCalendarMgr->SendCalendarClearPendingAction(calendarEventStatus.Guid);
        }
        else
            sCalendarMgr->SendCalendarCommandResult(guid, CALENDAR_ERROR_NO_INVITE);
    }
    else
        sCalendarMgr->SendCalendarCommandResult(guid, CALENDAR_ERROR_EVENT_INVALID);
}

void WorldSession::HandleCalendarEventModeratorStatus(WorldPackets::Calendar::CalendarEventModeratorStatus& calendarEventModeratorStatus)
{
    ObjectGuid guid = _player->GetGUID();

    if (CalendarEvent* calendarEvent = sCalendarMgr->GetEvent(calendarEventModeratorStatus.EventID))
    {
        if (calendarEvent->IsGuildEvent())
        {
            sCalendarMgr->SendCalendarCommandResult(guid, CALENDAR_ERROR_NO_MODERATOR);
            return;
        }

        if (CalendarInvite* invite = sCalendarMgr->GetInvite(calendarEventModeratorStatus.InviteID))
        {
            invite->SetRank(CalendarModerationRank(calendarEventModeratorStatus.Status));
            sCalendarMgr->UpdateInvite(invite);
            sCalendarMgr->SendCalendarEventModeratorStatus(*calendarEvent, *invite);
        }
        else
            sCalendarMgr->SendCalendarCommandResult(guid, CALENDAR_ERROR_NO_INVITE);
    }
    else
        sCalendarMgr->SendCalendarCommandResult(guid, CALENDAR_ERROR_EVENT_INVALID);
}

void WorldSession::HandleCalendarComplain(WorldPackets::Calendar::CalendarComplain& calendarComplain)
{
    if (sWorld->getBoolConfig(CONFIG_LOGSPAMREPORTS))
    {
        CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_INS_SPAM_REPORT);
        stmt->SetData(0, 2); // SpamType 2 = Calendar
        stmt->SetData(1, uint32(calendarComplain.InvitedByGUID.GetCounter()));
        stmt->SetData(2, 0);
        stmt->SetData(3, 0);
        stmt->SetData(4, 0);
        stmt->SetData(5, 0);
        stmt->SetData(6, "EventId: " + std::to_string(calendarComplain.EventID) + ", InviteId: " + std::to_string(calendarComplain.InviteID));
        stmt->SetData(7, uint32(GameTime::GetGameTime().count()));
        CharacterDatabase.Execute(stmt);
    }
}

void WorldSession::HandleCalendarGetNumPending(WorldPackets::Calendar::CalendarGetNumPending& /*calendarGetNumPending*/)
{
    SendPacket(WorldPackets::Calendar::CalendarSendNumPending(sCalendarMgr->GetPlayerNumPending(_player->GetGUID())).Write());
}

void WorldSession::HandleSetSavedInstanceExtend(WorldPackets::Calendar::SetSavedInstanceExtend& setSavedInstanceExtend)
{
    MapEntry const* entry = sMapStore.LookupEntry(setSavedInstanceExtend.MapID);
    if (!entry || !entry->IsRaid())
        return;

    InstancePlayerBind* instanceBind = sInstanceSaveMgr->PlayerGetBoundInstance(GetPlayer()->GetGUID(), setSavedInstanceExtend.MapID, Difficulty(setSavedInstanceExtend.DifficultyID));
    if (!instanceBind || !instanceBind->perm || setSavedInstanceExtend.Extend == instanceBind->extended)
        return;

    instanceBind->extended = setSavedInstanceExtend.Extend;

    // update in db
    CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_CHAR_INSTANCE_EXTENDED);
    stmt->SetData(0, setSavedInstanceExtend.Extend ? 1 : 0);
    stmt->SetData(1, GetPlayer()->GetGUID().GetCounter());
    stmt->SetData(2, instanceBind->save->GetInstanceId());
    CharacterDatabase.Execute(stmt);

    SendCalendarRaidLockoutUpdated(instanceBind->save, setSavedInstanceExtend.Extend);
}

// ----------------------------------- SEND ------------------------------------

void WorldSession::SendCalendarRaidLockout(InstanceSave const* save, bool add)
{
    time_t currTime = GameTime::GetGameTime().count();
    if (add)
    {
        WorldPackets::Calendar::CalendarRaidLockoutAdded calendarRaidLockoutAdded;
        calendarRaidLockoutAdded.InstanceID = save->GetInstanceId();
        calendarRaidLockoutAdded.ServerTime = uint32(currTime);
        calendarRaidLockoutAdded.MapID = int32(save->GetMapId());
        calendarRaidLockoutAdded.DifficultyID = save->GetDifficulty();
        calendarRaidLockoutAdded.TimeRemaining = int32(save->GetResetTime() >= currTime ? save->GetResetTime() - currTime : 0);
        SendPacket(calendarRaidLockoutAdded.Write());
    }
    else
    {
        WorldPackets::Calendar::CalendarRaidLockoutRemoved calendarRaidLockoutRemoved;
        calendarRaidLockoutRemoved.InstanceID = save->GetInstanceId();
        calendarRaidLockoutRemoved.MapID = int32(save->GetMapId());
        calendarRaidLockoutRemoved.DifficultyID = save->GetDifficulty();
        SendPacket(calendarRaidLockoutRemoved.Write());
    }
}

void WorldSession::SendCalendarRaidLockoutUpdated(InstanceSave const* save, bool isExtended)
{
    time_t currTime = GameTime::GetGameTime().count();
    time_t resetTime = isExtended ? save->GetExtendedResetTime() : save->GetResetTime();
    time_t resetTimeOp = isExtended ? save->GetResetTime() : save->GetExtendedResetTime();

    WorldPackets::Calendar::CalendarRaidLockoutUpdated packet;
    packet.DifficultyID = save->GetDifficulty();
    packet.MapID = int32(save->GetMapId());
    packet.ServerTime = currTime;
    packet.NewTimeRemaining = int32(resetTime >= currTime ? resetTime - currTime : 0);
    packet.OldTimeRemaining = int32(resetTimeOp >= currTime ? resetTimeOp - currTime : 0);
    SendPacket(packet.Write());
}
