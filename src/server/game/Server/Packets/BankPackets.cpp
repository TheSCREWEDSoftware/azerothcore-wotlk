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

#include "BankPackets.h"

void WorldPackets::Bank::AutoBankItem::Read()
{
    // 5.4.8 18414, from Project SkyFire; the item list bits after them are not used
    _worldPacket >> Slot;
    _worldPacket >> Bag;
    _worldPacket.rfinish();
}

void WorldPackets::Bank::AutoStoreBankItem::Read()
{
    // 5.4.8 18414, from Project SkyFire
    _worldPacket >> Slot;
    _worldPacket >> Bag;
    _worldPacket.rfinish();
}

void WorldPackets::Bank::BuyBankSlot::Read()
{
    // 5.4.8 18414, from Project SkyFire
    _worldPacket.ReadGuidMask(Banker, { 7, 6, 1, 3, 2, 0, 4, 5 });
    _worldPacket.ReadGuidBytes(Banker, { 3, 5, 1, 6, 7, 2, 0, 4 });
}

WorldPacket const* WorldPackets::Bank::BuyBankSlotResult::Write()
{
    _worldPacket << uint32(Result);

    return &_worldPacket;
}

WorldPacket const* WorldPackets::Bank::ShowBank::Write()
{
    // 5.4.8 18414, from Project SkyFire
    _worldPacket.WriteGuidMask(Banker, { 2, 4, 3, 6, 5, 1, 7, 0 });
    _worldPacket.FlushBits();
    _worldPacket.WriteGuidBytes(Banker, { 7, 0, 5, 3, 6, 1, 4, 2 });

    return &_worldPacket;
}
