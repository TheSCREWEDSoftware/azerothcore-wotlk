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

#ifndef BattlegroundPackets_h__
#define BattlegroundPackets_h__

#include "ObjectGuid.h"
#include "Packet.h"
#include <array>
#include <vector>

namespace WorldPackets
{
    namespace Battleground
    {
        class RequestRatedBgInfo final : public ClientPacket
        {
        public:
            RequestRatedBgInfo(WorldPacket&& packet) : ClientPacket(CMSG_REQUEST_RATED_BG_INFO, std::move(packet)) { }

            void Read() override;

            uint8 Mode = 0; // 0 = arena 2v2, 1 = arena 3v3, 2 = arena 5v5, 3 = rated battleground
        };

        class PVPOptionsEnabled final : public ServerPacket
        {
        public:
            PVPOptionsEnabled() : ServerPacket(SMSG_PVP_OPTIONS_ENABLED, 1) { }

            WorldPacket const* Write() override;

            bool RatedArenas = false;
            bool WargameBattlegrounds = false;
            bool RatedBattlegrounds = false;
        };

        struct ClientBracketInfo
        {
            int32 WeeklyPlayed = 0;
            int32 WeeklyWon = 0;
            int32 SeasonPlayed = 0;
            int32 SeasonWon = 0;
            int32 TotalPlayed = 0;
            int32 TotalWon = 0;
        };

        class RatedBattlefieldInfo final : public ServerPacket
        {
        public:
            RatedBattlefieldInfo() : ServerPacket(SMSG_RATED_BG_STATS, 72) { }

            WorldPacket const* Write() override;

            std::array<ClientBracketInfo, 3> Brackets; // 10v10, 15v15, 25v25
        };

        class BattlefieldRatedInfo final : public ServerPacket
        {
        public:
            BattlefieldRatedInfo() : ServerPacket(SMSG_BATTLEFIELD_RATED_INFO, 29) { }

            WorldPacket const* Write() override;

            int32 Reward = 0;
            int32 PersonalRating = 0;
            int32 RatedMaxRewardPointsThisWeek = 0;
            int32 MaxRewardPointsThisWeek = 0;
            int32 RewardPointsThisWeek = 0;
            int32 RatedRewardPointsThisWeek = 0;
            int8 Mode = 0;
        };

        class BattlefieldList final : public ServerPacket
        {
        public:
            BattlefieldList() : ServerPacket(SMSG_BATTLEFIELD_LIST, 1 + 7 * 4 + 2 + 3 + 8) { }

            WorldPacket const* Write() override;

            ObjectGuid BattlemasterGuid;
            int32 BattlemasterListID = 0;
            uint8 MinLevel = 0;
            uint8 MaxLevel = 0;
            std::vector<int32> Battlefields;
            bool PvpAnywhere = false;
            bool HasRandomWinToday = false;
            bool HasHolidayWinToday = false;
            bool IsRandomBG = false;
            int32 HonorBonusRandomWin = 0;
            int32 HonorBonusRandomLoss = 0;
            int32 HonorBonusHolidayWin = 0;
            int32 HonorBonusHolidayLoss = 0;
            int32 ConquestBonusRandom = 0;
            int32 ConquestBonusHoliday = 0;
        };

        class RequestPVPRewardsResponse final : public ServerPacket
        {
        public:
            RequestPVPRewardsResponse() : ServerPacket(SMSG_REQUEST_PVP_REWARDS_RESPONSE, 24) { }

            WorldPacket const* Write() override;

            int32 RatedRewardPointsThisWeek = 0;
            int32 ArenaRewardPointsThisWeek = 0;
            int32 RewardPointsThisWeek = 0;
            int32 RatedMaxRewardPointsThisWeek = 0;
            int32 ArenaMaxRewardPointsThisWeek = 0;
            int32 MaxRewardPointsThisWeek = 0;
        };
    }
}

#endif // BattlegroundPackets_h__
