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
#include "DatabaseEnv.h"
#include "Group.h"
#include "GroupMgr.h"
#include "LFGMgr.h"
#include "Language.h"
#include "Log.h"
#include "LootPackets.h"
#include "MapMgr.h"
#include "MiscPackets.h"
#include "ObjectMgr.h"
#include "Opcodes.h"
#include "PartyPackets.h"
#include "Pet.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "SocialMgr.h"
#include "SpellAuras.h"
#include "Util.h"
#include "Vehicle.h"
#include "World.h"
#include "WorldPacket.h"
#include "WorldSession.h"

class Aura;

/* differeces from off:
    -you can uninvite yourself - is is useful
    -you can accept invitation even if leader went offline
*/
/* todo:
    -group_destroyed msg is sent but not shown
    -reduce xp gaining when in raid group
    -quest sharing has to be corrected
    -FIX sending PartyMemberStats
*/

void WorldSession::SendPartyResult(PartyOperation operation, std::string const& member, PartyResult res, uint32 val /* = 0 */)
{
    WorldPackets::Party::PartyCommandResult packet;
    packet.Name = member;
    packet.Command = uint8(operation);
    packet.Result = uint8(res);
    packet.ResultData = val;                                // LFD cooldown related (used with ERR_PARTY_LFG_BOOT_COOLDOWN_S and ERR_PARTY_LFG_BOOT_NOT_ELIGIBLE_S)
    SendPacket(packet.Write());
}

void WorldSession::HandlePartyInviteOpcode(WorldPackets::Party::PartyInviteClient& packet)
{
    std::string membername = packet.TargetName;

    // attempt add selected player

    // cheating
    if (!normalizePlayerName(membername))
    {
        SendPartyResult(PARTY_OP_INVITE, membername, ERR_BAD_PLAYER_NAME_S);
        return;
    }

    Player* invitingPlayer = GetPlayer();
    Player* invitedPlayer = ObjectAccessor::FindPlayerByName(membername, false);

    // no player or cheat self-invite
    if (!invitedPlayer || invitedPlayer == invitingPlayer)
    {
        SendPartyResult(PARTY_OP_INVITE, membername, ERR_BAD_PLAYER_NAME_S);
        return;
    }

    if (!sScriptMgr->OnPlayerCanGroupInvite(invitingPlayer, membername))
        return;

    if (sWorld->getBoolConfig(CONFIG_TRIAL_RESTRICTION_PARTY) && IsTrialAccount())
    {
        SendPartyResult(PARTY_OP_INVITE, membername, ERR_INVITE_RESTRICTED);
        return;
    }

    if (invitingPlayer->IsSpectator() || invitedPlayer->IsSpectator())
    {
        SendPartyResult(PARTY_OP_INVITE, membername, ERR_INVITE_RESTRICTED);
        return;
    }

    // restrict invite to GMs
    if (!sWorld->getBoolConfig(CONFIG_ALLOW_GM_GROUP) && !invitingPlayer->IsGameMaster() && invitedPlayer->IsGameMaster())
    {
        SendPartyResult(PARTY_OP_INVITE, membername, ERR_BAD_PLAYER_NAME_S);
        return;
    }

    // target has opted out of receiving group invites
    if (!invitedPlayer->IsAcceptGroupInvites())
    {
        SendPartyResult(PARTY_OP_INVITE, membername, ERR_BAD_PLAYER_NAME_S);
        return;
    }

    // can't group with
    if (!invitingPlayer->IsGameMaster() && !sWorld->getBoolConfig(CONFIG_ALLOW_TWO_SIDE_INTERACTION_GROUP) && invitingPlayer->GetTeamId() != invitedPlayer->GetTeamId())
    {
        SendPartyResult(PARTY_OP_INVITE, membername, ERR_PLAYER_WRONG_FACTION);
        return;
    }
    if (invitingPlayer->GetInstanceId() != 0 && invitedPlayer->GetInstanceId() != 0 && invitingPlayer->GetInstanceId() != invitedPlayer->GetInstanceId() && invitingPlayer->GetMapId() == invitedPlayer->GetMapId())
    {
        SendPartyResult(PARTY_OP_INVITE, membername, ERR_TARGET_NOT_IN_INSTANCE_S);
        return;
    }

    if (invitedPlayer->GetSocial()->HasIgnore(invitingPlayer->GetGUID()))
    {
        SendPartyResult(PARTY_OP_INVITE, membername, ERR_IGNORING_YOU_S);
        return;
    }

    if (!invitedPlayer->GetSocial()->HasFriend(invitingPlayer->GetGUID()) && invitingPlayer->GetLevel() < sWorld->getIntConfig(CONFIG_PARTY_LEVEL_REQ))
    {
        SendPartyResult(PARTY_OP_INVITE, invitedPlayer->GetName(), ERR_INVITE_RESTRICTED);
        return;
    }

    // Battlefield raids (e.g. Wintergrasp) have their composition managed by the BF
    // system based on queue and team balance. Letting raid members recruit outsiders
    // bypasses Battlefield::AddOrSetPlayerToCorrectBfGroup, which on WG entry then
    // refuses to add the invitee because they are already in a BF group.
    if (Group* invitingGroup = invitingPlayer->GetGroup(); invitingGroup && invitingGroup->isBFGroup())
    {
        SendPartyResult(PARTY_OP_INVITE, membername, ERR_NOT_LEADER);
        return;
    }

    Group* group = invitingPlayer->GetGroup();
    if (group && group->isBGGroup())
        group = invitingPlayer->GetOriginalGroup();
    if (!group)
        group = invitingPlayer->GetGroupInvite();

    Group* group2 = invitedPlayer->GetGroup();
    if (group2 && group2->isBGGroup())
        group2 = invitedPlayer->GetOriginalGroup();
    // player already in another group or invited
    if (group2 || invitedPlayer->GetGroupInvite())
    {
        SendPartyResult(PARTY_OP_INVITE, membername, ERR_ALREADY_IN_GROUP_S);

        if (group2)
        {
            // tell the player that they were invited but it failed as they were already in a group
            WorldPackets::Party::PartyInvite partyInvite;
            partyInvite.Initialize(invitingPlayer, packet.ProposedRoles, false);
            invitedPlayer->SendDirectMessage(partyInvite.Write());
        }

        return;
    }

    if (group)
    {
        // not have permissions for invite
        if (!group->IsLeader(invitingPlayer->GetGUID()) && !group->IsAssistant(invitingPlayer->GetGUID()))
        {
            SendPartyResult(PARTY_OP_INVITE, "", ERR_NOT_LEADER);
            return;
        }
        // not have place
        if (group->IsFull())
        {
            SendPartyResult(PARTY_OP_INVITE, "", ERR_GROUP_FULL);
            return;
        }
    }

    // xinef: if player has no group, check group invite
    if (!group && invitingPlayer->GetGroupInvite() && invitingPlayer->GetGroupInvite()->GetLeaderGUID() == invitingPlayer->GetGUID())
        group = invitingPlayer->GetGroupInvite();

    // ok, but group not exist, start a new group
    // but don't create and save the group to the DB until
    // at least one person joins
    if (!group)
    {
        group = new Group();
        // new group: if can't add then delete
        if (!group->AddLeaderInvite(invitingPlayer))
        {
            delete group;
            return;
        }
        if (!group->AddInvite(invitedPlayer))
        {
            delete group;
            return;
        }
    }
    else
    {
        // already existed group: if can't add then just leave
        if (!group->AddInvite(invitedPlayer))
        {
            return;
        }
    }

    // ok, we do it
    WorldPackets::Party::PartyInvite partyInvite;
    partyInvite.Initialize(invitingPlayer, packet.ProposedRoles, true);
    invitedPlayer->SendDirectMessage(partyInvite.Write());

    SendPartyResult(PARTY_OP_INVITE, membername, ERR_PARTY_RESULT_OK);
}

void WorldSession::HandlePartyInviteResponseOpcode(WorldPackets::Party::PartyInviteResponse& packet)
{
    if (packet.Accept)
        AcceptPartyInvite();
    else
        DeclinePartyInvite();
}

void WorldSession::AcceptPartyInvite()
{
    Group* group = GetPlayer()->GetGroupInvite();

    if (!group)
        return;

    // Remove player from invitees in any case
    group->RemoveInvite(GetPlayer());

    if (GetPlayer()->IsSpectator())
    {
        SendPartyResult(PARTY_OP_INVITE, "", ERR_INVITE_RESTRICTED);
        return;
    }

    if (!sScriptMgr->OnPlayerCanGroupAccept(GetPlayer(), group))
        return;

    Player* leader = ObjectAccessor::FindConnectedPlayer(group->GetLeaderGUID());

    // Trial accounts cannot join a group whose existing members are above the trial level cap.
    if (sWorld->getBoolConfig(CONFIG_TRIAL_RESTRICTION_PARTY) && IsTrialAccount())
    {
        if (uint32 trialLevelCap = sWorld->getIntConfig(CONFIG_TRIAL_LEVEL_CAP))
        {
            uint8 leaderLevel = leader ? leader->GetLevel() : sCharacterCache->GetCharacterLevelByGuid(group->GetLeaderGUID());
            if (leaderLevel > trialLevelCap)
            {
                SendPartyResult(PARTY_OP_INVITE, "", ERR_INVITE_RESTRICTED);
                return;
            }

            for (auto const& slot : group->GetMemberSlots())
            {
                if (slot.guid == GetPlayer()->GetGUID())
                    continue;

                uint8 memberLevel = 0;
                if (Player* member = ObjectAccessor::FindConnectedPlayer(slot.guid))
                    memberLevel = member->GetLevel();
                else
                    memberLevel = sCharacterCache->GetCharacterLevelByGuid(slot.guid);

                if (memberLevel > trialLevelCap)
                {
                    SendPartyResult(PARTY_OP_INVITE, "", ERR_INVITE_RESTRICTED);
                    return;
                }
            }
        }
    }

    if (group->GetLeaderGUID() == GetPlayer()->GetGUID())
    {
        LOG_ERROR("network.opcode", "AcceptPartyInvite: player {} ({}) tried to accept an invite to his own group",
            GetPlayer()->GetName(), GetPlayer()->GetGUID().ToString());
        return;
    }

    // Group is full
    if (group->IsFull())
    {
        SendPartyResult(PARTY_OP_INVITE, "", ERR_GROUP_FULL);
        return;
    }

    // Forming a new group, create it
    if (!group->IsCreated())
    {
        // This can happen if the leader is zoning. To be removed once delayed actions for zoning are implemented
        if (!leader || leader->IsSpectator())
        {
            group->RemoveAllInvites();
            return;
        }

        // If we're about to create a group there really should be a leader present
        ASSERT(leader);
        group->RemoveInvite(leader);
        group->Create(leader);
        sGroupMgr->AddGroup(group);
    }

    // Everything is fine, do it, PLAYER'S GROUP IS SET IN ADDMEMBER!!!
    if (!group->AddMember(GetPlayer()))
        return;

    group->BroadcastGroupUpdate();
}

void WorldSession::DeclinePartyInvite()
{
    Group* group = GetPlayer()->GetGroupInvite();
    if (!group)
        return;

    // Remember leader if online (group pointer will be invalid if group gets disbanded)
    Player* leader = ObjectAccessor::FindConnectedPlayer(group->GetLeaderGUID());

    // uninvite, group can be deleted
    GetPlayer()->UninviteFromGroup();

    if (!leader)
        return;

    // report
    WorldPackets::Party::GroupDecline decline(GetPlayer()->GetName());
    leader->SendDirectMessage(decline.Write());
}

void WorldSession::HandlePartyUninviteOpcode(WorldPackets::Party::PartyUninvite& packet)
{
    // can't uninvite yourself
    if (packet.TargetGUID == GetPlayer()->GetGUID())
    {
        LOG_ERROR("network.opcode", "WorldSession::HandlePartyUninviteOpcode: leader {} ({}) tried to uninvite himself from the group.",
            GetPlayer()->GetName(), GetPlayer()->GetGUID().ToString());
        return;
    }

    std::string name;
    sCharacterCache->GetCharacterNameByGuid(packet.TargetGUID, name);

    PartyResult res = GetPlayer()->CanUninviteFromGroup(packet.TargetGUID);
    if (res != ERR_PARTY_RESULT_OK)
    {
        if (res == ERR_PARTY_LFG_BOOT_NOT_ELIGIBLE_S)
        {
            if (Player* kickTarget = ObjectAccessor::FindConnectedPlayer(packet.TargetGUID))
            {
                if (Aura* dungeonCooldownAura = kickTarget->GetAura(lfg::LFG_SPELL_DUNGEON_COOLDOWN))
                {
                    int32 elapsedTime = dungeonCooldownAura->GetMaxDuration() - dungeonCooldownAura->GetDuration();
                    if (static_cast<int32>(sWorld->getIntConfig(CONFIG_LFG_KICK_PREVENTION_TIMER)) > elapsedTime)
                        SendPartyResult(PARTY_OP_UNINVITE, name, res, (sWorld->getIntConfig(CONFIG_LFG_KICK_PREVENTION_TIMER) - elapsedTime) / 1000);
                }
            }
        }
        else
            SendPartyResult(PARTY_OP_UNINVITE, name, res);

        return;
    }

    Group* grp = GetPlayer()->GetGroup();
    if (!grp)
        return;

    // Xinef: do not allow to kick with empty reason, this will resend packet with given reason
    if (grp->isLFGGroup(true) && packet.Reason.empty())
    {
        SendPartyResult(PARTY_OP_UNINVITE, name, ERR_VOTE_KICK_REASON_NEEDED);
        return;
    }

    if (grp->IsLeader(packet.TargetGUID) && !grp->isLFGGroup(true))
    {
        SendPartyResult(PARTY_OP_UNINVITE, name, ERR_NOT_LEADER);
        return;
    }

    if (grp->IsMember(packet.TargetGUID))
    {
        Player::RemoveFromGroup(grp, packet.TargetGUID, GROUP_REMOVEMETHOD_KICK, GetPlayer()->GetGUID(), packet.Reason.c_str());
        return;
    }

    if (Player* player = grp->GetInvited(packet.TargetGUID))
    {
        player->UninviteFromGroup();
        return;
    }

    SendPartyResult(PARTY_OP_UNINVITE, name, ERR_TARGET_NOT_IN_GROUP_S);
}

void WorldSession::HandleSetPartyLeaderOpcode(WorldPackets::Party::SetPartyLeader& packet)
{
    Player* player = ObjectAccessor::FindConnectedPlayer(packet.TargetGUID);
    Group* group = GetPlayer()->GetGroup();

    if (!group || !player)
        return;

    if (!group->IsLeader(GetPlayer()->GetGUID()) || player->GetGroup() != group || packet.TargetGUID == GetPlayer()->GetGUID())
        return;

    // Everything's fine, accepted.
    group->ChangeLeader(packet.TargetGUID);
    group->SendUpdate();
}

void WorldSession::HandleLeaveGroupOpcode(WorldPackets::Party::LeaveGroup& /*packet*/)
{
    Group* grp = GetPlayer()->GetGroup();
    Group* grpInvite = GetPlayer()->GetGroupInvite();
    if (!grp && !grpInvite)
        return;

    if (_player->InBattleground()) // Do not leave group, give no error. Verified on TBC Classic
        return;

    // everything's fine, do it
    if (grp)
    {
        SendPartyResult(PARTY_OP_LEAVE, GetPlayer()->GetName(), ERR_PARTY_RESULT_OK);
        GetPlayer()->RemoveFromGroup(GROUP_REMOVEMETHOD_LEAVE);
    }
    else if (grpInvite && grpInvite->GetLeaderGUID() == GetPlayer()->GetGUID())
    {
        // pending group creation being cancelled
        SendPartyResult(PARTY_OP_LEAVE, GetPlayer()->GetName(), ERR_PARTY_RESULT_OK);
        grpInvite->Disband();
    }
}

void WorldSession::HandleSetLootMethodOpcode(WorldPackets::Party::SetLootMethod& packet)
{
    Group* group = GetPlayer()->GetGroup();
    if (!group)
        return;

    /** error handling **/
    // Xinef: Check if group is LFG
    if (!group->IsLeader(GetPlayer()->GetGUID()) || group->isLFGGroup(true))
        return;

    if (packet.LootMethod > NEED_BEFORE_GREED)
        return;

    if (packet.LootThreshold < ITEM_QUALITY_UNCOMMON || packet.LootThreshold > ITEM_QUALITY_ARTIFACT)
        return;

    if (packet.LootMethod == MASTER_LOOT && !group->IsMember(packet.LootMasterGUID))
        return;
    /********************/

    // everything's fine, do it
    group->SetLootMethod(LootMethod(packet.LootMethod));
    group->SetMasterLooterGuid(packet.LootMasterGUID);
    group->SetLootThreshold(ItemQualities(packet.LootThreshold));
    group->SendUpdate();
}

void WorldSession::HandleLootRoll(WorldPackets::Loot::LootRoll& packet)
{
    Group* group = GetPlayer()->GetGroup();
    if (!group)
        return;

    uint8 rollType = packet.RollType;
    if (GetPlayer()->HasPlayerFlag(PLAYER_FLAGS_NO_PLAY_TIME))
    {
        SendPlayTimeWarning(PTF_UNHEALTHY_TIME, 0);
        rollType = ROLL_PASS;
    }

    // 6.x LootListID is 1-based
    if (!packet.LootListID)
        return;

    Roll* roll = group->GetRoll(packet.LootObj, packet.LootListID - 1);
    if (!roll)
        return;

    group->CountRollVote(GetPlayer()->GetGUID(), roll->itemGUID, rollType);

    switch (rollType)
    {
        case ROLL_NEED:
            GetPlayer()->UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_ROLL_NEED, 1);
            break;
        case ROLL_GREED:
            GetPlayer()->UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_ROLL_GREED, 1);
            break;
        case ROLL_DISENCHANT:
            GetPlayer()->UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_ROLL_DISENCHANT, 1);
            break;
    }
}

void WorldSession::HandleMinimapPingOpcode(WorldPackets::Party::MinimapPingClient& packet)
{
    if (!sMapMgr->IsValidMapCoord(GetPlayer()->GetMap()->GetId(), packet.PositionX, packet.PositionY))
        return;

    Group* group = GetPlayer()->GetGroup();
    if (!group)
        return;

    group->DoMinimapPing(GetPlayer()->GetGUID(), packet.PositionX, packet.PositionY);
}

void WorldSession::HandleRandomRollOpcode(WorldPackets::Misc::RandomRollClient& packet)
{
    uint32 minimum, maximum;
    minimum = packet.Min;
    maximum = packet.Max;

    /** error handling **/
    if (minimum > maximum || maximum > sWorld->getIntConfig(CONFIG_RANDOM_ROLL_MAXIMUM))
        return;

    GetPlayer()->DoRandomRoll(minimum, maximum);
}

void WorldSession::HandleUpdateRaidTargetOpcode(WorldPackets::Party::UpdateRaidTarget& packet)
{
    Group* group = GetPlayer()->GetGroup();
    if (!group)
        return;

    if (packet.Symbol == -1)                                // target icon request
        group->SendTargetIconList(this, packet.PartyIndex);
    else                                                    // target icon update
    {
        if (group->isRaidGroup() && !group->IsLeader(GetPlayer()->GetGUID()) && !group->IsAssistant(GetPlayer()->GetGUID()))
            return;

        if (packet.Target.IsPlayer())
        {
            Player* target = ObjectAccessor::FindConnectedPlayer(packet.Target);
            if (!target || target->IsHostileTo(GetPlayer()))
                return;
        }

        group->SetTargetIcon(uint8(packet.Symbol), _player->GetGUID(), packet.Target, packet.PartyIndex);
    }
}

void WorldSession::HandleConvertRaidOpcode(WorldPackets::Party::ConvertRaid& packet)
{
    Group* group = GetPlayer()->GetGroup();
    if (!group)
        return;

    if (_player->InBattleground())
        return;

    /** error handling **/
    if (group->CheckLevelForRaid())
    {
        SendPartyResult(PARTY_OP_INVITE, "", ERR_RAID_DISALLOWED_BY_LEVEL);
        return;
    }

    if (!group->IsLeader(GetPlayer()->GetGUID()) || group->GetMembersCount() < 2 || group->isLFGGroup()) // pussywizard: not allowed for lfg groups, it is either raid from the beginning or not!
        return;
    /********************/

    // everything's fine, do it (is it 0 (PARTY_OP_INVITE) correct code)
    SendPartyResult(PARTY_OP_INVITE, "", ERR_PARTY_RESULT_OK);
    if (packet.Raid)
        group->ConvertToRaid();
    else
        group->ConvertToGroup();
}

void WorldSession::HandleRequestPartyJoinUpdates(WorldPackets::Party::RequestPartyJoinUpdates& packet)
{
    Group* group = GetPlayer()->GetGroup();
    if (!group)
        return;

    group->SendTargetIconList(this, packet.PartyIndex);
    group->SendRaidMarkersChanged(GetPlayer());
}

void WorldSession::HandleChangeSubGroupOpcode(WorldPackets::Party::ChangeSubGroup& packet)
{
    // we will get correct pointer for group here, so we don't have to check if group is BG raid
    Group* group = GetPlayer()->GetGroup();
    if (!group)
        return;

    if (packet.NewSubGroup >= MAX_RAID_SUBGROUPS)
        return;

    ObjectGuid senderGuid = GetPlayer()->GetGUID();
    if (!group->IsLeader(senderGuid) && !group->IsAssistant(senderGuid))
        return;

    if (!group->HasFreeSlotSubGroup(packet.NewSubGroup))
        return;

    if (!group->IsMember(packet.TargetGUID))
        return;

    group->ChangeMembersGroup(packet.TargetGUID, packet.NewSubGroup);
}

void WorldSession::HandleSwapSubGroupsOpcode(WorldPackets::Party::SwapSubGroups& packet)
{
    Group* group = GetPlayer()->GetGroup();
    if (!group || !group->isRaidGroup())
        return;

    if (!group->IsLeader(GetPlayer()->GetGUID()) && !group->IsAssistant(GetPlayer()->GetGUID()))
        return;

    if (!group->IsMember(packet.FirstTarget) || !group->IsMember(packet.SecondTarget))
    {
        std::string name;
        sCharacterCache->GetCharacterNameByGuid(packet.FirstTarget, name);
        SendPartyResult(PARTY_OP_SWAP, name, ERR_GROUP_SWAP_FAILED);
        return;
    }

    uint8 groupId1 = group->GetMemberGroup(packet.FirstTarget);
    uint8 groupId2 = group->GetMemberGroup(packet.SecondTarget);

    if (groupId1 == MAX_RAID_SUBGROUPS + 1 || groupId2 == MAX_RAID_SUBGROUPS + 1)
        return;

    if (groupId1 == groupId2)
        return;

    group->ChangeMembersGroup(packet.FirstTarget, groupId2);
    group->ChangeMembersGroup(packet.SecondTarget, groupId1);
}

void WorldSession::HandleSetAssistantLeaderOpcode(WorldPackets::Party::SetAssistantLeader& packet)
{
    Group* group = GetPlayer()->GetGroup();
    if (!group)
        return;

    if (!group->IsLeader(GetPlayer()->GetGUID()))
        return;

    group->SetGroupMemberFlag(packet.Target, packet.Apply, MEMBER_FLAG_ASSISTANT);
    group->SendUpdate();
}

void WorldSession::HandleSetPartyAssignment(WorldPackets::Party::SetPartyAssignment& packet)
{
    Group* group = GetPlayer()->GetGroup();
    if (!group)
        return;

    ObjectGuid senderGuid = GetPlayer()->GetGUID();
    if (!group->IsLeader(senderGuid) && !group->IsAssistant(senderGuid))
        return;

    switch (packet.Assignment)
    {
        case GROUP_ASSIGN_MAINASSIST:
            group->RemoveUniqueGroupMemberFlag(MEMBER_FLAG_MAINASSIST);
            group->SetGroupMemberFlag(packet.Target, packet.Set, MEMBER_FLAG_MAINASSIST);
            break;
        case GROUP_ASSIGN_MAINTANK:
            group->RemoveUniqueGroupMemberFlag(MEMBER_FLAG_MAINTANK);           // Remove main tank flag from current if any.
            group->SetGroupMemberFlag(packet.Target, packet.Set, MEMBER_FLAG_MAINTANK);
            break;
        default:
            break;
    }

    group->SendUpdate();
}

void WorldSession::HandleDoReadyCheckOpcode(WorldPackets::Party::DoReadyCheck& packet)
{
    Group* group = GetPlayer()->GetGroup();
    if (!group)
        return;

    ObjectGuid playerGuid = GetPlayer()->GetGUID();
    if (!group->IsLeader(playerGuid) && !group->IsAssistant(playerGuid))
        return;

    // check is also done client side
    if (group->ReadyCheckInProgress())
        return;

    // Check if Ready Check in BG is enabled
    if (sWorld->getBoolConfig(CONFIG_BATTLEGROUND_DISABLE_READY_CHECK_IN_BG) && _player->InBattleground())
    {
        ChatHandler(_player->GetSession()).SendNotification(LANG_BG_READY_CHECK_ERROR);
        return;
    }

    static constexpr uint32 READY_CHECK_DURATION = 35000;

    WorldPackets::Party::ReadyCheckStarted readyCheckStarted;
    readyCheckStarted.PartyIndex = packet.PartyIndex;
    readyCheckStarted.PartyGUID = group->GetGUID();
    readyCheckStarted.InitiatorGUID = playerGuid;
    readyCheckStarted.Duration = READY_CHECK_DURATION;
    group->BroadcastPacket(readyCheckStarted.Write(), false, -1);

    group->SetReadyCheckInProgress(true);
    group->ReadyCheckResetResponded();
    group->ReadyCheckMemberHasResponded(playerGuid);
    group->OfflineReadyCheck();

    // the initiator keeps track of the ready check timer
    GetPlayer()->SetReadyCheckTimer(READY_CHECK_DURATION);
}

void WorldSession::HandleReadyCheckResponseOpcode(WorldPackets::Party::ReadyCheckResponseClient& packet)
{
    Group* group = GetPlayer()->GetGroup();
    if (!group || !group->ReadyCheckInProgress())
        return;

    group->SendReadyCheckConfirm(GetPlayer()->GetGUID(), packet.IsReady);
    group->ReadyCheckMemberHasResponded(GetPlayer()->GetGUID());

    if (group->ReadyCheckAllResponded())
        GetPlayer()->CompleteReadyCheck(group);
}

void WorldSession::HandleRequestPartyMemberStatsOpcode(WorldPackets::Party::RequestPartyMemberStats& packet)
{
    WorldPackets::Party::PartyMemberStats partyMemberStats;

    Player* player = ObjectAccessor::FindConnectedPlayer(packet.TargetGUID);
    if (!player || !player->IsInSameRaidWith(_player))
    {
        partyMemberStats.MemberStats.GUID = packet.TargetGUID;
        partyMemberStats.MemberStats.Status = MEMBER_STATUS_OFFLINE;
    }
    else
        partyMemberStats.Initialize(player);

    SendPacket(partyMemberStats.Write());
}

void WorldSession::HandleSetRoleOpcode(WorldPackets::Party::SetRole& packet)
{
    Group* group = GetPlayer()->GetGroup();
    uint8 oldRole = group ? group->GetLfgRoles(packet.TargetGUID) : 0;
    if (oldRole == packet.Role)
        return;

    // Only the leader may set someone else's role
    if (packet.TargetGUID != GetPlayer()->GetGUID() && (!group || !group->IsLeader(GetPlayer()->GetGUID())))
        return;

    WorldPackets::Party::RoleChangedInform roleChangedInform;
    roleChangedInform.PartyIndex = packet.PartyIndex;
    roleChangedInform.From = GetPlayer()->GetGUID();
    roleChangedInform.ChangedUnit = packet.TargetGUID;
    roleChangedInform.OldRole = oldRole;
    roleChangedInform.NewRole = packet.Role;

    if (group)
    {
        group->BroadcastPacket(roleChangedInform.Write(), false);
        group->SetLfgRoles(packet.TargetGUID, uint8(packet.Role));
    }
    else
        SendPacket(roleChangedInform.Write());
}

void WorldSession::HandleSetEveryoneIsAssistant(WorldPackets::Party::SetEveryoneIsAssistant& packet)
{
    Group* group = GetPlayer()->GetGroup();
    if (!group || !group->isRaidGroup() || !group->IsLeader(GetPlayer()->GetGUID()))
        return;

    group->SetEveryoneIsAssistant(packet.EveryoneIsAssistant);
}

void WorldSession::HandleRequestRaidInfoOpcode(WorldPackets::Party::RequestRaidInfo& /*packet*/)
{
    // every time the player checks the character screen
    _player->SendRaidInfo();
}

void WorldSession::HandleOptOutOfLootOpcode(WorldPackets::Party::OptOutOfLoot& packet)
{
    // ignore if player not loaded
    if (!GetPlayer())
    {
        if (packet.PassOnLoot)
            LOG_ERROR("network.opcode", "CMSG_OPT_OUT_OF_LOOT value<>0 for not-loaded character!");
        return;
    }

    GetPlayer()->SetPassOnGroupLoot(packet.PassOnLoot);
}

void WorldSession::HandleInitiateRolePoll(WorldPackets::Party::InitiateRolePoll& packet)
{
    Group* group = GetPlayer()->GetGroup();
    if (!group)
        return;

    ObjectGuid guid = GetPlayer()->GetGUID();
    if (!group->IsLeader(guid) && !group->IsAssistant(guid))
        return;

    WorldPackets::Party::RolePollInform rolePollInform;
    rolePollInform.From = guid;
    rolePollInform.PartyIndex = packet.PartyIndex;
    group->BroadcastPacket(rolePollInform.Write(), true);
}

void WorldSession::HandleClearRaidMarker(WorldPackets::Party::ClearRaidMarker& packet)
{
    Group* group = GetPlayer()->GetGroup();
    if (!group)
        return;

    if (group->isRaidGroup() && !group->IsLeader(GetPlayer()->GetGUID()) && !group->IsAssistant(GetPlayer()->GetGUID()))
        return;

    // any slot past the last marker clears all of them
    if (packet.MarkerId >= MAX_RAID_MARKERS)
    {
        for (uint8 i = 0; i < MAX_RAID_MARKERS; ++i)
            group->ClearRaidMarker(i);
    }
    else
        group->ClearRaidMarker(packet.MarkerId);
}
