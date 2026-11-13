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

#include "DBCStores.h"
#include "GameTime.h"
#include "Group.h"
#include "LFGMgr.h"
#include "LFGPackets.h"
#include "ObjectAccessor.h"
#include "ObjectMgr.h"
#include "Opcodes.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "WorldPacket.h"
#include "WorldSession.h"

namespace
{
    // The 4.3.4 client keys every queue packet by a ticket and echoes it back. The server does not
    // keep tickets, so one is derived from the player's party: it stays the same for every packet
    // of one queue.
    WorldPackets::LFG::RideTicket MakeLfgTicket(Player const* player)
    {
        WorldPackets::LFG::RideTicket ticket;
        ticket.Type = WorldPackets::LFG::RideType::Lfg;
        if (Group const* group = player->GetGroup())
        {
            ticket.RequesterGuid = group->GetLeaderGUID();
            ticket.Id = group->GetGUID().GetCounter();
        }
        else
        {
            ticket.RequesterGuid = player->GetGUID();
            ticket.Id = player->GetGUID().GetCounter();
        }

        return ticket;
    }

    void AddQuestCurrencies(std::vector<WorldPackets::LFG::LfgPlayerQuestRewardCurrency>& currencies, Quest const* quest)
    {
        for (uint32 i = 0; i < QUEST_REWARD_CURRENCY_COUNT; ++i)
            if (uint32 currencyId = quest->RewardCurrencyId[i])
                if (CurrencyTypesEntry const* currency = sCurrencyTypesStore.LookupEntry(currencyId))
                    currencies.emplace_back(currencyId, quest->RewardCurrencyCount[i] * (currency->UsesPrecision() ? CURRENCY_PRECISION : 1));
    }
}

void WorldSession::HandleLfgJoinOpcode(WorldPackets::LFG::LFGJoin& packet)
{
    if (!sLFGMgr->isOptionEnabled(lfg::LFG_OPTION_ENABLE_DUNGEON_FINDER | lfg::LFG_OPTION_ENABLE_RAID_BROWSER | lfg::LFG_OPTION_ENABLE_SEASONAL_BOSSES) ||
        (GetPlayer()->GetGroup() && GetPlayer()->GetGroup()->GetLeaderGUID() != GetPlayer()->GetGUID() &&
         (GetPlayer()->GetGroup()->GetMembersCount() == MAXGROUPSIZE || !GetPlayer()->GetGroup()->isLFGGroup())))
        return;

    if (packet.Slots.empty())
    {
        LOG_DEBUG("lfg", "CMSG_LFG_JOIN {} no dungeons selected", GetPlayerInfo());
        return;
    }

    lfg::LfgDungeonSet newDungeons;
    for (uint32 slot : packet.Slots)
    {
        uint32 dungeon = slot & 0x00FFFFFF;                             // remove the type from the dungeon entry
        if (sLFGDungeonStore.LookupEntry(dungeon))
            newDungeons.insert(dungeon);
    }

    LOG_DEBUG("network", "CMSG_LFG_JOIN [{}] roles: {}, Dungeons: {}, Comment: {}",
                 GetPlayerInfo(), packet.Roles, newDungeons.size(), packet.Comment);

    sLFGMgr->JoinLfg(GetPlayer(), uint8(packet.Roles), newDungeons, packet.Comment);
    GetPlayer()->UpdateLFGChannel();
}

void WorldSession::HandleLfgLeaveOpcode(WorldPackets::LFG::LFGLeave& /*packet*/)
{
    Group* group = GetPlayer()->GetGroup();
    ObjectGuid guid = GetPlayer()->GetGUID();
    ObjectGuid gguid = group ? group->GetGUID() : guid;

    LOG_DEBUG("network", "CMSG_LFG_LEAVE [{}] in group: {}", guid.ToString(), group ? 1 : 0);

    // Check cheating - only leader can leave the queue
    if (!group || group->GetLeaderGUID() == guid)
    {
        sLFGMgr->LeaveLfg(sLFGMgr->GetState(guid) == lfg::LFG_STATE_RAIDBROWSER ? guid : gguid);
        sLFGMgr->LeaveAllLfgQueues(guid, true, group ? group->GetGUID() : ObjectGuid::Empty);
    }
    GetPlayer()->UpdateLFGChannel();
}

void WorldSession::HandleLfgProposalResultOpcode(WorldPackets::LFG::LFGProposalResponse& packet)
{
    LOG_DEBUG("network", "CMSG_LFG_PROPOSAL_RESULT [{}] proposal: {} accept: {}", GetPlayer()->GetGUID().ToString(), packet.ProposalID, packet.Accepted ? 1 : 0);
    sLFGMgr->UpdateProposal(packet.ProposalID, GetPlayer()->GetGUID(), packet.Accepted);
}

void WorldSession::HandleLfgSetRolesOpcode(WorldPackets::LFG::LFGSetRoles& packet)
{
    uint8 roles = uint8(packet.RolesDesired);
    ObjectGuid guid = GetPlayer()->GetGUID();
    Group* group = GetPlayer()->GetGroup();
    if (!group)
    {
        LOG_DEBUG("network", "CMSG_LFG_SET_ROLES [{}] Not in group", guid.ToString());
        return;
    }
    ObjectGuid gguid = group->GetGUID();
    LOG_DEBUG("network", "CMSG_LFG_SET_ROLES: Group [{}], Player [{}], Roles: {}", gguid.ToString(), guid.ToString(), roles);
    sLFGMgr->UpdateRoleCheck(gguid, guid, roles);
}

void WorldSession::HandleLfgSetCommentOpcode(WorldPackets::LFG::LFGSetComment& packet)
{
    ObjectGuid guid = GetPlayer()->GetGUID();
    LOG_DEBUG("network", "CMSG_LFG_SET_COMMENT [{}] comment: {}", guid.ToString(), packet.Comment);

    sLFGMgr->SetComment(GetPlayer()->GetGUID(), packet.Comment);
    sLFGMgr->LfrSetComment(GetPlayer(), packet.Comment);
}

void WorldSession::HandleLfgSetBootVoteOpcode(WorldPackets::LFG::LFGBootPlayerVote& packet)
{
    ObjectGuid guid = GetPlayer()->GetGUID();
    LOG_DEBUG("network", "CMSG_LFG_SET_BOOT_VOTE [{}] agree: {}", guid.ToString(), packet.Vote ? 1 : 0);
    sLFGMgr->UpdateBoot(guid, packet.Vote);
}

void WorldSession::HandleLfgTeleportOpcode(WorldPackets::LFG::LFGTeleport& packet)
{
    LOG_DEBUG("network", "CMSG_LFG_TELEPORT [{}] out: {}", GetPlayer()->GetGUID().ToString(), packet.TeleportOut ? 1 : 0);
    sLFGMgr->TeleportPlayer(GetPlayer(), packet.TeleportOut);
}

void WorldSession::HandleLfgGetLockInfoOpcode(WorldPackets::LFG::LFGGetSystemInfo& packet)
{
    LOG_DEBUG("network", "CMSG_LFG_LOCK_INFO_REQUEST [{}] for {}", GetPlayer()->GetGUID().ToString(), packet.Player ? "player" : "party");

    if (packet.Player)
        SendLfgPlayerLockInfo();
    else
        SendLfgPartyLockInfo();
}

void WorldSession::SendLfgPlayerLockInfo()
{
    ObjectGuid guid = GetPlayer()->GetGUID();

    // Get Random dungeons that can be done at a certain level and expansion
    uint8 level = GetPlayer()->GetLevel();
    lfg::LfgDungeonSet const& randomDungeons =
        sLFGMgr->GetRandomAndSeasonalDungeons(level, GetPlayer()->GetSession()->Expansion());

    // Get player locked Dungeons
    sLFGMgr->InitializeLockedDungeons(GetPlayer(), GetPlayer()->GetGroup()); // pussywizard

    WorldPackets::LFG::LFGPlayerInfo lfgPlayerInfo;
    for (auto const& [dungeon, lockStatus] : sLFGMgr->GetLockedDungeons(guid))
        lfgPlayerInfo.BlackList.Slot.emplace_back(dungeon, lockStatus, 0, 0);

    uint8 playerLevelForXP = level;
    sScriptMgr->OnPlayerBeforeGetLevelForXPGain(GetPlayer(), playerLevelForXP);

    for (uint32 slot : randomDungeons)
    {
        WorldPackets::LFG::LfgPlayerDungeonInfo& playerDungeonInfo = lfgPlayerInfo.Dungeon.emplace_back();
        playerDungeonInfo.Slot = slot;

        Quest const* quest = nullptr;
        bool done = false;
        if (lfg::LfgReward const* reward = sLFGMgr->GetRandomDungeonReward(slot, level))
        {
            quest = sObjectMgr->GetQuestTemplate(reward->firstQuest);
            if (quest)
            {
                done = !GetPlayer()->CanRewardQuest(quest, false);
                if (done)
                    quest = sObjectMgr->GetQuestTemplate(reward->otherQuest);
            }
        }

        playerDungeonInfo.FirstReward = !done;
        if (!quest)
            continue;

        playerDungeonInfo.Rewards.RewardMoney = quest->GetRewOrReqMoney(level);
        if (playerLevelForXP < GetPlayer()->GetUInt32Value(PLAYER_FIELD_MAX_LEVEL))
            playerDungeonInfo.Rewards.RewardXP = quest->XPValue(playerLevelForXP);

        for (uint8 i = 0; i < QUEST_REWARDS_COUNT; ++i)
            if (uint32 itemId = quest->RewardItemId[i])
                playerDungeonInfo.Rewards.Item.emplace_back(itemId, quest->RewardItemIdCount[i]);

        AddQuestCurrencies(playerDungeonInfo.Rewards.Currency, quest);
    }

    LOG_DEBUG("network", "SMSG_LFG_PLAYER_INFO [{}]", guid.ToString());
    SendPacket(lfgPlayerInfo.Write());
}

void WorldSession::SendLfgPartyLockInfo()
{
    ObjectGuid guid = GetPlayer()->GetGUID();
    Group* group = GetPlayer()->GetGroup();
    if (!group)
        return;

    // Get the locked dungeons of the other party members
    WorldPackets::LFG::LFGPartyInfo lfgPartyInfo;
    for (GroupReference* itr = group->GetFirstMember(); itr != nullptr; itr = itr->next())
    {
        Player* plrg = itr->GetSource();
        if (!plrg)
            continue;

        ObjectGuid pguid = plrg->GetGUID();
        if (pguid == guid)
            continue;

        sLFGMgr->InitializeLockedDungeons(plrg, group); // pussywizard

        WorldPackets::LFG::LFGBlackList& lfgBlackList = lfgPartyInfo.Player.emplace_back();
        lfgBlackList.PlayerGuid = pguid;
        for (auto const& [dungeon, lockStatus] : sLFGMgr->GetLockedDungeons(pguid))
            lfgBlackList.Slot.emplace_back(dungeon, lockStatus, 0, 0);
    }

    LOG_DEBUG("network", "SMSG_LFG_PARTY_INFO [{}]", guid.ToString());
    SendPacket(lfgPartyInfo.Write());
}

void WorldSession::HandleLfrSearchJoinOpcode(WorldPacket& recvData)
{
    uint32 dungeonId;
    recvData >> dungeonId;

    if (!sLFGMgr->isOptionEnabled(lfg::LFG_OPTION_ENABLE_RAID_BROWSER))
        return;

    dungeonId = (dungeonId & 0x00FFFFFF); // remove the type from the dungeon entry
    sLFGMgr->LfrSearchAdd(GetPlayer(), dungeonId);
    sLFGMgr->SendRaidBrowserCachedList(GetPlayer(), dungeonId);
}

void WorldSession::HandleLfrSearchLeaveOpcode(WorldPacket& recvData)
{
    uint32 dungeonId;
    recvData >> dungeonId;
    sLFGMgr->LfrSearchRemove(GetPlayer());
}

void WorldSession::HandleLfgGetStatus(WorldPacket& /*recvData*/)
{
    LOG_DEBUG("lfg", "CMSG_LFG_GET_STATUS {}", GetPlayerInfo());

    ObjectGuid guid = GetPlayer()->GetGUID();
    lfg::LfgUpdateData updateData = sLFGMgr->GetLfgStatus(guid);

    if (GetPlayer()->GetGroup())
    {
        SendLfgUpdateParty(updateData);
        updateData.dungeons.clear();
        SendLfgUpdatePlayer(updateData);
    }
    else
    {
        SendLfgUpdatePlayer(updateData);
        updateData.dungeons.clear();
        SendLfgUpdateParty(updateData);
    }
}

void WorldSession::SendLfgUpdatePlayer(lfg::LfgUpdateData const& updateData)
{
    SendLfgUpdateStatus(updateData, false);
}

void WorldSession::SendLfgUpdateParty(lfg::LfgUpdateData const& updateData)
{
    SendLfgUpdateStatus(updateData, true);
}

void WorldSession::SendLfgUpdateStatus(lfg::LfgUpdateData const& updateData, bool party)
{
    bool join = false;
    bool queued = false;

    switch (updateData.updateType)
    {
        case lfg::LFG_UPDATETYPE_JOIN_QUEUE_INITIAL:            // Joined queue outside the dungeon
            join = true;
            break;
        case lfg::LFG_UPDATETYPE_JOIN_QUEUE:
        case lfg::LFG_UPDATETYPE_ADDED_TO_QUEUE:                // Rolecheck Success
            join = true;
            queued = true;
            break;
        case lfg::LFG_UPDATETYPE_JOIN_RAIDBROWSER:
        case lfg::LFG_UPDATETYPE_PROPOSAL_BEGIN:
            join = true;
            break;
        case lfg::LFG_UPDATETYPE_UPDATE_STATUS:
            join = updateData.state != lfg::LFG_STATE_ROLECHECK && updateData.state != lfg::LFG_STATE_NONE &&
                updateData.state != lfg::LFG_STATE_DUNGEON && updateData.state != lfg::LFG_STATE_FINISHED_DUNGEON;
            queued = updateData.state == lfg::LFG_STATE_QUEUED || updateData.state == lfg::LFG_STATE_RAIDBROWSER;
            break;
        default:
            break;
    }

    LOG_DEBUG("lfg", "SMSG_LFG_UPDATE_STATUS {} updatetype: {}, party {}", GetPlayerInfo(), updateData.updateType, party);

    WorldPackets::LFG::LFGUpdateStatus lfgUpdateStatus;
    lfgUpdateStatus.Ticket = MakeLfgTicket(GetPlayer());
    lfgUpdateStatus.Reason = updateData.updateType;
    for (uint32 dungeonId : updateData.dungeons)
        lfgUpdateStatus.Slots.push_back(sLFGMgr->GetLFGDungeonEntry(dungeonId));
    lfgUpdateStatus.RequestedRoles = sLFGMgr->GetRoles(GetPlayer()->GetGUID());
    lfgUpdateStatus.IsParty = party;
    lfgUpdateStatus.Joined = join;
    lfgUpdateStatus.LfgJoined = updateData.updateType != lfg::LFG_UPDATETYPE_REMOVED_FROM_QUEUE;
    lfgUpdateStatus.Queued = queued;
    lfgUpdateStatus.Comment = updateData.comment;
    SendPacket(lfgUpdateStatus.Write());
}

void WorldSession::SendLfgRoleChosen(ObjectGuid guid, uint8 roles)
{
    LOG_DEBUG("network", "SMSG_LFG_ROLE_CHOSEN [{}] guid: [{}] roles: {}", GetPlayer()->GetGUID().ToString(), guid.ToString(), roles);

    WorldPackets::LFG::RoleChosen roleChosen;
    roleChosen.Player = guid;
    roleChosen.RoleMask = roles;
    roleChosen.Accepted = roles > 0;
    SendPacket(roleChosen.Write());
}

void WorldSession::SendLfgRoleCheckUpdate(lfg::LfgRoleCheck const& roleCheck)
{
    lfg::LfgDungeonSet dungeons;
    if (roleCheck.rDungeonId)
        dungeons.insert(roleCheck.rDungeonId);
    else
        dungeons = roleCheck.dungeons;

    LOG_DEBUG("network", "SMSG_LFG_ROLE_CHECK_UPDATE [{}]", GetPlayer()->GetGUID().ToString());

    WorldPackets::LFG::LFGRoleCheckUpdate lfgRoleCheckUpdate;
    lfgRoleCheckUpdate.RoleCheckStatus = roleCheck.state;
    lfgRoleCheckUpdate.IsBeginning = roleCheck.state == lfg::LFG_ROLECHECK_INITIALITING;
    for (uint32 dungeonId : dungeons)
        lfgRoleCheckUpdate.JoinSlots.push_back(sLFGMgr->GetLFGDungeonEntry(dungeonId));

    auto addMember = [&](ObjectGuid guid, uint8 roles)
    {
        Player* player = ObjectAccessor::FindConnectedPlayer(guid);
        lfgRoleCheckUpdate.Members.emplace_back(guid, roles, player ? player->GetLevel() : 0, roles > 0);
    };

    if (!roleCheck.roles.empty())
    {
        // Leader info MUST be sent 1st :S
        addMember(roleCheck.leader, roleCheck.roles.find(roleCheck.leader)->second);
        for (auto const& [guid, roles] : roleCheck.roles)
            if (guid != roleCheck.leader)
                addMember(guid, roles);
    }

    SendPacket(lfgRoleCheckUpdate.Write());
}

void WorldSession::SendLfgJoinResult(lfg::LfgJoinResultData const& joinData)
{
    LOG_DEBUG("network", "SMSG_LFG_JOIN_RESULT [{}] checkResult: {} checkValue: {}", GetPlayer()->GetGUID().ToString(), joinData.result, joinData.state);

    WorldPackets::LFG::LFGJoinResult lfgJoinResult;
    lfgJoinResult.Ticket = MakeLfgTicket(GetPlayer());
    lfgJoinResult.Result = joinData.result;
    if (joinData.result == lfg::LFG_JOIN_ROLE_CHECK_FAILED)
        lfgJoinResult.ResultDetail = joinData.state;

    for (auto const& [guid, locks] : joinData.lockmap)
    {
        WorldPackets::LFG::LFGJoinBlackList& blackList = lfgJoinResult.BlackList.emplace_back();
        blackList.Guid = guid;
        for (auto const& [dungeon, lockStatus] : locks)
            blackList.Slots.emplace_back(dungeon, lockStatus, 0, 0);
    }

    SendPacket(lfgJoinResult.Write());
}

void WorldSession::SendLfgQueueStatus(lfg::LfgQueueStatusData const& queueData)
{
    LOG_DEBUG("network", "SMSG_LFG_QUEUE_STATUS [{}] dungeon: {} - waitTime: {} - avgWaitTime: {} - waitTimeTanks: {} - waitTimeHealer: {} - waitTimeDps: {} - queuedTime: {} - tanks: {} - healers: {} - dps: {}",
        GetPlayer()->GetGUID().ToString(), queueData.dungeonId, queueData.waitTime, queueData.waitTimeAvg, queueData.waitTimeTank, queueData.waitTimeHealer, queueData.waitTimeDps, queueData.queuedTime, queueData.tanks, queueData.healers, queueData.dps);

    WorldPackets::LFG::LFGQueueStatus lfgQueueStatus;
    lfgQueueStatus.Ticket = MakeLfgTicket(GetPlayer());
    lfgQueueStatus.Slot = sLFGMgr->GetLFGDungeonEntry(queueData.dungeonId);
    lfgQueueStatus.AvgWaitTimeMe = queueData.waitTime;
    lfgQueueStatus.AvgWaitTime = queueData.waitTimeAvg;
    lfgQueueStatus.AvgWaitTimeByRole[0] = queueData.waitTimeTank;
    lfgQueueStatus.AvgWaitTimeByRole[1] = queueData.waitTimeHealer;
    lfgQueueStatus.AvgWaitTimeByRole[2] = queueData.waitTimeDps;
    lfgQueueStatus.LastNeeded[0] = queueData.tanks;
    lfgQueueStatus.LastNeeded[1] = queueData.healers;
    lfgQueueStatus.LastNeeded[2] = queueData.dps;
    lfgQueueStatus.QueuedTime = queueData.queuedTime;
    SendPacket(lfgQueueStatus.Write());
}

void WorldSession::SendLfgPlayerReward(lfg::LfgPlayerRewardData const& rewardData)
{
    if (!rewardData.rdungeonEntry || !rewardData.sdungeonEntry || !rewardData.quest)
        return;

    LOG_DEBUG("lfg", "SMSG_LFG_PLAYER_REWARD {} rdungeonEntry: {}, sdungeonEntry: {}, done: {}",
                   GetPlayerInfo(), rewardData.rdungeonEntry, rewardData.sdungeonEntry, rewardData.done);

    uint8 playerLevel = GetPlayer()->GetLevel();
    uint8 playerLevelForXP = playerLevel;
    sScriptMgr->OnPlayerBeforeGetLevelForXPGain(GetPlayer(), playerLevelForXP);

    WorldPackets::LFG::LFGPlayerReward lfgPlayerReward;
    lfgPlayerReward.QueuedSlot = rewardData.rdungeonEntry;
    lfgPlayerReward.ActualSlot = rewardData.sdungeonEntry;
    lfgPlayerReward.RewardMoney = rewardData.quest->GetRewOrReqMoney(playerLevel);
    lfgPlayerReward.AddedXP = rewardData.quest->XPValue(playerLevelForXP);

    for (uint8 i = 0; i < QUEST_REWARDS_COUNT; ++i)
        if (uint32 itemId = rewardData.quest->RewardItemId[i])
            lfgPlayerReward.Rewards.emplace_back(itemId, rewardData.quest->RewardItemIdCount[i], false);

    for (uint32 i = 0; i < QUEST_REWARD_CURRENCY_COUNT; ++i)
        if (uint32 currencyId = rewardData.quest->RewardCurrencyId[i])
            if (CurrencyTypesEntry const* currency = sCurrencyTypesStore.LookupEntry(currencyId))
                lfgPlayerReward.Rewards.emplace_back(currencyId, rewardData.quest->RewardCurrencyCount[i] * (currency->UsesPrecision() ? CURRENCY_PRECISION : 1), true);

    SendPacket(lfgPlayerReward.Write());
}

void WorldSession::SendLfgBootProposalUpdate(lfg::LfgPlayerBoot const& boot)
{
    ObjectGuid guid = GetPlayer()->GetGUID();
    lfg::LfgAnswer playerVote = boot.votes.find(guid)->second;
    uint8 votesNum = 0;
    uint8 agreeNum = 0;
    int32 secsleft = int32(boot.cancelTime - GameTime::GetGameTime().count());
    for (auto const& [voter, vote] : boot.votes)
    {
        if (vote != lfg::LFG_ANSWER_PENDING)
        {
            ++votesNum;
            if (vote == lfg::LFG_ANSWER_AGREE)
                ++agreeNum;
        }
    }

    LOG_DEBUG("network", "SMSG_LFG_BOOT_PROPOSAL_UPDATE [{}] inProgress: {} - didVote: {} - agree: {} - victim: [{}] votes: {} - agrees: {} - left: {} - needed: {} - reason {}",
        guid.ToString(), uint8(boot.inProgress), uint8(playerVote != lfg::LFG_ANSWER_PENDING), uint8(playerVote == lfg::LFG_ANSWER_AGREE),
        boot.victim.ToString(), votesNum, agreeNum, secsleft, lfg::LFG_GROUP_KICK_VOTES_NEEDED, boot.reason);

    WorldPackets::LFG::LfgBootPlayer lfgBootPlayer;
    lfgBootPlayer.Info.VoteInProgress = boot.inProgress;
    lfgBootPlayer.Info.VotePassed = agreeNum >= lfg::LFG_GROUP_KICK_VOTES_NEEDED;
    lfgBootPlayer.Info.MyVoteCompleted = playerVote != lfg::LFG_ANSWER_PENDING;
    lfgBootPlayer.Info.MyVote = playerVote == lfg::LFG_ANSWER_AGREE;
    lfgBootPlayer.Info.Target = boot.victim;
    lfgBootPlayer.Info.TotalVotes = votesNum;
    lfgBootPlayer.Info.BootVotes = agreeNum;
    lfgBootPlayer.Info.TimeLeft = secsleft;
    lfgBootPlayer.Info.VotesNeeded = lfg::LFG_GROUP_KICK_VOTES_NEEDED;
    lfgBootPlayer.Info.Reason = boot.reason;
    SendPacket(lfgBootPlayer.Write());
}

void WorldSession::SendLfgUpdateProposal(lfg::LfgProposal const& proposal)
{
    ObjectGuid guid = GetPlayer()->GetGUID();
    ObjectGuid gguid = proposal.players.find(guid)->second.group;
    bool silent = !proposal.isNew && gguid == proposal.group;
    uint32 dungeonEntry = proposal.dungeonId;

    LOG_DEBUG("network", "SMSG_LFG_PROPOSAL_UPDATE [{} state: {}", guid.ToString(), proposal.state);

    // show random dungeon if player selected random dungeon and it's not lfg group
    if (!silent)
    {
        lfg::LfgDungeonSet const& playerDungeons = sLFGMgr->GetSelectedDungeons(guid);
        if (playerDungeons.find(proposal.dungeonId) == playerDungeons.end())
            dungeonEntry = (*playerDungeons.begin());
    }

    WorldPackets::LFG::LFGProposalUpdate lfgProposalUpdate;
    lfgProposalUpdate.Ticket = MakeLfgTicket(GetPlayer());
    lfgProposalUpdate.ProposalID = proposal.id;
    lfgProposalUpdate.Slot = sLFGMgr->GetLFGDungeonEntry(dungeonEntry);
    lfgProposalUpdate.State = proposal.state;
    lfgProposalUpdate.CompletedMask = proposal.encounters;
    lfgProposalUpdate.ProposalSilent = silent;

    // Sort by roles: tank, healer, dps
    std::vector<ObjectGuid> tanks, healers, dps;
    for (auto const& [pguid, player] : proposal.players)
    {
        if (player.role & lfg::PLAYER_ROLE_TANK)
            tanks.push_back(pguid);
        else if (player.role & lfg::PLAYER_ROLE_HEALER)
            healers.push_back(pguid);
        else
            dps.push_back(pguid);
    }

    std::vector<ObjectGuid> ordered;
    ordered.reserve(proposal.players.size());
    ordered.insert(ordered.end(), tanks.begin(), tanks.end());
    ordered.insert(ordered.end(), healers.begin(), healers.end());
    ordered.insert(ordered.end(), dps.begin(), dps.end());

    for (ObjectGuid const& pguid : ordered)
    {
        lfg::LfgProposalPlayer const& player = proposal.players.find(pguid)->second;
        WorldPackets::LFG::LFGProposalUpdatePlayer& proposalPlayer = lfgProposalUpdate.Players.emplace_back();
        proposalPlayer.Roles = player.role;
        proposalPlayer.Me = pguid == guid;
        proposalPlayer.MyParty = player.group && player.group == proposal.group;
        proposalPlayer.SameParty = player.group && player.group == gguid;
        proposalPlayer.Responded = player.accept != lfg::LFG_ANSWER_PENDING;
        proposalPlayer.Accepted = player.accept == lfg::LFG_ANSWER_AGREE;
    }

    SendPacket(lfgProposalUpdate.Write());
}

void WorldSession::SendLfgLfrList(bool update)
{
    // 4.3.4 has no raid browser search flag packet
    LOG_DEBUG("network", "SMSG_LFG_LFR_LIST [{}] update: {}", GetPlayer()->GetGUID().ToString(), update ? 1 : 0);
}

void WorldSession::SendLfgDisabled()
{
    LOG_DEBUG("network", "SMSG_LFG_DISABLED [{}]", GetPlayer()->GetGUID().ToString());
    SendPacket(WorldPackets::LFG::LFGDisabled().Write());
}

void WorldSession::SendLfgOfferContinue(uint32 dungeonEntry)
{
    LOG_DEBUG("network", "SMSG_LFG_OFFER_CONTINUE [{}] dungeon entry: {}", GetPlayer()->GetGUID().ToString(), dungeonEntry);
    SendPacket(WorldPackets::LFG::LFGOfferContinue(dungeonEntry).Write());
}

void WorldSession::SendLfgTeleportError(uint8 err)
{
    LOG_DEBUG("network", "SMSG_LFG_TELEPORT_DENIED [{}] reason: {}", GetPlayer()->GetGUID().ToString(), err);
    SendPacket(WorldPackets::LFG::LFGTeleportDenied(err).Write());
}
