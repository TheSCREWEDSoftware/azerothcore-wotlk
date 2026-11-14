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

/// \addtogroup u2w
/// @{
/// \file

#ifndef __WORLDSESSION_H
#define __WORLDSESSION_H

#include "AccountMgr.h"
#include "AddonMgr.h"
#include "AuthDefines.h"
#include "CircularBuffer.h"
#include "Common.h"
#include "DatabaseEnv.h"
#include "Duration.h"
#include "GossipDef.h"
#include "Packet.h"
#include "Position.h"
#include "QueryHolder.h"
#include "SharedDefines.h"
#include "World.h"
#include <map>
#include <memory>
#include <utility>

class BlackMarketEntry;
class CollectionMgr;
class Creature;
struct SpellCastRequest;
struct Signatures;
class GameObject;
class InstanceSave;
class Item;
class LoadPetFromDBQueryHolder;
class Object;
class Pet;
class Player;
class Quest;
class SpellCastTargets;
class Unit;
class Warden;
class WorldPacket;
class WorldSocket;
class AsynchPetSummon;
struct AreaTableEntry;
struct AuctionEntry;
struct BlackMarketTemplate;
struct DeclinedName;
struct ItemTemplate;
struct MovementInfo;
struct TradeStatusInfo;

namespace WorldPackets
{
    namespace Achievement
    {
        class GuildSetFocusedAchievement;
    }

    namespace AuctionHouse
    {
        class AuctionHelloRequest;
        class AuctionListBidderItems;
        class AuctionListItems;
        class AuctionListOwnerItems;
        class AuctionListPendingSales;
        class AuctionPlaceBid;
        class AuctionRemoveItem;
        class AuctionReplicateItems;
        class AuctionSellItem;
    }

    namespace Auth
    {
        enum class ConnectToSerial : uint32;
    }

    namespace Bank
    {
        class AutoBankItem;
        class AutoStoreBankItem;
        class BuyBankSlot;
    }

    namespace Battlefield
    {
        class BFMgrEntryInviteResponse;
        class BFMgrQueueInviteResponse;
        class BFMgrQueueExitRequest;
    }

    namespace Battleground
    {
        class AreaSpiritHealerQuery;
        class AreaSpiritHealerQueue;
        class HearthAndResurrect;
        class PVPLogDataRequest;
        class BattlemasterJoin;
        class BattlemasterJoinArena;
        class BattlefieldLeave;
        class BattlefieldPort;
        class BattlefieldListRequest;
        class GetPVPOptionsEnabled;
        class RequestBattlefieldStatus;
        class ReportPvPPlayerAFK;
        class RequestPVPRewards;
        class RequestRatedBattlefieldInfo;
    }

    namespace Battlenet
    {
        class Request;
        class RequestRealmListTicket;
    }

    namespace BattlePet
    {
        class BattlePetRequestJournal;
        class BattlePetSetBattleSlot;
        class BattlePetModifyName;
        class BattlePetDeletePet;
        class BattlePetSetFlags;
        class BattlePetSummon;
        class CageBattlePet;
    }

    namespace BlackMarket
    {
        class BlackMarketOpen;
        class BlackMarketRequestItems;
        class BlackMarketBidOnItem;
        class BlackMarketOutbid;
    }

    namespace Calendar
    {
        class CalendarAddEvent;
        class CalendarCopyEvent;
        class CalendarEventInvite;
        class CalendarEventModeratorStatus;
        class CalendarEventRSVP;
        class CalendarEventSignUp;
        class CalendarEventStatus;
        class CalendarGetCalendar;
        class CalendarGetEvent;
        class CalendarGetNumPending;
        class CalendarGuildFilter;
        class CalendarRemoveEvent;
        class CalendarRemoveInvite;
        class CalendarUpdateEvent;
        class SetSavedInstanceExtend;
        class CalendarComplain;
    }

    namespace Character
    {
        struct CharacterCreateInfo;
        struct CharacterRenameInfo;
        struct CharCustomizeInfo;
        struct CharRaceOrFactionChangeInfo;
        struct CharacterUndeleteInfo;

        class AlterApperance;
        class EnumCharacters;
        class CreateCharacter;
        class CharDelete;
        class CharacterRenameRequest;
        class CharCustomize;
        class CharRaceOrFactionChange;
        class GenerateRandomCharacterName;
        class GetUndeleteCharacterCooldownStatus;
        class ReorderCharacters;
        class UndeleteCharacter;
        class PlayerLogin;
        class LogoutRequest;
        class LogoutCancel;
        class LoadingScreenNotify;
        class SetActionBarToggles;
        class RequestPlayedTime;
        class ShowingCloak;
        class ShowingHelm;
        class SetTitle;
        class SetFactionAtWar;
        class SetFactionNotAtWar;
        class SetFactionInactive;
        class SetWatchedFaction;
        class SetPlayerDeclinedNames;

        enum class LoginFailureReason : uint8;
    }

    namespace ClientConfig
    {
        class RequestAccountData;
        class UserClientUpdateAccountData;
        class SetAdvancedCombatLogging;
    }

    namespace Channel
    {
        class ChannelPlayerCommand;
        class JoinChannel;
        class LeaveChannel;
    }

    namespace Chat
    {
        class ChatMessage;
        class ChatMessageWhisper;
        class ChatMessageChannel;
        class ChatAddonMessage;
        class ChatAddonMessageWhisper;
        class ChatAddonMessageChannel;
        class ChatMessageAFK;
        class ChatMessageDND;
        class ChatMessageEmote;
        class CTextEmote;
        class EmoteClient;
        class ChatRegisterAddonPrefixes;
        class ChatUnregisterAllAddonPrefixes;
        class ChatReportIgnored;
    }

    namespace Combat
    {
        class AttackSwing;
        class AttackStop;
        class SetSheathed;
    }

    namespace Duel
    {
        class CanDuel;
        class DuelResponse;
    }

    namespace EquipmentSet
    {
        class SaveEquipmentSet;
        class DeleteEquipmentSet;
        class UseEquipmentSet;
    }

    namespace GameObject
    {
        class GameObjReportUse;
        class GameObjUse;
    }

    namespace Garrison
    {
        class GetGarrisonInfo;
        class GarrisonPurchaseBuilding;
        class GarrisonCancelConstruction;
        class GarrisonRequestBlueprintAndSpecializationData;
        class GarrisonGetBuildingLandmarks;
    }

    namespace Guild
    {
        class QueryGuildInfo;
        class GuildInviteByName;
        class AcceptGuildInvite;
        class DeclineGuildInvites;
        class GuildDeclineInvitation;
        class GuildGetRoster;
        class GuildPromoteMember;
        class GuildDemoteMember;
        class GuildOfficerRemoveMember;
        class GuildAssignMemberRank;
        class GuildLeave;
        class GuildDelete;
        class GuildUpdateMotdText;
        class GuildGetRanks;
        class GuildAddRank;
        class GuildDeleteRank;
        class GuildUpdateInfoText;
        class GuildSetMemberNote;
        class GuildEventLogQuery;
        class GuildBankRemainingWithdrawMoneyQuery;
        class GuildPermissionsQuery;
        class GuildSetRankPermissions;
        class GuildBankActivate;
        class GuildBankQueryTab;
        class GuildBankDepositMoney;
        class GuildBankWithdrawMoney;
        class GuildBankSwapItems;
        class GuildBankBuyTab;
        class GuildBankUpdateTab;
        class GuildBankLogQuery;
        class GuildBankTextQuery;
        class GuildBankSetTabText;
        class RequestGuildPartyState;
        class RequestGuildRewardsList;
        class GuildQueryNews;
        class GuildNewsUpdateSticky;
        class GuildSetGuildMaster;
        class GuildChallengeUpdateRequest;
        class SaveGuildEmblem;
        class GuildSetAchievementTracking;
    }

    namespace GuildFinder
    {
        class LFGuildAddRecruit;
        class LFGuildBrowse;
        class LFGuildDeclineRecruit;
        class LFGuildGetApplications;
        class LFGuildGetGuildPost;
        class LFGuildGetRecruits;
        class LFGuildRemoveRecruit;
        class LFGuildSetGuildPost;
    }

    namespace Inspect
    {
        class Inspect;
        class InspectPVPRequest;
        class QueryInspectAchievements;
        class RequestHonorStats;
    }

    namespace Instance
    {
        class InstanceInfo;
        class InstanceLockResponse;
        class ResetInstances;
    }

    namespace LFG
    {
        class DFBootPlayerVote;
        class DFGetJoinStatus;
        class DFGetSystemInfo;
        class DFJoin;
        class DFLeave;
        class DFProposalResponse;
        class DFSetRoles;
        class DFTeleport;
    }

    namespace Item
    {
        class AutoEquipItem;
        class AutoEquipItemSlot;
        class AutoStoreBagItem;
        class BuyItem;
        class BuyBackItem;
        class DestroyItem;
        class GetItemPurchaseData;
        class ItemPurchaseRefund;
        class RepairItem;
        class ReadItem;
        class SellItem;
        class SplitItem;
        class SwapInvItem;
        class SwapItem;
        class WrapItem;
        class CancelTempEnchantment;
        class TransmogrifyItems;
        class UseCritterItem;
        class UpgradeItem;
        class SocketGems;
        struct ItemInstance;
    }

    namespace Loot
    {
        class LootUnit;
        class LootItem;
        class LootRelease;
        class LootMoney;
        class LootRoll;
        class SetLootSpecialization;
    }

    namespace Mail
    {
        class MailCreateTextItem;
        class MailDelete;
        class MailGetList;
        class MailMarkAsRead;
        class MailQueryNextMailTime;
        class MailReturnToSender;
        class MailTakeItem;
        class MailTakeMoney;
        class SendMail;
    }

    namespace Misc
    {
        class AreaTrigger;
        class SetSelection;
        class ViolenceLevel;
        class TimeSyncResponse;
        class TutorialSetFlag;
        class SetDungeonDifficulty;
        class SetRaidDifficulty;
        class PortGraveyard;
        class ReclaimCorpse;
        class RepopRequest;
        class RequestCemeteryList;
        class ResurrectResponse;
        class StandStateChange;
        class UITimeRequest;
        class RandomRollClient;
        class ObjectUpdateFailed;
        class ObjectUpdateRescued;
        class CompleteCinematic;
        class NextCinematicCamera;
        class FarSight;
        class LoadCUFProfiles;
        class SaveCUFProfiles;
        class OpeningCinematic;
        class TogglePvP;
        class SetPvP;
        class WorldTeleport;
        class MountSpecial;
        class SetTaxiBenchmarkMode;
    }

    namespace Movement
    {
        class ClientPlayerMovement;
        class WorldPortResponse;
        class MoveTeleportAck;
        class MovementAckMessage;
        class MovementSpeedAck;
        class SetActiveMover;
        class MoveSetCollisionHeightAck;
        class MoveTimeSkipped;
        class SummonResponse;
        class MoveSplineDone;
        class SuspendTokenResponse;
    }

    namespace NPC
    {
        class Hello;
        class GossipSelectOption;
        class SpiritHealerActivate;
        class TrainerBuySpell;
        class RequestStabledPets;
    }

    namespace Party
    {
        class PartyCommandResult;
        class PartyInviteClient;
        class PartyInvite;
        class PartyInviteResponse;
        class PartyUninvite;
        class GroupDecline;
        class RequestPartyMemberStats;
        class PartyMemberStats;
        class SetPartyLeader;
        class SetPartyAssignment;
        class SetRole;
        class RoleChangedInform;
        class SetLootMethod;
        class LeaveGroup;
        class MinimapPingClient;
        class MinimapPing;
        class UpdateRaidTarget;
        class SendRaidTargetUpdateSingle;
        class SendRaidTargetUpdateAll;
        class ConvertRaid;
        class RequestPartyJoinUpdates;
        class SetAssistantLeader;
        class DoReadyCheck;
        class ReadyCheckStarted;
        class ReadyCheckResponseClient;
        class ReadyCheckResponse;
        class ReadyCheckCompleted;
        class RequestRaidInfo;
        class OptOutOfLoot;
        class InitiateRolePoll;
        class RolePollInform;
        class GroupNewLeader;
        class PartyUpdate;
        class SetEveryoneIsAssistant;
        class ChangeSubGroup;
        class SwapSubGroups;
        class RaidMarkersChanged;
        class ClearRaidMarker;
    }

    namespace Pet
    {
        class DismissCritter;
        class RequestPetInfo;
        class PetAbandon;
        class PetStopAttack;
        class PetSpellAutocast;
        class PetRename;
        class PetAction;
        class PetCancelAura;
        class PetSetAction;
        class LearnPetSpecializationGroup;
        class SetPetSlot;
    }

    namespace Petition
    {
        class DeclinePetition;
        class OfferPetition;
        class PetitionBuy;
        class PetitionRenameGuild;
        class PetitionShowList;
        class PetitionShowSignatures;
        class QueryPetition;
        class SignPetition;
        class TurnInPetition;
    }

    namespace Query
    {
        class QueryCreature;
        class QueryPlayerName;
        class QueryPageText;
        class QueryNPCText;
        class DBQueryBulk;
        class QueryGameObject;
        class QueryCorpseLocationFromClient;
        class QueryCorpseTransport;
        class QueryTime;
        class QueryPetName;
        class QuestPOIQuery;
        class QueryQuestCompletionNPCs;
        class ItemTextQuery;
    }

    namespace Quest
    {
        class QuestConfirmAccept;
        class QuestGiverStatusQuery;
        class QuestGiverStatusMultipleQuery;
        class QuestGiverHello;
        class QueryQuestInfo;
        class QuestGiverChooseReward;
        class QuestGiverCompleteQuest;
        class QuestGiverRequestReward;
        class QuestGiverQueryQuest;
        class QuestGiverAcceptQuest;
        class QuestLogRemoveQuest;
        class QuestPushResult;
        class PushQuestToParty;
    }

    namespace RaF
    {
        class AcceptLevelGrant;
        class GrantLevel;
    }

    namespace Reputation
    {
        class RequestForcedReactions;
    }

    namespace Toy
    {
        class AccountToysUpdate;
        class AddToy;
        class ToySetFavorite;
        class UseToy;
    }

    namespace Scenes
    {
        class SceneTriggerEvent;
        class ScenePlaybackComplete;
        class ScenePlaybackCanceled;
    }

    namespace Social
    {
        class AddFriend;
        class AddIgnore;
        class DelFriend;
        class DelIgnore;
        class SendContactList;
        class SetContactNotes;
    }

    namespace Spells
    {
        class CancelAura;
        class CancelAutoRepeatSpell;
        class CancelChannelling;
        class CancelGrowthAura;
        class CancelMountAura;
        class PetCancelAura;
        class RequestCategoryCooldowns;
        class CancelCast;
        class CastSpell;
        class PetCastSpell;
        class UseItem;
        class OpenItem;
        class SetActionButton;
        class UnlearnSkill;
        class SelfRes;
        class GetMirrorImageData;
        class SpellClick;
        class MissileTrajectoryCollision;
        class UpdateMissileTrajectory;
    }

    namespace Talent
    {
        class SetSpecialization;
        class LearnTalents;
        class ConfirmRespecWipe;
    }

    namespace Taxi
    {
        class ShowTaxiNodes;
        class TaxiNodeStatusQuery;
        class EnableTaxiNode;
        class TaxiQueryAvailableNodes;
        class ActivateTaxi;
        class TaxiRequestEarlyLanding;
    }

    namespace Ticket
    {
        class GMTicketGetSystemStatus;
        class GMTicketGetCaseStatus;
        class SupportTicketSubmitBug;
        class SupportTicketSubmitSuggestion;
        class SupportTicketSubmitComplaint;
        class BugReport;
        class Complaint;
    }

    namespace Token
    {
        class UpdateListedAuctionableTokens;
        class RequestWowTokenMarketPrice;
    }

    namespace Totem
    {
        class TotemDestroyed;
    }

    namespace Trade
    {
        class AcceptTrade;
        class BeginTrade;
        class BusyTrade;
        class CancelTrade;
        class ClearTradeItem;
        class IgnoreTrade;
        class InitiateTrade;
        class SetTradeCurrency;
        class SetTradeGold;
        class SetTradeItem;
        class UnacceptTrade;
        class TradeStatus;
    }

    namespace Vehicle
    {
        class MoveDismissVehicle;
        class RequestVehiclePrevSeat;
        class RequestVehicleNextSeat;
        class MoveChangeVehicleSeats;
        class RequestVehicleSwitchSeat;
        class RideVehicleInteract;
        class EjectPassenger;
        class RequestVehicleExit;
        class MoveSetVehicleRecIdAck;
    }

    namespace Voice
    {
        class VoiceSessionEnable;
        class SetActiveVoiceChannel;
    }

    namespace VoidStorage
    {
        class UnlockVoidStorage;
        class QueryVoidStorage;
        class VoidStorageTransfer;
        class SwapVoidItem;
    }

    namespace Warden
    {
        class WardenData;
    }

    namespace Who
    {
        class WhoIsRequest;
        class WhoRequestPkt;
    }

    class Null final : public ClientPacket
    {
    public:
        Null(WorldPacket&& packet) : ClientPacket(std::move(packet)) { }

        void Read() override { _worldPacket.rfinish(); }
    };
}

namespace lfg
{
    struct LfgJoinResultData;
    struct LfgPlayerBoot;
    struct LfgProposal;
    struct LfgQueueStatusData;
    struct LfgPlayerRewardData;
    struct LfgRoleCheck;
    struct LfgUpdateData;
}

enum AccountDataType
{
    GLOBAL_CONFIG_CACHE             = 0,                    // 0x01 g
    PER_CHARACTER_CONFIG_CACHE      = 1,                    // 0x02 p
    GLOBAL_BINDINGS_CACHE           = 2,                    // 0x04 g
    PER_CHARACTER_BINDINGS_CACHE    = 3,                    // 0x08 p
    GLOBAL_MACROS_CACHE             = 4,                    // 0x10 g
    PER_CHARACTER_MACROS_CACHE      = 5,                    // 0x20 p
    PER_CHARACTER_LAYOUT_CACHE      = 6,                    // 0x40 p
    PER_CHARACTER_CHAT_CACHE        = 7,                    // 0x80 p
};

#define NUM_ACCOUNT_DATA_TYPES        8

#define GLOBAL_CACHE_MASK           0x15
#define PER_CHARACTER_CACHE_MASK    0xEA

struct AccountData
{
    AccountData() :  Data("") {}

    time_t Time{0};
    std::string Data;
};

enum PartyOperation
{
    PARTY_OP_INVITE = 0,
    PARTY_OP_UNINVITE = 1,
    PARTY_OP_LEAVE = 2,
    PARTY_OP_SWAP = 4
};

enum TutorialAction : uint8
{
    TUTORIAL_ACTION_UPDATE  = 0,
    TUTORIAL_ACTION_CLEAR   = 1,
    TUTORIAL_ACTION_RESET   = 2
};

enum BarberShopResult
{
    BARBER_SHOP_RESULT_SUCCESS      = 0,
    BARBER_SHOP_RESULT_NO_MONEY     = 1,
    BARBER_SHOP_RESULT_NOT_ON_CHAIR = 2,
    BARBER_SHOP_RESULT_NO_MONEY_2   = 3
};

enum BFLeaveReason
{
    BF_LEAVE_REASON_CLOSE     = 0x00000001,
    //BF_LEAVE_REASON_UNK1      = 0x00000002, (not used)
    //BF_LEAVE_REASON_UNK2      = 0x00000004, (not used)
    BF_LEAVE_REASON_EXITED    = 0x00000008,
    BF_LEAVE_REASON_LOW_LEVEL = 0x00000010,
};

enum ChatRestrictionType
{
    ERR_CHAT_RESTRICTED = 0,
    ERR_CHAT_THROTTLED  = 1,
    ERR_USER_SQUELCHED  = 2,
    ERR_YELL_RESTRICTED = 3
};

enum DeclinedNameResult
{
    DECLINED_NAMES_RESULT_SUCCESS = 0,
    DECLINED_NAMES_RESULT_ERROR = 1
};

enum CharterTypes
{
    GUILD_CHARTER_TYPE                            = 9,
    ARENA_TEAM_CHARTER_2v2_TYPE                   = 2,
    ARENA_TEAM_CHARTER_3v3_TYPE                   = 3,
    ARENA_TEAM_CHARTER_5v5_TYPE                   = 5
};

class LoginQueryHolder : public CharacterDatabaseQueryHolder
{
public:
    LoginQueryHolder(uint32 accountId, ObjectGuid guid);

    ObjectGuid GetGuid() const { return _guid; }
    uint32 GetAccountId() const { return _accountId; }
    bool Initialize();

private:
    uint32 _accountId;
    ObjectGuid _guid;
};

constexpr Seconds PLAY_TIME_LIMIT_APPROACHING_PARTIAL = Hours(2) + Minutes(30);
constexpr Seconds PLAY_TIME_LIMIT_PARTIAL             = Hours(3);
constexpr Seconds PLAY_TIME_LIMIT_APPROACHING_FULL    = Hours(4) + Minutes(30);
constexpr Seconds PLAY_TIME_LIMIT_FULL                = Hours(5);

enum PlayTimeFlag : uint32
{
    PTF_APPROACHING_PARTIAL_PLAY_TIME = 0x1000,
    PTF_APPROACHING_NO_PLAY_TIME      = 0x2000,
    PTF_UNK_1                         = 0x20000000,
    PTF_UNK_2                         = 0x40000000,
    PTF_UNHEALTHY_TIME                = 0x80000000,
};

//class to deal with packet processing
//allows to determine if next packet is safe to be processed
class PacketFilter
{
public:
    explicit PacketFilter(WorldSession* pSession) : m_pSession(pSession) {}
    virtual ~PacketFilter() = default;

    virtual bool Process(WorldPacket* /*packet*/) { return true; }
    [[nodiscard]] virtual bool ProcessUnsafe() const { return true; }

protected:
    WorldSession* const m_pSession;
};
//process only thread-safe packets in Map::Update()
class MapSessionFilter : public PacketFilter
{
public:
    explicit MapSessionFilter(WorldSession* pSession) : PacketFilter(pSession) {}
    ~MapSessionFilter() override = default;

    bool Process(WorldPacket* packet) override;
    //in Map::Update() we do not process player logout!
    [[nodiscard]] bool ProcessUnsafe() const override { return false; }
};

//class used to filer only thread-unsafe packets from queue
//in order to update only be used in World::UpdateSessions()
class WorldSessionFilter : public PacketFilter
{
public:
    explicit WorldSessionFilter(WorldSession* pSession) : PacketFilter(pSession) {}
    ~WorldSessionFilter() override = default;

    bool Process(WorldPacket* packet) override;
};

// Proxy structure to contain data passed to callback function,
// only to prevent bloating the parameter list
class CharacterCreateInfo
{
    friend class WorldSession;
    friend class Player;

public:
    explicit CharacterCreateInfo(std::string name = "", uint8 race = 0, uint8 playerClass = 0,
        uint8 gender = GENDER_NONE, uint8 skin = 0, uint8 face = 0, uint8 hairStyle = 0, uint8 hairColor = 0,
        uint8 facialHair = 0)
        : Name(std::move(name)), Race(race), Class(playerClass), Gender(gender), Skin(skin), Face(face),
        HairStyle(hairStyle), HairColor(hairColor), FacialHair(facialHair) { }

protected:
    /// User specified variables
    std::string Name;
    uint8 Race = 0;
    uint8 Class = 0;
    uint8 Gender = GENDER_NONE;
    uint8 Skin = 0;
    uint8 Face = 0;
    uint8 HairStyle = 0;
    uint8 HairColor = 0;
    uint8 FacialHair = 0;
    uint8 OutfitId = 0;

    /// Server side data
    uint8 CharCount = 0;
};

struct CharacterRenameInfo
{
    friend class WorldSession;

protected:
    ObjectGuid Guid;
    std::string Name;
};

struct CharacterCustomizeInfo : public CharacterRenameInfo
{
    friend class Player;
    friend class WorldSession;

protected:
    uint8 Gender = GENDER_NONE;
    uint8 Skin = 0;
    uint8 Face = 0;
    uint8 HairStyle = 0;
    uint8 HairColor = 0;
    uint8 FacialHair = 0;
};

struct CharacterFactionChangeInfo : public CharacterCustomizeInfo
{
    friend class Player;
    friend class WorldSession;

protected:
    uint8 Race = 0;
    bool FactionChange = false;
};

struct PacketCounter
{
    time_t lastReceiveTime;
    uint16 amountCounter;
};

/// Player session in the World
namespace WorldPackets::Auth
{
    enum class ConnectToSerial : uint32;
}

class WorldSession
{
public:
    /// key of the 6.x instance connection (SMSG_CONNECT_TO / CMSG_AUTH_CONTINUED_SESSION)
    union ConnectToKey
    {
        struct
        {
            uint64 AccountId : 32;
            uint64 ConnectionType : 1;
            uint64 Key : 31;
        } Fields;

        uint64 Raw;
    };

    WorldSession(uint32 id, std::string&& name, uint32 accountFlags, std::shared_ptr<WorldSocket> sock,
        AccountTypes sec, uint8 expansion, time_t mute_time, LocaleConstant locale, uint32 recruiter, bool isARecruiter,
        bool skipQueue, uint32 TotalTime);
    ~WorldSession();

    uint32 GetAccountFlags() const { return _accountFlags; }
    bool HasAccountFlag(uint32 flag) const { return (_accountFlags & flag) != 0; }
    void UpdateAccountFlag(uint32 flag, bool remove = false);
    void ValidateAccountFlags();

    bool IsGMAccount() const;
    bool IsTrialAccount() const;
    bool IsInternetGameRoomAccount() const;
    bool IsRecurringBillingAccount() const;
    bool IsAffectedByCAIS() const;

    uint8 GetBillingPlanFlags() const;

    bool PlayerLoading() const { return m_playerLoading; }
    bool PlayerLogout() const { return m_playerLogout; }
    bool PlayerRecentlyLoggedOut() const { return m_playerRecentlyLogout; }
    bool PlayerLogoutWithSave() const { return m_playerLogout && m_playerSave; }

    void ReadAddonsInfo(ByteBuffer& data);
    void SendAddonsInfo();

    void SanitizeMovementFlags(MovementInfo* mi);
    void WriteMovementInfo(WorldPacket* data, MovementInfo* mi);
    void SynchronizeMovement(MovementInfo& movementInfo);
    void HandleMoverRelocation(MovementInfo& movementInfo, Unit* mover);
    bool VerifyMovementInfo(MovementInfo const& movementInfo, Player* plrMover, Unit* mover, OpcodeClient opcode) const;
    bool ProcessMovementInfo(MovementInfo& movementInfo, Unit* mover, Player* plrMover, WorldPacket& recvData);
    bool ProcessMovementInfo(MovementInfo& movementInfo, Unit* mover, Player* plrMover, OpcodeClient opcode);

    void SendPacket(WorldPacket const* packet);
    void SendPetNameInvalid(uint32 error, std::string const& name, DeclinedName* declinedName, uint32 petNumber = 0);
    void SendPartyResult(PartyOperation operation, std::string const& member, PartyResult res, uint32 val = 0);

    void SendAreaTriggerMessage(std::string_view str);

    template<typename... Args>
    void SendAreaTriggerMessage(char const* fmt, Args&&... args)
    {
        if (!m_playerLoading)
            SendAreaTriggerMessage(Acore::StringFormat(fmt, std::forward<Args>(args)...));
    }
    template<typename... Args>
    void SendAreaTriggerMessage(uint32 strId, Args&&... args)
    {
        if (!m_playerLoading)
            SendAreaTriggerMessage(Acore::StringFormat(GetAcoreString(strId), std::forward<Args>(args)...));
    }

    void SendTimeQueryResponse();

    /// code is a BattlenetRpcErrorCode; queued sends the login queue position instead of the account information
    void SendAuthResponse(uint32 code, bool queued, uint32 queuePos = 0);
    void SendClientCacheVersion(uint32 version);

    // 6.x instance connection
    void SendConnectToInstance(WorldPackets::Auth::ConnectToSerial serial);
    void AddInstanceConnection(std::shared_ptr<WorldSocket> sock) { m_Socket[CONNECTION_TYPE_INSTANCE] = sock; }
    uint64 GetConnectToInstanceKey() const { return _instanceConnectKey.Raw; }
    /// realm address of this realm for the 6.x client (region << 24 | battlegroup << 16 | realm)
    uint32 GetVirtualRealmAddress() const;

    AccountTypes GetSecurity() const { return _security; }
    bool CanSkipQueue() const { return _skipQueue; }

    // RBAC
    rbac::RBACData* GetRBACData() const { return _RBACData; }
    bool HasPermission(uint32 permissionId);
    void LoadPermissions();
    QueryCallback LoadPermissionsAsync();
    void InvalidateRBACData();

    /// For unit testing - initializes RBAC data without database access
    void InitRBACDataForTest();
    uint32 GetAccountId() const { return _accountId; }
    /// 6.x: the Battle.net account of this game account (0 when not linked)
    uint32 GetBattlenetAccountId() const { return _battlenetAccountId; }
    ObjectGuid GetAccountGUID() const { return ObjectGuid::Create<HighGuid::WowAccount>(GetAccountId()); }
    ObjectGuid GetBattlenetAccountGUID() const { return ObjectGuid::Create<HighGuid::BNetAccount>(GetBattlenetAccountId()); }
    CollectionMgr* GetCollectionMgr() const { return _collectionMgr.get(); }
    void SetBattlenetAccountId(uint32 battlenetAccountId) { _battlenetAccountId = battlenetAccountId; }
    Player* GetPlayer() const { return _player; }
    std::string const& GetPlayerName() const;
    std::string GetPlayerInfo() const;

    uint32 GetCurrentVendor() const { return m_currentVendorEntry; }
    void SetCurrentVendor(uint32 vendorEntry) { m_currentVendorEntry = vendorEntry; }

    ObjectGuid::LowType GetGuidLow() const;
    void SetSecurity(AccountTypes security) { _security = security; }
    std::string const& GetRemoteAddress() { return m_Address; }
    void SetPlayer(Player* player);
    uint8 Expansion() const { return m_expansion; }

    void SetTotalTime(uint32 TotalTime) { m_total_time = TotalTime; }
    uint32 GetTotalTime() const { return m_total_time; }

    void InitWarden(SessionKey const&, std::string const& os);
    Warden* GetWarden();

    /// Session in auth.queue currently
    void SetInQueue(bool state) { m_inQueue = state; }

    // Playtime limit
    Seconds GetCreateTime() const { return _createTime; }
    // Measured from session creation (authentication), so login queue and character select count
    // toward the limits. Matches the VMaNGOS behaviour this is ported from.
    Seconds GetConsecutivePlayTime(Seconds now) const { return (now - _createTime) + _previousPlayTime; }
    Seconds GetPreviousPlayedTime() const { return _previousPlayTime; }
    void SetPreviousPlayedTime(Seconds playedTime) { _previousPlayTime = playedTime; }
    void CheckPlayedTimeLimit(Seconds now);
    void SendPlayTimeWarning(PlayTimeFlag flag, int32 playTimeRemaining);

    /// Is the user engaged in a log out process?
    bool IsLoggingOut() const { return _logoutTime || m_playerLogout; }

    /// Engage the logout process for the user
    void SetLogoutStartTime(time_t requestTime)
    {
        _logoutTime = requestTime;
    }

    /// Is logout cooldown expired?
    bool ShouldLogOut(time_t currTime) const
    {
        return (_logoutTime > 0 && currTime >= _logoutTime + 20);
    }

    void LogoutPlayer(bool save, bool redirecting = false);
    void KickPlayer(bool setKicked = true) { return this->KickPlayer("Unknown reason", setKicked); }
    void KickPlayer(std::string const& reason, bool setKicked = true);

    // Returns true if all contained hyperlinks are valid
    // May kick player on false depending on world config (handler should abort)
    bool ValidateHyperlinksAndMaybeKick(std::string_view str);
    // Returns true if the message contains no hyperlinks
    // May kick player on false depending on world config (handler should abort)
    bool DisallowHyperlinksAndMaybeKick(std::string_view str);

    void QueuePacket(WorldPacket* new_packet);
    bool Update(uint32 diff, PacketFilter& updater);

    /// Handle the authentication waiting queue (to be completed)
    void SendAuthWaitQueue(uint32 position);

    //void SendTestCreatureQueryOpcode(uint32 entry, ObjectGuid guid, uint32 testvalue);
    void SendNameQueryOpcode(ObjectGuid guid);

    void SendTrainerList(Creature* npc);
    void SendListInventory(ObjectGuid guid, uint32 vendorEntry = 0);
    void SendShowBank(ObjectGuid guid);
    bool CanOpenMailBox(ObjectGuid guid);
    void SendShowMailBox(ObjectGuid guid);
    void SendMailList();
    void SendTabardVendorActivate(ObjectGuid guid);
    void SendSpiritResurrect();
    void SendBindPoint(Creature* npc);

    void SendBattleGroundList(ObjectGuid guid, BattlegroundTypeId bgTypeId = BATTLEGROUND_RB);

    void SendTradeStatus(TradeStatusInfo const& info);
    void SendUpdateTrade(bool trader_data = true);
    void SendCancelTrade(TradeStatus status);

    void SendPetitionQueryOpcode(ObjectGuid petitionguid);

    // Spell
    void ApplyCastRequestResearchData(SpellCastRequest const& request);

    // Pet
    void SendQueryPetNameResponse(ObjectGuid guid);
    void SendStablePet(ObjectGuid guid);
    void SendStableResult(uint8 guid);
    void SendPetSlotUpdated(int32 petNumberA, int32 petSlotA, int32 petNumberB, int32 petSlotB);
    bool CheckStableMaster(ObjectGuid guid);

    // Account Data
    AccountData* GetAccountData(AccountDataType type) { return &m_accountData[type]; }
    void SetAccountData(AccountDataType type, time_t tm, std::string const& data);
    void SendAccountDataTimes(uint32 mask);
    void LoadAccountData(PreparedQueryResult result, uint32 mask);

    void LoadTutorialsData(PreparedQueryResult result);
    void SendTutorialsData();
    void SaveTutorialsData(CharacterDatabaseTransaction trans);
    uint32 GetTutorialInt(uint8 index) const { return m_Tutorials[index]; }
    void SetTutorialInt(uint8 index, uint32 value)
    {
        if (m_Tutorials[index] != value)
        {
            m_Tutorials[index] = value;
            m_TutorialsChanged = true;
        }
    }
    //auction
    void SendAuctionHello(ObjectGuid guid, Creature* unit);
    void SendAuctionCommandResult(AuctionEntry const* auction, uint32 action, uint32 errorCode, uint32 bidError = 0);
    void SendAuctionOutBidNotification(AuctionEntry const* auction, Item const* item, uint32 newBid);
    void SendAuctionClosedNotification(AuctionEntry const* auction, float mailDelay, bool sold, Item const* item);
    void SendAuctionWonNotification(AuctionEntry const* auction, Item const* item);
    void SendAuctionOwnerBidNotification(AuctionEntry const* auction, Item const* item);

    //Item Enchantment
    void SendEnchantmentLog(ObjectGuid target, ObjectGuid caster, ObjectGuid itemGuid, uint32 itemId, uint32 enchantId, uint32 slot);
    void SendItemEnchantTimeUpdate(ObjectGuid Playerguid, ObjectGuid Itemguid, uint32 slot, uint32 Duration);

    //Taxi
    void SendTaxiStatus(ObjectGuid guid);
    void SendTaxiMenu(Creature* unit);
    void SendDoFlight(uint32 mountDisplayId, uint32 path, uint32 pathNode = 0);
    bool SendLearnNewTaxiNode(Creature* unit);
    void SendDiscoverNewTaxiNode(uint32 nodeid);

    // Guild/Arena Team
    void SendArenaTeamCommandResult(uint32 team_action, std::string const& team, std::string const& player, uint32 error_id = 0);
    void SendNotInArenaTeamPacket(uint8 type);
    void SendPetitionShowList(ObjectGuid guid);
    static WorldPacket BuildPetitionShowSignatures(ObjectGuid petitionGuid, ObjectGuid ownerGuid, uint32 petitionId, Signatures const* signatures);
    static WorldPacket BuildPetitionSignResults(ObjectGuid petitionGuid, ObjectGuid playerGuid, uint32 result);
    void SendTurnInPetitionResult(uint32 result);

    void DoLootRelease(ObjectGuid lguid);

    // Account mute time
    time_t m_muteTime;

    // Locales
    LocaleConstant GetSessionDbcLocale() const { return m_sessionDbcLocale; }
    LocaleConstant GetSessionDbLocaleIndex() const { return m_sessionDbLocaleIndex; }
    void SetSessionDbLocaleIndex(LocaleConstant locale);
    std::string GetAcoreString(uint32 entry) const;
    std::string const* GetModuleString(std::string module, uint32 id) const;

    uint32 GetLatency() const { return m_latency; }
    void SetLatency(uint32 latency) { m_latency = latency; }

    std::atomic<time_t> m_timeOutTime;
    void UpdateTimeOutTime(uint32 diff)
    {
        if (time_t(diff) > m_timeOutTime)
            m_timeOutTime = 0;
        else
            m_timeOutTime -= diff;
    }
    void ResetTimeOutTime(bool onlyActive)
    {
        if (GetPlayer())
            m_timeOutTime = int32(sWorld->getIntConfig(CONFIG_SOCKET_TIMEOUTTIME_ACTIVE));
        else if (!onlyActive)
            m_timeOutTime = int32(sWorld->getIntConfig(CONFIG_SOCKET_TIMEOUTTIME));
    }
    bool IsConnectionIdle() const
    {
        return (m_timeOutTime <= 0 && !m_inQueue);
    }

    // Recruit-A-Friend Handling
    uint32 GetRecruiterId() const { return recruiterId; }
    bool IsARecruiter() const { return isRecruiter; }

    // Packets cooldown
    time_t GetCalendarEventCreationCooldown() const { return _calendarEventCreationCooldown; }
    void SetCalendarEventCreationCooldown(time_t cooldown) { _calendarEventCreationCooldown = cooldown; }

    // Time Synchronisation
    void ResetTimeSync();
    void SendTimeSync();

    // Movement packet order
    uint32 GetOrderCounter() const { return _orderCounter; }
    void IncrementOrderCounter() { ++_orderCounter; }
public:                                                 // opcodes handlers
    // 6.2.4 opcode handlers, from TrinityCore 6.x (src/server/game/Server/WorldSession.h)
    void Handle_NULL(WorldPackets::Null& null);          // not used
    void Handle_EarlyProccess(WorldPacket& recvPacket); // just mark packets processed in WorldSocket::OnRead
    void HandleCharEnumOpcode(WorldPackets::Character::EnumCharacters& /*enumCharacters*/);
    void HandleCharUndeleteEnum(PreparedQueryResult result);
    void HandleCharUndeleteEnumOpcode(WorldPackets::Character::EnumCharacters& /*enumCharacters*/);
    void HandleCharDeleteOpcode(WorldPackets::Character::CharDelete& charDelete);
    void HandleCharCreateOpcode(WorldPackets::Character::CreateCharacter& charCreate);
    void HandleCharCreateCallback(PreparedQueryResult result, WorldPackets::Character::CharacterCreateInfo* createInfo);
    void HandlePlayerLoginOpcode(WorldPackets::Character::PlayerLogin& playerLogin);
    void HandleLoadScreenOpcode(WorldPackets::Character::LoadingScreenNotify& loadingScreenNotify);
    void HandlePlayerLogin(LoginQueryHolder * holder);
    void HandleCharRenameOpcode(WorldPackets::Character::CharacterRenameRequest& request);
    void HandleSetPlayerDeclinedNames(WorldPackets::Character::SetPlayerDeclinedNames& packet);
    void HandleAlterAppearance(WorldPackets::Character::AlterApperance& packet);
    void HandleCharCustomizeOpcode(WorldPackets::Character::CharCustomize& packet);
    void HandleCharRaceOrFactionChangeOpcode(WorldPackets::Character::CharRaceOrFactionChange& packet);
    void HandleCharRaceOrFactionChangeCallback(PreparedQueryResult result, WorldPackets::Character::CharRaceOrFactionChangeInfo* factionChangeInfo);
    void HandleRandomizeCharNameOpcode(WorldPackets::Character::GenerateRandomCharacterName& packet);
    void HandleReorderCharacters(WorldPackets::Character::ReorderCharacters& reorderChars);
    void HandleOpeningCinematic(WorldPackets::Misc::OpeningCinematic& packet);
    void HandleGetUndeleteCooldownStatus(WorldPackets::Character::GetUndeleteCharacterCooldownStatus& /*getCooldown*/);
    void HandleUndeleteCooldownStatusCallback(PreparedQueryResult result);
    void HandleCharUndeleteOpcode(WorldPackets::Character::UndeleteCharacter& undeleteInfo);
    void HandleCharUndeleteCallback(PreparedQueryResult result, WorldPackets::Character::CharacterUndeleteInfo* undeleteInfo);
    void HandlePlayedTime(WorldPackets::Character::RequestPlayedTime& packet);
    void HandleLookingForGroup(WorldPacket& recvPacket);
    void HandlePortGraveyard(WorldPackets::Misc::PortGraveyard& packet);
    void HandleRequestCemeteryList(WorldPackets::Misc::RequestCemeteryList& packet);
    void HandleInspectOpcode(WorldPackets::Inspect::Inspect& inspect);
    void HandleRequestHonorStatsOpcode(WorldPackets::Inspect::RequestHonorStats& request);
    void HandleInspectPVP(WorldPackets::Inspect::InspectPVPRequest& request);
    void HandleQueryInspectAchievements(WorldPackets::Inspect::QueryInspectAchievements& inspect);
    void HandleMountSpecialAnimOpcode(WorldPackets::Misc::MountSpecial& mountSpecial);
    void HandleShowingHelmOpcode(WorldPackets::Character::ShowingHelm& packet);
    void HandleShowingCloakOpcode(WorldPackets::Character::ShowingCloak& packet);
    void HandleRepairItemOpcode(WorldPackets::Item::RepairItem& packet);
    void HandleMoveKnockBackAck(WorldPackets::Movement::MovementAckMessage& movementAck);
    void HandleMoveTeleportAck(WorldPackets::Movement::MoveTeleportAck& packet);
    void HandleForceSpeedChangeAck(WorldPackets::Movement::MovementSpeedAck& packet);
    void HandleSetCollisionHeightAck(WorldPackets::Movement::MoveSetCollisionHeightAck& setCollisionHeightAck);
    void HandlePingOpcode(WorldPacket& recvPacket);
    void HandleRepopRequest(WorldPackets::Misc::RepopRequest& packet);
    void HandleAutostoreLootItemOpcode(WorldPackets::Loot::LootItem& packet);
    void HandleLootMoneyOpcode(WorldPackets::Loot::LootMoney& packet);
    void HandleLootOpcode(WorldPackets::Loot::LootUnit& packet);
    void HandleLootReleaseOpcode(WorldPackets::Loot::LootRelease& packet);
    void HandleLootMasterGiveOpcode(WorldPacket& recvPacket);
    void HandleSetLootSpecialization(WorldPackets::Loot::SetLootSpecialization& packet);
    void HandleWhoOpcode(WorldPackets::Who::WhoRequestPkt& whoRequest);
    void HandleLogoutRequestOpcode(WorldPackets::Character::LogoutRequest& logoutRequest);
    void HandleLogoutCancelOpcode(WorldPackets::Character::LogoutCancel& logoutCancel);
    void HandleGMTicketGetCaseStatusOpcode(WorldPackets::Ticket::GMTicketGetCaseStatus& packet);
    void HandleGMTicketSystemStatusOpcode(WorldPackets::Ticket::GMTicketGetSystemStatus& packet);
    void HandleSupportTicketSubmitBug(WorldPackets::Ticket::SupportTicketSubmitBug& packet);
    void HandleSupportTicketSubmitSuggestion(WorldPackets::Ticket::SupportTicketSubmitSuggestion& packet);
    void HandleSupportTicketSubmitComplaint(WorldPackets::Ticket::SupportTicketSubmitComplaint& packet);
    void HandleBugReportOpcode(WorldPackets::Ticket::BugReport& bugReport);
    void HandleComplaint(WorldPackets::Ticket::Complaint& packet);
    void HandleTogglePvP(WorldPackets::Misc::TogglePvP& packet);
    void HandleSetPvP(WorldPackets::Misc::SetPvP& packet);
    void HandleSetSelectionOpcode(WorldPackets::Misc::SetSelection& packet);
    void HandleStandStateChangeOpcode(WorldPackets::Misc::StandStateChange& packet);
    void HandleEmoteOpcode(WorldPackets::Chat::EmoteClient& packet);
    void HandleContactListOpcode(WorldPackets::Social::SendContactList& packet);
    void HandleAddFriendOpcode(WorldPackets::Social::AddFriend& packet);
    void HandleDelFriendOpcode(WorldPackets::Social::DelFriend& packet);
    void HandleAddIgnoreOpcode(WorldPackets::Social::AddIgnore& packet);
    void HandleDelIgnoreOpcode(WorldPackets::Social::DelIgnore& packet);
    void HandleSetContactNotesOpcode(WorldPackets::Social::SetContactNotes& packet);
    void HandleAreaTriggerOpcode(WorldPackets::Misc::AreaTrigger& packet);
    void HandleSetFactionAtWar(WorldPackets::Character::SetFactionAtWar& packet);
    void HandleSetFactionNotAtWar(WorldPackets::Character::SetFactionNotAtWar& packet);
    void HandleSetFactionCheat(WorldPacket& recvData);
    void HandleSetWatchedFactionOpcode(WorldPackets::Character::SetWatchedFaction& packet);
    void HandleSetFactionInactiveOpcode(WorldPackets::Character::SetFactionInactive& packet);
    void HandleRequestForcedReactionsOpcode(WorldPackets::Reputation::RequestForcedReactions& requestForcedReactions);
    void HandleUpdateAccountData(WorldPackets::ClientConfig::UserClientUpdateAccountData& packet);
    void HandleRequestAccountData(WorldPackets::ClientConfig::RequestAccountData& request);
    void HandleSetAdvancedCombatLogging(WorldPackets::ClientConfig::SetAdvancedCombatLogging& setAdvancedCombatLogging);
    void HandleSetActionButtonOpcode(WorldPackets::Spells::SetActionButton& packet);
    void HandleGameObjectUseOpcode(WorldPackets::GameObject::GameObjUse& packet);
    void HandleMeetingStoneInfo(WorldPacket& recPacket);
    void HandleGameobjectReportUse(WorldPackets::GameObject::GameObjReportUse& packet);
    void HandleNameQueryOpcode(WorldPackets::Query::QueryPlayerName& packet);
    void HandleQueryTimeOpcode(WorldPackets::Query::QueryTime& queryTime);
    void HandleCreatureQuery(WorldPackets::Query::QueryCreature& packet);
    void HandleDBQueryBulk(WorldPackets::Query::DBQueryBulk& packet);
    void HandleGameObjectQueryOpcode(WorldPackets::Query::QueryGameObject& packet);
    void HandleMoveWorldportAckOpcode(WorldPackets::Movement::WorldPortResponse& packet);
    void HandleMoveWorldportAckOpcode();                // for server-side calls
    void HandleSuspendTokenResponse(WorldPackets::Movement::SuspendTokenResponse& suspendTokenResponse);
    void HandleMovementOpcodes(WorldPackets::Movement::ClientPlayerMovement& packet);
    void HandleSetActiveMoverOpcode(WorldPackets::Movement::SetActiveMover& packet);
    void HandleMoveDismissVehicle(WorldPackets::Vehicle::MoveDismissVehicle& moveDismissVehicle);
    void HandleRequestVehiclePrevSeat(WorldPackets::Vehicle::RequestVehiclePrevSeat& requestVehiclePrevSeat);
    void HandleRequestVehicleNextSeat(WorldPackets::Vehicle::RequestVehicleNextSeat& requestVehicleNextSeat);
    void HandleMoveChangeVehicleSeats(WorldPackets::Vehicle::MoveChangeVehicleSeats& moveChangeVehicleSeats);
    void HandleRequestVehicleSwitchSeat(WorldPackets::Vehicle::RequestVehicleSwitchSeat& requestVehicleSwitchSeat);
    void HandleRideVehicleInteract(WorldPackets::Vehicle::RideVehicleInteract& rideVehicleInteract);
    void HandleEjectPassenger(WorldPackets::Vehicle::EjectPassenger& ejectPassenger);
    void HandleRequestVehicleExit(WorldPackets::Vehicle::RequestVehicleExit& requestVehicleExit);
    void HandleMoveSetVehicleRecAck(WorldPackets::Vehicle::MoveSetVehicleRecIdAck& setVehicleRecIdAck);
    void HandleMoveTimeSkippedOpcode(WorldPackets::Movement::MoveTimeSkipped& moveTimeSkipped);
    void HandleMovementAckMessage(WorldPackets::Movement::MovementAckMessage& movementAck);
    void HandleRequestRaidInfoOpcode(WorldPackets::Party::RequestRaidInfo& packet);
    void HandlePartyInviteOpcode(WorldPackets::Party::PartyInviteClient& packet);
    void HandlePartyInviteResponseOpcode(WorldPackets::Party::PartyInviteResponse& packet);
    void HandlePartyUninviteOpcode(WorldPackets::Party::PartyUninvite& packet);
    void HandleSetPartyLeaderOpcode(WorldPackets::Party::SetPartyLeader& packet);
    void HandleSetRoleOpcode(WorldPackets::Party::SetRole& packet);
    void HandleLeaveGroupOpcode(WorldPackets::Party::LeaveGroup& packet);
    void HandleOptOutOfLootOpcode(WorldPackets::Party::OptOutOfLoot& packet);
    void HandleSetLootMethodOpcode(WorldPackets::Party::SetLootMethod& packet);
    void HandleLootRoll(WorldPackets::Loot::LootRoll& packet);
    void HandleRequestPartyMemberStatsOpcode(WorldPackets::Party::RequestPartyMemberStats& packet);
    void HandleUpdateRaidTargetOpcode(WorldPackets::Party::UpdateRaidTarget& packet);
    void HandleDoReadyCheckOpcode(WorldPackets::Party::DoReadyCheck& packet);
    void HandleReadyCheckResponseOpcode(WorldPackets::Party::ReadyCheckResponseClient& packet);
    void HandleConvertRaidOpcode(WorldPackets::Party::ConvertRaid& packet);
    void HandleRequestPartyJoinUpdates(WorldPackets::Party::RequestPartyJoinUpdates& packet);
    void HandleChangeSubGroupOpcode(WorldPackets::Party::ChangeSubGroup& packet);
    void HandleSwapSubGroupsOpcode(WorldPackets::Party::SwapSubGroups& packet);
    void HandleSetAssistantLeaderOpcode(WorldPackets::Party::SetAssistantLeader& packet);
    void HandleSetPartyAssignment(WorldPackets::Party::SetPartyAssignment& packet);
    void HandleInitiateRolePoll(WorldPackets::Party::InitiateRolePoll& packet);
    void HandleSetEveryoneIsAssistant(WorldPackets::Party::SetEveryoneIsAssistant& packet);
    void HandleClearRaidMarker(WorldPackets::Party::ClearRaidMarker& packet);
    void HandleDeclinePetition(WorldPackets::Petition::DeclinePetition& packet);
    void HandleOfferPetition(WorldPackets::Petition::OfferPetition& packet);
    void HandlePetitionBuy(WorldPackets::Petition::PetitionBuy& packet);
    void HandlePetitionShowSignatures(WorldPackets::Petition::PetitionShowSignatures& packet);
    void HandleQueryPetition(WorldPackets::Petition::QueryPetition& packet);
    void HandlePetitionRenameGuild(WorldPackets::Petition::PetitionRenameGuild& packet);
    void HandleSignPetition(WorldPackets::Petition::SignPetition& packet);
    void HandleTurnInPetition(WorldPackets::Petition::TurnInPetition& packet);
    void HandleGuildQueryOpcode(WorldPackets::Guild::QueryGuildInfo& query);
    void HandleGuildInviteByName(WorldPackets::Guild::GuildInviteByName& packet);
    void HandleGuildOfficerRemoveMember(WorldPackets::Guild::GuildOfficerRemoveMember& packet);
    void HandleGuildAcceptInvite(WorldPackets::Guild::AcceptGuildInvite& invite);
    void HandleGuildDeclineInvitation(WorldPackets::Guild::GuildDeclineInvitation& decline);
    void HandleGuildEventLogQuery(WorldPackets::Guild::GuildEventLogQuery& packet);
    void HandleGuildGetRoster(WorldPackets::Guild::GuildGetRoster& packet);
    void HandleRequestGuildRewardsList(WorldPackets::Guild::RequestGuildRewardsList& packet);
    void HandleGuildPromoteMember(WorldPackets::Guild::GuildPromoteMember& promote);
    void HandleGuildDemoteMember(WorldPackets::Guild::GuildDemoteMember& demote);
    void HandleGuildAssignRank(WorldPackets::Guild::GuildAssignMemberRank& packet);
    void HandleGuildLeave(WorldPackets::Guild::GuildLeave& leave);
    void HandleGuildDelete(WorldPackets::Guild::GuildDelete& packet);
    void HandleGuildSetAchievementTracking(WorldPackets::Guild::GuildSetAchievementTracking& packet);
    void HandleGuildSetGuildMaster(WorldPackets::Guild::GuildSetGuildMaster& packet);
    void HandleGuildUpdateMotdText(WorldPackets::Guild::GuildUpdateMotdText& packet);
    void HandleGuildNewsUpdateSticky(WorldPackets::Guild::GuildNewsUpdateSticky& packet);
    void HandleGuildSetMemberNote(WorldPackets::Guild::GuildSetMemberNote& packet);
    void HandleGuildGetRanks(WorldPackets::Guild::GuildGetRanks& packet);
    void HandleGuildQueryNews(WorldPackets::Guild::GuildQueryNews& newsQuery);
    void HandleGuildSetRankPermissions(WorldPackets::Guild::GuildSetRankPermissions& packet);
    void HandleGuildAddRank(WorldPackets::Guild::GuildAddRank& packet);
    void HandleGuildDeleteRank(WorldPackets::Guild::GuildDeleteRank& packet);
    void HandleGuildUpdateInfoText(WorldPackets::Guild::GuildUpdateInfoText& packet);
    void HandleSaveGuildEmblem(WorldPackets::Guild::SaveGuildEmblem& packet);
    void HandleGuildRequestPartyState(WorldPackets::Guild::RequestGuildPartyState& packet);
    void HandleGuildChallengeUpdateRequest(WorldPackets::Guild::GuildChallengeUpdateRequest& packet);
    void HandleDeclineGuildInvites(WorldPackets::Guild::DeclineGuildInvites& packet);
    void HandleGuildFinderAddRecruit(WorldPackets::GuildFinder::LFGuildAddRecruit& lfGuildAddRecruit);
    void HandleGuildFinderBrowse(WorldPackets::GuildFinder::LFGuildBrowse& lfGuildBrowse);
    void HandleGuildFinderDeclineRecruit(WorldPackets::GuildFinder::LFGuildDeclineRecruit& lfGuildDeclineRecruit);
    void HandleGuildFinderGetApplications(WorldPackets::GuildFinder::LFGuildGetApplications& lfGuildGetApplications);
    void HandleGuildFinderGetGuildPost(WorldPackets::GuildFinder::LFGuildGetGuildPost& lfGuildGetGuildPost);
    void HandleGuildFinderGetRecruits(WorldPackets::GuildFinder::LFGuildGetRecruits& lfGuildGetRecruits);
    void HandleGuildFinderRemoveRecruit(WorldPackets::GuildFinder::LFGuildRemoveRecruit& lfGuildRemoveRecruit);
    void HandleGuildFinderSetGuildPost(WorldPackets::GuildFinder::LFGuildSetGuildPost& lfGuildSetGuildPost);
    void HandleEnableTaxiNodeOpcode(WorldPackets::Taxi::EnableTaxiNode& enableTaxiNode);
    void HandleTaxiNodeStatusQueryOpcode(WorldPackets::Taxi::TaxiNodeStatusQuery& taxiNodeStatusQuery);
    void HandleTaxiQueryAvailableNodesOpcode(WorldPackets::Taxi::TaxiQueryAvailableNodes& taxiQueryAvailableNodes);
    void HandleActivateTaxiOpcode(WorldPackets::Taxi::ActivateTaxi& activateTaxi);
    void HandleMoveSplineDoneOpcode(WorldPackets::Movement::MoveSplineDone& moveSplineDone);
    void HandleTaxiRequestEarlyLanding(WorldPackets::Taxi::TaxiRequestEarlyLanding& taxiRequestEarlyLanding);
    void HandleTabardVendorActivateOpcode(WorldPackets::NPC::Hello& packet);
    void HandleBankerActivateOpcode(WorldPackets::NPC::Hello& packet);
    void HandleTrainerListOpcode(WorldPackets::NPC::Hello& packet);
    void HandleTrainerBuySpellOpcode(WorldPackets::NPC::TrainerBuySpell& packet);
    void HandlePetitionShowList(WorldPackets::Petition::PetitionShowList& packet);
    void HandleGossipHelloOpcode(WorldPackets::NPC::Hello& packet);
    void HandleGossipSelectOptionOpcode(WorldPackets::NPC::GossipSelectOption& packet);
    void HandleSpiritHealerActivate(WorldPackets::NPC::SpiritHealerActivate& packet);
    void HandleNpcTextQueryOpcode(WorldPackets::Query::QueryNPCText& packet);
    void HandleBinderActivateOpcode(WorldPackets::NPC::Hello& packet);
    void HandleRequestStabledPets(WorldPackets::NPC::RequestStabledPets& packet);
    void HandleSetPetSlot(WorldPackets::Pet::SetPetSlot& packet);
    void HandleStablePet(WorldPacket& recvPacket);
    void HandleStablePetCallback(PreparedQueryResult result);
    void HandleUnstablePet(WorldPacket& recvPacket);
    void HandleUnstablePetCallback(PreparedQueryResult result, uint32 petId);
    void HandleBuyStableSlot(WorldPacket& recvPacket);
    void HandleStableRevivePet(WorldPacket& recvPacket);
    void HandleStableSwapPet(WorldPacket& recvPacket);
    void HandleStableSwapPetCallback(PreparedQueryResult result, uint32 petId);
    void HandleCanDuel(WorldPackets::Duel::CanDuel& packet);
    void HandleDuelResponseOpcode(WorldPackets::Duel::DuelResponse& duelResponse);
    void HandleAcceptTradeOpcode(WorldPackets::Trade::AcceptTrade& acceptTrade);
    void HandleBeginTradeOpcode(WorldPackets::Trade::BeginTrade& beginTrade);
    void HandleBusyTradeOpcode(WorldPackets::Trade::BusyTrade& busyTrade);
    void HandleCancelTradeOpcode(WorldPackets::Trade::CancelTrade& cancelTrade);
    void HandleClearTradeItemOpcode(WorldPackets::Trade::ClearTradeItem& clearTradeItem);
    void HandleIgnoreTradeOpcode(WorldPackets::Trade::IgnoreTrade& ignoreTrade);
    void HandleInitiateTradeOpcode(WorldPackets::Trade::InitiateTrade& initiateTrade);
    void HandleSetTradeCurrencyOpcode(WorldPackets::Trade::SetTradeCurrency& setTradeCurrency);
    void HandleSetTradeGoldOpcode(WorldPackets::Trade::SetTradeGold& setTradeGold);
    void HandleSetTradeItemOpcode(WorldPackets::Trade::SetTradeItem& setTradeItem);
    void HandleUnacceptTradeOpcode(WorldPackets::Trade::UnacceptTrade& unacceptTrade);
    void HandleAuctionHelloOpcode(WorldPackets::AuctionHouse::AuctionHelloRequest& packet);
    void HandleAuctionListItems(WorldPackets::AuctionHouse::AuctionListItems& packet);
    void HandleAuctionListBidderItems(WorldPackets::AuctionHouse::AuctionListBidderItems& packet);
    void HandleAuctionSellItem(WorldPackets::AuctionHouse::AuctionSellItem& packet);
    void HandleAuctionRemoveItem(WorldPackets::AuctionHouse::AuctionRemoveItem& packet);
    void HandleAuctionListOwnerItems(WorldPackets::AuctionHouse::AuctionListOwnerItems& packet);
    void HandleAuctionPlaceBid(WorldPackets::AuctionHouse::AuctionPlaceBid& packet);
    void HandleAuctionListPendingSales(WorldPackets::AuctionHouse::AuctionListPendingSales& packet);
    void HandleReplicateItems(WorldPackets::AuctionHouse::AuctionReplicateItems& packet);
    void HandleAutoBankItemOpcode(WorldPackets::Bank::AutoBankItem& packet);
    void HandleAutoStoreBankItemOpcode(WorldPackets::Bank::AutoStoreBankItem& packet);
    void HandleBuyBankSlotOpcode(WorldPackets::Bank::BuyBankSlot& packet);
    void HandleBlackMarketOpen(WorldPackets::BlackMarket::BlackMarketOpen& blackMarketOpen);
    void HandleBlackMarketRequestItems(WorldPackets::BlackMarket::BlackMarketRequestItems& blackMarketRequestItems);
    void HandleBlackMarketBidOnItem(WorldPackets::BlackMarket::BlackMarketBidOnItem& blackMarketBidOnItem);
    void HandleGetMailList(WorldPackets::Mail::MailGetList& packet);
    void HandleSendMail(WorldPackets::Mail::SendMail& packet);
    void HandleMailTakeMoney(WorldPackets::Mail::MailTakeMoney& packet);
    void HandleMailTakeItem(WorldPackets::Mail::MailTakeItem& packet);
    void HandleMailMarkAsRead(WorldPackets::Mail::MailMarkAsRead& packet);
    void HandleMailReturnToSender(WorldPackets::Mail::MailReturnToSender& packet);
    void HandleMailDelete(WorldPackets::Mail::MailDelete& packet);
    void HandleItemTextQuery(WorldPackets::Query::ItemTextQuery& itemTextQuery);
    void HandleMailCreateTextItem(WorldPackets::Mail::MailCreateTextItem& packet);
    void HandleQueryNextMailTime(WorldPackets::Mail::MailQueryNextMailTime& packet);
    void HandleCancelChanneling(WorldPackets::Spells::CancelChannelling& cancelChanneling);
    void HandleSplitItemOpcode(WorldPackets::Item::SplitItem& splitItem);
    void HandleSwapInvItemOpcode(WorldPackets::Item::SwapInvItem& swapInvItem);
    void HandleDestroyItemOpcode(WorldPackets::Item::DestroyItem& destroyItem);
    void HandleAutoEquipItemOpcode(WorldPackets::Item::AutoEquipItem& autoEquipItem);
    void HandleSellItemOpcode(WorldPackets::Item::SellItem& packet);
    void HandleBuyItemOpcode(WorldPackets::Item::BuyItem& packet);
    void HandleListInventoryOpcode(WorldPackets::NPC::Hello& packet);
    void HandleAutoStoreBagItemOpcode(WorldPackets::Item::AutoStoreBagItem& packet);
    void HandleReadItem(WorldPackets::Item::ReadItem& readItem);
    void HandleAutoEquipItemSlotOpcode(WorldPackets::Item::AutoEquipItemSlot& autoEquipItemSlot);
    void HandleSwapItem(WorldPackets::Item::SwapItem& swapItem);
    void HandleBuybackItem(WorldPackets::Item::BuyBackItem& packet);
    void HandleWrapItem(WorldPackets::Item::WrapItem& packet);
    void HandleUseCritterItem(WorldPackets::Item::UseCritterItem& packet);
    void HandleUpgradeItem(WorldPackets::Item::UpgradeItem& packet);
    void HandleAttackSwingOpcode(WorldPackets::Combat::AttackSwing& packet);
    void HandleAttackStopOpcode(WorldPackets::Combat::AttackStop& packet);
    void HandleSetSheathedOpcode(WorldPackets::Combat::SetSheathed& packet);
    void HandleUseItemOpcode(WorldPackets::Spells::UseItem& packet);
    void HandleOpenItemOpcode(WorldPackets::Spells::OpenItem& packet);
    void HandleCastSpellOpcode(WorldPackets::Spells::CastSpell& castRequest);
    void HandleCancelCastOpcode(WorldPackets::Spells::CancelCast& packet);
    void HandleCancelAuraOpcode(WorldPackets::Spells::CancelAura& cancelAura);
    void HandleCancelGrowthAuraOpcode(WorldPackets::Spells::CancelGrowthAura& cancelGrowthAura);
    void HandleCancelMountAuraOpcode(WorldPackets::Spells::CancelMountAura& cancelMountAura);
    void HandleCancelAutoRepeatSpellOpcode(WorldPackets::Spells::CancelAutoRepeatSpell& cancelAutoRepeatSpell);
    void HandleMissileTrajectoryCollision(WorldPackets::Spells::MissileTrajectoryCollision& packet);
    void HandleUpdateMissileTrajectory(WorldPackets::Spells::UpdateMissileTrajectory& packet);
    void HandleLearnTalentsOpcode(WorldPackets::Talent::LearnTalents& packet);
    void HandleConfirmRespecWipeOpcode(WorldPackets::Talent::ConfirmRespecWipe& confirmRespecWipe);
    void HandleUnlearnSkillOpcode(WorldPackets::Spells::UnlearnSkill& packet);
    void HandleSetSpecializationOpcode(WorldPackets::Talent::SetSpecialization& packet);
    void HandleQuestgiverStatusQueryOpcode(WorldPackets::Quest::QuestGiverStatusQuery& packet);
    void HandleQuestgiverStatusMultipleQuery(WorldPackets::Quest::QuestGiverStatusMultipleQuery& packet);
    void HandleQuestgiverHelloOpcode(WorldPackets::Quest::QuestGiverHello& packet);
    void HandleQuestgiverAcceptQuestOpcode(WorldPackets::Quest::QuestGiverAcceptQuest& packet);
    void HandleQuestgiverQueryQuestOpcode(WorldPackets::Quest::QuestGiverQueryQuest& packet);
    void HandleQuestgiverChooseRewardOpcode(WorldPackets::Quest::QuestGiverChooseReward& packet);
    void HandleQuestgiverRequestRewardOpcode(WorldPackets::Quest::QuestGiverRequestReward& packet);
    void HandleQuestQueryOpcode(WorldPackets::Quest::QueryQuestInfo& packet);
    void HandleQuestgiverCancel(WorldPacket& recvData);
    void HandleQuestLogRemoveQuest(WorldPackets::Quest::QuestLogRemoveQuest& packet);
    void HandleQuestConfirmAccept(WorldPackets::Quest::QuestConfirmAccept& packet);
    void HandleQuestgiverCompleteQuest(WorldPackets::Quest::QuestGiverCompleteQuest& packet);
    void HandleQuestgiverQuestAutoLaunch(WorldPacket& recvPacket);
    void HandlePushQuestToParty(WorldPackets::Quest::PushQuestToParty& packet);
    void HandleQuestPushResult(WorldPackets::Quest::QuestPushResult& packet);
    void HandleChatMessageOpcode(WorldPackets::Chat::ChatMessage& chatMessage);
    void HandleChatMessageWhisperOpcode(WorldPackets::Chat::ChatMessageWhisper& chatMessageWhisper);
    void HandleChatMessageChannelOpcode(WorldPackets::Chat::ChatMessageChannel& chatMessageChannel);
    void HandleChatMessage(ChatMsg type, uint32 lang, std::string msg, std::string target = "");
    void HandleChatAddonMessageOpcode(WorldPackets::Chat::ChatAddonMessage& chatAddonMessage);
    void HandleChatAddonMessageWhisperOpcode(WorldPackets::Chat::ChatAddonMessageWhisper& chatAddonMessageWhisper);
    void HandleChatAddonMessageChannelOpcode(WorldPackets::Chat::ChatAddonMessageChannel& chatAddonMessageChannel);
    void HandleChatAddonMessage(ChatMsg type, std::string prefix, std::string text, std::string target = "");
    void HandleChatMessageAFKOpcode(WorldPackets::Chat::ChatMessageAFK& chatMessageAFK);
    void HandleChatMessageDNDOpcode(WorldPackets::Chat::ChatMessageDND& chatMessageDND);
    void HandleChatMessageEmoteOpcode(WorldPackets::Chat::ChatMessageEmote& chatMessageEmote);
    void HandleTextEmoteOpcode(WorldPackets::Chat::CTextEmote& packet);
    void HandleChatIgnoredOpcode(WorldPackets::Chat::ChatReportIgnored& chatReportIgnored);
    void HandleUnregisterAllAddonPrefixesOpcode(WorldPackets::Chat::ChatUnregisterAllAddonPrefixes& packet);
    void HandleAddonRegisteredPrefixesOpcode(WorldPackets::Chat::ChatRegisterAddonPrefixes& packet);
    void HandleReclaimCorpse(WorldPackets::Misc::ReclaimCorpse& packet);
    void HandleQueryCorpseLocation(WorldPackets::Query::QueryCorpseLocationFromClient& packet);
    void HandleQueryCorpseTransport(WorldPackets::Query::QueryCorpseTransport& packet);
    void HandleResurrectResponse(WorldPackets::Misc::ResurrectResponse& packet);
    void HandleSummonResponseOpcode(WorldPackets::Movement::SummonResponse& packet);
    void HandleJoinChannel(WorldPackets::Channel::JoinChannel& packet);
    void HandleLeaveChannel(WorldPackets::Channel::LeaveChannel& packet);
    void HandleChannelCommand(WorldPackets::Channel::ChannelPlayerCommand& packet);
    void HandleChannelPlayerCommand(WorldPackets::Channel::ChannelPlayerCommand& packet);
    void HandleVoiceSessionEnable(WorldPackets::Voice::VoiceSessionEnable& packet);
    void HandleSetActiveVoiceChannel(WorldPackets::Voice::SetActiveVoiceChannel& packet);
    void HandleCompleteCinematic(WorldPackets::Misc::CompleteCinematic& packet);
    void HandleNextCinematicCamera(WorldPackets::Misc::NextCinematicCamera& packet);
    void HandleQueryPageText(WorldPackets::Query::QueryPageText& packet);
    void HandleTutorialFlag(WorldPackets::Misc::TutorialSetFlag& packet);
    void HandlePetAction(WorldPackets::Pet::PetAction& packet);
    void HandlePetStopAttack(WorldPackets::Pet::PetStopAttack& packet);
    void HandleQueryPetName(WorldPackets::Query::QueryPetName& packet);
    void HandlePetSetAction(WorldPackets::Pet::PetSetAction& packet);
    void HandlePetAbandon(WorldPackets::Pet::PetAbandon& packet);
    void HandlePetRename(WorldPackets::Pet::PetRename& packet);
    void HandlePetCancelAuraOpcode(WorldPackets::Spells::PetCancelAura& packet);
    void HandlePetSpellAutocastOpcode(WorldPackets::Pet::PetSpellAutocast& packet);
    void HandlePetCastSpellOpcode(WorldPackets::Spells::PetCastSpell& petCastSpell);
    void HandlePetSetSpecializationOpcode(WorldPackets::Pet::LearnPetSpecializationGroup& learnPetSpecializationGroup);
    void HandleSetActionBarToggles(WorldPackets::Character::SetActionBarToggles& packet);
    void HandleTotemDestroyed(WorldPackets::Totem::TotemDestroyed& totemDestroyed);
    void HandleDismissCritter(WorldPackets::Pet::DismissCritter& dismissCritter);
    void HandleBattlemasterHelloOpcode(WorldPackets::NPC::Hello& hello);
    void HandleBattlemasterJoinOpcode(WorldPackets::Battleground::BattlemasterJoin& battlemasterJoin);
    void HandlePVPLogDataOpcode(WorldPackets::Battleground::PVPLogDataRequest& pvpLogDataRequest);
    void HandleBattleFieldPortOpcode(WorldPackets::Battleground::BattlefieldPort& battlefieldPort);
    void HandleBattlefieldListOpcode(WorldPackets::Battleground::BattlefieldListRequest& battlefieldList);
    void HandleBattlefieldLeaveOpcode(WorldPackets::Battleground::BattlefieldLeave& battlefieldLeave);
    void HandleBattlemasterJoinArena(WorldPackets::Battleground::BattlemasterJoinArena& packet);
    void HandleReportPvPAFK(WorldPackets::Battleground::ReportPvPPlayerAFK& reportPvPPlayerAFK);
    void HandleRequestRatedBattlefieldInfo(WorldPackets::Battleground::RequestRatedBattlefieldInfo& packet);
    void HandleGetPVPOptionsEnabled(WorldPackets::Battleground::GetPVPOptionsEnabled& getPvPOptionsEnabled);
    void HandleRequestPvpReward(WorldPackets::Battleground::RequestPVPRewards& packet);
    void HandleAreaSpiritHealerQueryOpcode(WorldPackets::Battleground::AreaSpiritHealerQuery& areaSpiritHealerQuery);
    void HandleAreaSpiritHealerQueueOpcode(WorldPackets::Battleground::AreaSpiritHealerQueue& areaSpiritHealerQueue);
    void HandleHearthAndResurrect(WorldPackets::Battleground::HearthAndResurrect& hearthAndResurrect);
    void HandleRequestBattlefieldStatusOpcode(WorldPackets::Battleground::RequestBattlefieldStatus& requestBattlefieldStatus);
    void HandleBfEntryInviteResponse(WorldPackets::Battlefield::BFMgrEntryInviteResponse& bfMgrEntryInviteResponse);
    void HandleBfQueueInviteResponse(WorldPackets::Battlefield::BFMgrQueueInviteResponse& bfMgrQueueInviteResponse);
    void HandleBfQueueExitRequest(WorldPackets::Battlefield::BFMgrQueueExitRequest& bfMgrQueueExitRequest);
    void HandleWorldTeleportOpcode(WorldPackets::Misc::WorldTeleport& worldTeleport);
    void HandleMinimapPingOpcode(WorldPackets::Party::MinimapPingClient& packet);
    void HandleRandomRollOpcode(WorldPackets::Misc::RandomRollClient& packet);
    void HandleFarSightOpcode(WorldPackets::Misc::FarSight& packet);
    void HandleSetDungeonDifficultyOpcode(WorldPackets::Misc::SetDungeonDifficulty& setDungeonDifficulty);
    void HandleSetRaidDifficultyOpcode(WorldPackets::Misc::SetRaidDifficulty& setRaidDifficulty);
    void HandleSetTitleOpcode(WorldPackets::Character::SetTitle& packet);
    void HandleTimeSyncResponse(WorldPackets::Misc::TimeSyncResponse& packet);
    void HandleWhoIsOpcode(WorldPackets::Who::WhoIsRequest& packet);
    void HandleResetInstancesOpcode(WorldPackets::Instance::ResetInstances& packet);
    void HandleInstanceLockResponse(WorldPackets::Instance::InstanceLockResponse& packet);
    void HandleDFGetSystemInfo(WorldPackets::LFG::DFGetSystemInfo& packet);
    void HandleLfgJoinOpcode(WorldPackets::LFG::DFJoin& packet);
    void HandleLfgLeaveOpcode(WorldPackets::LFG::DFLeave& packet);
    void HandleLfgSetRolesOpcode(WorldPackets::LFG::DFSetRoles& packet);
    void HandleLfgProposalResultOpcode(WorldPackets::LFG::DFProposalResponse& packet);
    void HandleLfgSetBootVoteOpcode(WorldPackets::LFG::DFBootPlayerVote& packet);
    void HandleLfgTeleportOpcode(WorldPackets::LFG::DFTeleport& packet);
    void HandleDFGetJoinStatus(WorldPackets::LFG::DFGetJoinStatus& packet);
    void HandleSelfResOpcode(WorldPackets::Spells::SelfRes& packet);
    void HandleRequestPetInfo(WorldPackets::Pet::RequestPetInfo& packet);
    void HandleSocketGems(WorldPackets::Item::SocketGems& socketGems);
    void HandleCancelTempEnchantmentOpcode(WorldPackets::Item::CancelTempEnchantment& cancelTempEnchantment);
    void HandleGetItemPurchaseData(WorldPackets::Item::GetItemPurchaseData& packet);
    void HandleItemRefund(WorldPackets::Item::ItemPurchaseRefund& packet);
    void HandleSetTaxiBenchmark(WorldPackets::Misc::SetTaxiBenchmarkMode& packet);
    void HandleGuildPermissionsQuery(WorldPackets::Guild::GuildPermissionsQuery& packet);
    void HandleGuildBankMoneyWithdrawn(WorldPackets::Guild::GuildBankRemainingWithdrawMoneyQuery& packet);
    void HandleGuildBankActivate(WorldPackets::Guild::GuildBankActivate& packet);
    void HandleGuildBankQueryTab(WorldPackets::Guild::GuildBankQueryTab& packet);
    void HandleGuildBankLogQuery(WorldPackets::Guild::GuildBankLogQuery& packet);
    void HandleGuildBankDepositMoney(WorldPackets::Guild::GuildBankDepositMoney& packet);
    void HandleGuildBankWithdrawMoney(WorldPackets::Guild::GuildBankWithdrawMoney& packet);
    void HandleGuildBankSwapItems(WorldPackets::Guild::GuildBankSwapItems& packet);
    void HandleGuildBankUpdateTab(WorldPackets::Guild::GuildBankUpdateTab& packet);
    void HandleGuildBankBuyTab(WorldPackets::Guild::GuildBankBuyTab& packet);
    void HandleGuildBankTextQuery(WorldPackets::Guild::GuildBankTextQuery& packet);
    void HandleGuildBankSetTabText(WorldPackets::Guild::GuildBankSetTabText& packet);
    void HandleGrantLevel(WorldPackets::RaF::GrantLevel& grantLevel);
    void HandleAcceptGrantLevel(WorldPackets::RaF::AcceptLevelGrant& acceptLevelGrant);
    void HandleCalendarGetCalendar(WorldPackets::Calendar::CalendarGetCalendar& calendarGetCalendar);
    void HandleCalendarGetEvent(WorldPackets::Calendar::CalendarGetEvent& calendarGetEvent);
    void HandleCalendarGuildFilter(WorldPackets::Calendar::CalendarGuildFilter& calendarGuildFilter);
    void HandleCalendarAddEvent(WorldPackets::Calendar::CalendarAddEvent& calendarAddEvent);
    void HandleCalendarUpdateEvent(WorldPackets::Calendar::CalendarUpdateEvent& calendarUpdateEvent);
    void HandleCalendarRemoveEvent(WorldPackets::Calendar::CalendarRemoveEvent& calendarRemoveEvent);
    void HandleCalendarCopyEvent(WorldPackets::Calendar::CalendarCopyEvent& calendarCopyEvent);
    void HandleCalendarEventInvite(WorldPackets::Calendar::CalendarEventInvite& calendarEventInvite);
    void HandleCalendarEventRsvp(WorldPackets::Calendar::CalendarEventRSVP& calendarEventRSVP);
    void HandleCalendarEventRemoveInvite(WorldPackets::Calendar::CalendarRemoveInvite& calendarRemoveInvite);
    void HandleCalendarEventStatus(WorldPackets::Calendar::CalendarEventStatus& calendarEventStatus);
    void HandleCalendarEventModeratorStatus(WorldPackets::Calendar::CalendarEventModeratorStatus& calendarEventModeratorStatus);
    void HandleCalendarComplain(WorldPackets::Calendar::CalendarComplain& calendarComplain);
    void HandleCalendarGetNumPending(WorldPackets::Calendar::CalendarGetNumPending& calendarGetNumPending);
    void HandleCalendarEventSignup(WorldPackets::Calendar::CalendarEventSignUp& calendarEventSignUp);
    void HandleSetSavedInstanceExtend(WorldPackets::Calendar::SetSavedInstanceExtend& setSavedInstanceExtend);
    void HandleVoidStorageUnlock(WorldPackets::VoidStorage::UnlockVoidStorage& unlockVoidStorage);
    void HandleVoidStorageQuery(WorldPackets::VoidStorage::QueryVoidStorage& queryVoidStorage);
    void HandleVoidStorageTransfer(WorldPackets::VoidStorage::VoidStorageTransfer& voidStorageTransfer);
    void HandleVoidSwapItem(WorldPackets::VoidStorage::SwapVoidItem& swapVoidItem);
    void HandleTransmogrifyItems(WorldPackets::Item::TransmogrifyItems& transmogrifyItems);
    void HandleSpellClick(WorldPackets::Spells::SpellClick& spellClick);
    void HandleMirrorImageDataRequest(WorldPackets::Spells::GetMirrorImageData& getMirrorImageData);
    void HandleRemoveGlyph(WorldPacket& recvData);
    void HandleGuildSetFocusedAchievement(WorldPackets::Achievement::GuildSetFocusedAchievement& setFocusedAchievement);
    void HandleEquipmentSetSave(WorldPackets::EquipmentSet::SaveEquipmentSet& saveEquipmentSet);
    void HandleDeleteEquipmentSet(WorldPackets::EquipmentSet::DeleteEquipmentSet& deleteEquipmentSet);
    void HandleUseEquipmentSet(WorldPackets::EquipmentSet::UseEquipmentSet& useEquipmentSet);
    void HandleUITimeRequest(WorldPackets::Misc::UITimeRequest& /*request*/);
    void HandleQueryQuestCompletionNPCs(WorldPackets::Query::QueryQuestCompletionNPCs& queryQuestCompletionNPCs);
    void HandleQuestPOIQuery(WorldPackets::Query::QuestPOIQuery& questPoiQuery);
    void HandleViolenceLevel(WorldPackets::Misc::ViolenceLevel& violenceLevel);
    void HandleObjectUpdateFailedOpcode(WorldPackets::Misc::ObjectUpdateFailed& objectUpdateFailed);
    void HandleObjectUpdateRescuedOpcode(WorldPackets::Misc::ObjectUpdateRescued& objectUpdateRescued);
    void HandleRequestCategoryCooldowns(WorldPackets::Spells::RequestCategoryCooldowns& requestCategoryCooldowns);
    void HandleAddToy(WorldPackets::Toy::AddToy& packet);
    void HandleToySetFavorite(WorldPackets::Toy::ToySetFavorite& packet);
    void HandleUseToy(WorldPackets::Toy::UseToy& packet);
    void HandleSceneTriggerEvent(WorldPackets::Scenes::SceneTriggerEvent& sceneTriggerEvent);
    void HandleScenePlaybackComplete(WorldPackets::Scenes::ScenePlaybackComplete& scenePlaybackComplete);
    void HandleScenePlaybackCanceled(WorldPackets::Scenes::ScenePlaybackCanceled& scenePlaybackCanceled);
    void HandleUpdateListedAuctionableTokens(WorldPackets::Token::UpdateListedAuctionableTokens& updateListedAuctionableTokens);
    void HandleRequestWowTokenMarketPrice(WorldPackets::Token::RequestWowTokenMarketPrice& requestWowTokenMarketPrice);
    void HandleSaveCUFProfiles(WorldPackets::Misc::SaveCUFProfiles& packet);
    void HandleGetGarrisonInfo(WorldPackets::Garrison::GetGarrisonInfo& getGarrisonInfo);
    void HandleGarrisonPurchaseBuilding(WorldPackets::Garrison::GarrisonPurchaseBuilding& garrisonPurchaseBuilding);
    void HandleGarrisonCancelConstruction(WorldPackets::Garrison::GarrisonCancelConstruction& garrisonCancelConstruction);
    void HandleGarrisonRequestBlueprintAndSpecializationData(WorldPackets::Garrison::GarrisonRequestBlueprintAndSpecializationData& garrisonRequestBlueprintAndSpecializationData);
    void HandleGarrisonGetBuildingLandmarks(WorldPackets::Garrison::GarrisonGetBuildingLandmarks& garrisonGetBuildingLandmarks);
    void HandleBattlePetRequestJournal(WorldPackets::BattlePet::BattlePetRequestJournal& battlePetRequestJournal);
    void HandleBattlePetSetBattleSlot(WorldPackets::BattlePet::BattlePetSetBattleSlot& battlePetSetBattleSlot);
    void HandleBattlePetModifyName(WorldPackets::BattlePet::BattlePetModifyName& battlePetModifyName);
    void HandleBattlePetDeletePet(WorldPackets::BattlePet::BattlePetDeletePet& battlePetDeletePet);
    void HandleBattlePetSetFlags(WorldPackets::BattlePet::BattlePetSetFlags& battlePetSetFlags);
    void HandleBattlePetSummon(WorldPackets::BattlePet::BattlePetSummon& battlePetSummon);
    void HandleCageBattlePet(WorldPackets::BattlePet::CageBattlePet& cageBattlePet);
    void HandleWardenData(WorldPackets::Warden::WardenData& packet);
    void HandleBattlenetRequest(WorldPackets::Battlenet::Request& request);
    void HandleBattlenetRequestRealmListTicket(WorldPackets::Battlenet::RequestRealmListTicket& requestRealmListTicket);
    void Handle_Deprecated(WorldPacket& recvPacket);    // never used anymore by client

    void HandleCharEnum(PreparedQueryResult result);
    void HandlePlayerLoginFromDB(LoginQueryHolder const& holder);
    void HandlePlayerLoginToCharInWorld(Player* pCurrChar);
    void HandlePlayerLoginToCharOutOfWorld(Player* pCurrChar);
    void HandleCharFactionOrRaceChangeCallback(std::shared_ptr<CharacterFactionChangeInfo> factionChangeInfo, PreparedQueryResult result);

    void SendCharCreate(ResponseCodes result);
    void SendCharDelete(ResponseCodes result);
    void SendCharLoginFailed(LoginFailureReason reason);
    void SendCharRename(ResponseCodes result, CharacterRenameInfo const* renameInfo);
    void SendCharCustomize(ResponseCodes result, CharacterCustomizeInfo const* customizeInfo);
    void SendCharFactionChange(ResponseCodes result, CharacterFactionChangeInfo const* factionChangeInfo);
    void SendSetPlayerDeclinedNamesResult(DeclinedNameResult result, ObjectGuid guid);

    void HandleTC9PrepareForRedirect(WorldPacket& recvData);

    // played time

    // new

    // new inspect

    // new party stats

    // character view

    // repair

    // Knockback

    void AutoStoreLootItem(uint8 lootSlot);
    void HandleContinuePlayerLogin();
    void AbortLogin(LoginFailureReason reason);

    // GM Ticket opcodes

    void HandleSupportTicketSubmitComplaintOpcode(WorldPacket& recvData);

    void HandleMoveWorldportAck(); // for server-side calls

    void AcceptPartyInvite();
    void DeclinePartyInvite();
    // addon messages only go to clients that registered the prefix, unless they registered too many
    [[nodiscard]] bool IsAddonRegistered(std::string const& prefix) const;

    void SendActivateTaxiReply(ActivateTaxiReply reply);

    void HandleOpenWrappedItemCallback(uint8 bagIndex, uint8 slot, ObjectGuid::LowType itemLowGUID, PreparedQueryResult result);
    void HandleLoadActionsSwitchSpec(PreparedQueryResult result);
    void HandleCharacterAuraFrozen(PreparedQueryResult result);

    void HandleDuelAccepted(ObjectGuid guid);
    void HandleDuelCancelled();

    // Bank

    // Cata Reforging
    void SendReforgeResult(bool success);

    // Cata Transmogrification

    // Cata Void Storage
    void SendVoidStorageTransferResult(VoidTransferError result);

    void SendPlayerNotFoundNotice(std::string const& name);
    void SendPlayerAmbiguousNotice(std::string const& name);
    void SendWrongFactionNotice();
    void SendChatRestrictedNotice(ChatRestrictionType restriction);

    void HandleTutorialFlag (WorldPacket& recvData);

    //Pet
    void HandlePetActionHelper(Unit* pet, ObjectGuid guid1, uint32 spellid, uint16 flag, ObjectGuid guid2);

    void HandleCharRenameCallBack(std::shared_ptr<CharacterRenameInfo> renameInfo, PreparedQueryResult result);

    //Battleground

    // Battlefield
    void SendBfInvitePlayerToWar(uint32 battleId, uint32 zoneId, uint32 time);
    void SendBfInvitePlayerToQueue(uint32 battleId);
    void SendBfQueueInviteResponse(uint32 battleId, uint32 zoneId, bool canQueue = true, bool full = false);
    void SendBfEntered(uint32 battleId);
    void SendBfLeaveMessage(uint32 battleId, BFLeaveReason reason = BF_LEAVE_REASON_EXITED);

    // Looking for Dungeon/Raid
    void SendLfgPlayerLockInfo();
    void SendLfgPartyLockInfo();

    void SendLfgUpdatePlayer(lfg::LfgUpdateData const& updateData);
    void SendLfgUpdateParty(lfg::LfgUpdateData const& updateData);
    void SendLfgUpdateStatus(lfg::LfgUpdateData const& updateData, bool party);
    void SendLfgRoleChosen(ObjectGuid guid, uint8 roles);
    void SendLfgRoleCheckUpdate(lfg::LfgRoleCheck const& pRoleCheck);
    void SendLfgJoinResult(lfg::LfgJoinResultData const& joinData);
    void SendLfgQueueStatus(lfg::LfgQueueStatusData const& queueData);
    void SendLfgPlayerReward(lfg::LfgPlayerRewardData const& lfgPlayerRewardData);
    void SendLfgBootProposalUpdate(lfg::LfgPlayerBoot const& boot);
    void SendLfgUpdateProposal(lfg::LfgProposal const& proposal);
    void SendLfgDisabled();
    void SendLfgOfferContinue(uint32 dungeonEntry);
    void SendLfgTeleportError(uint8 err);

    // Arena Team

    // Socket gem

    // Guild Bank

    // Refer-a-Friend

    // Calendar

    // Black Market
    void SendBlackMarketOpenResult(ObjectGuid guid, Creature* auctioneer);
    void SendBlackMarketBidOnItemResult(int32 result, int32 marketId, WorldPackets::Item::ItemInstance& item);
    void SendBlackMarketWonNotification(BlackMarketEntry const* entry, Item const* item);
    void SendBlackMarketOutbidNotification(BlackMarketTemplate const* templ);

    void SendCalendarRaidLockout(InstanceSave const* save, bool add);
    void SendCalendarRaidLockoutUpdated(InstanceSave const* save, bool isExtended);

    void HandleCharCustomizeCallback(std::shared_ptr<CharacterCustomizeInfo> customizeInfo, PreparedQueryResult result);

    bool HandleSocketClosed();
    void SetOfflineTime(uint32 time) { _offlineTime = time; }
    uint32 GetOfflineTime() const { return _offlineTime; }
    bool IsKicked() const { return _kicked; }
    void SetKicked(bool val) { _kicked = val; }
    bool IsSocketClosed() const;

    /*
     * CALLBACKS
     */

    QueryCallbackProcessor& GetQueryProcessor() { return _queryProcessor; }
    TransactionCallback& AddTransactionCallback(TransactionCallback&& callback);
    SQLQueryHolderCallback& AddQueryHolderCallback(SQLQueryHolderCallback&& callback);
    void ProcessQueryCallbacks();

    void InitializeSession();
    void InitializeSessionCallback(CharacterDatabaseQueryHolder const& realmHolder, LoginDatabaseQueryHolder const& holder, uint32 clientCacheVersion);

    void SetPacketLogging(bool state);

    std::unique_ptr<WorldPacket> NextQueuedPacket();

    [[nodiscard]] bool IsHeadless() const { return _headless; }

private:
    QueryCallbackProcessor _queryProcessor;
    AsyncCallbackProcessor<TransactionCallback> _transactionCallbacks;
    AsyncCallbackProcessor<SQLQueryHolderCallback> _queryHolderProcessor;

    friend class World;
protected:
    class DosProtection
    {
        friend class World;
    public:
        enum class Policy
        {
            Process,
            Kick,
            Ban,
            Log,
            BlockingThrottle,
            DropPacket
        };

        DosProtection(WorldSession* s);
        Policy EvaluateOpcode(WorldPacket const& p, time_t const time) const;
    protected:
        WorldSession* Session;
    private:
        typedef std::unordered_map<uint16, PacketCounter> PacketThrottlingMap;
        // mark this member as "mutable" so it can be modified even in const functions
        mutable PacketThrottlingMap _PacketThrottlingMap;

        DosProtection(DosProtection const& right) = delete;
        DosProtection& operator=(DosProtection const& right) = delete;
    } AntiDOS;

private:
    // private trade methods
    void moveItems(Item* myItems[], Item* hisItems[]);

    bool CanUseBank(ObjectGuid bankerGUID = ObjectGuid::Empty) const;

    // logging helper
    void LogUnexpectedOpcode(WorldPacket* packet, char const* status, char const* reason);
    void LogUnprocessedTail(WorldPacket* packet);

    // EnumData helpers
    bool IsLegitCharacterForAccount(ObjectGuid guid)
    {
        return _legitCharacters.find(guid) != _legitCharacters.end();
    }

    // this stores the GUIDs of the characters who can login
    // characters who failed on Player::BuildEnumData shouldn't login
    GuidSet _legitCharacters;

    ObjectGuid::LowType m_GUIDLow;                     // set logined or recently logout player (while m_playerRecentlyLogout set)
    Player* _player;
    std::shared_ptr<WorldSocket> m_Socket[MAX_CONNECTION_TYPES];
    ConnectToKey _instanceConnectKey;
    uint32 _battlenetAccountId = 0;
    std::unique_ptr<CollectionMgr> _collectionMgr;
    std::string m_Address;

    AccountTypes _security;
    bool _skipQueue;
    uint32 _accountId;
    rbac::RBACData* _RBACData;
    std::string _accountName;
    uint32 _accountFlags;
    uint8 m_expansion;
    uint32 m_total_time;

    typedef std::list<AddonInfo> AddonsList;

    // Warden
    std::unique_ptr<Warden> _warden;                    // Remains nullptr if Warden system is not enabled by config

    Seconds _lastUpdateTime;                            // last time session was updated by world
    Seconds _createTime;                                // when session was created
    Seconds _previousPlayTime;                          // play time from previous session less than 5 hours ago
    time_t _logoutTime;
    bool m_inQueue;                                     // session wait in auth.queue
    bool m_playerLoading;                               // code processed in LoginPlayer
    ObjectGuid _playerLoadingGuid;                      // character waiting for the 6.x instance connection
    bool m_playerLogout;                                // code processed in LogoutPlayer
    bool m_playerRecentlyLogout;
    bool m_playerSave;
    LocaleConstant m_sessionDbcLocale;
    LocaleConstant m_sessionDbLocaleIndex;
    std::atomic<uint32> m_latency;
    AccountData m_accountData[NUM_ACCOUNT_DATA_TYPES];
    uint32 m_Tutorials[MAX_ACCOUNT_TUTORIAL_VALUES];
    bool   m_TutorialsChanged;
    AddonsList m_addonsList;
    std::vector<std::string> _registeredAddonPrefixes;
    bool _filterAddonMessages = false;
    uint32 recruiterId;
    bool isRecruiter;
    LockedQueue<WorldPacket*> _recvQueue;
    uint32 m_currentVendorEntry;
    ObjectGuid m_currentBankerGUID;
    uint32 _offlineTime;
    bool _kicked;
    // Packets cooldown
    time_t _calendarEventCreationCooldown;

    // Addon Message count for Metric
    std::atomic<uint32> _addonMessageReceiveCount;

    CircularBuffer<std::pair<int64, uint32>> _timeSyncClockDeltaQueue; // first member: clockDelta. Second member: latency of the packet exchange that was used to compute that clockDelta.
    int64 _timeSyncClockDelta;
    void ComputeNewClockDelta();

    std::map<uint32, uint32> _pendingTimeSyncRequests; // key: counter. value: server time when packet with that counter was sent.
    uint32 _timeSyncNextCounter;
    uint32 _timeSyncTimer;

    uint32 _orderCounter;

    bool const _headless;

    WorldSession(WorldSession const& right) = delete;
    WorldSession& operator=(WorldSession const& right) = delete;
};
#endif
/// @}
