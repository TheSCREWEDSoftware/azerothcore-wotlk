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

#ifndef ArchaeologyPackets_h__
#define ArchaeologyPackets_h__

#include "Packet.h"

// 6.x 19342: TC 6.x does not implement archaeology, layouts from WowPacketParser V6_0_2 (ArchaelogyHandler)
namespace WorldPackets
{
    namespace Archaeology
    {
        class RequestResearchHistory final : public ClientPacket
        {
        public:
            RequestResearchHistory(WorldPacket&& packet) : ClientPacket(CMSG_REQUEST_RESEARCH_HISTORY, std::move(packet)) { }

            void Read() override { }
        };

        struct ResearchHistory
        {
            ResearchHistory() { }
            ResearchHistory(int32 projectId, int32 completionCount, uint32 firstCompleted) :
                ProjectID(projectId), CompletionCount(completionCount), FirstCompleted(firstCompleted) { }

            int32 ProjectID = 0;
            int32 CompletionCount = 0;
            uint32 FirstCompleted = 0;
        };

        class SetupResearchHistory final : public ServerPacket
        {
        public:
            SetupResearchHistory() : ServerPacket(SMSG_SETUP_RESEARCH_HISTORY, 4) { }

            WorldPacket const* Write() override;

            std::vector<ResearchHistory> History;
        };

        class ResearchComplete final : public ServerPacket
        {
        public:
            ResearchComplete() : ServerPacket(SMSG_RESEARCH_COMPLETE, 12) { }

            WorldPacket const* Write() override;

            ResearchHistory Research;
        };

        class ArchaeologySurveyCast final : public ServerPacket
        {
        public:
            ArchaeologySurveyCast() : ServerPacket(SMSG_ARCHAEOLOGY_SURVERY_CAST, 4 + 4 + 4 + 1) { }

            WorldPacket const* Write() override;

            uint32 TotalFinds = 0;
            uint32 NumFindsCompleted = 0;
            int32 ResearchBranchID = 0;
            bool SuccessfulFind = false;
        };
    }
}

#endif // ArchaeologyPackets_h__
