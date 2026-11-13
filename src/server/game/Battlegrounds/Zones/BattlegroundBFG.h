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

#ifndef __BATTLEGROUNDBFG_H
#define __BATTLEGROUNDBFG_H

#include "Battleground.h"
#include "BattlegroundScore.h"
#include "EventMap.h"

// Battle for Gilneas is a three-node resource battleground like Arathi Basin;
// map data comes from TrinityCore's BattlegroundBFG
enum BG_BFG_Events
{
    BG_BFG_EVENT_UPDATE_BANNER_LIGHTHOUSE   = 1,
    BG_BFG_EVENT_UPDATE_BANNER_WATERWORKS   = 2,
    BG_BFG_EVENT_UPDATE_BANNER_MINE         = 3,

    BG_BFG_EVENT_CAPTURE_LIGHTHOUSE         = 4,
    BG_BFG_EVENT_CAPTURE_WATERWORKS         = 5,
    BG_BFG_EVENT_CAPTURE_MINE               = 6,

    BG_BFG_EVENT_ALLIANCE_TICK              = 7,
    BG_BFG_EVENT_HORDE_TICK                 = 8
};

enum BG_BFG_ObjectIds
{
    BG_BFG_OBJECTID_NODE_BANNER_0        = 208779,      // Lighthouse banner
    BG_BFG_OBJECTID_NODE_BANNER_1        = 208782,      // Waterworks banner
    BG_BFG_OBJECTID_NODE_BANNER_2        = 208785,      // Mine banner

    BG_BFG_OBJECTID_BANNER_A             = 208673,
    BG_BFG_OBJECTID_BANNER_CONT_A        = 208763,
    BG_BFG_OBJECTID_BANNER_H             = 208748,
    BG_BFG_OBJECTID_BANNER_CONT_H        = 208733,

    BG_BFG_OBJECTID_AURA_A               = 180100,
    BG_BFG_OBJECTID_AURA_H               = 180101,
    BG_BFG_OBJECTID_AURA_C               = 180102,

    BG_BFG_OBJECTID_GATE_A               = 207177,
    BG_BFG_OBJECTID_GATE_H               = 207178
};

enum BG_BFG_ObjectType
{
    BG_BFG_OBJECT_BANNER_NEUTRAL          = 0,
    BG_BFG_OBJECT_BANNER_ALLY             = 1,
    BG_BFG_OBJECT_BANNER_HORDE            = 2,
    BG_BFG_OBJECT_BANNER_CONT_A           = 3,
    BG_BFG_OBJECT_BANNER_CONT_H           = 4,
    BG_BFG_OBJECT_AURA_ALLY               = 5,
    BG_BFG_OBJECT_AURA_HORDE              = 6,
    BG_BFG_OBJECT_AURA_CONTESTED          = 7,
    BG_BFG_OBJECTS_PER_NODE               = 8,

    BG_BFG_OBJECT_GATE_A                  = 24,
    BG_BFG_OBJECT_GATE_H                  = 25,

    BG_BFG_OBJECT_SPEEDBUFF_LIGHTHOUSE    = 26,
    BG_BFG_OBJECT_REGENBUFF_LIGHTHOUSE    = 27,
    BG_BFG_OBJECT_BERSERKBUFF_LIGHTHOUSE  = 28,
    BG_BFG_OBJECT_SPEEDBUFF_WATERWORKS    = 29,
    BG_BFG_OBJECT_REGENBUFF_WATERWORKS    = 30,
    BG_BFG_OBJECT_BERSERKBUFF_WATERWORKS  = 31,
    BG_BFG_OBJECT_SPEEDBUFF_MINE          = 32,
    BG_BFG_OBJECT_REGENBUFF_MINE          = 33,
    BG_BFG_OBJECT_BERSERKBUFF_MINE        = 34,
    BG_BFG_OBJECT_MAX                     = 35,
};

enum BG_BFG_BattlegroundNodes
{
    BG_BFG_NODE_LIGHTHOUSE       = 0,
    BG_BFG_NODE_WATERWORKS       = 1,
    BG_BFG_NODE_MINE             = 2,
    BG_BFG_DYNAMIC_NODES_COUNT   = 3,                        // dynamic nodes that can be captured

    BG_BFG_SPIRIT_ALIANCE        = 3,
    BG_BFG_SPIRIT_HORDE          = 4,
    BG_BFG_ALL_NODES_COUNT       = 5,                        // all nodes (dynamic and static)
};

enum BG_BFG_BroadcastTexts
{
    BG_BFG_TEXT_ALLIANCE_NEAR_VICTORY    = 63126,
    BG_BFG_TEXT_HORDE_NEAR_VICTORY       = 63128,
};

struct BFGNodeInfo
{
    uint32 NodeId;
    uint32 TextAllianceAssaulted;
    uint32 TextHordeAssaulted;
    uint32 TextAllianceTaken;
    uint32 TextHordeTaken;
    uint32 TextAllianceDefended;
    uint32 TextHordeDefended;
    uint32 TextAllianceClaims;
    uint32 TextHordeClaims;
};

BFGNodeInfo const BFGNodes[BG_BFG_DYNAMIC_NODES_COUNT] =
{
    { BG_BFG_NODE_LIGHTHOUSE, 45903, 45904, 45905, 45906, 45907, 45908, 45909, 45910 },
    { BG_BFG_NODE_WATERWORKS, 45887, 45888, 45889, 45890, 45891, 45892, 45893, 45894 },
    { BG_BFG_NODE_MINE,       45895, 45896, 45897, 45898, 45899, 45900, 45901, 45902 }
};

enum BG_BFG_NodeStatus
{
    BG_BFG_NODE_STATE_NEUTRAL            = 0,
    BG_BFG_NODE_STATE_ALLY_OCCUPIED      = 1,
    BG_BFG_NODE_STATE_HORDE_OCCUPIED     = 2,
    BG_BFG_NODE_STATE_ALLY_CONTESTED     = 3,
    BG_BFG_NODE_STATE_HORDE_CONTESTED    = 4
};

enum BG_BFG_Sounds
{
    BG_BFG_SOUND_NODE_CLAIMED            = 8192,
    BG_BFG_SOUND_NODE_CAPTURED_ALLIANCE  = 8173,
    BG_BFG_SOUND_NODE_CAPTURED_HORDE     = 8213,
    BG_BFG_SOUND_NODE_ASSAULTED_ALLIANCE = 8212,
    BG_BFG_SOUND_NODE_ASSAULTED_HORDE    = 8174,
    BG_BFG_SOUND_NEAR_VICTORY_ALLIANCE   = 8456,
    BG_BFG_SOUND_NEAR_VICTORY_HORDE      = 8457
};

enum BG_BFG_Misc
{
    BG_BFG_OBJECTIVE_ASSAULT_BASE        = 370,
    BG_BFG_OBJECTIVE_DEFEND_BASE         = 371,
    BG_BFG_EVENT_START_BATTLE            = 5254, // Achievement: Newbs to Plowshares

    BG_BFG_HONOR_TICK_NORMAL             = 330,
    BG_BFG_HONOR_TICK_WEEKEND            = 200,

    BG_BFG_WARNING_NEAR_VICTORY_SCORE    = 1800,
    BG_BFG_MAX_TEAM_SCORE                = 2000,
};
constexpr Milliseconds BG_BFG_FLAG_CAPTURING_TIME = 60s;
constexpr Milliseconds BG_BFG_BANNER_UPDATE_TIME = 2s;

const Milliseconds BG_BFG_TickIntervals[BG_BFG_DYNAMIC_NODES_COUNT + 1] = {0ms, 12s, 6s, 1s};
const uint32 BG_BFG_TickPoints[BG_BFG_DYNAMIC_NODES_COUNT + 1] = {0, 10, 10, 30};
const uint32 BG_BFG_GraveyardIds[BG_BFG_ALL_NODES_COUNT] = {1736, 1738, 1735, 1740, 1739};

const float BG_BFG_BuffPositions[BG_BFG_DYNAMIC_NODES_COUNT][4] =
{
    {1063.57f, 1313.42f, 4.91f, 4.14f},                     // lighthouse
    {961.830f, 977.03f, 14.15f, 4.55f},                     // waterworks
    {1195.73f, 1020.3f, 7.97874f, 5.777041f}                // mine
};

const float BG_BFG_NodePositions[BG_BFG_DYNAMIC_NODES_COUNT][4] =
{
    {1057.91f, 1278.33f, 3.27763f, 4.97419f},               // lighthouse
    {980.08f, 948.707f, 12.7478f, 2.740162f},                // waterworks
    {1251.02f, 958.359f, 5.67407f, 2.775069f}                // mine
};

const float BG_BFG_DoorPositions[2][8] =
{
    {918.3906f, 1336.641f, 27.4252f, 2.844883f, 0.0f, 0.0f, 0.9890156f, 0.1478114f},     // Alliance gate
    {1395.97f, 977.0903f, 7.63597f, 6.274459f, 0.0f, 0.0f, -0.00436306f, 0.9999905f}     // Horde gate
};

const float BG_BFG_SpiritGuidePos[BG_BFG_ALL_NODES_COUNT][4] =
{
    {1034.82f, 1335.58f, 12.0095f, 5.15f},                  // lighthouse
    {887.578f, 937.337f, 23.7737f, 0.45f},                  // waterworks
    {1252.23f, 836.547f, 27.7895f, 1.60f},                  // mine
    {908.274f, 1338.6f, 27.6449f, 5.95f},                   // alliance starting base
    {1407.43f, 977.115f, 7.525484f, 3.141593f}              // horde starting base
};

struct BattlegroundBFGScore final : public BattlegroundScore
{
    friend class BattlegroundBFG;

protected:

    explicit BattlegroundBFGScore(ObjectGuid playerGuid) : BattlegroundScore(playerGuid) { }

    void UpdateScore(uint32 type, uint32 value) override
    {
        switch (type)
        {
        case SCORE_BASES_ASSAULTED:
            BasesAssaulted += value;
            break;
        case SCORE_BASES_DEFENDED:
            BasesDefended += value;
            break;
        default:
            BattlegroundScore::UpdateScore(type, value);
            break;
        }
    }

    void BuildObjectivesBlock(WorldPacket& data) final;

    uint32 GetAttr1() const override { return BasesAssaulted; }
    uint32 GetAttr2() const override { return BasesDefended; }

    uint32 BasesAssaulted = 0;
    uint32 BasesDefended = 0;
};

class AC_GAME_API BattlegroundBFG : public Battleground
{
public:
    BattlegroundBFG();
    ~BattlegroundBFG() override;

    void AddPlayer(Player* player) override;
    void StartingEventCloseDoors() override;
    void StartingEventOpenDoors() override;
    void RemovePlayer(Player* player) override;
    void HandleAreaTrigger(Player* player, uint32 trigger) override;
    bool SetupBattleground() override;
    void Init() override;
    void EndBattleground(TeamId winnerTeamId) override;
    GraveyardStruct const* GetClosestGraveyard(Player* player) override;

    bool UpdatePlayerScore(Player* player, uint32 type, uint32 value, bool doAddHonor = true) override;
    void FillInitialWorldStates(WorldPackets::WorldState::InitWorldStates& packet) override;
    void EventPlayerClickedOnFlag(Player* source, GameObject* gameObject) override;

    bool AllNodesConrolledByTeam(TeamId teamId) const override;
    bool IsTeamScores500Disadvantage(TeamId teamId) const { return _teamScores500Disadvantage[teamId]; }

    TeamId GetPrematureWinner() override;
private:
    void PostUpdateImpl(uint32 diff) override;

    void DeleteBanner(uint8 node);
    void CreateBanner(uint8 node, bool delay);
    void SendNodeUpdate(uint8 node);
    void NodeOccupied(uint8 node);
    void NodeDeoccupied(uint8 node);

    struct CapturePointInfo
    {
        CapturePointInfo() : _ownerTeamId(TEAM_NEUTRAL), _iconNone(0), _iconCapture(0), _state(BG_BFG_NODE_STATE_NEUTRAL), _captured(false)
        {
        }

        TeamId _ownerTeamId;
        uint32 _iconNone;
        uint32 _iconCapture;
        uint8 _state;

        bool _captured;
    };

    CapturePointInfo _capturePointInfo[BG_BFG_DYNAMIC_NODES_COUNT];
    EventMap _bgEvents;
    uint32 _honorTics;
    uint8 _controlledPoints[PVP_TEAMS_COUNT] {};
    bool _teamScores500Disadvantage[PVP_TEAMS_COUNT] {};
    uint32 _configurableMaxTeamScore;
};
#endif
