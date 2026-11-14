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

#include "CalendarMgr.h"
#include "CalendarPackets.h"
#include "GameTime.h"
#include "GuildMgr.h"
#include "Log.h"
#include "ObjectAccessor.h"
#include "Opcodes.h"
#include "Player.h"
#include "QueryResult.h"
#include <unordered_map>

CalendarInvite::CalendarInvite() : _inviteId(1), _eventId(0), _statusTime(GameTime::GetGameTime().count()),
_status(CALENDAR_STATUS_INVITED), _rank(CALENDAR_RANK_PLAYER), _text("") { }

CalendarInvite::~CalendarInvite()
{
    // Free _inviteId only if it's a real invite and not just a pre-invite or guild announcement
    if (_inviteId != 0 && _eventId != 0)
        sCalendarMgr->FreeInviteId(_inviteId);
}

CalendarEvent::~CalendarEvent()
{
    sCalendarMgr->FreeEventId(_eventId);
}

CalendarMgr::CalendarMgr() : _maxEventId(0), _maxInviteId(0) { }

CalendarMgr::~CalendarMgr()
{
    for (CalendarEventStore::iterator itr = _events.begin(); itr != _events.end(); ++itr)
        delete *itr;

    for (CalendarEventInviteStore::iterator itr = _invites.begin(); itr != _invites.end(); ++itr)
        for (CalendarInviteStore::iterator itr2 = itr->second.begin(); itr2 != itr->second.end(); ++itr2)
            delete *itr2;
}

CalendarMgr* CalendarMgr::instance()
{
    static CalendarMgr instance;
    return &instance;
}

void CalendarMgr::LoadFromDB()
{
    uint32 count = 0;
    _maxEventId = 0;
    _maxInviteId = 0;

    //                                                       0   1        2      3            4     5        6          7      8
    if (QueryResult result = CharacterDatabase.Query("SELECT id, creator, title, description, type, dungeon, eventtime, flags, time2 FROM calendar_events"))
        do
        {
            Field* fields = result->Fetch();

            uint64 eventId          = fields[0].Get<uint64>();
            ObjectGuid creatorGUID  = ObjectGuid::Create<HighGuid::Player>(fields[1].Get<uint32>());
            std::string title       = fields[2].Get<std::string>();
            std::string description = fields[3].Get<std::string>();
            CalendarEventType type  = CalendarEventType(fields[4].Get<uint8>());
            int32 dungeonId         = fields[5].Get<int32>();
            uint32 eventTime        = fields[6].Get<uint32>();
            uint32 flags            = fields[7].Get<uint32>();
            uint32 timezoneTime     = fields[8].Get<uint32>();
            uint32 guildId = 0;

            if (flags & CALENDAR_FLAG_GUILD_EVENT || flags & CALENDAR_FLAG_WITHOUT_INVITES)
            {
                guildId = sCharacterCache->GetCharacterGuildIdByGuid(creatorGUID);
            }

            CalendarEvent* calendarEvent = new CalendarEvent(eventId, creatorGUID, guildId, type, dungeonId, time_t(eventTime), flags, time_t(timezoneTime), title, description);
            _events.insert(calendarEvent);

            _maxEventId = std::max(_maxEventId, eventId);

            ++count;
        } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} calendar events", count);
    count = 0;

    //                                                       0   1      2        3       4       5            6      7
    if (QueryResult result = CharacterDatabase.Query("SELECT id, event, invitee, sender, status, statustime, `rank`, text FROM calendar_invites"))
        do
        {
            Field* fields = result->Fetch();

            uint64 inviteId             = fields[0].Get<uint64>();
            uint64 eventId              = fields[1].Get<uint64>();
            ObjectGuid invitee          = ObjectGuid::Create<HighGuid::Player>(fields[2].Get<uint32>());
            ObjectGuid senderGUID       = ObjectGuid::Create<HighGuid::Player>(fields[3].Get<uint32>());
            CalendarInviteStatus status = CalendarInviteStatus(fields[4].Get<uint8>());
            uint32 statusTime           = fields[5].Get<uint32>();
            CalendarModerationRank rank = CalendarModerationRank(fields[6].Get<uint8>());
            std::string text            = fields[7].Get<std::string>();

            CalendarInvite* invite = new CalendarInvite(inviteId, eventId, invitee, senderGUID, time_t(statusTime), status, rank, text);
            _invites[eventId].push_back(invite);

            _maxInviteId = std::max(_maxInviteId, inviteId);

            ++count;
        } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} calendar invites", count);
    LOG_INFO("server.loading", " ");

    for (uint64 i = 1; i < _maxEventId; ++i)
        if (!GetEvent(i))
            _freeEventIds.push_back(i);

    for (uint64 i = 1; i < _maxInviteId; ++i)
        if (!GetInvite(i))
            _freeInviteIds.push_back(i);
}

void CalendarMgr::AddEvent(CalendarEvent* calendarEvent, CalendarSendEventType sendType)
{
    _events.insert(calendarEvent);
    UpdateEvent(calendarEvent);
    SendCalendarEvent(calendarEvent->GetCreatorGUID(), *calendarEvent, sendType);
}

void CalendarMgr::AddInvite(CalendarEvent* calendarEvent, CalendarInvite* invite, CharacterDatabaseTransaction trans)
{
    if (!calendarEvent->IsGuildAnnouncement())
        SendCalendarEventInvite(*invite);

    if (!calendarEvent->IsGuildEvent() || invite->GetInviteeGUID() == calendarEvent->GetCreatorGUID())
        SendCalendarEventInviteAlert(*calendarEvent, *invite);

    if (!calendarEvent->IsGuildAnnouncement())
    {
        _invites[invite->GetEventId()].push_back(invite);
        UpdateInvite(invite, trans);
    }
}

CalendarEventStore::iterator CalendarMgr::RemoveEvent(uint64 eventId, ObjectGuid remover)
{
    CalendarEventStore::iterator current;
    CalendarEvent* calendarEvent = GetEvent(eventId, &current);

    if (!calendarEvent)
    {
        SendCalendarCommandResult(remover, CALENDAR_ERROR_EVENT_INVALID);
        return _events.end();
    }

    CalendarEventStore::const_iterator constItr(current);
    return RemoveEvent(calendarEvent, remover, &constItr);
}

CalendarEventStore::iterator CalendarMgr::RemoveEvent(CalendarEvent* calendarEvent, ObjectGuid remover, CalendarEventStore::const_iterator* currIt) {
    if (!calendarEvent)
    {
        SendCalendarCommandResult(remover, CALENDAR_ERROR_EVENT_INVALID);
        return _events.end();
    }

    SendCalendarEventRemovedAlert(*calendarEvent);

    CharacterDatabaseTransaction trans = CharacterDatabase.BeginTransaction();
    CharacterDatabasePreparedStatement* stmt;
    MailDraft mail(calendarEvent->BuildCalendarMailSubject(remover), calendarEvent->BuildCalendarMailBody());

    CalendarInviteStore& eventInvites = _invites[calendarEvent->GetEventId()];
    for (std::size_t i = 0; i < eventInvites.size(); ++i)
    {
        CalendarInvite* invite = eventInvites[i];
        stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_CALENDAR_INVITE);
        stmt->SetData(0, invite->GetInviteId());
        trans->Append(stmt);

        // guild events only? check invite status here?
        // When an event is deleted, all invited (accepted/declined? - verify) guildies are notified via in-game mail. (wowwiki)
        if (remover && invite->GetInviteeGUID() != remover)
            mail.SendMailTo(trans, MailReceiver(invite->GetInviteeGUID().GetCounter()), calendarEvent, MAIL_CHECK_MASK_COPIED);

        delete invite;
    }

    _invites.erase(calendarEvent->GetEventId());

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_CALENDAR_EVENT);
    stmt->SetData(0, calendarEvent->GetEventId());
    trans->Append(stmt);
    CharacterDatabase.CommitTransaction(trans);

    if (currIt)
    {
        delete calendarEvent;
        return _events.erase(*currIt);
    }

    if (auto it = _events.find(calendarEvent); it != _events.end())
    {
        delete calendarEvent;
        return _events.erase(it);
    }

    delete calendarEvent;
    return _events.end();
}

void CalendarMgr::RemoveInvite(uint64 inviteId, uint64 eventId, ObjectGuid /*remover*/)
{
    CalendarEvent* calendarEvent = GetEvent(eventId);

    if (!calendarEvent)
        return;

    CalendarInviteStore::iterator itr = _invites[eventId].begin();
    for (; itr != _invites[eventId].end(); ++itr)
        if ((*itr)->GetInviteId() == inviteId)
            break;

    if (itr == _invites[eventId].end())
        return;

    CharacterDatabaseTransaction trans = CharacterDatabase.BeginTransaction();
    CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_CALENDAR_INVITE);
    stmt->SetData(0, (*itr)->GetInviteId());
    trans->Append(stmt);
    CharacterDatabase.CommitTransaction(trans);

    if (!calendarEvent->IsGuildEvent())
        SendCalendarEventInviteRemoveAlert((*itr)->GetInviteeGUID(), *calendarEvent, CALENDAR_STATUS_REMOVED);

    SendCalendarEventInviteRemove(*calendarEvent, **itr, calendarEvent->GetFlags());

    // we need to find out how to use CALENDAR_INVITE_REMOVED_MAIL_SUBJECT to force client to display different mail
    //if ((*itr)->GetInviteeGUID() != remover)
    //    MailDraft(calendarEvent->BuildCalendarMailSubject(remover), calendarEvent->BuildCalendarMailBody())
    //        .SendMailTo(trans, MailReceiver((*itr)->GetInvitee()), calendarEvent, MAIL_CHECK_MASK_COPIED);

    delete *itr;
    _invites[eventId].erase(itr);
}

void CalendarMgr::UpdateEvent(CalendarEvent* calendarEvent)
{
    CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_REP_CALENDAR_EVENT);
    stmt->SetData(0, calendarEvent->GetEventId());
    stmt->SetData(1, calendarEvent->GetCreatorGUID().GetCounter());
    stmt->SetData(2, calendarEvent->GetTitle());
    stmt->SetData(3, calendarEvent->GetDescription());
    stmt->SetData(4, calendarEvent->GetType());
    stmt->SetData(5, calendarEvent->GetDungeonId());
    stmt->SetData(6, uint32(calendarEvent->GetEventTime()));
    stmt->SetData(7, calendarEvent->GetFlags());
    stmt->SetData(8, (int64)calendarEvent->GetTimeZoneTime()); // correct?
    CharacterDatabase.Execute(stmt);
}

void CalendarMgr::UpdateInvite(CalendarInvite* invite, CharacterDatabaseTransaction trans)
{
    CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_REP_CALENDAR_INVITE);
    stmt->SetData(0, invite->GetInviteId());
    stmt->SetData(1, invite->GetEventId());
    stmt->SetData(2, invite->GetInviteeGUID().GetCounter());
    stmt->SetData(3, invite->GetSenderGUID().GetCounter());
    stmt->SetData(4, invite->GetStatus());
    stmt->SetData(5, uint32(invite->GetStatusTime()));
    stmt->SetData(6, invite->GetRank());
    stmt->SetData(7, invite->GetText());
    CharacterDatabase.ExecuteOrAppend(trans, stmt);
}

void CalendarMgr::RemoveAllPlayerEventsAndInvites(ObjectGuid guid)
{
    for (CalendarEventStore::const_iterator itr = _events.begin(); itr != _events.end();)
    {
        if (CalendarEvent* event = *itr; event->GetCreatorGUID() == guid)
        {
            itr = RemoveEvent(event, ObjectGuid::Empty, &itr);
            continue;
        }
        ++itr;
    }

    CalendarInviteStore playerInvites = GetPlayerInvites(guid);
    for (CalendarInviteStore::const_iterator itr = playerInvites.begin(); itr != playerInvites.end(); ++itr)
        RemoveInvite((*itr)->GetInviteId(), (*itr)->GetEventId(), guid);
}

void CalendarMgr::RemovePlayerGuildEventsAndSignups(ObjectGuid guid, uint32 guildId)
{
    for (CalendarEventStore::const_iterator itr = _events.begin(); itr != _events.end();)
    {
        if ((*itr)->GetCreatorGUID() == guid && ((*itr)->IsGuildEvent() || (*itr)->IsGuildAnnouncement()))
        {
            itr = RemoveEvent((*itr)->GetEventId(), guid);
            continue;
        }
        ++itr;
    }

    CalendarInviteStore playerInvites = GetPlayerInvites(guid);
    for (CalendarInviteStore::const_iterator itr = playerInvites.begin(); itr != playerInvites.end(); ++itr)
        if (CalendarEvent* calendarEvent = GetEvent((*itr)->GetEventId(), nullptr))
            if (calendarEvent->IsGuildEvent() && calendarEvent->GetGuildId() == guildId)
                RemoveInvite((*itr)->GetInviteId(), (*itr)->GetEventId(), guid);
}

CalendarEvent* CalendarMgr::GetEvent(uint64 eventId, CalendarEventStore::iterator* it)
{
    for (CalendarEventStore::iterator itr = _events.begin(); itr != _events.end(); ++itr)
        if ((*itr)->GetEventId() == eventId)
        {
            if (it)
                *it = itr;

            return *itr;
        }

    return nullptr;
}

CalendarInvite* CalendarMgr::GetInvite(uint64 inviteId) const
{
    for (CalendarEventInviteStore::const_iterator itr = _invites.begin(); itr != _invites.end(); ++itr)
        for (CalendarInviteStore::const_iterator itr2 = itr->second.begin(); itr2 != itr->second.end(); ++itr2)
            if ((*itr2)->GetInviteId() == inviteId)
                return *itr2;

    LOG_DEBUG("entities.unit", "CalendarMgr::GetInvite: [{}] not found!", inviteId);
    return nullptr;
}

void CalendarMgr::FreeEventId(uint64 id)
{
    if (id == _maxEventId)
        --_maxEventId;
    else
        _freeEventIds.push_back(id);
}

uint64 CalendarMgr::GetFreeEventId()
{
    if (_freeEventIds.empty())
        return ++_maxEventId;

    uint64 eventId = _freeEventIds.front();
    _freeEventIds.pop_front();
    return eventId;
}

void CalendarMgr::FreeInviteId(uint64 id)
{
    if (id == _maxInviteId)
        --_maxInviteId;
    else
        _freeInviteIds.push_back(id);
}

uint64 CalendarMgr::GetFreeInviteId()
{
    if (_freeInviteIds.empty())
        return ++_maxInviteId;

    uint64 inviteId = _freeInviteIds.front();
    _freeInviteIds.pop_front();
    return inviteId;
}

void CalendarMgr::DeleteOldEvents()
{
    time_t oldEventsTime = GameTime::GetGameTime().count() - CALENDAR_OLD_EVENTS_DELETION_TIME;

    for (CalendarEventStore::const_iterator itr = _events.begin(); itr != _events.end();)
    {
        if (CalendarEvent* event = *itr; event->GetEventTime() < oldEventsTime)
        {
            itr = RemoveEvent(event, ObjectGuid::Empty, &itr);
            continue;
        }
        ++itr;
    }
}

CalendarEventStore CalendarMgr::GetEventsCreatedBy(ObjectGuid guid, bool includeGuildEvents)
{
    CalendarEventStore result;
    for (CalendarEventStore::const_iterator itr = _events.begin(); itr != _events.end(); ++itr)
        if ((*itr)->GetCreatorGUID() == guid && (includeGuildEvents || (!(*itr)->IsGuildEvent() && !(*itr)->IsGuildAnnouncement())))
            result.insert(*itr);

    return result;
}

CalendarEventStore CalendarMgr::GetGuildEvents(uint32 guildId)
{
    CalendarEventStore result;

    if (!guildId)
        return result;

    for (CalendarEventStore::const_iterator itr = _events.begin(); itr != _events.end(); ++itr)
        if ((*itr)->IsGuildEvent() || (*itr)->IsGuildAnnouncement())
            if ((*itr)->GetGuildId() == guildId)
                result.insert(*itr);

    return result;
}

CalendarEventStore CalendarMgr::GetPlayerEvents(ObjectGuid guid)
{
    CalendarEventStore events;

    for (CalendarEventInviteStore::const_iterator itr = _invites.begin(); itr != _invites.end(); ++itr)
        for (CalendarInviteStore::const_iterator itr2 = itr->second.begin(); itr2 != itr->second.end(); ++itr2)
            if ((*itr2)->GetInviteeGUID() == guid)
                if (CalendarEvent* event = GetEvent(itr->first)) // nullptr check added as attempt to fix #11512
                    events.insert(event);

    if (Player* player = ObjectAccessor::FindConnectedPlayer(guid))
        if (player->GetGuildId())
            for (CalendarEventStore::const_iterator itr = _events.begin(); itr != _events.end(); ++itr)
                if ((*itr)->GetGuildId() == player->GetGuildId())
                    events.insert(*itr);

    return events;
}

CalendarInviteStore const& CalendarMgr::GetEventInvites(uint64 eventId)
{
    return _invites[eventId];
}

CalendarInviteStore CalendarMgr::GetPlayerInvites(ObjectGuid guid)
{
    CalendarInviteStore invites;

    for (CalendarEventInviteStore::const_iterator itr = _invites.begin(); itr != _invites.end(); ++itr)
        for (CalendarInviteStore::const_iterator itr2 = itr->second.begin(); itr2 != itr->second.end(); ++itr2)
            if ((*itr2)->GetInviteeGUID() == guid)
                invites.push_back(*itr2);

    return invites;
}

uint32 CalendarMgr::GetPlayerNumPending(ObjectGuid guid)
{
    CalendarInviteStore const& invites = GetPlayerInvites(guid);

    uint32 pendingNum = 0;
    for (CalendarInviteStore::const_iterator itr = invites.begin(); itr != invites.end(); ++itr)
    {
        switch ((*itr)->GetStatus())
        {
            case CALENDAR_STATUS_INVITED:
            case CALENDAR_STATUS_TENTATIVE:
            case CALENDAR_STATUS_NOT_SIGNED_UP:
                ++pendingNum;
                break;
            default:
                break;
        }
    }

    return pendingNum;
}

std::string CalendarEvent::BuildCalendarMailSubject(ObjectGuid remover) const
{
    std::ostringstream strm;
    strm << remover << ':' << _title;
    return strm.str();
}

std::string CalendarEvent::BuildCalendarMailBody() const
{
    WorldPacket data;
    uint32 time;
    std::ostringstream strm;

    // we are supposed to send PackedTime so i used WorldPacket to pack it
    data.AppendPackedTime(_eventTime);
    data >> time;
    strm << time;
    return strm.str();
}

// 6.x 19342: packets ported from TrinityCore 6.x (Calendar/CalendarMgr.cpp)
void CalendarMgr::SendCalendarEventInvite(CalendarInvite const& invite)
{
    CalendarEvent* calendarEvent = GetEvent(invite.GetEventId());

    ObjectGuid invitee = invite.GetInviteeGUID();
    Player* player = ObjectAccessor::FindConnectedPlayer(invitee);

    WorldPackets::Calendar::SCalendarEventInvite packet;
    packet.EventID = calendarEvent ? calendarEvent->GetEventId() : 0;
    packet.InviteGuid = invitee;
    packet.InviteID = calendarEvent ? invite.GetInviteId() : 0;
    packet.Level = player ? player->GetLevel() : sCharacterCache->GetCharacterLevelByGuid(invitee);
    packet.ResponseTime = invite.GetStatusTime();
    packet.Status = invite.GetStatus();
    packet.Type = calendarEvent ? calendarEvent->IsGuildEvent() : 0;
    packet.ClearPending = calendarEvent ? !calendarEvent->IsGuildEvent() : true;
    packet.Write();

    if (!calendarEvent) // Pre-invite
    {
        if (Player* sender = ObjectAccessor::FindConnectedPlayer(invite.GetSenderGUID()))
            sender->SendDirectMessage(packet.GetRawPacket());
    }
    else if (calendarEvent->GetCreatorGUID() != invite.GetInviteeGUID()) // correct?
        SendPacketToAllEventRelatives(packet.Move(), *calendarEvent);
}

void CalendarMgr::SendCalendarEventUpdateAlert(CalendarEvent const& calendarEvent, time_t oldEventTime)
{
    WorldPackets::Calendar::CalendarEventUpdatedAlert packet;
    packet.ClearPending = true;
    packet.Date = calendarEvent.GetEventTime();
    packet.Description = calendarEvent.GetDescription();
    packet.EventID = calendarEvent.GetEventId();
    packet.EventName = calendarEvent.GetTitle();
    packet.EventType = calendarEvent.GetType();
    packet.Flags = calendarEvent.GetFlags();
    packet.LockDate = calendarEvent.GetTimeZoneTime();
    packet.OriginalDate = oldEventTime;
    packet.TextureID = calendarEvent.GetDungeonId();
    packet.Write();

    SendPacketToAllEventRelatives(packet.Move(), calendarEvent);
}

void CalendarMgr::SendCalendarEventStatus(CalendarEvent const& calendarEvent, CalendarInvite const& invite)
{
    WorldPackets::Calendar::CalendarEventInviteStatus packet;
    packet.ClearPending = true;
    packet.Date = calendarEvent.GetEventTime();
    packet.EventID = calendarEvent.GetEventId();
    packet.Flags = calendarEvent.GetFlags();
    packet.InviteGuid = invite.GetInviteeGUID();
    packet.ResponseTime = invite.GetStatusTime();
    packet.Status = invite.GetStatus();
    packet.Write();

    SendPacketToAllEventRelatives(packet.Move(), calendarEvent);
}

void CalendarMgr::SendCalendarEventStatusAlert(CalendarEvent const& calendarEvent, CalendarInvite const& invite)
{
    if (Player* player = ObjectAccessor::FindConnectedPlayer(invite.GetInviteeGUID()))
    {
        WorldPackets::Calendar::CalendarEventInviteStatusAlert packet;
        packet.EventID = calendarEvent.GetEventId();
        packet.Flags = calendarEvent.GetFlags();
        packet.Date = calendarEvent.GetEventTime();
        packet.Status = invite.GetStatus();
        player->SendDirectMessage(packet.Write());
    }
}

void CalendarMgr::SendCalendarEventRemovedAlert(CalendarEvent const& calendarEvent)
{
    WorldPackets::Calendar::CalendarEventRemovedAlert packet;
    packet.ClearPending = true;
    packet.Date = calendarEvent.GetEventTime();
    packet.EventID = calendarEvent.GetEventId();
    packet.Write();

    SendPacketToAllEventRelatives(packet.Move(), calendarEvent);
}

void CalendarMgr::SendCalendarEventInviteRemove(CalendarEvent const& calendarEvent, CalendarInvite const& invite, uint32 flags)
{
    WorldPackets::Calendar::CalendarEventInviteRemoved packet;
    packet.ClearPending = true;
    packet.EventID = calendarEvent.GetEventId();
    packet.Flags = flags;
    packet.InviteGuid = invite.GetInviteeGUID();
    packet.Write();

    SendPacketToAllEventRelatives(packet.Move(), calendarEvent);
}

void CalendarMgr::SendCalendarEventModeratorStatus(CalendarEvent const& calendarEvent, CalendarInvite const& invite)
{
    WorldPackets::Calendar::CalendarEventInviteModeratorStatus packet;
    packet.ClearPending = true;
    packet.EventID = calendarEvent.GetEventId();
    packet.InviteGuid = invite.GetInviteeGUID();
    packet.Status = invite.GetRank();
    packet.Write();

    SendPacketToAllEventRelatives(packet.Move(), calendarEvent);
}

void CalendarMgr::SendCalendarEventInviteAlert(CalendarEvent const& calendarEvent, CalendarInvite const& invite)
{
    Guild* guild = sGuildMgr->GetGuildById(calendarEvent.GetGuildId());

    WorldPackets::Calendar::CalendarEventInviteAlert packet;
    packet.Date = calendarEvent.GetEventTime();
    packet.EventID = calendarEvent.GetEventId();
    packet.EventName = calendarEvent.GetTitle();
    packet.EventType = calendarEvent.GetType();
    packet.Flags = calendarEvent.GetFlags();
    packet.InviteID = invite.GetInviteId();
    packet.InvitedByGuid = invite.GetSenderGUID();
    packet.ModeratorStatus = invite.GetRank();
    packet.OwnerGuid = calendarEvent.GetCreatorGUID();
    packet.Status = invite.GetStatus();
    packet.TextureID = calendarEvent.GetDungeonId();
    packet.EventGuildID = guild ? guild->GetGUID() : ObjectGuid::Empty;

    if (calendarEvent.IsGuildEvent() || calendarEvent.IsGuildAnnouncement())
    {
        if (guild)
            guild->BroadcastPacket(packet.Write());
    }
    else if (Player* player = ObjectAccessor::FindConnectedPlayer(invite.GetInviteeGUID()))
        player->SendDirectMessage(packet.Write());
}

void CalendarMgr::SendCalendarEvent(ObjectGuid guid, CalendarEvent const& calendarEvent, CalendarSendEventType sendType)
{
    Player* player = ObjectAccessor::FindConnectedPlayer(guid);
    if (!player)
        return;

    Guild* guild = sGuildMgr->GetGuildById(calendarEvent.GetGuildId());

    WorldPackets::Calendar::CalendarSendEvent packet;
    packet.Date = calendarEvent.GetEventTime();
    packet.Description = calendarEvent.GetDescription();
    packet.EventID = calendarEvent.GetEventId();
    packet.EventName = calendarEvent.GetTitle();
    packet.EventType = sendType;
    packet.Flags = calendarEvent.GetFlags();
    packet.GetEventType = calendarEvent.GetType();
    packet.LockDate = calendarEvent.GetTimeZoneTime();
    packet.OwnerGuid = calendarEvent.GetCreatorGUID();
    packet.TextureID = calendarEvent.GetDungeonId();
    packet.EventGuildID = guild ? guild->GetGUID() : ObjectGuid::Empty;

    for (CalendarInvite const* calendarInvite : _invites[calendarEvent.GetEventId()])
    {
        ObjectGuid inviteeGuid = calendarInvite->GetInviteeGUID();
        Player* invitee = ObjectAccessor::FindConnectedPlayer(inviteeGuid);

        uint32 inviteeGuildId = invitee ? invitee->GetGuildId() : sCharacterCache->GetCharacterGuildIdByGuid(inviteeGuid);

        WorldPackets::Calendar::CalendarEventInviteInfo inviteInfo;
        inviteInfo.Guid = inviteeGuid;
        inviteInfo.Level = invitee ? invitee->GetLevel() : sCharacterCache->GetCharacterLevelByGuid(inviteeGuid);
        inviteInfo.Status = calendarInvite->GetStatus();
        inviteInfo.Moderator = calendarInvite->GetRank();
        inviteInfo.InviteType = calendarEvent.IsGuildEvent() && calendarEvent.GetGuildId() == inviteeGuildId;
        inviteInfo.InviteID = calendarInvite->GetInviteId();
        inviteInfo.ResponseTime = calendarInvite->GetStatusTime();
        inviteInfo.Notes = calendarInvite->GetText();
        packet.Invites.push_back(inviteInfo);
    }

    player->SendDirectMessage(packet.Write());
}

void CalendarMgr::SendCalendarEventInviteRemoveAlert(ObjectGuid guid, CalendarEvent const& calendarEvent, CalendarInviteStatus status)
{
    if (Player* player = ObjectAccessor::FindConnectedPlayer(guid))
    {
        WorldPackets::Calendar::CalendarEventInviteRemovedAlert packet;
        packet.Date = calendarEvent.GetEventTime();
        packet.EventID = calendarEvent.GetEventId();
        packet.Flags = calendarEvent.GetFlags();
        packet.Status = status;
        player->SendDirectMessage(packet.Write());
    }
}

void CalendarMgr::SendCalendarClearPendingAction(ObjectGuid guid)
{
    if (Player* player = ObjectAccessor::FindConnectedPlayer(guid))
        player->SendDirectMessage(WorldPackets::Calendar::CalendarClearPendingAction().Write());
}

void CalendarMgr::SendCalendarCommandResult(ObjectGuid guid, CalendarError err, char const* param /*= nullptr*/)
{
    if (Player* player = ObjectAccessor::FindConnectedPlayer(guid))
    {
        WorldPackets::Calendar::CalendarCommandResult packet;
        packet.Command = 1; // FIXME
        packet.Result = err;

        switch (err)
        {
            case CALENDAR_ERROR_OTHER_INVITES_EXCEEDED:
            case CALENDAR_ERROR_ALREADY_INVITED_TO_EVENT_S:
            case CALENDAR_ERROR_IGNORING_YOU_S:
                if (param)
                    packet.Name = param;
                break;
            default:
                break;
        }

        player->SendDirectMessage(packet.Write());
    }
}

void CalendarMgr::SendPacketToAllEventRelatives(WorldPacket packet, CalendarEvent const& calendarEvent)
{
    // Send packet to all guild members
    if (calendarEvent.IsGuildEvent() || calendarEvent.IsGuildAnnouncement())
        if (Guild* guild = sGuildMgr->GetGuildById(calendarEvent.GetGuildId()))
            guild->BroadcastPacket(&packet);

    // Send packet to all invitees if event is non-guild, in other case only to non-guild invitees (packet was broadcasted for them)
    CalendarInviteStore invites = _invites[calendarEvent.GetEventId()];
    for (CalendarInviteStore::iterator itr = invites.begin(); itr != invites.end(); ++itr)
        if (Player* player = ObjectAccessor::FindConnectedPlayer((*itr)->GetInviteeGUID()))
            if (!calendarEvent.IsGuildEvent() || player->GetGuildId() != calendarEvent.GetGuildId())
                player->SendDirectMessage(&packet);
}
