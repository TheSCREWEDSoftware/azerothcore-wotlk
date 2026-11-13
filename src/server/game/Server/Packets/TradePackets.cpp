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

#include "TradePackets.h"

WorldPacket const* WorldPackets::Trade::TradeStatus::Write()
{
    _worldPacket.WriteBit(PartnerIsSameBnetAccount);
    _worldPacket.WriteBits(Status, 5);

    switch (Status)
    {
        case TRADE_STATUS_BEGIN_TRADE:
            _worldPacket.WriteBit(Partner[2]);
            _worldPacket.WriteBit(Partner[4]);
            _worldPacket.WriteBit(Partner[6]);
            _worldPacket.WriteBit(Partner[0]);
            _worldPacket.WriteBit(Partner[1]);
            _worldPacket.WriteBit(Partner[3]);
            _worldPacket.WriteBit(Partner[7]);
            _worldPacket.WriteBit(Partner[5]);
            _worldPacket.FlushBits();

            _worldPacket.WriteByteSeq(Partner[4]);
            _worldPacket.WriteByteSeq(Partner[1]);
            _worldPacket.WriteByteSeq(Partner[2]);
            _worldPacket.WriteByteSeq(Partner[3]);
            _worldPacket.WriteByteSeq(Partner[0]);
            _worldPacket.WriteByteSeq(Partner[7]);
            _worldPacket.WriteByteSeq(Partner[6]);
            _worldPacket.WriteByteSeq(Partner[5]);
            break;
        case TRADE_STATUS_INITIATED:
            _worldPacket.FlushBits();
            _worldPacket << uint32(ID);
            break;
        case TRADE_STATUS_FAILED:
            _worldPacket.WriteBit(FailureForYou);
            _worldPacket.FlushBits();

            _worldPacket << int32(BagResult);
            _worldPacket << uint32(ItemID);
            break;
        case TRADE_STATUS_WRONG_REALM:
        case TRADE_STATUS_NOT_ON_TAPLIST:
            _worldPacket.FlushBits();
            _worldPacket << uint8(TradeSlot);
            break;
        case TRADE_STATUS_CURRENCY:
        case TRADE_STATUS_CURRENCY_NOT_TRADABLE:
            _worldPacket.FlushBits();
            // Only two test currencies in CurrencyTypes.dbc can be traded; the client never sends these
            _worldPacket << int32(CurrencyType);
            _worldPacket << int32(CurrencyQuantity);
            break;
        default:
            _worldPacket.FlushBits();
            break;
    }

    return &_worldPacket;
}

WorldPacket const* WorldPackets::Trade::TradeUpdated::Write()
{
    _worldPacket << uint32(ID);
    _worldPacket << int32(CurrencyType);
    _worldPacket << uint64(Gold);
    _worldPacket << int32(ProposedEnchantment);
    _worldPacket << uint32(ClientStateIndex);
    _worldPacket << int32(CurrencyQuantity);
    _worldPacket << uint8(WhichPlayer);
    _worldPacket << uint32(CurrentStateIndex);
    _worldPacket.WriteBits(Items.size(), 22);

    for (TradeItem const& item : Items)
    {
        _worldPacket.WriteBit(item.GiftCreator[7]);
        _worldPacket.WriteBit(item.GiftCreator[1]);
        _worldPacket.WriteBit(item.Unwrapped.has_value());
        _worldPacket.WriteBit(item.GiftCreator[3]);

        if (item.Unwrapped)
        {
            _worldPacket.WriteBit(item.Unwrapped->Creator[7]);
            _worldPacket.WriteBit(item.Unwrapped->Creator[1]);
            _worldPacket.WriteBit(item.Unwrapped->Creator[4]);
            _worldPacket.WriteBit(item.Unwrapped->Creator[6]);
            _worldPacket.WriteBit(item.Unwrapped->Creator[2]);
            _worldPacket.WriteBit(item.Unwrapped->Creator[3]);
            _worldPacket.WriteBit(item.Unwrapped->Creator[5]);
            _worldPacket.WriteBit(item.Unwrapped->Lock);
            _worldPacket.WriteBit(item.Unwrapped->Creator[0]);
        }

        _worldPacket.WriteBit(item.GiftCreator[6]);
        _worldPacket.WriteBit(item.GiftCreator[4]);
        _worldPacket.WriteBit(item.GiftCreator[2]);
        _worldPacket.WriteBit(item.GiftCreator[0]);
        _worldPacket.WriteBit(item.GiftCreator[5]);
    }

    _worldPacket.FlushBits();

    for (TradeItem const& item : Items)
    {
        if (item.Unwrapped)
        {
            _worldPacket.WriteByteSeq(item.Unwrapped->Creator[1]);

            _worldPacket << int32(item.Unwrapped->EnchantID);
            for (int32 socketEnchant : item.Unwrapped->SocketEnchant)
                _worldPacket << int32(socketEnchant);

            _worldPacket << uint32(item.Unwrapped->MaxDurability);

            _worldPacket.WriteByteSeq(item.Unwrapped->Creator[6]);
            _worldPacket.WriteByteSeq(item.Unwrapped->Creator[2]);
            _worldPacket.WriteByteSeq(item.Unwrapped->Creator[7]);
            _worldPacket.WriteByteSeq(item.Unwrapped->Creator[4]);

            _worldPacket << int32(item.Unwrapped->ReforgeEnchantmentID);
            _worldPacket << uint32(item.Unwrapped->Durability);
            _worldPacket << int32(item.Unwrapped->RandomPropertiesID);

            _worldPacket.WriteByteSeq(item.Unwrapped->Creator[3]);

            _worldPacket << int32(0); // unk

            _worldPacket.WriteByteSeq(item.Unwrapped->Creator[0]);

            _worldPacket << int32(item.Unwrapped->Charges);
            _worldPacket << int32(item.Unwrapped->RandomPropertiesSeed);

            _worldPacket.WriteByteSeq(item.Unwrapped->Creator[5]);
        }

        _worldPacket.WriteByteSeq(item.GiftCreator[6]);
        _worldPacket.WriteByteSeq(item.GiftCreator[1]);
        _worldPacket.WriteByteSeq(item.GiftCreator[7]);
        _worldPacket.WriteByteSeq(item.GiftCreator[4]);

        _worldPacket << int32(item.EntryID);

        _worldPacket.WriteByteSeq(item.GiftCreator[0]);

        _worldPacket << int32(item.StackCount);

        _worldPacket.WriteByteSeq(item.GiftCreator[5]);

        _worldPacket << uint8(item.Slot);

        _worldPacket.WriteByteSeq(item.GiftCreator[2]);
        _worldPacket.WriteByteSeq(item.GiftCreator[3]);
    }

    return &_worldPacket;
}
