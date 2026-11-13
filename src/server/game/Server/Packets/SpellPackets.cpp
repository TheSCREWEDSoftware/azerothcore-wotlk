/*
 * This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
 *
 * You may redistribute it and/or modify it under the terms of the GNU General Public License
 * version 2 or, at your option, any later version.
 */

#include "SpellPackets.h"

WorldPacket const* WorldPackets::Spells::ResurrectRequest::Write()
{
    _worldPacket << ResurrectOffererGUID;
    _worldPacket << uint32(Name.size() + 1);
    _worldPacket << Name;
    _worldPacket << uint8(Sickness);
    _worldPacket << uint8(UseTimer);
    _worldPacket << SpellID;
    return &_worldPacket;
}

WorldPacket const* WorldPackets::Spells::SendKnownSpells::Write()
{
    // 5.4.8 18414: the spells only, cooldowns go out as SMSG_SEND_SPELL_HISTORY
    _worldPacket.reserve(1 + 3 + 4 * KnownSpells.size());
    _worldPacket.WriteBit(InitialLogin);
    _worldPacket.WriteBits(KnownSpells.size(), 22);
    _worldPacket.FlushBits();

    for (uint32 spellID : KnownSpells)
        _worldPacket << uint32(spellID);

    return &_worldPacket;
}

WorldPacket const* WorldPackets::Spells::SendSpellHistory::Write()
{
    // 5.4.8 18414 (SkyFire_548 Player::SendSpellCooldowns)
    _worldPacket.WriteBits(Entries.size(), 19);
    for (SpellHistoryEntry const& entry : Entries)
        _worldPacket.WriteBit(entry.OnHold);
    _worldPacket.FlushBits();

    for (SpellHistoryEntry const& entry : Entries)
    {
        _worldPacket << int32(entry.CategoryRecoveryTime);
        _worldPacket << uint32(entry.ItemID);
        _worldPacket << int32(entry.RecoveryTime);
        _worldPacket << uint32(entry.Category);
        _worldPacket << uint32(entry.SpellID);
    }

    return &_worldPacket;
}

WorldPacket const* WorldPackets::Spells::SendUnlearnSpells::Write()
{
    // 5.4.8 18414
    _worldPacket.WriteBits(Spells.size(), 22);
    _worldPacket.FlushBits();
    for (uint32 spellID : Spells)
        _worldPacket << spellID;

    return &_worldPacket;
}

WorldPacket const* WorldPackets::Spells::UpdateActionButtons::Write()
{
    _worldPacket.append(ActionButtons.data(), ActionButtons.size());
    _worldPacket << Reason;

    return &_worldPacket;
}

ByteBuffer& operator<<(ByteBuffer& data, WorldPackets::Spells::SpellModifierData const& spellModifierData)
{
    data << spellModifierData.ClassIndex;
    data << spellModifierData.ModifierValue;
    return data;
}

ByteBuffer& operator<<(ByteBuffer& data, WorldPackets::Spells::SpellModifier const& spellModifier)
{
    data << uint32(spellModifier.ModifierData.size());
    data << spellModifier.ModIndex;
    for (WorldPackets::Spells::SpellModifierData const& modifierData : spellModifier.ModifierData)
        data << modifierData;
    return data;
}

WorldPacket const* WorldPackets::Spells::SetSpellModifier::Write()
{
    _worldPacket << uint32(Modifiers.size());
    for (SpellModifier const& modifier : Modifiers)
        _worldPacket << modifier;
    return &_worldPacket;
}

WorldPacket const* WorldPackets::Spells::ResyncRunes::Write()
{
    _worldPacket << uint32(Runes.size());
    for (ResyncRune const& rune : Runes)
    {
        _worldPacket << rune.RuneType;
        _worldPacket << rune.Cooldown;
    }
    return &_worldPacket;
}

WorldPacket const* WorldPackets::Spells::PlaySpellVisual::Write()
{
    _worldPacket << float(TargetPosition.GetPositionZ());
    _worldPacket << int32(SpellVisualID);
    _worldPacket << uint16(MissReason);
    _worldPacket << float(TravelSpeed);
    _worldPacket << float(TargetPosition.GetPositionX());
    _worldPacket << uint16(ReflectStatus);
    _worldPacket << float(TargetPosition.GetPositionY());

    _worldPacket.WriteBit(Target[1]);
    _worldPacket.WriteBit(Source[3]);
    _worldPacket.WriteBit(Source[0]);
    _worldPacket.WriteBit(Target[2]);
    _worldPacket.WriteBit(Target[5]);
    _worldPacket.WriteBit(Source[2]);
    _worldPacket.WriteBit(Source[4]);
    _worldPacket.WriteBit(Target[6]);
    _worldPacket.WriteBit(Target[0]);
    _worldPacket.WriteBit(SpeedAsTime);
    _worldPacket.WriteBit(Source[6]);
    _worldPacket.WriteBit(Target[7]);
    _worldPacket.WriteBit(Source[5]);
    _worldPacket.WriteBit(Source[1]);
    _worldPacket.WriteBit(Source[7]);
    _worldPacket.WriteBit(Target[3]);
    _worldPacket.WriteBit(Target[4]);
    _worldPacket.FlushBits();

    _worldPacket.WriteByteSeq(Source[7]);
    _worldPacket.WriteByteSeq(Source[4]);
    _worldPacket.WriteByteSeq(Target[7]);
    _worldPacket.WriteByteSeq(Source[1]);
    _worldPacket.WriteByteSeq(Source[3]);
    _worldPacket.WriteByteSeq(Source[0]);
    _worldPacket.WriteByteSeq(Source[6]);
    _worldPacket.WriteByteSeq(Target[0]);
    _worldPacket.WriteByteSeq(Target[4]);
    _worldPacket.WriteByteSeq(Source[5]);
    _worldPacket.WriteByteSeq(Target[1]);
    _worldPacket.WriteByteSeq(Target[5]);
    _worldPacket.WriteByteSeq(Target[6]);
    _worldPacket.WriteByteSeq(Target[2]);
    _worldPacket.WriteByteSeq(Source[2]);
    _worldPacket.WriteByteSeq(Target[3]);

    return &_worldPacket;
}

WorldPacket const* WorldPackets::Spells::PlaySpellVisualKit::Write()
{
    _worldPacket << uint32(Duration);
    _worldPacket << int32(KitRecID);
    _worldPacket << int32(KitType);

    _worldPacket.WriteBit(Unit[4]);
    _worldPacket.WriteBit(Unit[7]);
    _worldPacket.WriteBit(Unit[5]);
    _worldPacket.WriteBit(Unit[3]);
    _worldPacket.WriteBit(Unit[1]);
    _worldPacket.WriteBit(Unit[2]);
    _worldPacket.WriteBit(Unit[0]);
    _worldPacket.WriteBit(Unit[6]);
    _worldPacket.FlushBits();

    _worldPacket.WriteByteSeq(Unit[0]);
    _worldPacket.WriteByteSeq(Unit[4]);
    _worldPacket.WriteByteSeq(Unit[1]);
    _worldPacket.WriteByteSeq(Unit[6]);
    _worldPacket.WriteByteSeq(Unit[7]);
    _worldPacket.WriteByteSeq(Unit[2]);
    _worldPacket.WriteByteSeq(Unit[3]);
    _worldPacket.WriteByteSeq(Unit[5]);

    return &_worldPacket;
}
