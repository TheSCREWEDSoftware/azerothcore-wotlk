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

#include "Chat.h"
#include "Common.h"
#include "DatabaseEnv.h"
#include "GameTime.h"
#include "Language.h"
#include "ObjectMgr.h"
#include "Opcodes.h"
#include "Player.h"
#include "SupportMgr.h"
#include "TicketMgr.h"
#include "TicketPackets.h"
#include "Util.h"
#include "World.h"
#include "WorldPacket.h"
#include "WorldSession.h"

// 6.x: the in game GM ticket of older clients is gone, the client opens bug, suggestion and complaint
// reports (TC SupportMgr). AC's TicketMgr stays for the GM commands and existing tickets.

void WorldSession::HandleGMTicketGetCaseStatusOpcode(WorldPackets::Ticket::GMTicketGetCaseStatus& /*packet*/)
{
    // TODO: Implement GmCase and handle this packet properly
    WorldPackets::Ticket::GMTicketCaseStatus status;
    SendPacket(status.Write());
}

void WorldSession::HandleGMTicketSystemStatusOpcode(WorldPackets::Ticket::GMTicketGetSystemStatus& /*packet*/)
{
    // Note: This only disables the ticket UI at client side and is not fully reliable
    // Note: This disables the whole customer support UI after trying to send a ticket in disabled state (MessageBox: "GM Help Tickets are currently unavailable."). UI remains disabled until the character relogs.
    WorldPackets::Ticket::GMTicketSystemStatus response;
    response.Status = sSupportMgr->GetSupportSystemStatus() && sTicketMgr->GetStatus() ? GMTICKET_QUEUE_STATUS_ENABLED : GMTICKET_QUEUE_STATUS_DISABLED;
    SendPacket(response.Write());
}

void WorldSession::HandleSupportTicketSubmitBug(WorldPackets::Ticket::SupportTicketSubmitBug& packet)
{
    if (!sSupportMgr->GetBugSystemStatus())
        return;

    BugTicket* ticket = new BugTicket(GetPlayer());
    ticket->SetPosition(packet.Header.MapID, packet.Header.Position);
    ticket->SetFacing(packet.Header.Facing);
    ticket->SetNote(packet.Note);

    sSupportMgr->AddTicket(ticket);
}

void WorldSession::HandleSupportTicketSubmitSuggestion(WorldPackets::Ticket::SupportTicketSubmitSuggestion& packet)
{
    if (!sSupportMgr->GetSuggestionSystemStatus())
        return;

    SuggestionTicket* ticket = new SuggestionTicket(GetPlayer());
    ticket->SetPosition(packet.Header.MapID, packet.Header.Position);
    ticket->SetFacing(packet.Header.Facing);
    ticket->SetNote(packet.Note);

    sSupportMgr->AddTicket(ticket);
}

void WorldSession::HandleSupportTicketSubmitComplaint(WorldPackets::Ticket::SupportTicketSubmitComplaint& packet)
{
    if (!sSupportMgr->GetComplaintSystemStatus())
        return;

    ComplaintTicket* comp = new ComplaintTicket(GetPlayer());
    comp->SetPosition(packet.Header.MapID, packet.Header.Position);
    comp->SetFacing(packet.Header.Facing);
    comp->SetChatLog(packet.ChatLog);
    comp->SetTargetCharacterGuid(packet.TargetCharacterGUID);
    comp->SetComplaintType(GMSupportComplaintType(packet.ComplaintType));
    comp->SetNote(packet.Note);

    sSupportMgr->AddTicket(comp);
}

void WorldSession::HandleBugReportOpcode(WorldPackets::Ticket::BugReport& bugReport)
{
    // Note: There is no way to trigger this with standard UI except /script ReportBug("text")
    if (!sSupportMgr->GetBugSystemStatus())
        return;

    CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_INS_BUG_REPORT);
    stmt->SetData(0, bugReport.Text);
    stmt->SetData(1, bugReport.DiagInfo);
    CharacterDatabase.Execute(stmt);
}

void WorldSession::HandleComplaint(WorldPackets::Ticket::Complaint& packet)
{
    // NOTE: all chat messages from this spammer are automatically ignored by the spam reporter until logout in case of chat spam.
    // if it's mail spam - ALL mails from this spammer are automatically removed by client

    WorldPackets::Ticket::ComplaintResult result;
    result.ComplaintType = packet.ComplaintType;
    result.Result = 0;
    SendPacket(result.Write());

    if (sWorld->getBoolConfig(CONFIG_LOGSPAMREPORTS))
    {
        bool const isChat = packet.ComplaintType == SUPPORT_SPAM_TYPE_CHAT;

        CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_INS_SPAM_REPORT);
        stmt->SetData(0, packet.ComplaintType);
        stmt->SetData(1, uint32(packet.Offender.PlayerGuid.GetCounter()));
        stmt->SetData(2, packet.Offender.RealmAddress);
        stmt->SetData(3, isChat ? packet.Chat.Command : packet.MailID);
        stmt->SetData(4, packet.Chat.ChannelID);
        stmt->SetData(5, packet.Offender.TimeSinceOffence);
        stmt->SetData(6, packet.Chat.MessageLog);
        stmt->SetData(7, uint32(GameTime::GetGameTime().count()));
        CharacterDatabase.Execute(stmt);
    }
}
