/*
 * This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Affero General Public License as published by the
 * Free Software Foundation; either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "SpellCastRequest.h"
#include "Object.h"
#include "Spell.h"
#include "SpellPackets.h"

// The cast data of the 6.x CMSG_CAST_SPELL, CMSG_USE_ITEM and CMSG_PET_CAST_SPELL (TrinityCore 6.x SpellCastRequest)

void SpellCastRequest::FillTargets(SpellCastTargets& targets, WorldObject* caster) const
{
    // a location the client did not send is the caster's, like SpellCastTargets::Read does
    ObjectGuid casterTransport = caster->GetTransGUID();
    Position casterPos = casterTransport
        ? Position(caster->GetTransOffsetX(), caster->GetTransOffsetY(), caster->GetTransOffsetZ(), caster->GetTransOffsetO())
        : Position(caster->GetPositionX(), caster->GetPositionY(), caster->GetPositionZ(), caster->GetOrientation());

    bool hasSrc = targetMask & TARGET_FLAG_SOURCE_LOCATION;
    bool hasDst = targetMask & TARGET_FLAG_DEST_LOCATION;
    targets.Initialize(caster, targetMask, targetGuid, itemTargetGuid,
        hasSrc ? srcTransportGuid : casterTransport, hasSrc ? srcPos : casterPos,
        hasDst ? destTransportGuid : casterTransport, hasDst ? destPos : casterPos, targetString);
    targets.SetElevation(elevation);
    targets.SetSpeed(missileSpeed);
}

SpellCastRequest SpellCastRequest::FromPacket(WorldPackets::Spells::SpellCastRequest const& cast)
{
    SpellCastRequest request;
    request.castCount = cast.CastID;
    request.castFlags = cast.SendCastFlags;
    request.spellId = uint32(cast.SpellID);
    request.glyphIndex = uint32(cast.Misc[0]);
    request.targetMask = cast.Target.Flags;
    request.elevation = cast.MissileTrajectory.Pitch;
    request.missileSpeed = cast.MissileTrajectory.Speed;
    request.targetGuid = cast.Target.Unit;
    request.itemTargetGuid = cast.Target.Item;
    request.targetString = cast.Target.Name;

    if (cast.Target.SrcLocation)
    {
        request.srcTransportGuid = cast.Target.SrcLocation->Transport;
        request.srcPos.Relocate(cast.Target.SrcLocation->Location);
    }

    if (cast.Target.DstLocation)
    {
        request.destTransportGuid = cast.Target.DstLocation->Transport;
        request.destPos.Relocate(cast.Target.DstLocation->Location);
    }

    if (cast.Target.Orientation)
        request.destPos.SetOrientation(*cast.Target.Orientation);

    // archaeology: keystones (type 1, item) and fragments (type 2, currency) used for a project
    for (WorldPackets::Spells::SpellWeight const& weight : cast.Weight)
    {
        switch (weight.Type)
        {
            case 1:
                request.researchData.KeyId = uint32(weight.ID);
                request.researchData.KeyCount = weight.Quantity;
                request.hasResearchData = true;
                break;
            case 2:
                request.researchData.FragId = uint32(weight.ID);
                request.researchData.FragCount = weight.Quantity;
                request.hasResearchData = true;
                break;
            default:
                break;
        }
    }

    return request;
}
