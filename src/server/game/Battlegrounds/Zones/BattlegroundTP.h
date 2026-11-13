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

#ifndef __BATTLEGROUNDTP_H
#define __BATTLEGROUNDTP_H

#include "Battleground.h"
#include "BattlegroundScore.h"
#include "EventMap.h"

// Twin Peaks is a capture-the-flag battleground like Warsong Gulch and shares its worldstates
enum BG_TP_Events
{
    BG_TP_EVENT_UPDATE_GAME_TIME    = 1,
    BG_TP_EVENT_NO_TIME_LEFT        = 2,
    BG_TP_EVENT_RESPAWN_BOTH_FLAGS  = 3,
    BG_TP_EVENT_ALLIANCE_DROP_FLAG  = 4,
    BG_TP_EVENT_HORDE_DROP_FLAG     = 5,
    BG_TP_EVENT_BOTH_FLAGS_KEPT10   = 6,
    BG_TP_EVENT_BOTH_FLAGS_KEPT15   = 7,
    BG_TP_EVENT_DESPAWN_DOORS       = 8
};

enum BG_TP_TimerOrScore
{
    BG_TP_MAX_TEAM_SCORE            = 3,

    BG_TP_TOTAL_GAME_TIME           = 27 * MINUTE * IN_MILLISECONDS,
    BG_TP_FLAG_RESPAWN_TIME         = 23 * IN_MILLISECONDS
};
constexpr Milliseconds BG_TP_FLAG_DROP_TIME = 10s;
constexpr Milliseconds BG_TP_SPELL_FORCE_TIME = 600s;
constexpr Milliseconds BG_TP_SPELL_BRUTAL_TIME = 900s;
constexpr Milliseconds BG_TP_DOOR_DESPAWN_TIME = 5s;

enum BG_TP_BroadcastTexts
{
    BG_TP_TEXT_START_ONE_MINUTE         = 41186,
    BG_TP_TEXT_START_HALF_MINUTE        = 41187,
    BG_TP_TEXT_BATTLE_HAS_BEGUN         = 10014,

    BG_TP_TEXT_CAPTURED_HORDE_FLAG      = 9801,
    BG_TP_TEXT_CAPTURED_ALLIANCE_FLAG   = 9802,
    BG_TP_TEXT_FLAG_PLACED_ALLIANCE     = 24891,
    BG_TP_TEXT_FLAG_PLACED_HORDE        = 24892,
    BG_TP_TEXT_ALLIANCE_FLAG_PICKED_UP  = 9804,
    BG_TP_TEXT_ALLIANCE_FLAG_DROPPED    = 9805,
    BG_TP_TEXT_HORDE_FLAG_PICKED_UP     = 9807,
    BG_TP_TEXT_HORDE_FLAG_DROPPED       = 9806,
    BG_TP_TEXT_ALLIANCE_FLAG_RETURNED   = 9808,
    BG_TP_TEXT_HORDE_FLAG_RETURNED      = 9809,
};

enum BG_TP_Sound
{
    BG_TP_SOUND_FLAG_CAPTURED_ALLIANCE  = 8173,
    BG_TP_SOUND_FLAG_CAPTURED_HORDE     = 8213,
    BG_TP_SOUND_FLAG_PLACED             = 8232,
    BG_TP_SOUND_FLAG_RETURNED           = 8192,
    BG_TP_SOUND_HORDE_FLAG_PICKED_UP    = 8212,
    BG_TP_SOUND_ALLIANCE_FLAG_PICKED_UP = 8174,
    BG_TP_SOUND_FLAGS_RESPAWNED         = 8232
};

enum BG_TP_SpellId
{
    BG_TP_SPELL_HORDE_FLAG              = 23333,
    BG_TP_SPELL_HORDE_FLAG_DROPPED      = 23334,
    BG_TP_SPELL_HORDE_FLAG_PICKED       = 61266,    // fake spell, does not exist but used as timer start event
    BG_TP_SPELL_ALLIANCE_FLAG           = 23335,
    BG_TP_SPELL_ALLIANCE_FLAG_DROPPED   = 23336,
    BG_TP_SPELL_ALLIANCE_FLAG_PICKED    = 61265,    // fake spell, does not exist but used as timer start event
    BG_TP_SPELL_FOCUSED_ASSAULT         = 46392,
    BG_TP_SPELL_BRUTAL_ASSAULT          = 46393
};

enum BG_TP_ObjectTypes
{
    BG_TP_OBJECT_DOOR_A_1       = 0,
    BG_TP_OBJECT_DOOR_A_2       = 1,
    BG_TP_OBJECT_DOOR_A_3       = 2,
    BG_TP_OBJECT_DOOR_H_1       = 3,
    BG_TP_OBJECT_DOOR_H_2       = 4,
    BG_TP_OBJECT_DOOR_H_3       = 5,
    BG_TP_OBJECT_DOOR_H_4       = 6,
    BG_TP_OBJECT_A_FLAG         = 7,
    BG_TP_OBJECT_H_FLAG         = 8,
    BG_TP_OBJECT_SPEEDBUFF_1    = 9,
    BG_TP_OBJECT_SPEEDBUFF_2    = 10,
    BG_TP_OBJECT_REGENBUFF_1    = 11,
    BG_TP_OBJECT_REGENBUFF_2    = 12,
    BG_TP_OBJECT_BERSERKBUFF_1  = 13,
    BG_TP_OBJECT_BERSERKBUFF_2  = 14,
    BG_TP_OBJECT_MAX            = 15
};

enum BG_TP_ObjectEntry
{
    BG_OBJECT_DOOR_A_1_TP_ENTRY          = 206655,
    BG_OBJECT_DOOR_A_2_TP_ENTRY          = 206654,
    BG_OBJECT_DOOR_A_3_TP_ENTRY          = 206653,
    BG_OBJECT_DOOR_H_1_TP_ENTRY          = 208205,
    BG_OBJECT_DOOR_H_2_TP_ENTRY          = 208206,
    BG_OBJECT_DOOR_H_3_TP_ENTRY          = 208207,
    BG_OBJECT_DOOR_H_4_TP_ENTRY          = 203710,
    BG_OBJECT_A_FLAG_TP_ENTRY            = 179830,
    BG_OBJECT_H_FLAG_TP_ENTRY            = 179831,
    BG_OBJECT_A_FLAG_GROUND_TP_ENTRY     = 179785,
    BG_OBJECT_H_FLAG_GROUND_TP_ENTRY     = 179786,
    BG_OBJECT_SPEEDBUFF_1_TP_ENTRY       = 179899,
    BG_OBJECT_BERSERKBUFF_1_TP_ENTRY     = 179907,
    BG_OBJECT_REGENBUFF_1_TP_ENTRY       = 179906
};

enum BG_TP_FlagState
{
    BG_TP_FLAG_STATE_ON_BASE      = 1,
    BG_TP_FLAG_STATE_ON_PLAYER    = 2,
    BG_TP_FLAG_STATE_ON_GROUND    = 3
};

// Three graveyards per team: flag room, start and middle
enum BG_TP_Graveyards
{
    TP_GRAVEYARD_FLAGROOM_ALLIANCE = 1726,
    TP_GRAVEYARD_FLAGROOM_HORDE    = 1727,
    TP_GRAVEYARD_START_ALLIANCE    = 1729,
    TP_GRAVEYARD_START_HORDE       = 1728,
    TP_GRAVEYARD_MIDDLE_ALLIANCE   = 1749,
    TP_GRAVEYARD_MIDDLE_HORDE      = 1750
};

enum BG_TP_CreatureTypes
{
    TP_SPIRIT_MAIN_ALLIANCE   = 0,
    TP_SPIRIT_MAIN_HORDE      = 1,

    BG_CREATURES_MAX_TP       = 2
};

enum BG_TP_Objectives
{
    TP_OBJECTIVE_CAPTURE_FLAG   = 290,
    TP_OBJECTIVE_RETURN_FLAG    = 291,

    TP_EVENT_START_BATTLE       = 8563
};

enum BG_TP_Trigger
{
    BG_TP_TRIGGER_ALLIANCE_FLAG_SPAWN   = 5904,
    BG_TP_TRIGGER_HORDE_FLAG_SPAWN      = 5905
};

struct BattlegroundTPScore final : public BattlegroundScore
{
    friend class BattlegroundTP;

protected:
    BattlegroundTPScore(ObjectGuid playerGuid) : BattlegroundScore(playerGuid) { }

    void UpdateScore(uint32 type, uint32 value) override
    {
        switch (type)
        {
        case SCORE_FLAG_CAPTURES:   // Flags captured
            FlagCaptures += value;
            break;
        case SCORE_FLAG_RETURNS:    // Flags returned
            FlagReturns += value;
            break;
        default:
            BattlegroundScore::UpdateScore(type, value);
            break;
        }
    }

    void BuildObjectivesBlock(WorldPacket& data) final;

    uint32 GetAttr1() const override { return FlagCaptures; }
    uint32 GetAttr2() const override { return FlagReturns; }

    uint32 FlagCaptures = 0;
    uint32 FlagReturns = 0;
};

class AC_GAME_API BattlegroundTP : public Battleground
{
public:
    /* Construction */
    BattlegroundTP();
    ~BattlegroundTP() override;

    /* inherited from BattlegroundClass */
    void AddPlayer(Player* player) override;
    void StartingEventCloseDoors() override;
    void StartingEventOpenDoors() override;

    /* BG Flags */
    ObjectGuid GetFlagPickerGUID(TeamId teamId) const override { return _flagKeepers[teamId];  }
    void SetFlagPicker(ObjectGuid guid, TeamId teamId) { _flagKeepers[teamId] = guid; }
    void RespawnFlagAfterDrop(TeamId teamId);
    uint8 GetFlagState(TeamId teamId) const { return _flagState[teamId]; }
    void CheckFlagKeeperInArea(TeamId teamId);

    /* Battleground Events */
    void EventPlayerDroppedFlag(Player* player) override;
    void EventPlayerClickedOnFlag(Player* player, GameObject* gameObject) override;
    void EventPlayerCapturedFlag(Player* player);

    void RemovePlayer(Player* player) override;
    void HandleAreaTrigger(Player* player, uint32 trigger) override;
    void HandleKillPlayer(Player* player, Player* killer) override;
    bool SetupBattleground() override;
    void Init() override;
    void EndBattleground(TeamId winnerTeamId) override;
    GraveyardStruct const* GetClosestGraveyard(Player* player) override;

    void UpdateFlagState(TeamId teamId, uint32 value);
    bool UpdatePlayerScore(Player* player, uint32 type, uint32 value, bool doAddHonor = true) override;
    void SetDroppedFlagGUID(ObjectGuid guid, TeamId teamId) override { _droppedFlagGUID[teamId] = guid; }
    ObjectGuid GetDroppedFlagGUID(TeamId teamId) const { return _droppedFlagGUID[teamId];}
    void FillInitialWorldStates(WorldPackets::WorldState::InitWorldStates& packet) override;

    /* Scorekeeping */
    void AddPoints(TeamId teamId, uint32 points) { m_TeamScores[teamId] += points; }

    TeamId GetPrematureWinner() override;
    uint32 GetMatchTime() const { return 1 + (BG_TP_TOTAL_GAME_TIME - GetStartTime()) / (MINUTE * IN_MILLISECONDS); }
    uint32 GetAssaultSpellId() const;
    void RemoveAssaultAuras();

private:
    EventMap _bgEvents;

    ObjectGuid _flagKeepers[2];
    ObjectGuid _droppedFlagGUID[2];
    uint8  _flagState[2];
    TeamId _lastFlagCaptureTeam;
    uint32 _honorWinKills;
    uint32 _honorEndKills;
    uint32 _configurableMaxTeamScore;

    void PostUpdateImpl(uint32 diff) override;
};

#endif
