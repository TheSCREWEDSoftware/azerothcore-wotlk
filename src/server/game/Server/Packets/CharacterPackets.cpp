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

#include "CharacterPackets.h"
#include "StringConvert.h"
#include "Tokenize.h"
#include "ObjectMgr.h"
#include "PacketUtilities.h"
#include "World.h"

WorldPacket const* WorldPackets::Character::EnumCharactersResult::Write()
{
    _worldPacket.reserve(9 + Characters.size() * sizeof(CharacterInfo) + FactionChangeRestrictions.size() * sizeof(RestrictedFactionChangeRuleInfo));

    _worldPacket.WriteBit(Success);
    _worldPacket.WriteBit(IsDeletedCharacters);
    _worldPacket << uint32(Characters.size());
    _worldPacket << uint32(FactionChangeRestrictions.size());

    for (CharacterInfo const& charInfo : Characters)
    {
        _worldPacket << charInfo.Guid;
        _worldPacket << uint8(charInfo.ListPosition);
        _worldPacket << uint8(charInfo.Race);
        _worldPacket << uint8(charInfo.Class);
        _worldPacket << uint8(charInfo.Sex);
        _worldPacket << uint8(charInfo.Skin);
        _worldPacket << uint8(charInfo.Face);
        _worldPacket << uint8(charInfo.HairStyle);
        _worldPacket << uint8(charInfo.HairColor);
        _worldPacket << uint8(charInfo.FacialHair);
        _worldPacket << uint8(charInfo.Level);
        _worldPacket << int32(charInfo.ZoneId);
        _worldPacket << int32(charInfo.MapId);
        _worldPacket << charInfo.PreLoadPosition;
        _worldPacket << charInfo.GuildGuid;
        _worldPacket << uint32(charInfo.Flags);
        _worldPacket << uint32(charInfo.CustomizationFlag);
        _worldPacket << uint32(charInfo.Flags3);
        _worldPacket << uint32(charInfo.Pet.CreatureDisplayId);
        _worldPacket << uint32(charInfo.Pet.Level);
        _worldPacket << uint32(charInfo.Pet.CreatureFamily);

        _worldPacket << uint32(charInfo.ProfessionIds[0]);
        _worldPacket << uint32(charInfo.ProfessionIds[1]);

        for (uint8 slot = 0; slot < INVENTORY_SLOT_BAG_END; ++slot)
        {
            _worldPacket << uint32(charInfo.VisualItems[slot].DisplayId);
            _worldPacket << uint32(charInfo.VisualItems[slot].DisplayEnchantId);
            _worldPacket << uint8(charInfo.VisualItems[slot].InventoryType);
        }

        _worldPacket << uint32(charInfo.LastPlayedTime);
        _worldPacket.WriteBits(charInfo.Name.length(), 6);
        _worldPacket.WriteBit(charInfo.FirstLogin);
        _worldPacket.WriteBit(charInfo.BoostInProgress);
        _worldPacket.WriteBits(charInfo.unkWod61x, 5);
        _worldPacket.FlushBits();

        _worldPacket.WriteString(charInfo.Name);
    }

    for (RestrictedFactionChangeRuleInfo const& rule : FactionChangeRestrictions)
    {
        _worldPacket << int32(rule.Mask);
        _worldPacket << uint8(rule.Race);
    }

    return &_worldPacket;
}

void WorldPackets::Character::CreateCharacter::Read()
{
    CreateInfo.reset(new CharacterCreateInfo());
    uint32 nameLength = _worldPacket.ReadBits(6);
    bool const hasTemplateSet = _worldPacket.ReadBit();

    _worldPacket >> CreateInfo->Race;
    _worldPacket >> CreateInfo->Class;
    _worldPacket >> CreateInfo->Sex;
    _worldPacket >> CreateInfo->Skin;
    _worldPacket >> CreateInfo->Face;
    _worldPacket >> CreateInfo->HairStyle;
    _worldPacket >> CreateInfo->HairColor;
    _worldPacket >> CreateInfo->FacialHairStyle;
    _worldPacket >> CreateInfo->OutfitId;
    CreateInfo->Name = _worldPacket.ReadString(nameLength);
    if (hasTemplateSet)
        CreateInfo->TemplateSet = _worldPacket.read<int32>();
}

WorldPacket const* WorldPackets::Character::CreateChar::Write()
{
    _worldPacket << uint8(Code);
    return &_worldPacket;
}

void WorldPackets::Character::CharDelete::Read()
{
    _worldPacket >> Guid;
}

WorldPacket const* WorldPackets::Character::DeleteChar::Write()
{
    _worldPacket << uint8(Code);
    return &_worldPacket;
}

void WorldPackets::Character::CharacterRenameRequest::Read()
{
    RenameInfo.reset(new CharacterRenameInfo());
    _worldPacket >> RenameInfo->Guid;
    RenameInfo->NewName = _worldPacket.ReadString(_worldPacket.ReadBits(6));
}

WorldPacket const* WorldPackets::Character::CharacterRenameResult::Write()
{
    _worldPacket << uint8(Result);
    _worldPacket.WriteBit(Guid.has_value());
    _worldPacket.WriteBits(Name.length(), 6);
    _worldPacket.FlushBits();

    if (Guid)
        _worldPacket << *Guid;

    _worldPacket.WriteString(Name);

    return &_worldPacket;
}

void WorldPackets::Character::CharCustomize::Read()
{
    CustomizeInfo.reset(new CharCustomizeInfo());
    _worldPacket >> CustomizeInfo->CharGUID;
    _worldPacket >> CustomizeInfo->SexID;
    _worldPacket >> CustomizeInfo->SkinID;
    _worldPacket >> CustomizeInfo->HairColorID;
    _worldPacket >> CustomizeInfo->HairStyleID;
    _worldPacket >> CustomizeInfo->FacialHairStyleID;
    _worldPacket >> CustomizeInfo->FaceID;
    CustomizeInfo->CharName = _worldPacket.ReadString(_worldPacket.ReadBits(6));
}

void WorldPackets::Character::CharRaceOrFactionChange::Read()
{
    RaceOrFactionChangeInfo.reset(new CharRaceOrFactionChangeInfo());

    RaceOrFactionChangeInfo->FactionChange = _worldPacket.ReadBit();

    uint32 nameLength = _worldPacket.ReadBits(6);

    bool const hasSkinID = _worldPacket.ReadBit();
    bool const hasHairColorID = _worldPacket.ReadBit();
    bool const hasHairStyleID = _worldPacket.ReadBit();
    bool const hasFacialHairStyleID = _worldPacket.ReadBit();
    bool const hasFaceID = _worldPacket.ReadBit();

    _worldPacket >> RaceOrFactionChangeInfo->Guid;
    _worldPacket >> RaceOrFactionChangeInfo->SexID;
    _worldPacket >> RaceOrFactionChangeInfo->RaceID;

    RaceOrFactionChangeInfo->Name = _worldPacket.ReadString(nameLength);

    if (hasSkinID)
        RaceOrFactionChangeInfo->SkinID = _worldPacket.read<uint8>();

    if (hasHairColorID)
        RaceOrFactionChangeInfo->HairColorID = _worldPacket.read<uint8>();

    if (hasHairStyleID)
        RaceOrFactionChangeInfo->HairStyleID = _worldPacket.read<uint8>();

    if (hasFacialHairStyleID)
        RaceOrFactionChangeInfo->FacialHairStyleID = _worldPacket.read<uint8>();

    if (hasFaceID)
        RaceOrFactionChangeInfo->FaceID = _worldPacket.read<uint8>();
}

WorldPacket const* WorldPackets::Character::CharFactionChangeResult::Write()
{
    _worldPacket << uint8(Result);
    _worldPacket << Guid;
    _worldPacket.WriteBit(Display.has_value());
    _worldPacket.FlushBits();

    if (Display)
    {
        _worldPacket.WriteBits(Display->Name.length(), 6);
        _worldPacket << uint8(Display->SexID);
        _worldPacket << uint8(Display->SkinID);
        _worldPacket << uint8(Display->HairColorID);
        _worldPacket << uint8(Display->HairStyleID);
        _worldPacket << uint8(Display->FacialHairStyleID);
        _worldPacket << uint8(Display->FaceID);
        _worldPacket << uint8(Display->RaceID);
        _worldPacket.WriteString(Display->Name);
    }

    return &_worldPacket;
}

void WorldPackets::Character::GenerateRandomCharacterName::Read()
{
    _worldPacket >> Race;
    _worldPacket >> Sex;
}

WorldPacket const* WorldPackets::Character::GenerateRandomCharacterNameResult::Write()
{
    _worldPacket.WriteBit(Success);
    _worldPacket.WriteBits(Name.length(), 6);
    _worldPacket.FlushBits();

    _worldPacket.WriteString(Name);

    return &_worldPacket;
}

WorldPackets::Character::ReorderCharacters::ReorderCharacters(WorldPacket&& packet) : ClientPacket(CMSG_REORDER_CHARACTERS, std::move(packet)),
    Entries(sWorld->getIntConfig(CONFIG_CHARACTERS_PER_REALM))
{

}

void WorldPackets::Character::ReorderCharacters::Read()
{
    Entries.resize(_worldPacket.ReadBits(9));
    for (ReorderInfo& reorderInfo : Entries)
    {
        _worldPacket >> reorderInfo.PlayerGUID;
        _worldPacket >> reorderInfo.NewPosition;
    }
}

void WorldPackets::Character::UndeleteCharacter::Read()
{
    UndeleteInfo.reset(new CharacterUndeleteInfo());
    _worldPacket >> UndeleteInfo->ClientToken;
    _worldPacket >> UndeleteInfo->CharacterGuid;
}

WorldPacket const* WorldPackets::Character::UndeleteCharacterResponse::Write()
{
    ASSERT(UndeleteInfo);
    _worldPacket << int32(UndeleteInfo->ClientToken);
    _worldPacket << uint32(Result);
    _worldPacket << UndeleteInfo->CharacterGuid;
    return &_worldPacket;
}

WorldPacket const* WorldPackets::Character::UndeleteCooldownStatusResponse::Write()
{
    _worldPacket.WriteBit(OnCooldown);
    _worldPacket << uint32(MaxCooldown);
    _worldPacket << uint32(CurrentCooldown);
    return &_worldPacket;
}

void WorldPackets::Character::PlayerLogin::Read()
{
    _worldPacket >> Guid;
    _worldPacket >> FarClip;
}

WorldPacket const* WorldPackets::Character::LoginVerifyWorld::Write()
{
    _worldPacket << int32(MapID);
    _worldPacket << Pos.PositionXYZOStream();
    _worldPacket << uint32(Reason);
    return &_worldPacket;
}

WorldPacket const* WorldPackets::Character::CharacterLoginFailed::Write()
{
    _worldPacket << uint8(Code);
    return &_worldPacket;
}

WorldPacket const* WorldPackets::Character::LogoutResponse::Write()
{
    _worldPacket << int32(LogoutResult);
    _worldPacket.WriteBit(Instant);
    _worldPacket.FlushBits();
    return &_worldPacket;
}

WorldPacket const* WorldPackets::Character::LogoutComplete::Write()
{
    _worldPacket << SwitchToCharacter;
    return &_worldPacket;
}

void WorldPackets::Character::LoadingScreenNotify::Read()
{
    _worldPacket >> MapID;
    Showing = _worldPacket.ReadBit();
}

WorldPacket const* WorldPackets::Character::InitialSetup::Write()
{
    _worldPacket << uint8(ServerExpansionLevel);
    _worldPacket << uint8(ServerExpansionTier);
    _worldPacket << int32(ServerRegionID);
    _worldPacket << uint32(RaidOrigin);

    return &_worldPacket;
}

void WorldPackets::Character::SetActionBarToggles::Read()
{
    _worldPacket >> Mask;
}

void WorldPackets::Character::RequestPlayedTime::Read()
{
    TriggerScriptEvent = _worldPacket.ReadBit();
}

WorldPacket const* WorldPackets::Character::PlayedTime::Write()
{
    _worldPacket << int32(TotalTime);
    _worldPacket << int32(LevelTime);
    _worldPacket.WriteBit(TriggerEvent);
    _worldPacket.FlushBits();

    return &_worldPacket;
}

void WorldPackets::Character::ShowingCloak::Read()
{
    ShowCloak = _worldPacket.ReadBit();
}

void WorldPackets::Character::ShowingHelm::Read()
{
    ShowHelm = _worldPacket.ReadBit();
}

void WorldPackets::Character::SetTitle::Read()
{
    _worldPacket >> TitleID;
}

void WorldPackets::Character::AlterApperance::Read()
{
    _worldPacket >> NewHairStyle;
    _worldPacket >> NewHairColor;
    _worldPacket >> NewFacialHair;
    _worldPacket >> NewSkinColor;
    _worldPacket >> NewFace;
}

WorldPacket const* WorldPackets::Character::BarberShopResultServer::Write()
{
    _worldPacket << int32(Result);
    return &_worldPacket;
}

WorldPacket const* WorldPackets::Character::LogXPGain::Write()
{
    _worldPacket << Victim;
    _worldPacket << int32(Original);
    _worldPacket << uint8(Reason);
    _worldPacket << int32(Amount);
    _worldPacket << float(GroupBonus);
    _worldPacket.WriteBit(ReferAFriend);

    _worldPacket.FlushBits();

    return &_worldPacket;
}

WorldPacket const* WorldPackets::Character::TitleEarned::Write()
{
    _worldPacket << uint32(Index);

    return &_worldPacket;
}

void WorldPackets::Character::SetFactionAtWar::Read()
{
    _worldPacket >> FactionIndex;
}

void WorldPackets::Character::SetFactionNotAtWar::Read()
{
    _worldPacket >> FactionIndex;
}

void WorldPackets::Character::SetFactionInactive::Read()
{
    _worldPacket >> Index;
    State = _worldPacket.ReadBit();
}

void WorldPackets::Character::SetWatchedFaction::Read()
{
    _worldPacket >> FactionIndex;
}

WorldPacket const* WorldPackets::Character::SetFactionVisible::Write()
{
    _worldPacket << FactionIndex;
    return &_worldPacket;
}

WorldPackets::Character::CharCustomizeResponse::CharCustomizeResponse(WorldPackets::Character::CharCustomizeInfo const* info)
    : ServerPacket(SMSG_CHAR_CUSTOMIZE, 16 + 1 + 1 + 1 + 1 + 1 + 1 + 1)
{
    CharGUID = info->CharGUID;
    SexID = info->SexID;
    SkinID = info->SkinID;
    HairColorID = info->HairColorID;
    HairStyleID = info->HairStyleID;
    FacialHairStyleID = info->FacialHairStyleID;
    FaceID = info->FaceID;
    CharName = info->CharName;
}

WorldPacket const* WorldPackets::Character::CharCustomizeResponse::Write()
{
    _worldPacket << CharGUID;
    _worldPacket << uint8(SexID);
    _worldPacket << uint8(SkinID);
    _worldPacket << uint8(HairColorID);
    _worldPacket << uint8(HairStyleID);
    _worldPacket << uint8(FacialHairStyleID);
    _worldPacket << uint8(FaceID);
    _worldPacket.WriteBits(CharName.length(), 6);
    _worldPacket.FlushBits();
    _worldPacket.WriteString(CharName);

    return &_worldPacket;
}

WorldPacket const* WorldPackets::Character::CharCustomizeFailed::Write()
{
    _worldPacket << uint8(Result);
    _worldPacket << CharGUID;

    return &_worldPacket;
}

void WorldPackets::Character::SetPlayerDeclinedNames::Read()
{
    _worldPacket >> Player;

    uint8 stringLengths[MAX_DECLINED_NAME_CASES];

    for (uint8 i = 0; i < MAX_DECLINED_NAME_CASES; ++i)
        stringLengths[i] = _worldPacket.ReadBits(7);

    for (uint8 i = 0; i < MAX_DECLINED_NAME_CASES; ++i)
        DeclinedNames.name[i] = _worldPacket.ReadString(stringLengths[i]);
}

WorldPacket const * WorldPackets::Character::SetPlayerDeclinedNamesResult::Write()
{
    _worldPacket << int32(ResultCode);
    _worldPacket << Player;

    return &_worldPacket;
}
