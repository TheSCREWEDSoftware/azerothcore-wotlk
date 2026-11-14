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

#include "GameTime.h"
#include "MovementPackets.h"
#include "ObjectMgr.h"
#include "Opcodes.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "TaxiPackets.h"
#include "TaxiPathGraph.h"
#include "WaypointMovementGenerator.h"
#include "WorldPacket.h"
#include "WorldSession.h"

void WorldSession::HandleEnableTaxiNodeOpcode(WorldPackets::Taxi::EnableTaxiNode& enableTaxiNode)
{
    if (Creature* unit = GetPlayer()->GetNPCIfCanInteractWith(enableTaxiNode.Unit, UNIT_NPC_FLAG_FLIGHTMASTER))
        SendLearnNewTaxiNode(unit);
}

void WorldSession::HandleTaxiNodeStatusQueryOpcode(WorldPackets::Taxi::TaxiNodeStatusQuery& taxiNodeStatusQuery)
{
    SendTaxiStatus(taxiNodeStatusQuery.UnitGUID);
}

void WorldSession::SendTaxiStatus(ObjectGuid guid)
{
    Player* const player = GetPlayer();
    Creature* unit = ObjectAccessor::GetCreature(*player, guid);
    if (!unit || !unit->HasNpcFlag(UNIT_NPC_FLAG_FLIGHTMASTER))
    {
        LOG_DEBUG("network", "WorldSession::SendTaxiStatus - Unit ({}) not found.", guid.ToString());
        return;
    }

    uint32 nearest = sObjectMgr->GetNearestTaxiNode(*unit, player->GetTeamId(true));

    WorldPackets::Taxi::TaxiNodeStatus data;
    data.Unit = guid;

    // reaction must be checked both ways: the Dark Portal flight masters are neutral toward opposite-faction players, while players are hostile toward them
    if (!nearest)
        data.Status = TAXISTATUS_NONE;
    else if (unit->GetReactionTo(player) >= REP_NEUTRAL && !player->IsHostileTo(unit))
        data.Status = player->m_taxi.IsTaximaskNodeKnown(nearest) ? TAXISTATUS_LEARNED : TAXISTATUS_UNLEARNED;
    else
        data.Status = TAXISTATUS_NOT_ELIGIBLE;

    SendPacket(data.Write());
}

void WorldSession::HandleTaxiQueryAvailableNodesOpcode(WorldPackets::Taxi::TaxiQueryAvailableNodes& taxiQueryAvailableNodes)
{
    // cheating checks
    Creature* unit = GetPlayer()->GetNPCIfCanInteractWith(taxiQueryAvailableNodes.Unit, UNIT_NPC_FLAG_FLIGHTMASTER);
    if (!unit)
    {
        LOG_DEBUG("network", "WORLD: HandleTaxiQueryAvailableNodes - Unit ({}) not found or you can't interact with him.", taxiQueryAvailableNodes.Unit.ToString());
        return;
    }

    // remove fake death
    if (GetPlayer()->HasUnitState(UNIT_STATE_DIED))
        GetPlayer()->RemoveAurasByType(SPELL_AURA_FEIGN_DEATH);

    // unknown taxi node case
    if (SendLearnNewTaxiNode(unit))
        return;

    // known taxi node case
    SendTaxiMenu(unit);
}

void WorldSession::SendTaxiMenu(Creature* unit)
{
    // find current node
    uint32 curloc = sObjectMgr->GetNearestTaxiNode(*unit, GetPlayer()->GetTeamId(true));
    if (!curloc)
        return;

    bool lastTaxiCheaterState = GetPlayer()->isTaxiCheater();
    if (unit->GetEntry() == 29480)
        GetPlayer()->SetTaxiCheater(true); // Grimwing in Ebon Hold, special case. NOTE: Not perfect, Zul'Aman should not be included according to WoWhead, and I think taxicheat includes it.

    WorldPackets::Taxi::ShowTaxiNodes data;
    data.WindowInfo.emplace();
    data.WindowInfo->UnitGUID = unit->GetGUID();
    data.WindowInfo->CurrentNode = curloc;

    GetPlayer()->m_taxi.AppendTaximaskTo(data, GetPlayer()->isTaxiCheater());
    SendPacket(data.Write());

    GetPlayer()->SetTaxiCheater(lastTaxiCheaterState);
}

void WorldSession::SendDoFlight(uint32 mountDisplayId, uint32 path, uint32 pathNode)
{
    // remove fake death
    if (GetPlayer()->HasUnitState(UNIT_STATE_DIED))
        GetPlayer()->RemoveAurasByType(SPELL_AURA_FEIGN_DEATH);

    while (GetPlayer()->GetMotionMaster()->GetCurrentMovementGeneratorType() == FLIGHT_MOTION_TYPE)
        GetPlayer()->GetMotionMaster()->MovementExpired(false);

    if (mountDisplayId)
        GetPlayer()->Mount(mountDisplayId);

    if (Creature* critter = ObjectAccessor::GetCreature(*GetPlayer(), GetPlayer()->GetCritterGUID()))
        critter->DespawnOrUnsummon();

    GetPlayer()->GetMotionMaster()->MoveTaxiFlight(path, pathNode);
}

bool WorldSession::SendLearnNewTaxiNode(Creature* unit)
{
    // find current node
    uint32 curloc = sObjectMgr->GetNearestTaxiNode(*unit, GetPlayer()->GetTeamId(true));
    if (curloc == 0)
        return true;                                        // `true` send to avoid WorldSession::SendTaxiMenu call with one more curlock seartch with same false result.

    if (GetPlayer()->m_taxi.SetTaximaskNode(curloc))
    {
        SendPacket(WorldPackets::Taxi::NewTaxiPath().Write());

        WorldPackets::Taxi::TaxiNodeStatus data;
        data.Unit = unit->GetGUID();
        data.Status = TAXISTATUS_LEARNED;
        SendPacket(data.Write());

        sScriptMgr->OnPlayerLearnTaxiNode(GetPlayer(), curloc);
        return true;
    }

    return false;
}

void WorldSession::SendDiscoverNewTaxiNode(uint32 nodeid)
{
    if (GetPlayer()->m_taxi.SetTaximaskNode(nodeid))
    {
        SendPacket(WorldPackets::Taxi::NewTaxiPath().Write());
        sScriptMgr->OnPlayerLearnTaxiNode(GetPlayer(), nodeid);
    }
}

void WorldSession::HandleActivateTaxiOpcode(WorldPackets::Taxi::ActivateTaxi& activateTaxi)
{
    GetPlayer()->SetCanTeleport(true);

    Creature* unit = GetPlayer()->GetNPCIfCanInteractWith(activateTaxi.Vendor, UNIT_NPC_FLAG_FLIGHTMASTER);
    if (!unit)
    {
        LOG_DEBUG("network", "WORLD: HandleActivateTaxiOpcode - Unit ({}) not found or you can't interact with it.", activateTaxi.Vendor.ToString());
        SendActivateTaxiReply(ERR_TAXITOOFARAWAY);
        return;
    }

    uint32 curloc = sObjectMgr->GetNearestTaxiNode(*unit, GetPlayer()->GetTeamId(true));
    if (!curloc)
        return;

    TaxiNodesEntry const* from = sTaxiNodesStore.LookupEntry(curloc);
    TaxiNodesEntry const* to = sTaxiNodesStore.LookupEntry(activateTaxi.Node);
    if (!from || !to)
        return;

    if (!GetPlayer()->isTaxiCheater())
    {
        if (!GetPlayer()->m_taxi.IsTaximaskNodeKnown(curloc) || !GetPlayer()->m_taxi.IsTaximaskNodeKnown(activateTaxi.Node))
        {
            SendActivateTaxiReply(ERR_TAXINOTVISITED);
            return;
        }
    }

    // 6.x: the client sends only the destination; the route goes through the known nodes
    std::vector<uint32> nodes;
    sTaxiPathGraph.GetCompleteNodeRoute(from, to, GetPlayer(), nodes);
    if (nodes.size() < 2)
    {
        SendActivateTaxiReply(ERR_TAXINOSUCHPATH);
        return;
    }

    LOG_DEBUG("network", "WORLD: Received CMSG_ACTIVATE_TAXI from {} to {}", nodes.front(), nodes.back());
    GetPlayer()->ActivateTaxiPathTo(nodes, unit, 0);
}

void WorldSession::SendActivateTaxiReply(ActivateTaxiReply reply)
{
    GetPlayer()->SetCanTeleport(true);

    WorldPackets::Taxi::ActivateTaxiReply data;
    data.Reply = reply;
    SendPacket(data.Write());
}

void WorldSession::HandleTaxiRequestEarlyLanding(WorldPackets::Taxi::TaxiRequestEarlyLanding& /*taxiRequestEarlyLanding*/)
{
    if (GetPlayer()->GetMotionMaster()->GetCurrentMovementGeneratorType() != FLIGHT_MOTION_TYPE)
        return;

    if (!GetPlayer()->m_taxi.RequestEarlyLanding())
        return;

    if (FlightPathMovementGenerator* flight = dynamic_cast<FlightPathMovementGenerator*>(GetPlayer()->GetMotionMaster()->top()))
    {
        flight->LoadPath(GetPlayer());           // the current node keeps its index: the path only lost its end
        flight->DoReset(GetPlayer());
    }
}

void WorldSession::HandleMoveSplineDoneOpcode(WorldPackets::Movement::MoveSplineDone& moveSplineDone)
{
    MovementInfo& movementInfo = moveSplineDone.movementInfo;
    Unit* mover = _player->m_mover;

    if (_player->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_TAXI_FLIGHT)) // taxi spline case
    {
        // in taxi flight packet received in 2 case:
        // 1) end taxi path in far (multi-node) flight
        // 2) switch from one map to other in case multim-map taxi path
        // we need process only (1)

        uint32 curDest = GetPlayer()->m_taxi.GetTaxiDestination();
        if (curDest)
        {
            TaxiNodesEntry const* curDestNode = sTaxiNodesStore.LookupEntry(curDest);

            // far teleport case
            if (curDestNode && curDestNode->MapID != GetPlayer()->GetMapId() && GetPlayer()->GetMotionMaster()->GetCurrentMovementGeneratorType() == FLIGHT_MOTION_TYPE)
            {
                if (FlightPathMovementGenerator* flight = dynamic_cast<FlightPathMovementGenerator*>(GetPlayer()->GetMotionMaster()->top()))
                {
                    // short preparations to continue flight
                    flight->SetCurrentNodeAfterTeleport();
                    TaxiPathNodeEntry const* node = flight->GetPath()[flight->GetCurrentNode()];
                    flight->SkipCurrentNode();

                    GetPlayer()->TeleportTo(curDestNode->MapID, node->Loc.X, node->Loc.Y, node->Loc.Z, GetPlayer()->GetOrientation(), TELE_TO_NOT_LEAVE_TAXI);
                }
            }

            return;
        }

        // at this point only 1 node is expected (final destination)
        if (GetPlayer()->m_taxi.GetPath().size() != 1)
            return;

        GetPlayer()->CleanupAfterTaxiFlight();
        GetPlayer()->SetFallInformation(GameTime::GetGameTime().count(), GetPlayer()->GetPositionZ());
        if (GetPlayer()->pvpInfo.IsHostile)
            GetPlayer()->CastSpell(GetPlayer(), 2479, true);

        return;
    }

    if (mover->GetGUID() != movementInfo.guid)
        return;

    if (!_player->IsExpectingChangeTransport() || !mover->movespline || mover->movespline->GetId() != uint32(moveSplineDone.SplineID))
        return;

    // the transport change spline ended: the others see the final position
    _player->SetExpectingChangeTransport(false);
    WorldPackets::Movement::MoveUpdate moveUpdate;
    moveUpdate.movementInfo = &movementInfo;
    mover->SendMessageToSet(moveUpdate.Write(), _player);
}
