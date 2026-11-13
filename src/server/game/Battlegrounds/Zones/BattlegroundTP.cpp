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

#include "BattlegroundTP.h"
#include "BattlegroundMgr.h"
#include "GameGraveyard.h"
#include "GameObject.h"
#include "Object.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "World.h"
#include "WorldPacket.h"
#include "WorldStateDefines.h"

void BattlegroundTPScore::BuildObjectivesBlock(WorldPacket& data)
{
    data << uint32(2); // Objectives Count
    data << uint32(FlagCaptures);
    data << uint32(FlagReturns);
}

BattlegroundTP::BattlegroundTP()
{
    BgObjects.resize(BG_TP_OBJECT_MAX);
    BgCreatures.resize(BG_CREATURES_MAX_TP);

    StartMessageIds[BG_STARTING_EVENT_SECOND] = BG_TP_TEXT_START_ONE_MINUTE;
    StartMessageIds[BG_STARTING_EVENT_THIRD] = BG_TP_TEXT_START_HALF_MINUTE;
    StartMessageIds[BG_STARTING_EVENT_FOURTH] = BG_TP_TEXT_BATTLE_HAS_BEGUN;

    _flagState[TEAM_ALLIANCE] = BG_TP_FLAG_STATE_ON_BASE;
    _flagState[TEAM_HORDE] = BG_TP_FLAG_STATE_ON_BASE;
    _lastFlagCaptureTeam = TEAM_NEUTRAL;
    _honorWinKills = 0;
    _honorEndKills = 0;
}

BattlegroundTP::~BattlegroundTP()
{
}

void BattlegroundTP::PostUpdateImpl(uint32 diff)
{
    if (GetStatus() == STATUS_IN_PROGRESS)
    {
        _bgEvents.Update(diff);
        switch (_bgEvents.ExecuteEvent())
        {
            case BG_TP_EVENT_UPDATE_GAME_TIME:
                UpdateWorldState(WORLD_STATE_BATTLEGROUND_WS_STATE_TIMER, GetMatchTime());
                _bgEvents.ScheduleEvent(BG_TP_EVENT_UPDATE_GAME_TIME, Milliseconds(((BG_TP_TOTAL_GAME_TIME - GetStartTime()) % (MINUTE * IN_MILLISECONDS)) + 1));
                break;
            case BG_TP_EVENT_NO_TIME_LEFT:
                if (GetTeamScore(TEAM_ALLIANCE) == GetTeamScore(TEAM_HORDE))
                    EndBattleground(_lastFlagCaptureTeam);
                else
                    EndBattleground(GetTeamScore(TEAM_HORDE) > GetTeamScore(TEAM_ALLIANCE) ? TEAM_HORDE : TEAM_ALLIANCE);
                break;
            case BG_TP_EVENT_RESPAWN_BOTH_FLAGS:
                SpawnBGObject(BG_TP_OBJECT_H_FLAG, RESPAWN_IMMEDIATELY);
                SpawnBGObject(BG_TP_OBJECT_A_FLAG, RESPAWN_IMMEDIATELY);
                SendBroadcastText(BG_TP_TEXT_FLAG_PLACED_ALLIANCE, CHAT_MSG_BG_SYSTEM_NEUTRAL);
                SendBroadcastText(BG_TP_TEXT_FLAG_PLACED_HORDE, CHAT_MSG_BG_SYSTEM_NEUTRAL);
                PlaySoundToAll(BG_TP_SOUND_FLAGS_RESPAWNED);
                break;
            case BG_TP_EVENT_ALLIANCE_DROP_FLAG:
                RespawnFlagAfterDrop(TEAM_ALLIANCE);
                break;
            case BG_TP_EVENT_HORDE_DROP_FLAG:
                RespawnFlagAfterDrop(TEAM_HORDE);
                break;
            case BG_TP_EVENT_BOTH_FLAGS_KEPT10:
                if (Player* player = ObjectAccessor::GetPlayer(FindBgMap(), GetFlagPickerGUID(TEAM_ALLIANCE)))
                    player->CastSpell(player, BG_TP_SPELL_FOCUSED_ASSAULT, true);
                if (Player* player = ObjectAccessor::GetPlayer(FindBgMap(), GetFlagPickerGUID(TEAM_HORDE)))
                    player->CastSpell(player, BG_TP_SPELL_FOCUSED_ASSAULT, true);
                break;
            case BG_TP_EVENT_BOTH_FLAGS_KEPT15:
                if (Player* player = ObjectAccessor::GetPlayer(FindBgMap(), GetFlagPickerGUID(TEAM_ALLIANCE)))
                {
                    player->RemoveAurasDueToSpell(BG_TP_SPELL_FOCUSED_ASSAULT);
                    player->CastSpell(player, BG_TP_SPELL_BRUTAL_ASSAULT, true);
                }
                if (Player* player = ObjectAccessor::GetPlayer(FindBgMap(), GetFlagPickerGUID(TEAM_HORDE)))
                {
                    player->RemoveAurasDueToSpell(BG_TP_SPELL_FOCUSED_ASSAULT);
                    player->CastSpell(player, BG_TP_SPELL_BRUTAL_ASSAULT, true);
                }
                break;
            case BG_TP_EVENT_DESPAWN_DOORS:
                SpawnBGObject(BG_TP_OBJECT_DOOR_H_1, RESPAWN_ONE_DAY);
                SpawnBGObject(BG_TP_OBJECT_DOOR_H_2, RESPAWN_ONE_DAY);
                break;
        }
    }
}

void BattlegroundTP::StartingEventCloseDoors()
{
    for (uint32 i = BG_TP_OBJECT_DOOR_A_1; i <= BG_TP_OBJECT_DOOR_H_4; ++i)
    {
        DoorClose(i);
        SpawnBGObject(i, RESPAWN_IMMEDIATELY);
    }
    for (uint32 i = BG_TP_OBJECT_A_FLAG; i <= BG_TP_OBJECT_BERSERKBUFF_2; ++i)
        SpawnBGObject(i, RESPAWN_ONE_DAY);
}

void BattlegroundTP::StartingEventOpenDoors()
{
    for (uint32 i = BG_TP_OBJECT_DOOR_A_1; i <= BG_TP_OBJECT_DOOR_A_3; ++i)
        DoorOpen(i);
    for (uint32 i = BG_TP_OBJECT_DOOR_H_1; i <= BG_TP_OBJECT_DOOR_H_4; ++i)
        DoorOpen(i);

    for (uint32 i = BG_TP_OBJECT_A_FLAG; i <= BG_TP_OBJECT_BERSERKBUFF_2; ++i)
        SpawnBGObject(i, RESPAWN_IMMEDIATELY);

    StartTimedAchievement(ACHIEVEMENT_TIMED_TYPE_EVENT, TP_EVENT_START_BATTLE);
    UpdateWorldState(WORLD_STATE_BATTLEGROUND_WS_STATE_TIMER_ACTIVE, 1);
    _bgEvents.ScheduleEvent(BG_TP_EVENT_UPDATE_GAME_TIME, 0ms);
    _bgEvents.ScheduleEvent(BG_TP_EVENT_NO_TIME_LEFT, Milliseconds(BG_TP_TOTAL_GAME_TIME - 2 * MINUTE * IN_MILLISECONDS)); // 27 - 2 = 25 minutes
    _bgEvents.ScheduleEvent(BG_TP_EVENT_DESPAWN_DOORS, BG_TP_DOOR_DESPAWN_TIME);
}

void BattlegroundTP::AddPlayer(Player* player)
{
    Battleground::AddPlayer(player);
    PlayerScores.emplace(player->GetGUID().GetCounter(), new BattlegroundTPScore(player->GetGUID()));
}

void BattlegroundTP::RespawnFlagAfterDrop(TeamId teamId)
{
    if (GetStatus() != STATUS_IN_PROGRESS || GetFlagState(teamId) != BG_TP_FLAG_STATE_ON_GROUND)
        return;

    UpdateFlagState(teamId, BG_TP_FLAG_STATE_ON_BASE);
    SpawnBGObject(teamId == TEAM_ALLIANCE ? BG_TP_OBJECT_A_FLAG : BG_TP_OBJECT_H_FLAG, RESPAWN_IMMEDIATELY);
    SendBroadcastText(teamId == TEAM_ALLIANCE ? BG_TP_TEXT_FLAG_PLACED_ALLIANCE : BG_TP_TEXT_FLAG_PLACED_HORDE, CHAT_MSG_BG_SYSTEM_NEUTRAL);
    PlaySoundToAll(BG_TP_SOUND_FLAGS_RESPAWNED);

    if (GameObject* flag = GetBgMap()->GetGameObject(GetDroppedFlagGUID(teamId)))
        flag->Delete();

    SetDroppedFlagGUID(ObjectGuid::Empty, teamId);
    _bgEvents.CancelEvent(BG_TP_EVENT_BOTH_FLAGS_KEPT10);
    _bgEvents.CancelEvent(BG_TP_EVENT_BOTH_FLAGS_KEPT15);
    RemoveAssaultAuras();

    CheckFlagKeeperInArea(teamId == TEAM_ALLIANCE ? TEAM_HORDE : TEAM_ALLIANCE);
}

void BattlegroundTP::CheckFlagKeeperInArea(TeamId teamId)
{
    if (GetStatus() != STATUS_IN_PROGRESS || GetFlagState(teamId) != BG_TP_FLAG_STATE_ON_PLAYER)
    {
        return;
    }

    uint32 triggerId = teamId == TEAM_ALLIANCE ? BG_TP_TRIGGER_HORDE_FLAG_SPAWN : BG_TP_TRIGGER_ALLIANCE_FLAG_SPAWN;
    AreaTrigger const* areaTrigger = sObjectMgr->GetAreaTrigger(triggerId);
    if (Player* player = ObjectAccessor::GetPlayer(FindBgMap(), GetFlagPickerGUID(teamId)))
    {
        if (areaTrigger && player->IsInAreaTriggerRadius(areaTrigger))
        {
            HandleAreaTrigger(player, triggerId);
        }
    }
}

void BattlegroundTP::EventPlayerCapturedFlag(Player* player)
{
    if (GetStatus() != STATUS_IN_PROGRESS)
        return;

    player->RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_ENTER_PVP_COMBAT);
    RemoveAssaultAuras();

    AddPoints(player->GetTeamId(), 1);
    SetFlagPicker(ObjectGuid::Empty, GetOtherTeamId(player->GetTeamId()));
    UpdateFlagState(GetOtherTeamId(player->GetTeamId()), BG_TP_FLAG_STATE_ON_BASE);

    // Twin Peaks gives no reputation for flag captures
    if (player->GetTeamId() == TEAM_ALLIANCE)
    {
        player->RemoveAurasDueToSpell(BG_TP_SPELL_HORDE_FLAG);
        PlaySoundToAll(BG_TP_SOUND_FLAG_CAPTURED_ALLIANCE);
        SendBroadcastText(BG_TP_TEXT_CAPTURED_HORDE_FLAG, CHAT_MSG_BG_SYSTEM_ALLIANCE, player);
    }
    else
    {
        player->RemoveAurasDueToSpell(BG_TP_SPELL_ALLIANCE_FLAG);
        PlaySoundToAll(BG_TP_SOUND_FLAG_CAPTURED_HORDE);
        SendBroadcastText(BG_TP_TEXT_CAPTURED_ALLIANCE_FLAG, CHAT_MSG_BG_SYSTEM_HORDE, player);
    }

    SpawnBGObject(BG_TP_OBJECT_H_FLAG, BG_TP_FLAG_RESPAWN_TIME);
    SpawnBGObject(BG_TP_OBJECT_A_FLAG, BG_TP_FLAG_RESPAWN_TIME);

    UpdateWorldState(player->GetTeamId() == TEAM_ALLIANCE ? WORLD_STATE_BATTLEGROUND_WS_FLAG_CAPTURES_ALLIANCE : WORLD_STATE_BATTLEGROUND_WS_FLAG_CAPTURES_HORDE, GetTeamScore(player->GetTeamId()));
    UpdatePlayerScore(player, SCORE_FLAG_CAPTURES, 1);      // +1 flag captures
    _lastFlagCaptureTeam = player->GetTeamId();

    RewardHonorToTeam(GetBonusHonorFromKill(2), player->GetTeamId());

    if (GetTeamScore(TEAM_ALLIANCE) == _configurableMaxTeamScore || GetTeamScore(TEAM_HORDE) == _configurableMaxTeamScore)
    {
        UpdateWorldState(WORLD_STATE_BATTLEGROUND_WS_STATE_TIMER_ACTIVE, 0);
        EndBattleground(GetTeamScore(TEAM_HORDE) == _configurableMaxTeamScore ? TEAM_HORDE : TEAM_ALLIANCE);
    }
    else
        _bgEvents.ScheduleEvent(BG_TP_EVENT_RESPAWN_BOTH_FLAGS, Milliseconds(BG_TP_FLAG_RESPAWN_TIME));

    _bgEvents.CancelEvent(BG_TP_EVENT_BOTH_FLAGS_KEPT10);
    _bgEvents.CancelEvent(BG_TP_EVENT_BOTH_FLAGS_KEPT15);
}

void BattlegroundTP::EventPlayerDroppedFlag(Player* player)
{
    if (GetFlagPickerGUID(TEAM_HORDE) != player->GetGUID() && GetFlagPickerGUID(TEAM_ALLIANCE) != player->GetGUID())
        return;

    SetFlagPicker(ObjectGuid::Empty, GetOtherTeamId(player->GetTeamId()));
    player->RemoveAurasDueToSpell(BG_TP_SPELL_HORDE_FLAG);
    player->RemoveAurasDueToSpell(BG_TP_SPELL_ALLIANCE_FLAG);
    player->RemoveAurasDueToSpell(BG_TP_SPELL_FOCUSED_ASSAULT);
    player->RemoveAurasDueToSpell(BG_TP_SPELL_BRUTAL_ASSAULT);

    if (GetStatus() != STATUS_IN_PROGRESS)
        return;

    player->CastSpell(player, SPELL_RECENTLY_DROPPED_FLAG, true);
    if (player->GetTeamId() == TEAM_ALLIANCE)
    {
        UpdateFlagState(TEAM_HORDE, BG_TP_FLAG_STATE_ON_GROUND);
        player->CastSpell(player, BG_TP_SPELL_HORDE_FLAG_DROPPED, true);
        SendBroadcastText(BG_TP_TEXT_HORDE_FLAG_DROPPED, CHAT_MSG_BG_SYSTEM_HORDE, player);
        _bgEvents.RescheduleEvent(BG_TP_EVENT_HORDE_DROP_FLAG, BG_TP_FLAG_DROP_TIME);
    }
    else
    {
        UpdateFlagState(TEAM_ALLIANCE, BG_TP_FLAG_STATE_ON_GROUND);
        player->CastSpell(player, BG_TP_SPELL_ALLIANCE_FLAG_DROPPED, true);
        SendBroadcastText(BG_TP_TEXT_ALLIANCE_FLAG_DROPPED, CHAT_MSG_BG_SYSTEM_ALLIANCE, player);
        _bgEvents.RescheduleEvent(BG_TP_EVENT_ALLIANCE_DROP_FLAG, BG_TP_FLAG_DROP_TIME);
    }
}

void BattlegroundTP::EventPlayerClickedOnFlag(Player* player, GameObject* gameObject)
{
    if (GetStatus() != STATUS_IN_PROGRESS)
        return;

    player->RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_ENTER_PVP_COMBAT);

    // Alliance Flag picked up from base
    if (player->GetTeamId() == TEAM_HORDE && GetFlagState(TEAM_ALLIANCE) == BG_TP_FLAG_STATE_ON_BASE && BgObjects[BG_TP_OBJECT_A_FLAG] == gameObject->GetGUID())
    {
        SpawnBGObject(BG_TP_OBJECT_A_FLAG, RESPAWN_ONE_DAY);
        SetFlagPicker(player->GetGUID(), TEAM_ALLIANCE);
        UpdateFlagState(TEAM_ALLIANCE, BG_TP_FLAG_STATE_ON_PLAYER);
        player->CastSpell(player, BG_TP_SPELL_ALLIANCE_FLAG, true);
        player->StartTimedAchievement(ACHIEVEMENT_TIMED_TYPE_SPELL_TARGET, BG_TP_SPELL_ALLIANCE_FLAG_PICKED);

        PlaySoundToAll(BG_TP_SOUND_ALLIANCE_FLAG_PICKED_UP);
        SendBroadcastText(BG_TP_TEXT_ALLIANCE_FLAG_PICKED_UP, CHAT_MSG_BG_SYSTEM_HORDE, player);

        if (GetFlagState(TEAM_HORDE) != BG_TP_FLAG_STATE_ON_BASE)
        {
            _bgEvents.RescheduleEvent(BG_TP_EVENT_BOTH_FLAGS_KEPT10, BG_TP_SPELL_FORCE_TIME);
            _bgEvents.RescheduleEvent(BG_TP_EVENT_BOTH_FLAGS_KEPT15, BG_TP_SPELL_BRUTAL_TIME);
        }
        return;
    }

    // Horde Flag picked up from base
    if (player->GetTeamId() == TEAM_ALLIANCE && GetFlagState(TEAM_HORDE) == BG_TP_FLAG_STATE_ON_BASE && BgObjects[BG_TP_OBJECT_H_FLAG] == gameObject->GetGUID())
    {
        SpawnBGObject(BG_TP_OBJECT_H_FLAG, RESPAWN_ONE_DAY);
        SetFlagPicker(player->GetGUID(), TEAM_HORDE);
        UpdateFlagState(TEAM_HORDE, BG_TP_FLAG_STATE_ON_PLAYER);
        player->CastSpell(player, BG_TP_SPELL_HORDE_FLAG, true);
        player->StartTimedAchievement(ACHIEVEMENT_TIMED_TYPE_SPELL_TARGET, BG_TP_SPELL_HORDE_FLAG_PICKED);

        PlaySoundToAll(BG_TP_SOUND_HORDE_FLAG_PICKED_UP);
        SendBroadcastText(BG_TP_TEXT_HORDE_FLAG_PICKED_UP, CHAT_MSG_BG_SYSTEM_ALLIANCE, player);

        if (GetFlagState(TEAM_ALLIANCE) != BG_TP_FLAG_STATE_ON_BASE)
        {
            _bgEvents.RescheduleEvent(BG_TP_EVENT_BOTH_FLAGS_KEPT10, BG_TP_SPELL_FORCE_TIME);
            _bgEvents.RescheduleEvent(BG_TP_EVENT_BOTH_FLAGS_KEPT15, BG_TP_SPELL_BRUTAL_TIME);
        }
        return;
    }
    if (player->IsMounted())
    {
        player->Dismount();
        player->RemoveAurasByType(SPELL_AURA_MOUNTED);
    }
    // Alliance Flag on ground
    if (GetFlagState(TEAM_ALLIANCE) == BG_TP_FLAG_STATE_ON_GROUND && player->IsWithinDistInMap(gameObject, 10.0f) && gameObject->GetEntry() == BG_OBJECT_A_FLAG_GROUND_TP_ENTRY)
    {
        SetDroppedFlagGUID(ObjectGuid::Empty, TEAM_ALLIANCE);
        if (player->GetTeamId() == TEAM_ALLIANCE)
        {
            UpdateFlagState(TEAM_ALLIANCE, BG_TP_FLAG_STATE_ON_BASE);
            SpawnBGObject(BG_TP_OBJECT_A_FLAG, RESPAWN_IMMEDIATELY);
            UpdatePlayerScore(player, SCORE_FLAG_RETURNS, 1);

            PlaySoundToAll(BG_TP_SOUND_FLAG_RETURNED);
            SendBroadcastText(BG_TP_TEXT_ALLIANCE_FLAG_RETURNED, CHAT_MSG_BG_SYSTEM_ALLIANCE, player);
            _bgEvents.CancelEvent(BG_TP_EVENT_BOTH_FLAGS_KEPT10);
            _bgEvents.CancelEvent(BG_TP_EVENT_BOTH_FLAGS_KEPT15);
            RemoveAssaultAuras();

            CheckFlagKeeperInArea(TEAM_HORDE);
            return;
        }
        else
        {
            SetFlagPicker(player->GetGUID(), TEAM_ALLIANCE);
            UpdateFlagState(TEAM_ALLIANCE, BG_TP_FLAG_STATE_ON_PLAYER);
            player->CastSpell(player, BG_TP_SPELL_ALLIANCE_FLAG, true);
            if (uint32 assaultSpellId = GetAssaultSpellId())
                player->CastSpell(player, assaultSpellId, true);

            PlaySoundToAll(BG_TP_SOUND_ALLIANCE_FLAG_PICKED_UP);
            SendBroadcastText(BG_TP_TEXT_ALLIANCE_FLAG_PICKED_UP, CHAT_MSG_BG_SYSTEM_HORDE, player);
            return;
        }
    }

    // Horde Flag on ground
    if (GetFlagState(TEAM_HORDE) == BG_TP_FLAG_STATE_ON_GROUND && player->IsWithinDistInMap(gameObject, 10.0f) && gameObject->GetEntry() == BG_OBJECT_H_FLAG_GROUND_TP_ENTRY)
    {
        SetDroppedFlagGUID(ObjectGuid::Empty, TEAM_HORDE);
        if (player->GetTeamId() == TEAM_HORDE)
        {
            UpdateFlagState(TEAM_HORDE, BG_TP_FLAG_STATE_ON_BASE);
            SpawnBGObject(BG_TP_OBJECT_H_FLAG, RESPAWN_IMMEDIATELY);
            UpdatePlayerScore(player, SCORE_FLAG_RETURNS, 1);

            PlaySoundToAll(BG_TP_SOUND_FLAG_RETURNED);
            SendBroadcastText(BG_TP_TEXT_HORDE_FLAG_RETURNED, CHAT_MSG_BG_SYSTEM_HORDE, player);
            _bgEvents.CancelEvent(BG_TP_EVENT_BOTH_FLAGS_KEPT10);
            _bgEvents.CancelEvent(BG_TP_EVENT_BOTH_FLAGS_KEPT15);
            RemoveAssaultAuras();

            CheckFlagKeeperInArea(TEAM_ALLIANCE);
            return;
        }
        else
        {
            SetFlagPicker(player->GetGUID(), TEAM_HORDE);
            UpdateFlagState(TEAM_HORDE, BG_TP_FLAG_STATE_ON_PLAYER);
            player->CastSpell(player, BG_TP_SPELL_HORDE_FLAG, true);
            if (uint32 assaultSpellId = GetAssaultSpellId())
                player->CastSpell(player, assaultSpellId, true);

            PlaySoundToAll(BG_TP_SOUND_HORDE_FLAG_PICKED_UP);
            SendBroadcastText(BG_TP_TEXT_HORDE_FLAG_PICKED_UP, CHAT_MSG_BG_SYSTEM_ALLIANCE, player);
            return;
        }
    }
}

void BattlegroundTP::RemovePlayer(Player* player)
{
    if (GetFlagPickerGUID(TEAM_ALLIANCE) == player->GetGUID() || GetFlagPickerGUID(TEAM_HORDE) == player->GetGUID())
        EventPlayerDroppedFlag(player);
}

void BattlegroundTP::UpdateFlagState(TeamId teamId, uint32 value)
{
    _flagState[teamId] = value;
    UpdateWorldState(teamId == TEAM_ALLIANCE ? WORLD_STATE_BATTLEGROUND_WS_FLAG_STATE_HORDE : WORLD_STATE_BATTLEGROUND_WS_FLAG_STATE_ALLIANCE, value);
}

void BattlegroundTP::HandleAreaTrigger(Player* player, uint32 trigger)
{
    if (GetStatus() != STATUS_IN_PROGRESS || !player->IsAlive())
        return;

    switch (trigger)
    {
        case BG_TP_TRIGGER_ALLIANCE_FLAG_SPAWN: // Alliance Flag spawn
            if (GetFlagState(TEAM_ALLIANCE) == BG_TP_FLAG_STATE_ON_BASE && GetFlagPickerGUID(TEAM_HORDE) == player->GetGUID())
                EventPlayerCapturedFlag(player);
            break;
        case BG_TP_TRIGGER_HORDE_FLAG_SPAWN: // Horde Flag spawn
            if (GetFlagState(TEAM_HORDE) == BG_TP_FLAG_STATE_ON_BASE && GetFlagPickerGUID(TEAM_ALLIANCE) == player->GetGUID())
                EventPlayerCapturedFlag(player);
            break;
        // Twin Peaks scenery-only triggers (towers, houses, start-zone
        // markers) - real TrinityCore also no-ops these.
        case 5908: case 5909: case 5910: case 5911:
        case 5914: case 5916: case 5917: case 5918: case 5920: case 5921:
            break;
    }
}

bool BattlegroundTP::SetupBattleground()
{
    if (sBattlegroundMgr->IsBGWeekend(GetBgTypeID(true)))
    {
        _honorWinKills = 3;
        _honorEndKills = 4;
    }
    else
    {
        _honorWinKills = 1;
        _honorEndKills = 2;
    }

    // flags
    AddObject(BG_TP_OBJECT_A_FLAG, BG_OBJECT_A_FLAG_TP_ENTRY, 2118.210f, 191.621f, 44.052f, 5.741259f, 0, 0, 0.9996573f, 0.02617699f, RESPAWN_IMMEDIATELY);
    AddObject(BG_TP_OBJECT_H_FLAG, BG_OBJECT_H_FLAG_TP_ENTRY, 1578.337f, 344.0451f, 2.418409f, 2.792518f, 0, 0, 0.984807f, 0.1736523f, RESPAWN_IMMEDIATELY);
    // buffs
    AddObject(BG_TP_OBJECT_SPEEDBUFF_1, BG_OBJECT_SPEEDBUFF_1_TP_ENTRY, 1544.55f, 303.852f, 0.692371f, 6.265733f, 0, 0, 0, 0, SPEED_BUFF_RESPAWN_TIME);
    AddObject(BG_TP_OBJECT_SPEEDBUFF_2, BG_OBJECTID_SPEEDBUFF_ENTRY, 2175.87f, 226.622f, 43.7629f, 2.60053f, 0, 0, 0, 0, SPEED_BUFF_RESPAWN_TIME);
    AddObject(BG_TP_OBJECT_REGENBUFF_1, BG_OBJECT_REGENBUFF_1_TP_ENTRY, 1754.163f, 242.125f, -14.13157f, 1.151916f, 0, 0, 0, 0, RESTORATION_BUFF_RESPAWN_TIME);
    AddObject(BG_TP_OBJECT_REGENBUFF_2, BG_OBJECTID_REGENBUFF_ENTRY, 1951.18f, 383.795f, -10.5257f, 4.06662f, 0, 0, 0, 0, RESTORATION_BUFF_RESPAWN_TIME);
    AddObject(BG_TP_OBJECT_BERSERKBUFF_1, BG_OBJECTID_BERSERKERBUFF_ENTRY, 1932.832f, 226.7917f, -17.05979f, 2.44346f, 0, 0, 0, 0, BERSERKING_BUFF_RESPAWN_TIME);
    AddObject(BG_TP_OBJECT_BERSERKBUFF_2, BG_OBJECT_BERSERKBUFF_1_TP_ENTRY, 1737.569f, 435.845f, -8.0863f, 5.51524f, 0, 0, 0, 0, BERSERKING_BUFF_RESPAWN_TIME);
    // alliance gates
    AddObject(BG_TP_OBJECT_DOOR_A_1, BG_OBJECT_DOOR_A_1_TP_ENTRY, 2118.088f, 154.6754f, 43.57089f, 2.609261f, 0, 0, 0, 0, RESPAWN_IMMEDIATELY);
    AddObject(BG_TP_OBJECT_DOOR_A_2, BG_OBJECT_DOOR_A_2_TP_ENTRY, 2156.0f, 219.2059f, 43.6256f, 2.609261f, 0, 0, 0, 0, RESPAWN_IMMEDIATELY);
    AddObject(BG_TP_OBJECT_DOOR_A_3, BG_OBJECT_DOOR_A_3_TP_ENTRY, 2135.525f, 218.926f, 43.60946f, 5.750861f, 0, 0, 0, 0, RESPAWN_IMMEDIATELY);
    // horde gates
    AddObject(BG_TP_OBJECT_DOOR_H_1, BG_OBJECT_DOOR_H_1_TP_ENTRY, 1556.656f, 314.7127f, 1.589001f, 6.178466f, 0, 0, 0, 0, RESPAWN_IMMEDIATELY);
    AddObject(BG_TP_OBJECT_DOOR_H_2, BG_OBJECT_DOOR_H_2_TP_ENTRY, 1574.605f, 321.2421f, 1.58989f, 6.178466f, 0, 0, 0, 0, RESPAWN_IMMEDIATELY);
    AddObject(BG_TP_OBJECT_DOOR_H_3, BG_OBJECT_DOOR_H_3_TP_ENTRY, 1558.088f, 372.7654f, 1.723727f, 6.178466f, 0, 0, 0, 0, RESPAWN_IMMEDIATELY);
    AddObject(BG_TP_OBJECT_DOOR_H_4, BG_OBJECT_DOOR_H_4_TP_ENTRY, 1558.623f, 379.1595f, -6.409669f, 4.607672f, 0, 0, 0, 0, RESPAWN_IMMEDIATELY);

    GraveyardStruct const* sg = sGraveyard->GetGraveyard(TP_GRAVEYARD_START_ALLIANCE);
    AddSpiritGuide(TP_SPIRIT_MAIN_ALLIANCE, sg->x, sg->y, sg->z, 0.0f, TEAM_ALLIANCE);

    sg = sGraveyard->GetGraveyard(TP_GRAVEYARD_START_HORDE);
    AddSpiritGuide(TP_SPIRIT_MAIN_HORDE, sg->x, sg->y, sg->z, 0.0f, TEAM_HORDE);

    for (uint32 i = BG_TP_OBJECT_DOOR_A_1; i < BG_TP_OBJECT_MAX; ++i)
        if (!BgObjects[i])
        {
            LOG_ERROR("sql.sql", "BattlegroundTP: Failed to spawn some object Battleground not created!");
            return false;
        }

    for (uint32 i = TP_SPIRIT_MAIN_ALLIANCE; i < BG_CREATURES_MAX_TP; ++i)
        if (!BgCreatures[i])
        {
            LOG_ERROR("sql.sql", "BattlegroundTP: Failed to spawn spirit guides Battleground not created!");
            return false;
        }

    return true;
}

void BattlegroundTP::Init()
{
    //call parent's class reset
    Battleground::Init();

    _bgEvents.Reset();
    _flagKeepers[TEAM_ALLIANCE].Clear();
    _flagKeepers[TEAM_HORDE].Clear();
    _droppedFlagGUID[TEAM_ALLIANCE].Clear();
    _droppedFlagGUID[TEAM_HORDE].Clear();
    _flagState[TEAM_ALLIANCE]       = BG_TP_FLAG_STATE_ON_BASE;
    _flagState[TEAM_HORDE]          = BG_TP_FLAG_STATE_ON_BASE;
    _lastFlagCaptureTeam            = TEAM_NEUTRAL;
    _configurableMaxTeamScore       = static_cast<uint32>(BG_TP_MAX_TEAM_SCORE);
}

void BattlegroundTP::EndBattleground(TeamId winnerTeamId)
{
    // Win reward
    RewardHonorToTeam(GetBonusHonorFromKill(_honorWinKills), winnerTeamId);

    // Complete map_end rewards (even if no team wins)
    RewardHonorToTeam(GetBonusHonorFromKill(_honorEndKills), TEAM_ALLIANCE);
    RewardHonorToTeam(GetBonusHonorFromKill(_honorEndKills), TEAM_HORDE);

    Battleground::EndBattleground(winnerTeamId);
}

void BattlegroundTP::HandleKillPlayer(Player* player, Player* killer)
{
    if (GetStatus() != STATUS_IN_PROGRESS)
        return;

    EventPlayerDroppedFlag(player);
    Battleground::HandleKillPlayer(player, killer);
}

bool BattlegroundTP::UpdatePlayerScore(Player* player, uint32 type, uint32 value, bool doAddHonor)
{
    if (!Battleground::UpdatePlayerScore(player, type, value, doAddHonor))
        return false;

    switch (type)
    {
        case SCORE_FLAG_CAPTURES:
            player->UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_BG_OBJECTIVE_CAPTURE, TP_OBJECTIVE_CAPTURE_FLAG);
            break;
        case SCORE_FLAG_RETURNS:
            player->UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_BG_OBJECTIVE_CAPTURE, TP_OBJECTIVE_RETURN_FLAG);
            break;
    }

    return true;
}

// Before the start players use the flag room graveyard, then the start or middle one,
// whichever is closer to the enemy base
GraveyardStruct const* BattlegroundTP::GetClosestGraveyard(Player* player)
{
    TeamId teamId = player->GetTeamId();

    if (GetStatus() != STATUS_IN_PROGRESS)
        return sGraveyard->GetGraveyard(teamId == TEAM_ALLIANCE ? TP_GRAVEYARD_FLAGROOM_ALLIANCE : TP_GRAVEYARD_FLAGROOM_HORDE);

    GraveyardStruct const* enemyBase = sGraveyard->GetGraveyard(teamId == TEAM_ALLIANCE ? TP_GRAVEYARD_FLAGROOM_HORDE : TP_GRAVEYARD_FLAGROOM_ALLIANCE);
    GraveyardStruct const* enemyMiddle = sGraveyard->GetGraveyard(teamId == TEAM_ALLIANCE ? TP_GRAVEYARD_MIDDLE_HORDE : TP_GRAVEYARD_MIDDLE_ALLIANCE);

    float distToEnemyBase = player->GetDistance2d(enemyBase->x, enemyBase->y);
    float distToEnemyMiddle = player->GetDistance2d(enemyMiddle->x, enemyMiddle->y);

    if (distToEnemyBase < distToEnemyMiddle)
        return sGraveyard->GetGraveyard(teamId == TEAM_ALLIANCE ? TP_GRAVEYARD_START_ALLIANCE : TP_GRAVEYARD_START_HORDE);

    return sGraveyard->GetGraveyard(teamId == TEAM_ALLIANCE ? TP_GRAVEYARD_MIDDLE_ALLIANCE : TP_GRAVEYARD_MIDDLE_HORDE);
}

void BattlegroundTP::FillInitialWorldStates(WorldPackets::WorldState::InitWorldStates& packet)
{
    // Twin Peaks uses the Warsong Gulch worldstates
    packet.Worldstates.reserve(7);
    packet.Worldstates.emplace_back(WORLD_STATE_BATTLEGROUND_WS_FLAG_CAPTURES_ALLIANCE, GetTeamScore(TEAM_ALLIANCE));
    packet.Worldstates.emplace_back(WORLD_STATE_BATTLEGROUND_WS_FLAG_CAPTURES_HORDE, GetTeamScore(TEAM_HORDE));
    packet.Worldstates.emplace_back(WORLD_STATE_BATTLEGROUND_WS_FLAG_CAPTURES_MAX, _configurableMaxTeamScore);

    packet.Worldstates.emplace_back(WORLD_STATE_BATTLEGROUND_WS_STATE_TIMER_ACTIVE, GetStatus() == STATUS_IN_PROGRESS ? 1 : 0);
    packet.Worldstates.emplace_back(WORLD_STATE_BATTLEGROUND_WS_STATE_TIMER, GetMatchTime());

    packet.Worldstates.emplace_back(WORLD_STATE_BATTLEGROUND_WS_FLAG_STATE_HORDE, GetFlagState(TEAM_HORDE));
    packet.Worldstates.emplace_back(WORLD_STATE_BATTLEGROUND_WS_FLAG_STATE_ALLIANCE, GetFlagState(TEAM_ALLIANCE));
}

TeamId BattlegroundTP::GetPrematureWinner()
{
    if (GetTeamScore(TEAM_ALLIANCE) > GetTeamScore(TEAM_HORDE))
        return TEAM_ALLIANCE;

    return GetTeamScore(TEAM_HORDE) > GetTeamScore(TEAM_ALLIANCE) ? TEAM_HORDE : Battleground::GetPrematureWinner();
}

uint32 BattlegroundTP::GetAssaultSpellId() const
{
    if ((!GetFlagPickerGUID(TEAM_ALLIANCE) && GetFlagState(TEAM_ALLIANCE) != BG_TP_FLAG_STATE_ON_GROUND) ||
            (!GetFlagPickerGUID(TEAM_HORDE) && GetFlagState(TEAM_HORDE) != BG_TP_FLAG_STATE_ON_GROUND) ||
            _bgEvents.HasTimeUntilEvent(BG_TP_EVENT_BOTH_FLAGS_KEPT10))
        return 0;

    return _bgEvents.HasTimeUntilEvent(BG_TP_EVENT_BOTH_FLAGS_KEPT15) ? BG_TP_SPELL_FOCUSED_ASSAULT : BG_TP_SPELL_BRUTAL_ASSAULT;
}

void BattlegroundTP::RemoveAssaultAuras()
{
    if (Player* player = ObjectAccessor::GetPlayer(FindBgMap(), GetFlagPickerGUID(TEAM_ALLIANCE)))
    {
        player->RemoveAurasDueToSpell(BG_TP_SPELL_FOCUSED_ASSAULT);
        player->RemoveAurasDueToSpell(BG_TP_SPELL_BRUTAL_ASSAULT);
    }
    if (Player* player = ObjectAccessor::GetPlayer(FindBgMap(), GetFlagPickerGUID(TEAM_HORDE)))
    {
        player->RemoveAurasDueToSpell(BG_TP_SPELL_FOCUSED_ASSAULT);
        player->RemoveAurasDueToSpell(BG_TP_SPELL_BRUTAL_ASSAULT);
    }
}
