/*
 * Copyright (C) 2008-2010 Trinity <http://www.trinitycore.org/>
 * Copyright (C) 2005-2009 MaNGOS <http://getmangos.com/>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#ifndef AUCTION_HOUSE_BOT_H
#define AUCTION_HOUSE_BOT_H

#include "Common.h"

struct AuctionEntry;
class Player;
class WorldSession;

#include "ItemTemplate.h"

#define AHB_GREY        0
#define AHB_WHITE       1
#define AHB_GREEN       2
#define AHB_BLUE        3
#define AHB_PURPLE      4
#define AHB_ORANGE      5
#define AHB_YELLOW      6
#define AHB_8 6
#define AHB_GREY_TG     0
#define AHB_WHITE_TG    1
#define AHB_GREEN_TG    2
#define AHB_BLUE_TG     3
#define AHB_PURPLE_TG   4
#define AHB_ORANGE_TG   5
#define AHB_YELLOW_TG   6
#define AHB_GREY_I      7
#define AHB_WHITE_I     8
#define AHB_GREEN_I     9
#define AHB_BLUE_I      10
#define AHB_PURPLE_I    11
#define AHB_ORANGE_I    12
#define AHB_YELLOW_I    13


class qualityData {
public:
    qualityData() { itemTypeConfig.clear(); itemListing.clear(); }
    vector<uint32> itemTypeConfig;
    vector<uint32> itemListing;
};
class AHBConfig
{
    
private:
    uint32 AHID;
    uint32 AHFID;
    uint32 minItems;
    uint32 currentItems;
    uint32 buyerBidsPerInterval;
    uint32 biddingInterval;
    uint32 totalWeights;
    //classMap sorts Class : SubClass : Quality : qualityData
    unordered_map < uint32, unordered_map<uint32, unordered_map<uint32, qualityData*>>> classMap;
public:
    AHBConfig(uint32 ahid){
        AHID = ahid;
        AHFID = 120;
        if (ahid == 2) AHFID = 55;
        if (ahid == 6) AHFID = 29;
        if (ahid == 7) AHFID = 120;
    }
    AHBConfig(){    }
    uint32 GetAHID()    {return AHID;}
    uint32 GetAHFID()   {return AHFID; }
    void SetBidsPerInterval(uint32 value){buyerBidsPerInterval = value;}
    uint32 GetBidsPerInterval() { return buyerBidsPerInterval; }
    void SetBiddingInterval(uint32 value) { biddingInterval = value; }
    uint32 GetBiddingInterval() { return biddingInterval; }
    uint32 GetTotalWeights() { return totalWeights; }
    void SetTotalWeights(uint32 value) { totalWeights = value; }
    uint32 calculateTotalWeights() {
        totalWeights = 0;
        for (uint32 i = 0; i < 16; i++) {
            for (uint32 j = 0; j < 20; j++) {
                for (uint32 k = 0; k < 8; k++) {
                    try {
                        if (classMap.at(i).at(j).at(k)->itemListing.size() < 1) {
                            classMap.at(i).at(j).at(k)->itemTypeConfig[0] = 0;
                        }
                        totalWeights += classMap.at(i).at(j).at(k)->itemTypeConfig[0];
                    }
                    catch (out_of_range) { continue; }
                }
            }
        }
        return totalWeights;
    }
    bool LoadItem(uint32 Class, uint32 subClass, uint32 quality, uint32 itemID) {
        try {
            classMap.at(Class).at(subClass).at(quality)->itemListing.push_back(itemID);
            return true;
        }
        catch (out_of_range) { return false; }
    }
    void SetMinItems(uint32 value) { minItems = value; }
    uint32 GetMinItems() { return minItems; }
    void SetCurrentItems(uint32 value) { currentItems = value; }
    uint32 GetCurrentItems() { return currentItems; }
    uint32 IncrementItems() { return ++currentItems; }
    uint32 DecrementItems() { if (currentItems > 0)return --currentItems; return 0; }
    unordered_map < uint32, unordered_map<uint32, unordered_map<uint32, qualityData*>>>& GetClassMap() { return classMap ; }
    qualityData* getQualityData(int Class, int subclass, int quality) { try { return classMap.at(Class).at(subclass).at(quality); } catch (out_of_range) { return nullptr; }; }
    void insertSubclass(int Class, unordered_map<uint32, unordered_map<uint32, qualityData*>> subclass) { classMap.insert_or_assign(Class, subclass); }
    void insertQuality(int Class, int subclass, unordered_map<uint32, qualityData*> quality) {
        try { classMap.at(Class).insert_or_assign(subclass, quality); }
        catch (out_of_range) {
            sLog->outString("AHBotError: Class %u not in ClassMap", Class);
        }
    }
    void insertQualityData(int Class, int subclass, int quality, int weight, int MinPrice, int MaxPrice, int Minbid, int Maxbid, int MaxStack, int BuyerPrice) {
        try {
            qualityData* qd = new qualityData;
            qd->itemTypeConfig.push_back(weight);
            qd->itemTypeConfig.push_back(MinPrice);
            qd->itemTypeConfig.push_back(MaxPrice);
            qd->itemTypeConfig.push_back(Minbid);
            qd->itemTypeConfig.push_back(Maxbid);
            qd->itemTypeConfig.push_back(MaxStack);
            qd->itemTypeConfig.push_back(BuyerPrice);
            classMap.at(Class).at(subclass).insert_or_assign(quality, qd);
        }
        catch (out_of_range) {
            sLog->outString("AHBotError: Class %u subclass %u not in ClassMap", Class, subclass);
        }
    }
    void insertItem(int Class, int subclass, int quality, int itemID) {
        classMap.at(Class).at(subclass).at(quality)->itemListing.push_back(itemID);
    }
    
    ~AHBConfig()
    {
    }
};
class AuctionHouseBot
{
private:

    bool debug_Out;
    bool debug_Out_Filters;

    bool AHBSeller;
    bool AHBBuyer;
    bool BuyMethod;
    bool SellMethod;

    uint32 AHBplayerAccount;
    uint32 AHBplayerGUID;
    uint32 ItemsPerCycle;

    //Begin Filters

    bool Vendor_Items;
    bool Loot_Items;
    bool Other_Items;
    bool Vendor_TGs;
    bool Loot_TGs;
    bool Other_TGs;

    bool No_Bind;
    bool Bind_When_Picked_Up;
    bool Bind_When_Equipped;
    bool Bind_When_Use;
    bool Bind_Quest_Item;

    bool DisableNoBuyPrice;
    bool DisableNoSellPrice;

    bool DisablePermEnchant;
    bool DisableConjured;
    bool DisableGems;
    bool DisableMoney;
    bool DisableMoneyLoot;
    bool DisableLootable;
    bool DisableKeys;
    bool DisableDuration;
    bool DisableBOP_Or_Quest_NoReqLevel;

    bool DisableWarriorItems;
    bool DisablePaladinItems;
    bool DisableHunterItems;
    bool DisableRogueItems;
    bool DisablePriestItems;
    bool DisableDKItems;
    bool DisableShamanItems;
    bool DisableMageItems;
    bool DisableWarlockItems;
    bool DisableUnusedClassItems;
    bool DisableDruidItems;

    uint32 DisableItemsBelowLevel;
    uint32 DisableItemsAboveLevel;
    uint32 DisableTGsBelowLevel;
    uint32 DisableTGsAboveLevel;
    uint32 DisableItemsBelowGUID;
    uint32 DisableItemsAboveGUID;
    uint32 DisableTGsBelowGUID;
    uint32 DisableTGsAboveGUID;
    uint32 DisableItemsBelowReqLevel;
    uint32 DisableItemsAboveReqLevel;
    uint32 DisableTGsBelowReqLevel;
    uint32 DisableTGsAboveReqLevel;
    uint32 DisableItemsBelowReqSkillRank;
    uint32 DisableItemsAboveReqSkillRank;
    uint32 DisableTGsBelowReqSkillRank;
    uint32 DisableTGsAboveReqSkillRank;

    std::set<uint32> DisableItemStore;

    //End Filters

    AHBConfig AllianceConfig;
    AHBConfig HordeConfig;
    AHBConfig NeutralConfig;

    time_t _lastrun_a;
    time_t _lastrun_h;
    time_t _lastrun_n;

    inline uint32 minValue(uint32 a, uint32 b) { return a <= b ? a : b; };
    void addNewAuctions(Player *AHBplayer, AHBConfig *config);
    void addNewAuctionBuyerBotBid(Player *AHBplayer, AHBConfig *config, WorldSession *session);

//    friend class ACE_Singleton<AuctionHouseBot, ACE_Null_Mutex>;
    AuctionHouseBot();

public:
    static AuctionHouseBot* instance()
    {
        static AuctionHouseBot instance;
        return &instance;
    }

    ~AuctionHouseBot();
    void Update();
    void Initialize();
    void loadItem(AHBConfig * config, ItemTemplate item);
    void InitializeConfiguration();
    void LoadValues(AHBConfig*);
    void DecrementItemCounts(AuctionEntry* ah, uint32 itemEntry);
    void IncrementItemCounts(AuctionEntry* ah);
    void Commands(uint32, uint32, uint32, char*);
    uint32 GetAHBplayerGUID() { return AHBplayerGUID; };
};

#define auctionbot AuctionHouseBot::instance()

#endif
