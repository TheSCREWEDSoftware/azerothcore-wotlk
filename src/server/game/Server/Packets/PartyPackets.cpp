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

#include "PartyPackets.h"
#include "Group.h"
#include "Pet.h"
#include "Player.h"
#include "Realm.h"
#include "SpellAuraEffects.h"
#include "SpellAuras.h"
#include "Timer.h"
#include "Vehicle.h"
#include "World.h"

namespace
{
    void FillAuraStates(Unit* unit, uint64 auraMask, std::vector<WorldPackets::Party::PartyMemberAuraStates>& auras)
    {
        for (uint8 i = 0; i < MAX_AURAS_GROUP_UPDATE; ++i)
        {
            if (!(auraMask & (uint64(1) << i)))
                continue;

            WorldPackets::Party::PartyMemberAuraStates aura;
            if (AuraApplication const* aurApp = unit->GetVisibleAura(i))
            {
                aura.SpellID = aurApp->GetBase()->GetId();
                aura.Flags = aurApp->GetFlags();

                if (aurApp->GetFlags() & AFLAG_ANY_EFFECT_AMOUNT_SENT)
                {
                    for (uint8 j = 0; j < MAX_SPELL_EFFECTS; ++j)
                    {
                        if (AuraEffect const* aurEff = aurApp->GetBase()->GetEffect(j))
                            aura.Points.push_back(aurEff->GetAmount());
                        else
                            aura.Points.push_back(0);
                    }
                }
            }

            auras.push_back(std::move(aura));
        }
    }
}

void WorldPackets::Party::PartyMemberState::Initialize(Player const* constPlayer)
{
    Player* player = const_cast<Player*>(constPlayer);
    MemberGuid = player->GetGUID();

    if (FullUpdate)
    {
        ChangeMask = GROUP_UPDATE_FLAG_STATUS | GROUP_UPDATE_FLAG_CUR_HP | GROUP_UPDATE_FLAG_MAX_HP
            | GROUP_UPDATE_FLAG_CUR_POWER | GROUP_UPDATE_FLAG_MAX_POWER | GROUP_UPDATE_FLAG_LEVEL
            | GROUP_UPDATE_FLAG_ZONE | GROUP_UPDATE_FLAG_WMO_GROUP_ID | GROUP_UPDATE_FLAG_POSITION | GROUP_UPDATE_FLAG_AURAS
            | GROUP_UPDATE_FLAG_PET_AURAS | GROUP_UPDATE_FLAG_PHASE;

        if (player->getPowerType() != POWER_MANA)
            ChangeMask |= GROUP_UPDATE_FLAG_POWER_TYPE;

        if (player->GetPet())
            ChangeMask |= GROUP_UPDATE_FLAG_PET_GUID | GROUP_UPDATE_FLAG_PET_NAME | GROUP_UPDATE_FLAG_PET_MODEL_ID
                | GROUP_UPDATE_FLAG_PET_CUR_HP | GROUP_UPDATE_FLAG_PET_MAX_HP | GROUP_UPDATE_FLAG_PET_POWER_TYPE
                | GROUP_UPDATE_FLAG_PET_CUR_POWER | GROUP_UPDATE_FLAG_PET_MAX_POWER;

        if (player->GetVehicle())
            ChangeMask |= GROUP_UPDATE_FLAG_VEHICLE_SEAT;
    }
    else
    {
        ChangeMask = player->GetGroupUpdateFlag();

        if (ChangeMask & GROUP_UPDATE_FLAG_POWER_TYPE)
            ChangeMask |= (GROUP_UPDATE_FLAG_CUR_POWER | GROUP_UPDATE_FLAG_MAX_POWER);

        if (ChangeMask & GROUP_UPDATE_FLAG_PET_POWER_TYPE)
            ChangeMask |= (GROUP_UPDATE_FLAG_PET_CUR_POWER | GROUP_UPDATE_FLAG_PET_MAX_POWER);
    }

    MemberStats.Status = MEMBER_STATUS_ONLINE;

    if (player->IsPvP())
        MemberStats.Status |= MEMBER_STATUS_PVP;

    if (!player->IsAlive())
    {
        if (player->HasPlayerFlag(PLAYER_FLAGS_GHOST))
            MemberStats.Status |= MEMBER_STATUS_GHOST;
        else
            MemberStats.Status |= MEMBER_STATUS_DEAD;
    }

    if (player->IsFFAPvP())
        MemberStats.Status |= MEMBER_STATUS_PVP_FFA;

    if (player->isAFK())
        MemberStats.Status |= MEMBER_STATUS_AFK;

    if (player->isDND())
        MemberStats.Status |= MEMBER_STATUS_DND;

    MemberStats.Level = player->GetLevel();
    MemberStats.CurrentHealth = player->GetHealth();
    MemberStats.MaxHealth = player->GetMaxHealth();

    Powers powerType = player->getPowerType();
    MemberStats.PowerType = powerType;
    MemberStats.CurrentPower = player->GetPower(powerType);
    MemberStats.MaxPower = player->GetMaxPower(powerType);

    MemberStats.ZoneID = player->GetZoneId();
    MemberStats.PositionX = int16(player->GetPositionX());
    MemberStats.PositionY = int16(player->GetPositionY());
    MemberStats.PositionZ = int16(player->GetPositionZ());

    if (Vehicle* vehicle = player->GetVehicle())
        if (VehicleEntry const* vehicleInfo = vehicle->GetVehicleInfo())
            MemberStats.VehicleSeat = vehicleInfo->m_seatID[player->m_movementInfo.transport.seat];

    MemberStats.AuraMask = player->GetAuraUpdateMaskForRaid();
    FillAuraStates(player, MemberStats.AuraMask, MemberStats.Auras);

    MemberStats.Phases.PhaseShiftFlags = player->IsUnphased() ? 0x8 /*Unphased*/ : 0x0;
    for (uint32 phaseId : player->GetPhases())
        MemberStats.Phases.List.push_back(uint16(phaseId));

    if (::Pet* pet = player->GetPet())
    {
        MemberStats.PetStats.emplace();
        MemberStats.PetStats->GUID = pet->GetGUID();
        MemberStats.PetStats->Name = pet->GetName();
        MemberStats.PetStats->ModelId = pet->GetDisplayId();
        MemberStats.PetStats->CurrentHealth = pet->GetHealth();
        MemberStats.PetStats->MaxHealth = pet->GetMaxHealth();

        Powers petPowerType = pet->getPowerType();
        MemberStats.PetStats->PowerType = petPowerType;
        MemberStats.PetStats->CurrentPower = pet->GetPower(petPowerType);
        MemberStats.PetStats->MaxPower = pet->GetMaxPower(petPowerType);

        MemberStats.PetStats->AuraMask = pet->GetAuraUpdateMaskForRaid();
        FillAuraStates(pet, MemberStats.PetStats->AuraMask, MemberStats.PetStats->Auras);
    }
}

uint32 WorldPackets::Party::PartyMemberState::GetClientChangeMask(uint32 changeMask)
{
    // 5.4.8 adds unused bits between AC's flags, from Project SkyFire's GroupUpdateFlags
    static constexpr std::pair<uint32, uint32> ClientBits[] =
    {
        { GROUP_UPDATE_FLAG_STATUS,         0x00000001 },
        { GROUP_UPDATE_FLAG_CUR_HP,         0x00000004 },
        { GROUP_UPDATE_FLAG_MAX_HP,         0x00000008 },
        { GROUP_UPDATE_FLAG_POWER_TYPE,     0x00000010 },
        { GROUP_UPDATE_FLAG_CUR_POWER,      0x00000040 },
        { GROUP_UPDATE_FLAG_MAX_POWER,      0x00000080 },
        { GROUP_UPDATE_FLAG_LEVEL,          0x00000100 },
        { GROUP_UPDATE_FLAG_ZONE,           0x00000200 },
        { GROUP_UPDATE_FLAG_WMO_GROUP_ID,   0x00000400 },
        { GROUP_UPDATE_FLAG_POSITION,       0x00000800 },
        { GROUP_UPDATE_FLAG_AURAS,          0x00001000 },
        { GROUP_UPDATE_FLAG_PET_GUID,       0x00002000 },
        { GROUP_UPDATE_FLAG_PET_NAME,       0x00004000 },
        { GROUP_UPDATE_FLAG_PET_MODEL_ID,   0x00008000 },
        { GROUP_UPDATE_FLAG_PET_CUR_HP,     0x00010000 },
        { GROUP_UPDATE_FLAG_PET_MAX_HP,     0x00020000 },
        { GROUP_UPDATE_FLAG_PET_POWER_TYPE, 0x00040000 },
        { GROUP_UPDATE_FLAG_PET_CUR_POWER,  0x00100000 },
        { GROUP_UPDATE_FLAG_PET_MAX_POWER,  0x00200000 },
        { GROUP_UPDATE_FLAG_PET_AURAS,      0x00400000 },
        { GROUP_UPDATE_FLAG_VEHICLE_SEAT,   0x00800000 },
        { GROUP_UPDATE_FLAG_PHASE,          0x01000000 },
    };

    uint32 clientMask = 0;
    for (auto const& [flag, clientBit] : ClientBits)
        if (changeMask & flag)
            clientMask |= clientBit;

    return clientMask;
}

WorldPacket const* WorldPackets::Party::PartyMemberState::Write()
{
    // 5.4.8 18414 SMSG_PARTY_MEMBER_STATE, from Project SkyFire. The 5.4.8 aura block is not known
    // (SkyFire does not send it either), so the aura flags are left out.
    uint32 changeMask = ChangeMask & ~(GROUP_UPDATE_FLAG_AURAS | GROUP_UPDATE_FLAG_PET_AURAS);

    ByteBuffer data;
    if (changeMask & GROUP_UPDATE_FLAG_STATUS)
        data << uint16(MemberStats.Status);

    if (changeMask & GROUP_UPDATE_FLAG_CUR_HP)
        data << int32(MemberStats.CurrentHealth);

    if (changeMask & GROUP_UPDATE_FLAG_MAX_HP)
        data << int32(MemberStats.MaxHealth);

    if (changeMask & GROUP_UPDATE_FLAG_POWER_TYPE)
        data << uint8(MemberStats.PowerType);

    if (changeMask & GROUP_UPDATE_FLAG_CUR_POWER)
        data << uint16(MemberStats.CurrentPower);

    if (changeMask & GROUP_UPDATE_FLAG_MAX_POWER)
        data << uint16(MemberStats.MaxPower);

    if (changeMask & GROUP_UPDATE_FLAG_LEVEL)
        data << uint16(MemberStats.Level);

    if (changeMask & GROUP_UPDATE_FLAG_ZONE)
        data << uint16(MemberStats.ZoneID);

    if (changeMask & GROUP_UPDATE_FLAG_WMO_GROUP_ID)
        data << int16(MemberStats.WmoGroupID);

    if (changeMask & GROUP_UPDATE_FLAG_POSITION)
    {
        data << int16(MemberStats.PositionX);
        data << int16(MemberStats.PositionY);
        data << int16(MemberStats.PositionZ);
    }

    // pet fields are sent empty when there is no pet
    PartyMemberPetStats const emptyPet;
    PartyMemberPetStats const& pet = MemberStats.PetStats ? *MemberStats.PetStats : emptyPet;

    if (changeMask & GROUP_UPDATE_FLAG_PET_GUID)
        data << pet.GUID;

    if (changeMask & GROUP_UPDATE_FLAG_PET_NAME)
        data << pet.Name;

    if (changeMask & GROUP_UPDATE_FLAG_PET_MODEL_ID)
        data << int16(pet.ModelId);

    if (changeMask & GROUP_UPDATE_FLAG_PET_CUR_HP)
        data << int32(pet.CurrentHealth);

    if (changeMask & GROUP_UPDATE_FLAG_PET_MAX_HP)
        data << int32(pet.MaxHealth);

    if (changeMask & GROUP_UPDATE_FLAG_PET_POWER_TYPE)
        data << uint8(pet.PowerType);

    if (changeMask & GROUP_UPDATE_FLAG_PET_CUR_POWER)
        data << uint16(pet.CurrentPower);

    if (changeMask & GROUP_UPDATE_FLAG_PET_MAX_POWER)
        data << uint16(pet.MaxPower);

    if (changeMask & GROUP_UPDATE_FLAG_VEHICLE_SEAT)
        data << int32(MemberStats.VehicleSeat);

    if (changeMask & GROUP_UPDATE_FLAG_PHASE)
    {
        data << uint32(MemberStats.Phases.PhaseShiftFlags);
        data.WriteBits(MemberStats.Phases.List.size(), 23);
        data.FlushBits();
        for (uint16 phaseId : MemberStats.Phases.List)
            data << uint16(phaseId);
    }

    _worldPacket.WriteGuidMask(MemberGuid, { 0, 5 });
    _worldPacket.WriteBit(ForEnemy);
    _worldPacket.WriteGuidMask(MemberGuid, { 1, 4 });
    _worldPacket.WriteBit(FullUpdate);
    _worldPacket.WriteGuidMask(MemberGuid, { 6, 2, 7, 3 });
    _worldPacket.FlushBits();

    _worldPacket.WriteGuidBytes(MemberGuid, { 3, 2, 6, 7, 5 });
    _worldPacket << uint32(GetClientChangeMask(changeMask));
    _worldPacket.WriteGuidBytes(MemberGuid, { 1, 4, 0 });
    _worldPacket << uint32(data.size());
    _worldPacket.append(data);

    return &_worldPacket;
}

void WorldPackets::Party::SetEveryoneIsAssistant::Read()
{
    // 5.4.8 18414, from Project SkyFire
    _worldPacket.read_skip<uint8>();                        // party index
    EveryoneIsAssistant = _worldPacket.ReadBit();
}

WorldPacket const* WorldPackets::Party::PartyCommandResult::Write()
{
    _worldPacket << uint32(Command);
    _worldPacket << Name;
    _worldPacket << uint32(Result);
    _worldPacket << uint32(ResultData);
    _worldPacket << ResultGUID;

    return &_worldPacket;
}

void WorldPackets::Party::PartyInviteClient::Read()
{
    // 5.4.8 18414, from Project SkyFire
    _worldPacket >> TargetCfgRealmID;                       // non-zero in cross realm invites
    _worldPacket.read_skip<uint8>();                        // party index
    _worldPacket >> ProposedRoles;

    _worldPacket.ReadGuidMask(TargetGUID, { 7 });
    uint32 targetRealmLen = _worldPacket.ReadBits(9);
    _worldPacket.ReadGuidMask(TargetGUID, { 3 });
    uint32 targetNameLen = _worldPacket.ReadBits(9);
    _worldPacket.ReadGuidMask(TargetGUID, { 2, 5, 4, 0, 1, 6 });

    _worldPacket.ReadGuidBytes(TargetGUID, { 7, 6, 0, 4 });
    TargetRealm = _worldPacket.ReadString(targetRealmLen);
    _worldPacket.ReadGuidBytes(TargetGUID, { 1, 2, 3 });
    TargetName = _worldPacket.ReadString(targetNameLen);
    _worldPacket.ReadGuidBytes(TargetGUID, { 5 });
}

void WorldPackets::Party::PartyInvite::Initialize(Player const* inviter, uint32 proposedRoles, bool canAccept)
{
    CanAccept = canAccept;
    InviterName = inviter->GetName();
    InviterGUID = inviter->GetGUID();
    InviterRealmName = realm.Name;
    InviterCfgRealmID = realm.Id.Realm;
    Timestamp = getMSTime();
    ProposedRoles = proposedRoles;
}

WorldPacket const* WorldPackets::Party::PartyInvite::Write()
{
    // 5.4.8 18414 SMSG_GROUP_INVITE, from Project SkyFire
    _worldPacket.WriteBits(InviterRealmName.length(), 8);
    _worldPacket.WriteBits(0, 8);
    _worldPacket.WriteGuidMask(InviterGUID, { 2 });
    _worldPacket.WriteBit(false);
    _worldPacket.WriteBits(InviterName.length(), 6);
    _worldPacket.WriteGuidMask(InviterGUID, { 7, 5 });
    _worldPacket.WriteBit(CanAccept);                       // false when already in a group
    _worldPacket.WriteBit(false);                           // auto decline
    _worldPacket.WriteGuidMask(InviterGUID, { 1 });
    _worldPacket.WriteBit(IsXRealm);                        // cross realm invite (adds the hyphen between the inviter and the realm)
    _worldPacket.WriteBit(MustBeBNetFriend);                // realm transfer warning
    _worldPacket.WriteBits(LfgSlots.size(), 22);
    _worldPacket.WriteGuidMask(InviterGUID, { 3, 0, 4, 6 });
    _worldPacket.FlushBits();

    _worldPacket.WriteGuidBytes(InviterGUID, { 6 });
    _worldPacket.WriteString(InviterRealmName);
    _worldPacket.WriteGuidBytes(InviterGUID, { 7, 2, 0 });
    _worldPacket << uint64(0);                              // battle.net account id
    _worldPacket << uint32(Timestamp);
    _worldPacket << uint32(ProposedRoles);
    _worldPacket.WriteGuidBytes(InviterGUID, { 1, 5, 4 });
    _worldPacket << int32(InviterCfgRealmID);
    _worldPacket.WriteString(InviterName);
    _worldPacket.WriteGuidBytes(InviterGUID, { 3 });
    _worldPacket << uint32(LfgCompletedMask);

    for (uint32 lfgSlot : LfgSlots)
        _worldPacket << uint32(lfgSlot);

    return &_worldPacket;
}

void WorldPackets::Party::PartyInviteResponse::Read()
{
    // 5.4.8 18414, from Project SkyFire
    _worldPacket.read_skip<uint8>();                        // party index
    bool hasRolesDesired = _worldPacket.ReadBit();
    Accept = _worldPacket.ReadBit();

    if (hasRolesDesired)
        RolesDesired = _worldPacket.read<uint32>();
}

WorldPacket const* WorldPackets::Party::GroupDecline::Write()
{
    _worldPacket << Name;

    return &_worldPacket;
}

WorldPacket const* WorldPackets::Party::PartyUpdate::Write()
{
    // 5.4.8 18414 SMSG_GROUP_LIST, from Project SkyFire. An empty player list is the "left the
    // group" update, which has no difficulty and loot blocks.
    bool hasSettings = !PlayerList.empty();
    bool isLfg = hasSettings && LfgInfo.has_value();
    ObjectGuid looterGuid = LootSettings.LootMaster;

    _worldPacket.WriteGuidMask(PartyGUID, { 0 });
    _worldPacket.WriteGuidMask(LeaderGUID, { 7, 1 });
    _worldPacket.WriteBit(hasSettings);                     // has dungeon and raid difficulty
    _worldPacket.WriteGuidMask(PartyGUID, { 7 });
    _worldPacket.WriteGuidMask(LeaderGUID, { 6, 5 });
    _worldPacket.WriteBits(PlayerList.size(), 21);

    ByteBuffer memberData;
    for (PartyPlayerInfo const& playerInfo : PlayerList)
    {
        _worldPacket.WriteGuidMask(playerInfo.Guid, { 1, 2, 5, 6 });
        _worldPacket.WriteBits(playerInfo.Name.size(), 6);
        _worldPacket.WriteGuidMask(playerInfo.Guid, { 7, 3, 0, 4 });

        memberData.WriteGuidBytes(playerInfo.Guid, { 6, 3 });
        memberData << uint8(playerInfo.RolesAssigned);
        memberData << uint8(playerInfo.Connected);
        memberData.WriteGuidBytes(playerInfo.Guid, { 7, 4, 1 });
        memberData.WriteString(playerInfo.Name);
        memberData.WriteGuidBytes(playerInfo.Guid, { 5, 2 });
        memberData << uint8(playerInfo.Subgroup);
        memberData.WriteGuidBytes(playerInfo.Guid, { 0 });
        memberData << uint8(playerInfo.Flags);
    }

    _worldPacket.WriteGuidMask(LeaderGUID, { 3, 0 });
    _worldPacket.WriteBit(hasSettings);                     // has loot mode
    _worldPacket.WriteGuidMask(PartyGUID, { 5 });
    if (hasSettings)
        _worldPacket.WriteGuidMask(looterGuid, { 6, 4, 5, 2, 1, 0, 7, 3 });
    _worldPacket.WriteGuidMask(PartyGUID, { 2, 4, 1 });
    _worldPacket.WriteBit(isLfg);
    _worldPacket.WriteGuidMask(LeaderGUID, { 2 });
    _worldPacket.WriteGuidMask(PartyGUID, { 6 });
    if (isLfg)
    {
        _worldPacket.WriteBit(LfgInfo->LfgAborted);
        _worldPacket.WriteBit(false);                       // first reward
    }
    _worldPacket.WriteGuidMask(LeaderGUID, { 4 });
    _worldPacket.WriteGuidMask(PartyGUID, { 3 });
    _worldPacket.FlushBits();

    _worldPacket.WriteGuidBytes(LeaderGUID, { 0 });
    if (hasSettings)
    {
        _worldPacket << uint32(DifficultySettings.RaidDifficulty);
        _worldPacket << uint32(DifficultySettings.DungeonDifficulty);
    }
    _worldPacket.append(memberData);
    _worldPacket.WriteGuidBytes(PartyGUID, { 1 });

    if (isLfg)
    {
        _worldPacket << float(0.0f);                        // gear difference
        _worldPacket << uint8(0);                           // stranger count
        _worldPacket << uint8(LfgInfo->MyLfgFlags);         // 0 in progress, 2 complete
        _worldPacket << uint32(LfgInfo->LfgSlot);           // dungeon the eye teleports to
        _worldPacket << uint8(0);                           // boot count
        _worldPacket << uint8(0);                           // kick votes
        _worldPacket << uint8(RolesAssigned);
        _worldPacket << uint32(0);                          // random slot
    }

    _worldPacket.WriteGuidBytes(LeaderGUID, { 4, 2 });
    if (hasSettings)
    {
        _worldPacket << uint8(LootSettings.LootMethod);
        _worldPacket.WriteGuidBytes(looterGuid, { 0, 5, 4, 3, 2 });
        _worldPacket << uint8(LootSettings.LootThreshold);
        _worldPacket.WriteGuidBytes(looterGuid, { 7, 1, 6 });
    }

    _worldPacket.WriteGuidBytes(PartyGUID, { 6, 4 });
    _worldPacket << uint8(PartyFlags);
    _worldPacket << uint8(InstanceParty ? 1 : 0);           // 0 home party, 1 instance party
    _worldPacket << int32(MyIndex);
    _worldPacket.WriteGuidBytes(PartyGUID, { 7 });
    _worldPacket.WriteGuidBytes(LeaderGUID, { 3, 1 });
    _worldPacket << uint32(SequenceNum);
    _worldPacket.WriteGuidBytes(PartyGUID, { 0, 2, 5, 3 });
    _worldPacket.WriteGuidBytes(LeaderGUID, { 7 });
    _worldPacket << uint8(isLfg ? RolesAssigned : 0);
    _worldPacket.WriteGuidBytes(LeaderGUID, { 5, 6 });

    return &_worldPacket;
}

WorldPacket const* WorldPackets::Party::SummonRaidMemberValidateFailed::Write()
{
    _worldPacket.WriteBits(Members.size(), 23);

    for (Reason const& reason : Members)
    {
        _worldPacket.WriteBit(reason.Member[5]);
        _worldPacket.WriteBit(reason.Member[3]);
        _worldPacket.WriteBit(reason.Member[1]);
        _worldPacket.WriteBit(reason.Member[7]);
        _worldPacket.WriteBit(reason.Member[2]);
        _worldPacket.WriteBit(reason.Member[0]);
        _worldPacket.WriteBit(reason.Member[6]);
        _worldPacket.WriteBit(reason.Member[4]);
    }

    _worldPacket.FlushBits();

    for (Reason const& reason : Members)
    {
        _worldPacket.WriteByteSeq(reason.Member[4]);
        _worldPacket.WriteByteSeq(reason.Member[2]);
        _worldPacket.WriteByteSeq(reason.Member[0]);
        _worldPacket.WriteByteSeq(reason.Member[6]);
        _worldPacket.WriteByteSeq(reason.Member[5]);
        _worldPacket << int32(reason.ReasonCode);
        _worldPacket.WriteByteSeq(reason.Member[7]);
        _worldPacket.WriteByteSeq(reason.Member[3]);
        _worldPacket.WriteByteSeq(reason.Member[1]);
    }

    return &_worldPacket;
}

void WorldPackets::Party::SetRole::Read()
{
    // 5.4.8 18414, from Project SkyFire
    _worldPacket.read_skip<uint8>();                        // party index
    _worldPacket >> Role;
    _worldPacket.ReadGuidMask(ChangedUnit, { 2, 0, 7, 4, 1, 3, 6, 5 });
    _worldPacket.ReadGuidBytes(ChangedUnit, { 1, 5, 2, 6, 7, 0, 4, 3 });
}

WorldPacket const* WorldPackets::Party::RoleChangedInform::Write()
{
    // 5.4.8 18414 SMSG_GROUP_SET_ROLE, from Project SkyFire
    _worldPacket.WriteGuidMask(From, { 1 });
    _worldPacket.WriteGuidMask(ChangedUnit, { 7, 6, 4, 1, 0 });
    _worldPacket.WriteGuidMask(From, { 0, 7 });
    _worldPacket.WriteGuidMask(ChangedUnit, { 3 });
    _worldPacket.WriteGuidMask(From, { 6 });
    _worldPacket.WriteGuidMask(ChangedUnit, { 2 });
    _worldPacket.WriteGuidMask(From, { 4, 5, 2 });
    _worldPacket.WriteGuidMask(ChangedUnit, { 5 });
    _worldPacket.WriteGuidMask(From, { 3 });
    _worldPacket.FlushBits();

    _worldPacket.WriteGuidBytes(From, { 1, 6, 2 });
    _worldPacket.WriteGuidBytes(ChangedUnit, { 3 });
    _worldPacket << uint32(OldRole);
    _worldPacket.WriteGuidBytes(From, { 7 });
    _worldPacket.WriteGuidBytes(ChangedUnit, { 5 });
    _worldPacket.WriteGuidBytes(From, { 3 });
    _worldPacket.WriteGuidBytes(ChangedUnit, { 4, 7 });
    _worldPacket.WriteGuidBytes(From, { 5 });
    _worldPacket.WriteGuidBytes(ChangedUnit, { 6, 2, 1, 0 });
    _worldPacket.WriteGuidBytes(From, { 4 });
    _worldPacket << uint8(0);                               // party index
    _worldPacket.WriteGuidBytes(From, { 0 });
    _worldPacket << uint32(NewRole);

    return &_worldPacket;
}
