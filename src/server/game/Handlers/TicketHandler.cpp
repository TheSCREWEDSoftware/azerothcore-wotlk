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
#include "GameTime.h"
#include "Language.h"
#include "Opcodes.h"
#include "Player.h"
#include "TicketMgr.h"
#include "Util.h"
#include "World.h"
#include "WorldPacket.h"
#include "WorldSession.h"
#include <zlib.h>

void WorldSession::HandleGMTicketCreateOpcode(WorldPacket& recvData)
{
    // Don't accept tickets if the ticket queue is disabled. (Ticket UI is greyed out but not fully dependable)
    if (sTicketMgr->GetStatus() == GMTICKET_QUEUE_STATUS_DISABLED)
        return;

    if (GetPlayer()->GetLevel() < sWorld->getIntConfig(CONFIG_TICKET_LEVEL_REQ))
    {
        ChatHandler(this).SendNotification(LANG_TICKET_REQ, sWorld->getIntConfig(CONFIG_TICKET_LEVEL_REQ));
        return;
    }

    GMTicketResponse response = GMTICKET_RESPONSE_CREATE_ERROR;
    GmTicket* ticket = sTicketMgr->GetTicketByPlayer(GetPlayer()->GetGUID());

    if (ticket && ticket->IsCompleted())
        sTicketMgr->CloseTicket(ticket->GetId(), GetPlayer()->GetGUID());

    // Player must not have ticket
    if (!ticket || ticket->IsClosed())
    {
        uint32 mapId;
        float x, y, z;
        std::string message;
        uint32 needResponse;
        bool needMoreHelp;
        uint32 count;
        std::list<uint32> times;
        std::string chatLog;

        // 5.4.8 18414, from Project SkyFire
        recvData >> mapId;
        recvData >> z >> y;
        recvData.read_skip<uint8>();                        // flags
        recvData >> x;
        recvData >> count;

        if (count > 0)
        {
            uint8 textCount;
            recvData >> textCount;
            for (uint8 i = 0; i < textCount; ++i)
                times.push_back(recvData.read<uint32>());

            uint32 decompressedSize;
            recvData >> decompressedSize;

            if (decompressedSize && decompressedSize < 0xFFFF)
            {
                ByteBuffer dest;
                dest.resize(decompressedSize);

                // inflate tells how much of the packet the compressed chat log used, the request text follows it
                z_stream stream = {};
                stream.next_in = recvData.contents() + recvData.rpos();
                stream.avail_in = uInt(recvData.size() - recvData.rpos());
                stream.next_out = dest.contents();
                stream.avail_out = uInt(decompressedSize);
                bool ok = inflateInit(&stream) == Z_OK;
                ok = ok && inflate(&stream, Z_FINISH) == Z_STREAM_END;
                inflateEnd(&stream);

                if (!ok)
                {
                    LOG_ERROR("network.opcode", "CMSG_GMTICKET_CREATE possibly corrupt. Uncompression failed.");
                    recvData.rfinish();
                    return;
                }

                recvData.rpos(recvData.size() - stream.avail_in);
                dest >> chatLog;
            }
        }

        recvData.ResetBitPos();
        needResponse = recvData.ReadBit();
        needMoreHelp = recvData.ReadBit();
        message = recvData.ReadString(recvData.ReadBits(11));

        if (!ValidateHyperlinksAndMaybeKick(message))
        {
            return;
        }

        if (!chatLog.empty() && !ValidateHyperlinksAndMaybeKick(chatLog))
        {
            return;
        }

        ticket = new GmTicket(GetPlayer());
        ticket->SetPosition(mapId, x, y, z);
        ticket->SetMessage(message);
        ticket->SetGmAction(needResponse, needMoreHelp);

        if (!chatLog.empty())
            ticket->SetChatLog(times, chatLog);

        sTicketMgr->AddTicket(ticket);
        sTicketMgr->UpdateLastChange(ticket);

        ChatHandler(nullptr).SendGMText(LANG_COMMAND_TICKETNEW, GetPlayer()->GetName(), ticket->GetId());

        response = GMTICKET_RESPONSE_CREATE_SUCCESS;
    }

    TicketMgr::SendTicketUpdate(this, response);
}

void WorldSession::HandleGMTicketUpdateOpcode(WorldPacket& recv_data)
{
    std::string message = recv_data.ReadString(recv_data.ReadBits(11));

    if (!ValidateHyperlinksAndMaybeKick(message))
    {
        return;
    }

    GMTicketResponse response = GMTICKET_RESPONSE_UPDATE_ERROR;
    if (GmTicket* ticket = sTicketMgr->GetTicketByPlayer(GetPlayer()->GetGUID()))
    {
        CharacterDatabaseTransaction trans = CharacterDatabaseTransaction(nullptr);
        ticket->SetMessage(message);
        ticket->SaveToDB(trans);

        ChatHandler(nullptr).SendGMText(LANG_COMMAND_TICKETUPDATED, GetPlayer()->GetName(), ticket->GetId());

        response = GMTICKET_RESPONSE_UPDATE_SUCCESS;
    }

    TicketMgr::SendTicketUpdate(this, response);
}

void WorldSession::HandleGMTicketDeleteOpcode(WorldPacket& /*recv_data*/)
{
    if (GmTicket* ticket = sTicketMgr->GetTicketByPlayer(GetPlayer()->GetGUID()))
    {
        TicketMgr::SendTicketUpdate(this, GMTICKET_RESPONSE_TICKET_DELETED);

        ChatHandler(nullptr).SendGMText(LANG_COMMAND_TICKETPLAYERABANDON, GetPlayer()->GetName(), ticket->GetId());

        sTicketMgr->CloseTicket(ticket->GetId(), GetPlayer()->GetGUID());
        sTicketMgr->SendTicket(this, nullptr);
    }
}

void WorldSession::HandleGMTicketGetTicketOpcode(WorldPacket& /*recv_data*/)
{
    SendTimeQueryResponse();

    if (GmTicket* ticket = sTicketMgr->GetTicketByPlayer(GetPlayer()->GetGUID()))
    {
        if (ticket->IsCompleted())
            ticket->SendResponse(this);
        else
            sTicketMgr->SendTicket(this, ticket);
    }
    else
        sTicketMgr->SendTicket(this, nullptr);
}

void WorldSession::HandleGMTicketSystemStatusOpcode(WorldPacket& /*recv_data*/)
{
    // Note: This only disables the ticket UI at client side and is not fully reliable
    // are we sure this is a uint32? Should ask Zor
    WorldPacket data(SMSG_GMTICKET_SYSTEMSTATUS, 4);
    data << uint32(sTicketMgr->GetStatus() ? GMTICKET_QUEUE_STATUS_ENABLED : GMTICKET_QUEUE_STATUS_DISABLED);
    SendPacket(&data);
}

// 5.4.8 18414, from Project SkyFire
void WorldSession::HandleGMSurveySubmit(WorldPacket& recv_data)
{
    struct SurveyAnswer
    {
        uint32 QuestionId;
        uint8 Rank;
        std::string Comment;
    };

    uint32 mainSurvey; // GMSurveyCurrentSurvey.dbc, column 1 (all 9) ref to GMSurveySurveys.dbc
    recv_data >> mainSurvey;

    uint8 answerCount = recv_data.ReadBits(4);
    std::vector<uint32> commentLengths(answerCount);
    for (uint8 i = 0; i < answerCount; ++i)
        commentLengths[i] = recv_data.ReadBits(9);

    std::string overallComment = recv_data.ReadString(recv_data.ReadBits(11));
    if (!ValidateHyperlinksAndMaybeKick(overallComment))
        return;

    std::unordered_set<uint32> surveyIds;
    std::vector<SurveyAnswer> answers;
    answers.reserve(answerCount);

    for (uint8 i = 0; i < answerCount; ++i)
    {
        SurveyAnswer answer;
        recv_data >> answer.QuestionId;                    // ref to GMSurveyQuestions.dbc
        recv_data >> answer.Rank;                          // probably some sort of ref to GMSurveyAnswers.dbc
        answer.Comment = recv_data.ReadString(commentLengths[i]);

        if (!ValidateHyperlinksAndMaybeKick(answer.Comment))
            return;

        // make sure the same sub survey is not added to DB twice
        if (!answer.QuestionId || !surveyIds.insert(answer.QuestionId).second)
            continue;

        answers.push_back(std::move(answer));
    }

    uint32 nextSurveyID = sTicketMgr->GetNextSurveyID();
    CharacterDatabaseTransaction trans = CharacterDatabase.BeginTransaction();

    for (SurveyAnswer const& answer : answers)
    {
        CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_INS_GM_SUBSURVEY);
        stmt->SetData(0, nextSurveyID);
        stmt->SetData(1, answer.QuestionId);
        stmt->SetData(2, answer.Rank);
        stmt->SetData(3, answer.Comment);
        trans->Append(stmt);
    }

    CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_INS_GM_SURVEY);
    stmt->SetData(0, GetPlayer()->GetGUID().GetCounter());
    stmt->SetData(1, nextSurveyID);
    stmt->SetData(2, mainSurvey);
    stmt->SetData(3, overallComment);
    trans->Append(stmt);

    CharacterDatabase.CommitTransaction(trans);
}

// 5.4.8 18414, from Project SkyFire
void WorldSession::HandleGMTicketCaseStatusOpcode(WorldPacket& /*recvData*/)
{
    WorldPacket data(SMSG_GM_TICKET_CASE_STATUS, 3 + 4 + 4);
    data.WriteBits(0, 20);                                  // case count
    data.FlushBits();
    data.AppendPackedTime(0);                               // update time
    data.AppendPackedTime(GameTime::GetGameTime().count()); // oldest ticket time
    SendPacket(&data);
}

void WorldSession::HandleReportLag(WorldPacket& recv_data)
{
    // just put the lag report into the database...
    // can't think of anything else to do with it
    uint32 lagType, mapId;
    recv_data >> lagType;
    recv_data >> mapId;
    float x, y, z;
    recv_data >> x;
    recv_data >> y;
    recv_data >> z;

    CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_INS_LAG_REPORT);
    stmt->SetData(0, GetPlayer()->GetGUID().GetCounter());
    stmt->SetData (1, lagType);
    stmt->SetData(2, mapId);
    stmt->SetData (3, x);
    stmt->SetData (4, y);
    stmt->SetData (5, z);
    stmt->SetData(6, GetLatency());
    stmt->SetData(7, GameTime::GetGameTime().count());
    CharacterDatabase.Execute(stmt);
}

void WorldSession::HandleGMResponseResolve(WorldPacket& /*recvPacket*/)
{
    // empty packet
    if (GmTicket* ticket = sTicketMgr->GetTicketByPlayer(GetPlayer()->GetGUID()))
    {
        uint8 getSurvey = 0;
        if (float(rand_chance()) < sWorld->getFloatConfig(CONFIG_CHANCE_OF_GM_SURVEY))
            getSurvey = 1;

        WorldPacket data(SMSG_GM_TICKET_RESOLVE_RESPONSE, 1);
        data.WriteBit(getSurvey);
        data.FlushBits();
        SendPacket(&data);

        TicketMgr::SendTicketUpdate(this, GMTICKET_RESPONSE_TICKET_DELETED);

        sTicketMgr->CloseTicket(ticket->GetId(), GetPlayer()->GetGUID());
        sTicketMgr->SendTicket(this, nullptr);
    }
}
