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

#include "CommandScript.h"
#include "Language.h"
#include "ObjectMgr.h"
#include "Pet.h"
#include "Player.h"
#include "PlayerCommand.h"
#include "RBAC.h"
#include "SpellInfo.h"
#include "SpellMgr.h"

using namespace Acore::ChatCommands;

class learn_commandscript : public CommandScript
{
public:
    learn_commandscript() : CommandScript("learn_commandscript") { }

    ChatCommandTable GetCommands() const override
    {
        static ChatCommandTable learnAllMyCommandTable =
        {
            { "class",      HandleLearnAllMyClassCommand,      rbac::RBAC_PERM_COMMAND_LEARN_ALL_MY_CLASS, Console::No },
            { "pettalents", HandleLearnAllMyPetTalentsCommand, rbac::RBAC_PERM_COMMAND_LEARN_MY_PETTALENTS, Console::No },
            { "trainer",    HandleLearnAllMyTrainerSpellsCommand, rbac::RBAC_PERM_COMMAND_LEARN_ALL_MY_SPELLS, Console::No },
            { "talents",    HandleLearnAllMyTalentsCommand,    rbac::RBAC_PERM_COMMAND_LEARN_ALL_TALENTS, Console::No },
            { "quest",      HandleLearnAllMyQuestSpells,       rbac::RBAC_PERM_COMMAND_LEARN_ALL_MY_SPELLS, Console::No }
        };

        static ChatCommandTable learnAllCommandTable =
        {
            { "my",        learnAllMyCommandTable },
            { "gm",        HandleLearnAllGMCommand,            rbac::RBAC_PERM_COMMAND_LEARN_ALL_GM, Console::No },
            { "crafts",    HandleLearnAllCraftsCommand,        rbac::RBAC_PERM_COMMAND_LEARN_ALL_CRAFTS, Console::No },
            { "default",   HandleLearnAllDefaultCommand,       rbac::RBAC_PERM_COMMAND_LEARN_ALL_DEFAULT, Console::No },
            { "lang",      HandleLearnAllLangCommand,          rbac::RBAC_PERM_COMMAND_LEARN_ALL_LANG, Console::No },
            { "recipes",   HandleLearnAllRecipesCommand,       rbac::RBAC_PERM_COMMAND_LEARN_ALL_RECIPES, Console::No },
        };

        static ChatCommandTable learnCommandTable =
        {
            { "all",  learnAllCommandTable },
            { "",     HandleLearnCommand,                      rbac::RBAC_PERM_COMMAND_LEARN, Console::No }
        };

        static ChatCommandTable commandTable =
        {
            { "learn",   learnCommandTable },
            { "unlearn", HandleUnLearnCommand,             rbac::RBAC_PERM_COMMAND_UNLEARN, Console::No }
        };
        return commandTable;
    }

    static bool HandleLearnCommand(ChatHandler* handler, SpellInfo const* spell, Optional<EXACT_SEQUENCE("all")> allRanks)
    {
        Player* targetPlayer = handler->getSelectedPlayer();

        if (!targetPlayer)
        {
            handler->SendErrorMessage(LANG_PLAYER_NOT_FOUND);
            return false;
        }

        return Acore::PlayerCommand::HandleLearnSpellCommand(handler, targetPlayer, spell, allRanks);
    }

    static bool HandleLearnAllGMCommand(ChatHandler* handler)
    {
        for (uint32 i = 0; i < sSpellMgr->GetSpellInfoStoreSize(); ++i)
        {
            SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(i);
            if (!spellInfo || !SpellMgr::IsSpellValid(spellInfo))
                continue;

            if (!spellInfo->IsAbilityOfSkillType(SKILL_INTERNAL))
                continue;

            handler->GetSession()->GetPlayer()->learnSpell(i);
        }

        handler->SendSysMessage(LANG_LEARNING_GM_SKILLS);
        return true;
    }

    static bool HandleLearnAllMyClassCommand(ChatHandler* handler)
    {
        HandleLearnAllMyTrainerSpellsCommand(handler);
        HandleLearnAllMyTalentsCommand(handler);
        HandleLearnAllMyQuestSpells(handler);
        return true;
    }

    static bool HandleLearnAllMyQuestSpells(ChatHandler* handler)
    {
        Player* player = handler->GetPlayer();
        for (auto const& questPair : sObjectMgr->GetQuestTemplates())
        {
            Quest const* quest = questPair.second;
            if (quest->GetRequiredClasses() && player->SatisfyQuestClass(quest, false))
                player->learnQuestRewardedSpells(quest);
        }

        return true;
    }

    static bool HandleLearnAllMyTrainerSpellsCommand(ChatHandler* handler)
    {
        if (!sChrClassesStore.LookupEntry(handler->GetSession()->GetPlayer()->getClass()))
            return true;

        Player* player = handler->GetPlayer();
        std::vector<Trainer::Trainer const*> const& trainers = sObjectMgr->GetClassTrainers(player->getClass());

        bool hadNew;
        do
        {
            hadNew = false;
            for (Trainer::Trainer const* trainer : trainers)
            {
                if (!trainer->IsTrainerValidForPlayer(player))
                    continue;

                for (Trainer::Spell const& trainerSpell : trainer->GetSpells())
                {
                    if (!trainer->CanTeachSpell(player, &trainerSpell))
                        continue;

                    if (trainerSpell.IsCastable())
                        player->CastSpell(player, trainerSpell.SpellId, true);
                    else
                        player->learnSpell(trainerSpell.SpellId, false);

                    // a script can refuse the learn or force the spell's state, so only go around
                    // again when the spell actually stopped being teachable
                    if (!trainer->CanTeachSpell(player, &trainerSpell))
                        hadNew = true;
                }
            }
        } while (hadNew);

        handler->SendSysMessage(LANG_COMMAND_LEARN_CLASS_SPELLS);
        return true;
    }

    static bool HandleLearnAllMyTalentsCommand(ChatHandler* handler)
    {
        Player* player = handler->GetSession()->GetPlayer();

        // 5.4.8: one talent per tier, so this learns the first talent of every tier
        for (TalentEntry const* talentInfo : sTalentStore)
        {
            if (talentInfo->ClassID != player->getClass())
                continue;

            SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(talentInfo->SpellId);
            if (!spellInfo || !SpellMgr::IsSpellValid(spellInfo))
                continue;

            player->LearnTalent(talentInfo->TalentID, true);
        }

        player->SetFreeTalentPoints(0);
        player->SendTalentsInfoData(false);

        handler->SendSysMessage(LANG_COMMAND_LEARN_CLASS_TALENTS);
        return true;
    }

    static bool HandleLearnAllMyPetTalentsCommand(ChatHandler* handler)
    {
        // 5.4.8: hunter pets have specializations instead of talent trees
        handler->SendErrorMessage(LANG_WRONG_PET_TYPE);
        return false;
    }

    static bool HandleLearnAllLangCommand(ChatHandler* handler)
    {
        for (LanguageDesc const& langDesc : lang_description)
            if (uint32 langSpellId = langDesc.spell_id)
            {
                handler->GetPlayer()->learnSpell(langSpellId);
                handler->GetPlayer()->SetSkill(langDesc.skill_id, 0, 300, 300);
            }

        handler->SendSysMessage(LANG_COMMAND_LEARN_ALL_LANG);
        return true;
    }

    static bool HandleLearnAllDefaultCommand(ChatHandler* handler, Optional<PlayerIdentifier> player)
    {
        if (!player)
            player = PlayerIdentifier::FromTargetOrSelf(handler);
        if (!player || !player->IsConnected())
            return false;

        Player* target = player->GetConnectedPlayer();
        target->LearnDefaultSkills();
        target->LearnCustomSpells();
        target->learnQuestRewardedSpells();

        handler->PSendSysMessage(LANG_COMMAND_LEARN_ALL_DEFAULT_AND_QUEST, handler->GetNameLink(target));
        return true;
    }

    static bool HandleLearnAllCraftsCommand(ChatHandler* handler)
    {
        Player* target = handler->GetSession()->GetPlayer();

        for (uint32 i = 0; i < sSkillLineStore.GetNumRows(); ++i)
        {
            SkillLineEntry const* skillInfo = sSkillLineStore.LookupEntry(i);
            if (!skillInfo)
                continue;

            if ((skillInfo->categoryId == SKILL_CATEGORY_PROFESSION || skillInfo->categoryId == SKILL_CATEGORY_SECONDARY) &&
                    skillInfo->canLink)                             // only prof. with recipes have
            {
                HandleLearnSkillRecipesHelper(target, skillInfo->id);

                uint16 const maxLevel = target->GetPureMaxSkillValue(skillInfo->id);
                target->SetSkill(skillInfo->id, target->GetSkillStep(skillInfo->id), maxLevel, maxLevel);
            }
        }

        handler->SendSysMessage(LANG_COMMAND_LEARN_ALL_CRAFT);
        return true;
    }

    static bool HandleLearnAllRecipesCommand(ChatHandler* handler, WTail namePart)
    {
        //  Learns all recipes of specified profession and sets skill to max
        //  Example: .learn all_recipes enchanting

        Player* target = handler->getSelectedPlayer();
        if (!target)
        {
            handler->SendSysMessage(LANG_PLAYER_NOT_FOUND);
            return false;
        }

        if (namePart.empty())
            return false;

        // converting string that we try to find to lower case
        wstrToLower(namePart);

        SkillLineEntry const* targetSkillInfo = nullptr;
        char const* name = nullptr;
        for (uint32 i = 1; i < sSkillLineStore.GetNumRows(); ++i)
        {
            SkillLineEntry const* skillInfo = sSkillLineStore.LookupEntry(i);
            if (!skillInfo)
                continue;

            if ((skillInfo->categoryId != SKILL_CATEGORY_PROFESSION &&
                 skillInfo->categoryId != SKILL_CATEGORY_SECONDARY) ||
                !skillInfo->canLink)                            // only prof with recipes have set
                continue;

            uint8 locale = 0;
            for (; locale < TOTAL_LOCALES; ++locale)
            {
                name = skillInfo->name[locale];
                if (!name || !*name)
                    continue;

                if (Utf8FitTo(name, namePart))
                    break;
            }

            if (locale < TOTAL_LOCALES)
            {
                targetSkillInfo = skillInfo;
                break;
            }
        }

        if (!(name && targetSkillInfo))
            return false;

        HandleLearnSkillRecipesHelper(target, targetSkillInfo->id);

        uint16 maxLevel = target->GetPureMaxSkillValue(targetSkillInfo->id);
        target->SetSkill(targetSkillInfo->id, target->GetSkillStep(targetSkillInfo->id), maxLevel, maxLevel);
        handler->PSendSysMessage(LANG_COMMAND_LEARN_ALL_RECIPES, name);
        return true;
    }

    static void HandleLearnSkillRecipesHelper(Player* player, uint32 skillId)
    {
        // Rank spells (Apprentice -> Grand Master) must be learned so that the
        // skill-cleanup loop in Player::SetSkill (which calls removeSpell on the
        // first spell in each chain) can walk forward and strip every rank on
        // profession unlearn. Without the first rank in the spellbook that loop
        // bails out and the leftover rank spells re-grant the skill after relog
        // (issue #2330).
        for (uint32 rankSpell : sSpellMgr->GetSkillRankSpells(skillId))
            player->learnSpell(rankSpell);

        uint32 classmask = player->getClassMask();

        for (SkillLineAbilityEntry const* skillLine : GetSkillLineAbilitiesBySkillLine(skillId))
        {
            // not high rank
            if (skillLine->SupercededBySpell)
                continue;

            // skip racial skills
            if (skillLine->RaceMask != 0)
                continue;

            // skip wrong class skills
            if (skillLine->ClassMask && (skillLine->ClassMask & classmask) == 0)
                continue;

            SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(skillLine->Spell);
            if (!spellInfo || !SpellMgr::IsSpellValid(spellInfo))
                continue;

            player->learnSpell(skillLine->Spell);
        }
    }

    static bool HandleUnLearnCommand(ChatHandler* handler, SpellInfo const* spell, Optional<EXACT_SEQUENCE("all")> allRanks)
    {
        Player* target = handler->getSelectedPlayer();
        if (!target)
        {
            handler->SendErrorMessage(LANG_NO_CHAR_SELECTED);
            return false;
        }

        return Acore::PlayerCommand::HandleUnlearnSpellCommand(handler, target, spell, allRanks);
    }
};

void AddSC_learn_commandscript()
{
    new learn_commandscript();
}
