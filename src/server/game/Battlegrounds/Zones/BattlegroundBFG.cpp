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

#include "BattlegroundBFG.h"
#include "BattlegroundMgr.h"
#include "Creature.h"
#include "GameGraveyard.h"
#include "Player.h"
#include "Util.h"
#include "WorldPacket.h"
#include "WorldSession.h"
#include "WorldStatePackets.h"
#include "WorldStateDefines.h"

void BattlegroundBFGScore::BuildObjectivesBlock(WorldPacket& data)
{
    data << uint32(2);
    data << uint32(BasesAssaulted);
    data << uint32(BasesDefended);
}

BattlegroundBFG::BattlegroundBFG()
{
    m_BuffChange = true;
    BgObjects.resize(BG_BFG_OBJECT_MAX);
    BgCreatures.resize(BG_BFG_ALL_NODES_COUNT + BG_BFG_DYNAMIC_NODES_COUNT); // +BG_BFG_DYNAMIC_NODES_COUNT buff triggers

    _controlledPoints[TEAM_ALLIANCE] = 0;
    _controlledPoints[TEAM_HORDE] = 0;
    _teamScores500Disadvantage[TEAM_ALLIANCE] = false;
    _teamScores500Disadvantage[TEAM_HORDE] = false;
    _honorTics = 0;
}

BattlegroundBFG::~BattlegroundBFG() = default;

void BattlegroundBFG::PostUpdateImpl(uint32 diff)
{
    if (GetStatus() == STATUS_IN_PROGRESS)
    {
        _bgEvents.Update(diff);
        while (uint32 eventId = _bgEvents.ExecuteEvent())
            switch (eventId)
            {
                case BG_BFG_EVENT_UPDATE_BANNER_LIGHTHOUSE:
                case BG_BFG_EVENT_UPDATE_BANNER_WATERWORKS:
                case BG_BFG_EVENT_UPDATE_BANNER_MINE:
                    CreateBanner(eventId - BG_BFG_EVENT_UPDATE_BANNER_LIGHTHOUSE, false);
                    break;
                case BG_BFG_EVENT_CAPTURE_LIGHTHOUSE:
                case BG_BFG_EVENT_CAPTURE_WATERWORKS:
                case BG_BFG_EVENT_CAPTURE_MINE:
                    {
                        uint8 node = eventId - BG_BFG_EVENT_CAPTURE_LIGHTHOUSE;
                        TeamId teamId = _capturePointInfo[node]._state == BG_BFG_NODE_STATE_ALLY_CONTESTED ? TEAM_ALLIANCE : TEAM_HORDE;
                        DeleteBanner(node);
                        _capturePointInfo[node]._ownerTeamId = teamId;
                        _capturePointInfo[node]._state = teamId == TEAM_ALLIANCE ? BG_BFG_NODE_STATE_ALLY_OCCUPIED : BG_BFG_NODE_STATE_HORDE_OCCUPIED;
                        _capturePointInfo[node]._captured = true;

                        CreateBanner(node, false);
                        NodeOccupied(node);
                        SendNodeUpdate(node);

                        if (teamId == TEAM_ALLIANCE)
                        {
                            SendBroadcastText(BFGNodes[node].TextAllianceTaken, CHAT_MSG_BG_SYSTEM_ALLIANCE);
                            PlaySoundToAll(BG_BFG_SOUND_NODE_CAPTURED_ALLIANCE);
                        }
                        else
                        {
                            SendBroadcastText(BFGNodes[node].TextHordeTaken, CHAT_MSG_BG_SYSTEM_HORDE);
                            PlaySoundToAll(BG_BFG_SOUND_NODE_CAPTURED_HORDE);
                        }

                        break;
                    }
                case BG_BFG_EVENT_ALLIANCE_TICK:
                case BG_BFG_EVENT_HORDE_TICK:
                    {
                        auto teamId = TeamId(eventId - BG_BFG_EVENT_ALLIANCE_TICK);
                        uint8 controlledPoints = _controlledPoints[teamId];
                        if (controlledPoints == 0)
                        {
                            _bgEvents.ScheduleEvent(eventId, 3s);
                            break;
                        }

                        auto honorRewards = uint8(m_TeamScores[teamId] / _honorTics);
                        auto information = uint8(m_TeamScores[teamId] / BG_BFG_WARNING_NEAR_VICTORY_SCORE);
                        m_TeamScores[teamId] += BG_BFG_TickPoints[controlledPoints];
                        if (m_TeamScores[teamId] > static_cast<int32>(_configurableMaxTeamScore))
                            m_TeamScores[teamId] = _configurableMaxTeamScore;

                        if (honorRewards < uint8(m_TeamScores[teamId] / _honorTics))
                            RewardHonorToTeam(GetBonusHonorFromKill(1), teamId);

                        // Battle for Gilneas gives no reputation for resource ticks

                        if (information < uint8(m_TeamScores[teamId] / BG_BFG_WARNING_NEAR_VICTORY_SCORE))
                        {
                            if (teamId == TEAM_ALLIANCE)
                            {
                                SendBroadcastText(BG_BFG_TEXT_ALLIANCE_NEAR_VICTORY, CHAT_MSG_BG_SYSTEM_NEUTRAL);
                                PlaySoundToAll(BG_BFG_SOUND_NEAR_VICTORY_ALLIANCE);
                            }
                            else
                            {
                                SendBroadcastText(BG_BFG_TEXT_HORDE_NEAR_VICTORY, CHAT_MSG_BG_SYSTEM_NEUTRAL);
                                PlaySoundToAll(BG_BFG_SOUND_NEAR_VICTORY_HORDE);
                            }
                        }

                        UpdateWorldState(teamId == TEAM_ALLIANCE ? WORLD_STATE_BATTLEGROUND_AB_RESOURCES_ALLIANCE : WORLD_STATE_BATTLEGROUND_AB_RESOURCES_HORDE, m_TeamScores[teamId]);
                        if (m_TeamScores[teamId] > m_TeamScores[GetOtherTeamId(teamId)] + 500)
                            _teamScores500Disadvantage[GetOtherTeamId(teamId)] = true;
                        if (m_TeamScores[teamId] >= static_cast<int32>(_configurableMaxTeamScore))
                            EndBattleground(teamId);

                        _bgEvents.ScheduleEvent(eventId, BG_BFG_TickIntervals[controlledPoints]);
                        break;
                    }
                default:
                    break;
            }
    }
}

void BattlegroundBFG::StartingEventCloseDoors()
{
    for (uint32 obj = BG_BFG_OBJECT_BANNER_NEUTRAL; obj < static_cast<uint8>(BG_BFG_DYNAMIC_NODES_COUNT) * BG_BFG_OBJECTS_PER_NODE; ++obj)
        SpawnBGObject(obj, RESPAWN_ONE_DAY);
    for (uint32 i = 0; i < BG_BFG_DYNAMIC_NODES_COUNT * 3; ++i)
        SpawnBGObject(BG_BFG_OBJECT_SPEEDBUFF_LIGHTHOUSE + i, RESPAWN_ONE_DAY);

    // Starting doors
    SpawnBGObject(BG_BFG_OBJECT_GATE_A, RESPAWN_IMMEDIATELY);
    SpawnBGObject(BG_BFG_OBJECT_GATE_H, RESPAWN_IMMEDIATELY);
    DoorClose(BG_BFG_OBJECT_GATE_A);
    DoorClose(BG_BFG_OBJECT_GATE_H);
}

void BattlegroundBFG::StartingEventOpenDoors()
{
    for (uint32 banner = BG_BFG_OBJECT_BANNER_NEUTRAL, i = 0; i < BG_BFG_DYNAMIC_NODES_COUNT; banner += BG_BFG_OBJECTS_PER_NODE, ++i)
        SpawnBGObject(banner, RESPAWN_IMMEDIATELY);

    for (uint32 i = 0; i < BG_BFG_DYNAMIC_NODES_COUNT; ++i)
        SpawnBGObject(BG_BFG_OBJECT_SPEEDBUFF_LIGHTHOUSE + urand(0, 2) + i * 3, RESPAWN_IMMEDIATELY);

    DoorOpen(BG_BFG_OBJECT_GATE_A);
    DoorOpen(BG_BFG_OBJECT_GATE_H);
    StartTimedAchievement(ACHIEVEMENT_TIMED_TYPE_EVENT, BG_BFG_EVENT_START_BATTLE);
    _bgEvents.ScheduleEvent(BG_BFG_EVENT_ALLIANCE_TICK, 3s);
    _bgEvents.ScheduleEvent(BG_BFG_EVENT_HORDE_TICK, 3s);
}

void BattlegroundBFG::AddPlayer(Player* player)
{
    Battleground::AddPlayer(player);
    PlayerScores.emplace(player->GetGUID().GetCounter(), new BattlegroundBFGScore(player->GetGUID()));
}

void BattlegroundBFG::RemovePlayer(Player* /*player*/)
{
}

void BattlegroundBFG::HandleAreaTrigger(Player* /*player*/, uint32 /*trigger*/)
{
    // The node area triggers need no handling
}

void BattlegroundBFG::CreateBanner(uint8 node, bool delay)
{
    // Just put it into the queue
    if (delay)
    {
        _bgEvents.RescheduleEvent(BG_BFG_EVENT_UPDATE_BANNER_LIGHTHOUSE + node, BG_BFG_BANNER_UPDATE_TIME);
        return;
    }

    SpawnBGObject(node * BG_BFG_OBJECTS_PER_NODE + _capturePointInfo[node]._state, RESPAWN_IMMEDIATELY);
    SpawnBGObject(node * BG_BFG_OBJECTS_PER_NODE + BG_BFG_OBJECT_AURA_ALLY + _capturePointInfo[node]._ownerTeamId, RESPAWN_IMMEDIATELY);
}

void BattlegroundBFG::DeleteBanner(uint8 node)
{
    SpawnBGObject(node * BG_BFG_OBJECTS_PER_NODE + _capturePointInfo[node]._state, RESPAWN_ONE_DAY);
    SpawnBGObject(node * BG_BFG_OBJECTS_PER_NODE + BG_BFG_OBJECT_AURA_ALLY + _capturePointInfo[node]._ownerTeamId, RESPAWN_ONE_DAY);
}

void BattlegroundBFG::FillInitialWorldStates(WorldPackets::WorldState::InitWorldStates& packet)
{
    packet.Worldstates.reserve(BG_BFG_DYNAMIC_NODES_COUNT + 6);
    for (auto& node : _capturePointInfo)
    {
        if (node._state == BG_BFG_NODE_STATE_NEUTRAL)
            packet.Worldstates.emplace_back(node._iconNone, 1);

        for (uint8 i = BG_BFG_NODE_STATE_ALLY_OCCUPIED; i <= BG_BFG_NODE_STATE_HORDE_CONTESTED; ++i)
            packet.Worldstates.emplace_back(node._iconCapture + i - 1, node._state == i ? 1 : 0);
    }

    packet.Worldstates.emplace_back(WORLD_STATE_BATTLEGROUND_AB_OCCUPIED_BASES_ALLIANCE, _controlledPoints[TEAM_ALLIANCE]);
    packet.Worldstates.emplace_back(WORLD_STATE_BATTLEGROUND_AB_OCCUPIED_BASES_HORDE, _controlledPoints[TEAM_HORDE]);
    packet.Worldstates.emplace_back(WORLD_STATE_BATTLEGROUND_AB_RESOURCES_MAX, _configurableMaxTeamScore);
    packet.Worldstates.emplace_back(WORLD_STATE_BATTLEGROUND_AB_RESOURCES_WARNING, BG_BFG_WARNING_NEAR_VICTORY_SCORE);
    packet.Worldstates.emplace_back(WORLD_STATE_BATTLEGROUND_AB_RESOURCES_ALLIANCE, m_TeamScores[TEAM_ALLIANCE]);
    packet.Worldstates.emplace_back(WORLD_STATE_BATTLEGROUND_AB_RESOURCES_HORDE, m_TeamScores[TEAM_HORDE]);
}

void BattlegroundBFG::SendNodeUpdate(uint8 node)
{
    UpdateWorldState(_capturePointInfo[node]._iconNone, 0);
    for (uint8 i = BG_BFG_NODE_STATE_ALLY_OCCUPIED; i <= BG_BFG_NODE_STATE_HORDE_CONTESTED; ++i)
        UpdateWorldState(_capturePointInfo[node]._iconCapture + i - 1, _capturePointInfo[node]._state == i);

    UpdateWorldState(WORLD_STATE_BATTLEGROUND_AB_OCCUPIED_BASES_ALLIANCE, _controlledPoints[TEAM_ALLIANCE]);
    UpdateWorldState(WORLD_STATE_BATTLEGROUND_AB_OCCUPIED_BASES_HORDE, _controlledPoints[TEAM_HORDE]);
}

void BattlegroundBFG::NodeOccupied(uint8 node)
{
    AddSpiritGuide(node, BG_BFG_SpiritGuidePos[node][0], BG_BFG_SpiritGuidePos[node][1], BG_BFG_SpiritGuidePos[node][2], BG_BFG_SpiritGuidePos[node][3], _capturePointInfo[node]._ownerTeamId);

    ++_controlledPoints[_capturePointInfo[node]._ownerTeamId];
    if (_controlledPoints[_capturePointInfo[node]._ownerTeamId] >= BG_BFG_DYNAMIC_NODES_COUNT)
        CastSpellOnTeam(SPELL_AB_QUEST_REWARD_5_BASES, _capturePointInfo[node]._ownerTeamId);

    Creature* trigger = GetBgMap()->GetCreature(BgCreatures[BG_BFG_ALL_NODES_COUNT + node]);
    if (!trigger)
        trigger = AddCreature(WORLD_TRIGGER, BG_BFG_ALL_NODES_COUNT + node, BG_BFG_NodePositions[node][0], BG_BFG_NodePositions[node][1], BG_BFG_NodePositions[node][2], BG_BFG_NodePositions[node][3]);

    if (trigger)
    {
        trigger->SetFaction(_capturePointInfo[node]._ownerTeamId == TEAM_ALLIANCE ? FACTION_ALLIANCE_GENERIC : FACTION_HORDE_GENERIC);
        trigger->CastSpell(trigger, SPELL_HONORABLE_DEFENDER_25Y, false);
    }
}

void BattlegroundBFG::NodeDeoccupied(uint8 node)
{
    --_controlledPoints[_capturePointInfo[node]._ownerTeamId];

    _capturePointInfo[node]._ownerTeamId = TEAM_NEUTRAL;

    _reviveEvents.AddEventAtOffset([this, node]()
    {
        RelocateDeadPlayers(BgCreatures[node]);
        DelCreature(node); // Delete spirit healer
    }, 500ms);

    DelCreature(BG_BFG_ALL_NODES_COUNT + node); // Delete aura trigger
}

// Unlike Arathi Basin, nodes give no kill credit
void BattlegroundBFG::EventPlayerClickedOnFlag(Player* player, GameObject* gameObject)
{
    if (GetStatus() != STATUS_IN_PROGRESS || !player->IsWithinDistInMap(gameObject, 10.0f))
        return;

    uint8 node = BG_BFG_NODE_LIGHTHOUSE;
    for (; node < BG_BFG_DYNAMIC_NODES_COUNT; ++node)
        if (player->GetDistance2d(BG_BFG_NodePositions[node][0], BG_BFG_NodePositions[node][1]) < 10.0f)
            break;

    if (node == BG_BFG_DYNAMIC_NODES_COUNT || _capturePointInfo[node]._ownerTeamId == player->GetTeamId() ||
            (_capturePointInfo[node]._state == BG_BFG_NODE_STATE_ALLY_CONTESTED && player->GetTeamId() == TEAM_ALLIANCE) ||
            (_capturePointInfo[node]._state == BG_BFG_NODE_STATE_HORDE_CONTESTED && player->GetTeamId() == TEAM_HORDE))
        return;

    player->RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_ENTER_PVP_COMBAT);

    uint32 sound = 0;
    TeamId teamid = player->GetTeamId();

    DeleteBanner(node);
    CreateBanner(node, true);

    if (_capturePointInfo[node]._state == BG_BFG_NODE_STATE_NEUTRAL)
    {
        UpdatePlayerScore(player, SCORE_BASES_ASSAULTED, 1);
        _capturePointInfo[node]._state = static_cast<uint8>(BG_BFG_NODE_STATE_ALLY_CONTESTED) + player->GetTeamId();
        _capturePointInfo[node]._ownerTeamId = TEAM_NEUTRAL;
        _bgEvents.RescheduleEvent(BG_BFG_EVENT_CAPTURE_LIGHTHOUSE + node, BG_BFG_FLAG_CAPTURING_TIME);
        sound = BG_BFG_SOUND_NODE_CLAIMED;

        if (teamid == TEAM_ALLIANCE)
        {
            SendBroadcastText(BFGNodes[node].TextAllianceClaims, CHAT_MSG_BG_SYSTEM_ALLIANCE, player);
        }
        else
        {
            SendBroadcastText(BFGNodes[node].TextHordeClaims, CHAT_MSG_BG_SYSTEM_HORDE, player);
        }
    }
    else if (_capturePointInfo[node]._state == BG_BFG_NODE_STATE_ALLY_CONTESTED || _capturePointInfo[node]._state == BG_BFG_NODE_STATE_HORDE_CONTESTED)
    {
        if (!_capturePointInfo[node]._captured)
        {
            UpdatePlayerScore(player, SCORE_BASES_ASSAULTED, 1);
            _capturePointInfo[node]._state = static_cast<uint8>(BG_BFG_NODE_STATE_ALLY_CONTESTED) + player->GetTeamId();
            _capturePointInfo[node]._ownerTeamId = TEAM_NEUTRAL;
            _bgEvents.RescheduleEvent(BG_BFG_EVENT_CAPTURE_LIGHTHOUSE + node, BG_BFG_FLAG_CAPTURING_TIME);

            if (teamid == TEAM_ALLIANCE)
            {
                SendBroadcastText(BFGNodes[node].TextAllianceAssaulted, CHAT_MSG_BG_SYSTEM_ALLIANCE, player);
            }
            else
            {
                SendBroadcastText(BFGNodes[node].TextHordeAssaulted, CHAT_MSG_BG_SYSTEM_HORDE, player);
            }
        }
        else
        {
            UpdatePlayerScore(player, SCORE_BASES_DEFENDED, 1);
            _capturePointInfo[node]._state = static_cast<uint8>(BG_BFG_NODE_STATE_ALLY_OCCUPIED) + player->GetTeamId();
            _capturePointInfo[node]._ownerTeamId = player->GetTeamId();
            _bgEvents.CancelEvent(BG_BFG_EVENT_CAPTURE_LIGHTHOUSE + node);
            NodeOccupied(node); // after setting team owner

            if (teamid == TEAM_ALLIANCE)
            {
                SendBroadcastText(BFGNodes[node].TextAllianceDefended, CHAT_MSG_BG_SYSTEM_ALLIANCE, player);
            }
            else
            {
                SendBroadcastText(BFGNodes[node].TextHordeDefended, CHAT_MSG_BG_SYSTEM_HORDE, player);
            }
        }

        sound = player->GetTeamId() == TEAM_ALLIANCE ? BG_BFG_SOUND_NODE_ASSAULTED_ALLIANCE : BG_BFG_SOUND_NODE_ASSAULTED_HORDE;
    }
    else
    {
        UpdatePlayerScore(player, SCORE_BASES_ASSAULTED, 1);
        NodeDeoccupied(node); // before setting team owner to neutral

        _capturePointInfo[node]._state = static_cast<uint8>(BG_BFG_NODE_STATE_ALLY_CONTESTED) + player->GetTeamId();

        _bgEvents.RescheduleEvent(BG_BFG_EVENT_CAPTURE_LIGHTHOUSE + node, BG_BFG_FLAG_CAPTURING_TIME);
        sound = player->GetTeamId() == TEAM_ALLIANCE ? BG_BFG_SOUND_NODE_ASSAULTED_ALLIANCE : BG_BFG_SOUND_NODE_ASSAULTED_HORDE;

        if (teamid == TEAM_ALLIANCE)
        {
            SendBroadcastText(BFGNodes[node].TextAllianceAssaulted, CHAT_MSG_BG_SYSTEM_ALLIANCE, player);
        }
        else
        {
            SendBroadcastText(BFGNodes[node].TextHordeAssaulted, CHAT_MSG_BG_SYSTEM_HORDE, player);
        }
    }

    SendNodeUpdate(node);
    PlaySoundToAll(sound);
}

TeamId BattlegroundBFG::GetPrematureWinner()
{
    if (_controlledPoints[TEAM_ALLIANCE] > _controlledPoints[TEAM_HORDE])
        return TEAM_ALLIANCE;
    return _controlledPoints[TEAM_HORDE] > _controlledPoints[TEAM_ALLIANCE] ? TEAM_HORDE : Battleground::GetPrematureWinner();
}

bool BattlegroundBFG::SetupBattleground()
{
    _honorTics = BattlegroundMgr::IsBGWeekend(GetBgTypeID(true)) ? BG_BFG_HONOR_TICK_WEEKEND : BG_BFG_HONOR_TICK_NORMAL;

    for (uint32 i = 0; i < BG_BFG_DYNAMIC_NODES_COUNT; ++i)
    {
        AddObject(BG_BFG_OBJECT_BANNER_NEUTRAL + BG_BFG_OBJECTS_PER_NODE * i, i == 0 ? BG_BFG_OBJECTID_NODE_BANNER_0 : i == 1 ? BG_BFG_OBJECTID_NODE_BANNER_1 : BG_BFG_OBJECTID_NODE_BANNER_2, BG_BFG_NodePositions[i][0], BG_BFG_NodePositions[i][1], BG_BFG_NodePositions[i][2], BG_BFG_NodePositions[i][3], 0, 0, std::sin(BG_BFG_NodePositions[i][3] / 2), cos(BG_BFG_NodePositions[i][3] / 2), RESPAWN_ONE_DAY);
        AddObject(BG_BFG_OBJECT_BANNER_ALLY + BG_BFG_OBJECTS_PER_NODE * i, BG_BFG_OBJECTID_BANNER_A, BG_BFG_NodePositions[i][0], BG_BFG_NodePositions[i][1], BG_BFG_NodePositions[i][2], BG_BFG_NodePositions[i][3], 0, 0, std::sin(BG_BFG_NodePositions[i][3] / 2), cos(BG_BFG_NodePositions[i][3] / 2), RESPAWN_ONE_DAY);
        AddObject(BG_BFG_OBJECT_BANNER_HORDE + BG_BFG_OBJECTS_PER_NODE * i, BG_BFG_OBJECTID_BANNER_H, BG_BFG_NodePositions[i][0], BG_BFG_NodePositions[i][1], BG_BFG_NodePositions[i][2], BG_BFG_NodePositions[i][3], 0, 0, std::sin(BG_BFG_NodePositions[i][3] / 2), cos(BG_BFG_NodePositions[i][3] / 2), RESPAWN_ONE_DAY);
        AddObject(BG_BFG_OBJECT_BANNER_CONT_A + BG_BFG_OBJECTS_PER_NODE * i, BG_BFG_OBJECTID_BANNER_CONT_A, BG_BFG_NodePositions[i][0], BG_BFG_NodePositions[i][1], BG_BFG_NodePositions[i][2], BG_BFG_NodePositions[i][3], 0, 0, std::sin(BG_BFG_NodePositions[i][3] / 2), cos(BG_BFG_NodePositions[i][3] / 2), RESPAWN_ONE_DAY);
        AddObject(BG_BFG_OBJECT_BANNER_CONT_H + BG_BFG_OBJECTS_PER_NODE * i, BG_BFG_OBJECTID_BANNER_CONT_H, BG_BFG_NodePositions[i][0], BG_BFG_NodePositions[i][1], BG_BFG_NodePositions[i][2], BG_BFG_NodePositions[i][3], 0, 0, std::sin(BG_BFG_NodePositions[i][3] / 2), cos(BG_BFG_NodePositions[i][3] / 2), RESPAWN_ONE_DAY);
        AddObject(BG_BFG_OBJECT_AURA_ALLY + BG_BFG_OBJECTS_PER_NODE * i, BG_BFG_OBJECTID_AURA_A, BG_BFG_NodePositions[i][0], BG_BFG_NodePositions[i][1], BG_BFG_NodePositions[i][2], BG_BFG_NodePositions[i][3], 0, 0, std::sin(BG_BFG_NodePositions[i][3] / 2), cos(BG_BFG_NodePositions[i][3] / 2), RESPAWN_ONE_DAY);
        AddObject(BG_BFG_OBJECT_AURA_HORDE + BG_BFG_OBJECTS_PER_NODE * i, BG_BFG_OBJECTID_AURA_H, BG_BFG_NodePositions[i][0], BG_BFG_NodePositions[i][1], BG_BFG_NodePositions[i][2], BG_BFG_NodePositions[i][3], 0, 0, std::sin(BG_BFG_NodePositions[i][3] / 2), cos(BG_BFG_NodePositions[i][3] / 2), RESPAWN_ONE_DAY);
        AddObject(BG_BFG_OBJECT_AURA_CONTESTED + BG_BFG_OBJECTS_PER_NODE * i, BG_BFG_OBJECTID_AURA_C, BG_BFG_NodePositions[i][0], BG_BFG_NodePositions[i][1], BG_BFG_NodePositions[i][2], BG_BFG_NodePositions[i][3], 0, 0, std::sin(BG_BFG_NodePositions[i][3] / 2), cos(BG_BFG_NodePositions[i][3] / 2), RESPAWN_ONE_DAY);
    }

    AddObject(BG_BFG_OBJECT_GATE_A, BG_BFG_OBJECTID_GATE_A, BG_BFG_DoorPositions[0][0], BG_BFG_DoorPositions[0][1], BG_BFG_DoorPositions[0][2], BG_BFG_DoorPositions[0][3], BG_BFG_DoorPositions[0][4], BG_BFG_DoorPositions[0][5], BG_BFG_DoorPositions[0][6], BG_BFG_DoorPositions[0][7], RESPAWN_IMMEDIATELY);
    AddObject(BG_BFG_OBJECT_GATE_H, BG_BFG_OBJECTID_GATE_H, BG_BFG_DoorPositions[1][0], BG_BFG_DoorPositions[1][1], BG_BFG_DoorPositions[1][2], BG_BFG_DoorPositions[1][3], BG_BFG_DoorPositions[1][4], BG_BFG_DoorPositions[1][5], BG_BFG_DoorPositions[1][6], BG_BFG_DoorPositions[1][7], RESPAWN_IMMEDIATELY);

    for (uint32 i = 0; i < BG_BFG_DYNAMIC_NODES_COUNT; ++i)
    {
        AddObject(BG_BFG_OBJECT_SPEEDBUFF_LIGHTHOUSE + 3 * i, Buff_Entries[0], BG_BFG_BuffPositions[i][0], BG_BFG_BuffPositions[i][1], BG_BFG_BuffPositions[i][2], BG_BFG_BuffPositions[i][3], 0, 0, std::sin(BG_BFG_BuffPositions[i][3] / 2), cos(BG_BFG_BuffPositions[i][3] / 2), RESPAWN_ONE_DAY);
        AddObject(BG_BFG_OBJECT_SPEEDBUFF_LIGHTHOUSE + 3 * i + 1, Buff_Entries[1], BG_BFG_BuffPositions[i][0], BG_BFG_BuffPositions[i][1], BG_BFG_BuffPositions[i][2], BG_BFG_BuffPositions[i][3], 0, 0, std::sin(BG_BFG_BuffPositions[i][3] / 2), cos(BG_BFG_BuffPositions[i][3] / 2), RESPAWN_ONE_DAY);
        AddObject(BG_BFG_OBJECT_SPEEDBUFF_LIGHTHOUSE + 3 * i + 2, Buff_Entries[2], BG_BFG_BuffPositions[i][0], BG_BFG_BuffPositions[i][1], BG_BFG_BuffPositions[i][2], BG_BFG_BuffPositions[i][3], 0, 0, std::sin(BG_BFG_BuffPositions[i][3] / 2), cos(BG_BFG_BuffPositions[i][3] / 2), RESPAWN_ONE_DAY);
    }

    AddSpiritGuide(BG_BFG_SPIRIT_ALIANCE, BG_BFG_SpiritGuidePos[BG_BFG_SPIRIT_ALIANCE][0], BG_BFG_SpiritGuidePos[BG_BFG_SPIRIT_ALIANCE][1], BG_BFG_SpiritGuidePos[BG_BFG_SPIRIT_ALIANCE][2], BG_BFG_SpiritGuidePos[BG_BFG_SPIRIT_ALIANCE][3], TEAM_ALLIANCE);
    AddSpiritGuide(BG_BFG_SPIRIT_HORDE, BG_BFG_SpiritGuidePos[BG_BFG_SPIRIT_HORDE][0], BG_BFG_SpiritGuidePos[BG_BFG_SPIRIT_HORDE][1], BG_BFG_SpiritGuidePos[BG_BFG_SPIRIT_HORDE][2], BG_BFG_SpiritGuidePos[BG_BFG_SPIRIT_HORDE][3], TEAM_HORDE);

    for (uint32 i = BG_BFG_OBJECT_BANNER_NEUTRAL; i < BG_BFG_OBJECT_MAX; ++i)
        if (!BgObjects[i])
        {
            LOG_ERROR("sql.sql", "BattlegroundBFG: Failed to spawn some object Battleground not created!");
            return false;
        }

    for (uint32 i = BG_BFG_SPIRIT_ALIANCE; i <= BG_BFG_SPIRIT_HORDE; ++i)
        if (!BgCreatures[i])
        {
            LOG_ERROR("sql.sql", "BattlegroundBFG: Failed to spawn spirit guides Battleground not created!");
            return false;
        }

    return true;
}

void BattlegroundBFG::Init()
{
    //call parent's class reset
    Battleground::Init();

    _bgEvents.Reset();

    _capturePointInfo[BG_BFG_NODE_LIGHTHOUSE]._iconNone = WORLD_STATE_BATTLEGROUND_AB_STABLE_ICON;
    _capturePointInfo[BG_BFG_NODE_WATERWORKS]._iconNone = WORLD_STATE_BATTLEGROUND_AB_BLACKSMITH_ICON;
    _capturePointInfo[BG_BFG_NODE_MINE]._iconNone = WORLD_STATE_BATTLEGROUND_AB_FARM_ICON;
    _capturePointInfo[BG_BFG_NODE_LIGHTHOUSE]._iconCapture = WORLD_STATE_BATTLEGROUND_AB_STABLE_STATE_ALLIANCE;
    _capturePointInfo[BG_BFG_NODE_WATERWORKS]._iconCapture = WORLD_STATE_BATTLEGROUND_AB_BLACKSMITH_STATE_ALLIANCE;
    _capturePointInfo[BG_BFG_NODE_MINE]._iconCapture = WORLD_STATE_BATTLEGROUND_AB_FARM_STATE_ALLIANCE;

    // The nodes use Arathi Basin's stable, blacksmith and farm worldstates

    uint32 bgGilneasCapturePointsConfig = sWorld->getIntConfig(CONFIG_BATTLEGROUND_ARATHI_CAPTUREPOINTS);
    _configurableMaxTeamScore = bgGilneasCapturePointsConfig > 0
        ? bgGilneasCapturePointsConfig
        : static_cast<uint32>(BG_BFG_MAX_TEAM_SCORE);
}

void BattlegroundBFG::EndBattleground(TeamId winnerTeamId)
{
    RewardHonorToTeam(GetBonusHonorFromKill(1), winnerTeamId);
    RewardHonorToTeam(GetBonusHonorFromKill(1), TEAM_HORDE);
    RewardHonorToTeam(GetBonusHonorFromKill(1), TEAM_ALLIANCE);
    Battleground::EndBattleground(winnerTeamId);
    _bgEvents.Reset();
}

GraveyardStruct const* BattlegroundBFG::GetClosestGraveyard(Player* player)
{
    GraveyardStruct const* entry = sGraveyard->GetGraveyard(BG_BFG_GraveyardIds[static_cast<uint8>(BG_BFG_SPIRIT_ALIANCE) + player->GetTeamId()]);
    GraveyardStruct const* nearestEntry = entry;

    float pX = player->GetPositionX();
    float pY = player->GetPositionY();
    float dist = (entry->x - pX) * (entry->x - pX) + (entry->y - pY) * (entry->y - pY);
    float minDist = dist;

    for (uint8 i = BG_BFG_NODE_LIGHTHOUSE; i < BG_BFG_DYNAMIC_NODES_COUNT; ++i)
        if (_capturePointInfo[i]._ownerTeamId == player->GetTeamId())
        {
            entry = sGraveyard->GetGraveyard(BG_BFG_GraveyardIds[i]);
            dist = (entry->x - pX) * (entry->x - pX) + (entry->y - pY) * (entry->y - pY);
            if (dist < minDist)
            {
                minDist = dist;
                nearestEntry = entry;
            }
        }

    return nearestEntry;
}

bool BattlegroundBFG::UpdatePlayerScore(Player* player, uint32 type, uint32 value, bool doAddHonor)
{
    if (!Battleground::UpdatePlayerScore(player, type, value, doAddHonor))
        return false;

    switch (type)
    {
        case SCORE_BASES_ASSAULTED:
            player->UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_BG_OBJECTIVE_CAPTURE, BG_BFG_OBJECTIVE_ASSAULT_BASE);
            break;
        case SCORE_BASES_DEFENDED:
            player->UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_BG_OBJECTIVE_CAPTURE, BG_BFG_OBJECTIVE_DEFEND_BASE);
            break;
        default:
            break;
    }

    return true;
}

bool BattlegroundBFG::AllNodesConrolledByTeam(TeamId teamId) const
{
    return _controlledPoints[teamId] == BG_BFG_DYNAMIC_NODES_COUNT;
}
