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
    strm << remover.GetRawValue() << ':' << _title;
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

// 5.4.8 18414, from Project SkyFire
void CalendarMgr::SendCalendarEventInvite(CalendarInvite const& invite)
{
    CalendarEvent* calendarEvent = GetEvent(invite.GetEventId());
    time_t statusTime = invite.GetStatusTime();
    bool hasStatusTime = statusTime != 946684800;   // 01/01/2000 00:00:00

    ObjectGuid invitee = invite.GetInviteeGUID();
    Player* player = ObjectAccessor::FindConnectedPlayer(invitee);

    uint8 level = player ? player->GetLevel() : sCharacterCache->GetCharacterLevelByGuid(invitee);

    WorldPacket data(SMSG_CALENDAR_EVENT_INVITE, 8 + 8 + 8 + 1 + 1 + 1 + (hasStatusTime ? 4 : 0) + 2);
    data << uint8(invite.GetSenderGUID() == invite.GetInviteeGUID()); // true only if the invite is sign-up
    data << uint8(invite.GetStatus());
    data << uint64(invite.GetInviteId());
    data << uint8(level);
    data << uint64(invite.GetEventId());

    data.WriteGuidMask(invitee, { 6, 4, 1, 3, 7, 0, 2, 5 });
    data.WriteBit(!hasStatusTime);
    data.WriteBit(true);    // clear pending action
    data.FlushBits();

    data.WriteGuidBytes(invitee, { 7, 0, 5 });
    if (hasStatusTime)
        data.AppendPackedTime(statusTime);
    data.WriteGuidBytes(invitee, { 2, 3, 4, 1, 6 });

    if (!calendarEvent) // Pre-invite
    {
        if (Player* sender = ObjectAccessor::FindConnectedPlayer(invite.GetSenderGUID()))
            sender->SendDirectMessage(&data);
    }
    else
    {
        if (calendarEvent->GetCreatorGUID() != invite.GetInviteeGUID()) // correct?
            SendPacketToAllEventRelatives(data, *calendarEvent);
    }
}

// 5.4.8 18414, from Project SkyFire
void CalendarMgr::SendCalendarEventUpdateAlert(CalendarEvent const& calendarEvent, time_t oldEventTime)
{
    WorldPacket data(SMSG_CALENDAR_EVENT_UPDATED_ALERT, 4 + 1 + 4 + 4 + 8 + 4 + 4 + 3 +
                     calendarEvent.GetTitle().size() + calendarEvent.GetDescription().size());
    data << uint32(calendarEvent.GetFlags());
    data << uint8(calendarEvent.GetType());
    data << uint32(0);      // lock date
    data.AppendPackedTime(oldEventTime);
    data << uint64(calendarEvent.GetEventId());
    data.AppendPackedTime(calendarEvent.GetEventTime());
    data << int32(calendarEvent.GetDungeonId());
    data.WriteBit(true);    // clear pending action
    data.WriteBits(calendarEvent.GetDescription().size(), 11);
    data.WriteBits(calendarEvent.GetTitle().size(), 8);
    data.FlushBits();

    data.WriteString(calendarEvent.GetTitle());
    data.WriteString(calendarEvent.GetDescription());

    SendPacketToAllEventRelatives(data, calendarEvent);
}

// 5.4.8 18414, from Project SkyFire
void CalendarMgr::SendCalendarEventStatus(CalendarEvent const& calendarEvent, CalendarInvite const& invite)
{
    ObjectGuid invitee = invite.GetInviteeGUID();
    WorldPacket data(SMSG_CALENDAR_EVENT_INVITE_STATUS, 8 + 8 + 4 + 4 + 1 + 4 + 2);
    data.WriteGuidMask(invitee, { 5, 0 });
    data.WriteBit(true);    // clear pending action
    data.WriteGuidMask(invitee, { 2, 1, 4, 6, 7, 3 });
    data.FlushBits();

    data.WriteGuidBytes(invitee, { 1, 6, 7 });
    data.AppendPackedTime(invite.GetStatusTime());
    data.WriteGuidBytes(invitee, { 2, 4 });
    data << uint64(calendarEvent.GetEventId());
    data.WriteGuidBytes(invitee, { 0, 3, 5 });
    data << uint8(invite.GetStatus());
    data << uint32(calendarEvent.GetFlags());
    data.AppendPackedTime(calendarEvent.GetEventTime());

    SendPacketToAllEventRelatives(data, calendarEvent);
}

// 5.4.8 18414, from Project SkyFire
void CalendarMgr::SendCalendarEventStatusAlert(CalendarEvent const& calendarEvent, CalendarInvite const& invite)
{
    if (Player* player = ObjectAccessor::FindConnectedPlayer(invite.GetInviteeGUID()))
    {
        WorldPacket data(SMSG_CALENDAR_EVENT_INVITE_STATUS_ALERT, 8 + 4 + 1 + 4);
        data << uint64(calendarEvent.GetEventId());
        data << uint32(calendarEvent.GetFlags());
        data << uint8(invite.GetStatus());
        data.AppendPackedTime(calendarEvent.GetEventTime());

        player->SendDirectMessage(&data);
    }
}

// 5.4.8 18414, from Project SkyFire
void CalendarMgr::SendCalendarEventRemovedAlert(CalendarEvent const& calendarEvent)
{
    WorldPacket data(SMSG_CALENDAR_EVENT_REMOVED_ALERT, 8 + 4 + 1);
    data << uint64(calendarEvent.GetEventId());
    data.AppendPackedTime(calendarEvent.GetEventTime());
    data.WriteBit(true);    // clear pending action
    data.FlushBits();

    SendPacketToAllEventRelatives(data, calendarEvent);
}

// 5.4.8 18414, from Project SkyFire
void CalendarMgr::SendCalendarEventInviteRemove(CalendarEvent const& calendarEvent, CalendarInvite const& invite, uint32 flags)
{
    ObjectGuid invitee = invite.GetInviteeGUID();
    WorldPacket data(SMSG_CALENDAR_EVENT_INVITE_REMOVED, 8 + 8 + 4 + 1);
    data.WriteGuidMask(invitee, { 6, 7, 3, 0, 2, 4, 1, 5 });
    data.WriteBit(true);    // clear pending action
    data.FlushBits();

    data.WriteGuidBytes(invitee, { 0, 4, 3, 5 });
    data << uint64(invite.GetEventId());
    data.WriteGuidBytes(invitee, { 7, 1, 2 });
    data << uint32(flags);
    data.WriteGuidBytes(invitee, { 6 });

    SendPacketToAllEventRelatives(data, calendarEvent);
}

// 5.4.8 18414, from Project SkyFire
void CalendarMgr::SendCalendarEventModeratorStatus(CalendarEvent const& calendarEvent, CalendarInvite const& invite)
{
    ObjectGuid invitee = invite.GetInviteeGUID();
    WorldPacket data(SMSG_CALENDAR_EVENT_MODERATOR_STATUS, 8 + 8 + 1 + 1);
    data.WriteGuidMask(invitee, { 3, 7, 2, 4, 1, 6, 5 });
    data.WriteBit(true);    // clear pending action
    data.WriteGuidMask(invitee, { 0 });
    data.FlushBits();

    data.WriteGuidBytes(invitee, { 5, 4, 6, 1, 7, 2, 3 });
    data << uint8(invite.GetRank());
    data << uint64(invite.GetEventId());
    data.WriteGuidBytes(invitee, { 0 });

    SendPacketToAllEventRelatives(data, calendarEvent);
}

// 5.4.8 18414, from Project SkyFire
void CalendarMgr::SendCalendarEventInviteAlert(CalendarEvent const& calendarEvent, CalendarInvite const& invite)
{
    Guild* guild = sGuildMgr->GetGuildById(calendarEvent.GetGuildId());
    ObjectGuid guildGuid = guild ? guild->GetGUID() : ObjectGuid::Empty;
    ObjectGuid creatorGuid = calendarEvent.GetCreatorGUID();
    ObjectGuid senderGuid = invite.GetSenderGUID();

    WorldPacket data(SMSG_CALENDAR_EVENT_INVITE_ALERT, 8 + 4 + 1 + 8 + 4 + 1 + 4 + 1 + 4 + 3 * 8 + calendarEvent.GetTitle().size());
    data << uint64(calendarEvent.GetEventId());
    data << int32(calendarEvent.GetDungeonId());
    data << uint8(calendarEvent.GetType());
    data << uint64(invite.GetInviteId());
    data << uint32(calendarEvent.GetFlags());
    data << uint8(invite.GetStatus());
    data.AppendPackedTime(calendarEvent.GetEventTime());
    data << uint8(invite.GetRank());

    data.WriteGuidMask(guildGuid, { 7 });
    data.WriteGuidMask(creatorGuid, { 6 });
    data.WriteGuidMask(guildGuid, { 4, 0 });
    data.WriteGuidMask(creatorGuid, { 3 });
    data.WriteGuidMask(senderGuid, { 1 });
    data.WriteGuidMask(guildGuid, { 5 });
    data.WriteGuidMask(creatorGuid, { 2, 0 });
    data.WriteGuidMask(guildGuid, { 1 });
    data.WriteGuidMask(creatorGuid, { 5 });
    data.WriteGuidMask(guildGuid, { 6, 3 });
    data.WriteGuidMask(senderGuid, { 6, 2 });
    data.WriteGuidMask(creatorGuid, { 4, 7 });
    data.WriteGuidMask(senderGuid, { 4 });
    data.WriteGuidMask(creatorGuid, { 1 });
    data.WriteGuidMask(senderGuid, { 3 });
    data.WriteGuidMask(guildGuid, { 2 });
    data.WriteGuidMask(senderGuid, { 0 });
    data.WriteBits(calendarEvent.GetTitle().size(), 8);
    data.WriteGuidMask(senderGuid, { 5, 7 });
    data.FlushBits();

    data.WriteGuidBytes(creatorGuid, { 5 });
    data.WriteGuidBytes(guildGuid, { 6, 0 });
    data.WriteGuidBytes(creatorGuid, { 6 });
    data.WriteGuidBytes(senderGuid, { 5, 4 });
    data.WriteString(calendarEvent.GetTitle());
    data.WriteGuidBytes(creatorGuid, { 0, 1 });
    data.WriteGuidBytes(guildGuid, { 2, 7 });
    data.WriteGuidBytes(senderGuid, { 6 });
    data.WriteGuidBytes(guildGuid, { 1 });
    data.WriteGuidBytes(creatorGuid, { 2 });
    data.WriteGuidBytes(senderGuid, { 3, 7, 0 });
    data.WriteGuidBytes(creatorGuid, { 3 });
    data.WriteGuidBytes(senderGuid, { 2 });
    data.WriteGuidBytes(creatorGuid, { 7 });
    data.WriteGuidBytes(guildGuid, { 5, 4, 3 });
    data.WriteGuidBytes(senderGuid, { 1 });
    data.WriteGuidBytes(creatorGuid, { 4 });

    if (calendarEvent.IsGuildEvent() || calendarEvent.IsGuildAnnouncement())
    {
        if (guild)
            guild->BroadcastPacket(&data);
    }
    else if (Player* player = ObjectAccessor::FindConnectedPlayer(invite.GetInviteeGUID()))
        player->SendDirectMessage(&data);
}

// 5.4.8 18414, from Project SkyFire
void CalendarMgr::SendCalendarEvent(ObjectGuid guid, CalendarEvent const& calendarEvent, CalendarSendEventType sendType)
{
    Player* player = ObjectAccessor::FindConnectedPlayer(guid);
    if (!player)
        return;

    CalendarInviteStore const& eventInviteeList = _invites[calendarEvent.GetEventId()];

    Guild* guild = sGuildMgr->GetGuildById(calendarEvent.GetGuildId());
    ObjectGuid creatorGuid = calendarEvent.GetCreatorGUID();
    ObjectGuid guildGuid = guild ? guild->GetGUID() : ObjectGuid::Empty;

    ByteBuffer inviteeData;
    WorldPacket data(SMSG_CALENDAR_SEND_EVENT, 60 + eventInviteeList.size() * 32);
    data.WriteBits(eventInviteeList.size(), 20);

    for (CalendarInviteStore::const_iterator itr = eventInviteeList.begin(); itr != eventInviteeList.end(); ++itr)
    {
        CalendarInvite const* calendarInvite = (*itr);
        ObjectGuid inviteeGuid = calendarInvite->GetInviteeGUID();
        Player* invitee = ObjectAccessor::FindConnectedPlayer(inviteeGuid);

        uint8 inviteeLevel = invitee ? invitee->GetLevel() : sCharacterCache->GetCharacterLevelByGuid(inviteeGuid);
        uint32 inviteeGuildId = invitee ? invitee->GetGuildId() : sCharacterCache->GetCharacterGuildIdByGuid(inviteeGuid);

        data.WriteGuidMask(inviteeGuid, { 1, 2, 0, 7, 3, 5, 6 });
        data.WriteBits(calendarInvite->GetText().size(), 8);
        data.WriteGuidMask(inviteeGuid, { 4 });

        inviteeData.AppendPackedTime(calendarInvite->GetStatusTime());
        inviteeData.WriteGuidBytes(inviteeGuid, { 5 });
        inviteeData << uint8(calendarEvent.IsGuildEvent() && calendarEvent.GetGuildId() == inviteeGuildId);
        inviteeData.WriteGuidBytes(inviteeGuid, { 1, 2, 6 });
        inviteeData.WriteString(calendarInvite->GetText());
        inviteeData << uint8(inviteeLevel);
        inviteeData.WriteGuidBytes(inviteeGuid, { 7 });
        inviteeData << uint8(calendarInvite->GetRank());
        inviteeData << uint64(calendarInvite->GetInviteId());
        inviteeData.WriteGuidBytes(inviteeGuid, { 0, 3, 4 });
        inviteeData << uint8(calendarInvite->GetStatus());
    }

    data.WriteBits(calendarEvent.GetTitle().size(), 8);
    data.WriteGuidMask(creatorGuid, { 2, 0 });
    data.WriteGuidMask(guildGuid, { 4, 5 });
    data.WriteGuidMask(creatorGuid, { 1, 5, 3 });
    data.WriteGuidMask(guildGuid, { 6 });
    data.WriteBits(calendarEvent.GetDescription().size(), 11);
    data.WriteGuidMask(guildGuid, { 1, 7 });
    data.WriteGuidMask(creatorGuid, { 6 });
    data.WriteGuidMask(guildGuid, { 2, 0 });
    data.WriteGuidMask(creatorGuid, { 4 });
    data.WriteGuidMask(guildGuid, { 3 });
    data.WriteGuidMask(creatorGuid, { 7 });
    data.FlushBits();

    data.append(inviteeData);
    data.WriteGuidBytes(creatorGuid, { 0, 5 });
    data << uint32(calendarEvent.GetFlags());
    data.WriteGuidBytes(guildGuid, { 1 });
    data.WriteGuidBytes(creatorGuid, { 7, 3 });
    data.AppendPackedTime(calendarEvent.GetEventTime());
    data.WriteGuidBytes(guildGuid, { 6 });
    data.WriteGuidBytes(creatorGuid, { 4 });
    data << int32(calendarEvent.GetDungeonId());
    data << uint32(0);      // lock date
    data.WriteGuidBytes(guildGuid, { 4 });
    data << uint8(calendarEvent.GetType());
    data.WriteString(calendarEvent.GetTitle());
    data.WriteGuidBytes(creatorGuid, { 6 });
    data.WriteGuidBytes(guildGuid, { 3 });
    data.WriteGuidBytes(creatorGuid, { 2 });
    data.WriteGuidBytes(guildGuid, { 7 });
    data.WriteGuidBytes(creatorGuid, { 1 });
    data.WriteString(calendarEvent.GetDescription());
    data.WriteGuidBytes(guildGuid, { 2, 5, 0 });
    data << uint64(calendarEvent.GetEventId());
    data << uint8(sendType);

    player->SendDirectMessage(&data);
}

// 5.4.8 18414, from Project SkyFire
void CalendarMgr::SendCalendarEventInviteRemoveAlert(ObjectGuid guid, CalendarEvent const& calendarEvent, CalendarInviteStatus status)
{
    if (Player* player = ObjectAccessor::FindConnectedPlayer(guid))
    {
        WorldPacket data(SMSG_CALENDAR_EVENT_INVITE_REMOVED_ALERT, 4 + 1 + 8 + 4);
        data << uint32(calendarEvent.GetFlags());
        data << uint8(status);
        data << uint64(calendarEvent.GetEventId());
        data.AppendPackedTime(calendarEvent.GetEventTime());

        player->SendDirectMessage(&data);
    }
}

void CalendarMgr::SendCalendarClearPendingAction(ObjectGuid guid)
{
    if (Player* player = ObjectAccessor::FindConnectedPlayer(guid))
    {
        WorldPacket data(SMSG_CALENDAR_CLEAR_PENDING_ACTION, 0);
        player->SendDirectMessage(&data);
    }
}

// 5.4.8 18414, from Project SkyFire
void CalendarMgr::SendCalendarCommandResult(ObjectGuid guid, CalendarError err, char const* param /*= nullptr*/)
{
    if (Player* player = ObjectAccessor::FindConnectedPlayer(guid))
    {
        std::string_view name;
        switch (err)
        {
            case CALENDAR_ERROR_OTHER_INVITES_EXCEEDED:
            case CALENDAR_ERROR_ALREADY_INVITED_TO_EVENT_S:
            case CALENDAR_ERROR_IGNORING_YOU_S:
                if (param)
                    name = param;
                break;
            default:
                break;
        }

        WorldPacket data(SMSG_CALENDAR_COMMAND_RESULT, 2 + 1 + 1 + name.length());
        data.WriteBits(name.length(), 9);
        data.FlushBits();

        data << uint8(0);   // command
        data << uint8(err);
        data.WriteString(name);

        player->SendDirectMessage(&data);
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
