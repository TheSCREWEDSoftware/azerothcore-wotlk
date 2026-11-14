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

#include "Log.h"
#include "ObjectAccessor.h"
#include "Opcodes.h"
#include "Player.h"
#include "Vehicle.h"
#include "VehiclePackets.h"
#include "WorldPacket.h"
#include "WorldSession.h"

namespace
{
    // the seat the player sits on allows switching seats
    bool CanSwitchSeat(Player* player, char const* handler)
    {
        if (!player->GetVehicleBase())
            return false;

        VehicleSeatEntry const* seat = player->GetVehicle()->GetSeatForPassenger(player);
        if (!seat || !seat->CanSwitchFromSeat())
        {
            LOG_ERROR("network.opcode", "{}: Player {} tried to switch seats but current seatflags {} don't permit that.",
                handler, player->GetGUID().ToString(), seat ? seat->Flags : 0);
            return false;
        }

        return true;
    }
}

void WorldSession::HandleMoveDismissVehicle(WorldPackets::Vehicle::MoveDismissVehicle& moveDismissVehicle)
{
    if (!_player->GetCharmGUID())                           // something wrong here...
        return;

    MovementInfo& mi = moveDismissVehicle.Status;

    // pussywizard: typical check for incomming movement packets
    if (!_player->m_mover || !_player->m_mover->IsInWorld() || _player->m_mover->IsDuringRemoveFromWorld() || mi.guid != _player->m_mover->GetGUID())
    {
        _player->ExitVehicle();
        return;
    }

    if (_player->m_mover->IsRooted()) // for some reason client sends it without it even if rooted
        mi.AddMovementFlag(MOVEMENTFLAG_ROOT);

    ProcessMovementInfo(mi, _player->m_mover, _player->m_mover->ToPlayer(), CMSG_MOVE_DISMISS_VEHICLE);

    _player->ExitVehicle();
}

void WorldSession::HandleRequestVehiclePrevSeat(WorldPackets::Vehicle::RequestVehiclePrevSeat& /*requestVehiclePrevSeat*/)
{
    if (CanSwitchSeat(GetPlayer(), "HandleRequestVehiclePrevSeat"))
        GetPlayer()->ChangeSeat(-1, false);
}

void WorldSession::HandleRequestVehicleNextSeat(WorldPackets::Vehicle::RequestVehicleNextSeat& /*requestVehicleNextSeat*/)
{
    if (CanSwitchSeat(GetPlayer(), "HandleRequestVehicleNextSeat"))
        GetPlayer()->ChangeSeat(-1, true);
}

void WorldSession::HandleMoveChangeVehicleSeats(WorldPackets::Vehicle::MoveChangeVehicleSeats& moveChangeVehicleSeats)
{
    if (!CanSwitchSeat(GetPlayer(), "HandleMoveChangeVehicleSeats"))
        return;

    Unit* vehicle_base = GetPlayer()->GetVehicleBase();

    // pussywizard:
    if (vehicle_base->GetGUID() != moveChangeVehicleSeats.Status.guid)
        return;

    vehicle_base->m_movementInfo = moveChangeVehicleSeats.Status;

    if (moveChangeVehicleSeats.DstVehicle.IsEmpty())
        GetPlayer()->ChangeSeat(-1, moveChangeVehicleSeats.DstSeatIndex != 255); // prev/next
    else if (Unit* vehUnit = ObjectAccessor::GetUnit(*GetPlayer(), moveChangeVehicleSeats.DstVehicle))
        if (Vehicle* vehicle = vehUnit->GetVehicleKit())
            if (vehicle->HasEmptySeat(int8(moveChangeVehicleSeats.DstSeatIndex)))
                vehUnit->HandleSpellClick(GetPlayer(), int8(moveChangeVehicleSeats.DstSeatIndex));
}

void WorldSession::HandleRequestVehicleSwitchSeat(WorldPackets::Vehicle::RequestVehicleSwitchSeat& requestVehicleSwitchSeat)
{
    if (!CanSwitchSeat(GetPlayer(), "HandleRequestVehicleSwitchSeat"))
        return;

    Unit* vehicle_base = GetPlayer()->GetVehicleBase();
    int8 seatId = int8(requestVehicleSwitchSeat.SeatIndex);
    if (vehicle_base->GetGUID() == requestVehicleSwitchSeat.Vehicle)
        GetPlayer()->ChangeSeat(seatId);
    else if (Unit* vehUnit = ObjectAccessor::GetUnit(*GetPlayer(), requestVehicleSwitchSeat.Vehicle))
        if (Vehicle* vehicle = vehUnit->GetVehicleKit())
            if (vehicle->HasEmptySeat(seatId))
                vehUnit->HandleSpellClick(GetPlayer(), seatId);
}

void WorldSession::HandleRideVehicleInteract(WorldPackets::Vehicle::RideVehicleInteract& rideVehicleInteract)
{
    if (Player* player = ObjectAccessor::GetPlayer(*_player, rideVehicleInteract.Vehicle))
    {
        Vehicle* vehicle = player->GetVehicleKit();
        if (!vehicle || !vehicle->IsBoardableByPlayers())
            return;
        if (!player->IsInRaidWith(_player))
            return;
        if (!player->IsWithinDistInMap(_player, INTERACTION_DISTANCE))
            return;
        // Xinef:
        if (!_player->FindMap() || _player->FindMap()->IsBattleArena())
            return;

        _player->EnterVehicle(player);
    }
}

void WorldSession::HandleEjectPassenger(WorldPackets::Vehicle::EjectPassenger& ejectPassenger)
{
    Vehicle* vehicle = _player->GetVehicleKit();
    if (!vehicle)
    {
        LOG_ERROR("network.opcode", "HandleEjectPassenger: Player {} is not in a vehicle!", GetPlayer()->GetGUID().ToString());
        return;
    }

    ObjectGuid guid = ejectPassenger.Passenger;
    if (!guid.IsUnit())
    {
        LOG_ERROR("network.opcode", "HandleEjectPassenger: Player {} tried to eject invalid {}", GetPlayer()->GetGUID().ToString(), guid.ToString());
        return;
    }

    Unit* unit = ObjectAccessor::GetUnit(*_player, guid);
    if (!unit) // creatures can be ejected too from player mounts
    {
        LOG_ERROR("network.opcode", "Player {} tried to eject {} from vehicle, but the latter was not found in world!", GetPlayer()->GetGUID().ToString(), guid.ToString());
        return;
    }

    if (!unit->IsOnVehicle(vehicle->GetBase()))
    {
        LOG_ERROR("network.opcode", "Player {} tried to eject {}, but they are not in the same vehicle", GetPlayer()->GetGUID().ToString(), guid.ToString());
        return;
    }

    VehicleSeatEntry const* seat = vehicle->GetSeatForPassenger(unit);
    ASSERT(seat);
    if (!seat->IsEjectable())
    {
        LOG_ERROR("network.opcode", "Player {} attempted to eject {} from non-ejectable seat.", GetPlayer()->GetGUID().ToString(), guid.ToString());
        return;
    }

    if (!unit->IsPlayer())
        ASSERT(GetPlayer() == vehicle->GetBase());

    unit->ExitVehicle();
}

void WorldSession::HandleRequestVehicleExit(WorldPackets::Vehicle::RequestVehicleExit& /*requestVehicleExit*/)
{
    if (Vehicle* vehicle = GetPlayer()->GetVehicle())
    {
        if (VehicleSeatEntry const* seat = vehicle->GetSeatForPassenger(GetPlayer()))
        {
            if (seat->CanEnterOrExit())
                GetPlayer()->ExitVehicle();
            else
                LOG_ERROR("network.opcode", "Player {} tried to exit vehicle, but seatflags {} (ID: {}) don't permit that.",
                    GetPlayer()->GetGUID().ToString(), seat->Flags, seat->ID);
        }
    }
}

void WorldSession::HandleMoveSetVehicleRecAck(WorldPackets::Vehicle::MoveSetVehicleRecIdAck& setVehicleRecIdAck)
{
    // the client acknowledged the vehicle id change: keep its movement state
    MovementInfo& movementInfo = setVehicleRecIdAck.Data.movementInfo;
    if (_player->m_mover && movementInfo.guid == _player->m_mover->GetGUID())
        _player->m_mover->m_movementInfo = movementInfo;
}
