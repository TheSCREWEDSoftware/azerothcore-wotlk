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

#include "ArchaeologyPackets.h"

ByteBuffer& operator<<(ByteBuffer& data, WorldPackets::Archaeology::ResearchHistory const& researchHistory)
{
    data << int32(researchHistory.ProjectID);
    data << uint32(researchHistory.FirstCompleted);
    data << int32(researchHistory.CompletionCount);
    return data;
}

WorldPacket const* WorldPackets::Archaeology::SetupResearchHistory::Write()
{
    _worldPacket << int32(History.size());
    for (WorldPackets::Archaeology::ResearchHistory const& history : History)
        _worldPacket << history;

    return &_worldPacket;
}

WorldPacket const* WorldPackets::Archaeology::ResearchComplete::Write()
{
    _worldPacket << Research;
    return &_worldPacket;
}

WorldPacket const* WorldPackets::Archaeology::ArchaeologySurveyCast::Write()
{
    _worldPacket << uint32(TotalFinds);
    _worldPacket << uint32(NumFindsCompleted);
    _worldPacket << int32(ResearchBranchID);
    _worldPacket.WriteBit(SuccessfulFind);
    _worldPacket.FlushBits();
    return &_worldPacket;
}
