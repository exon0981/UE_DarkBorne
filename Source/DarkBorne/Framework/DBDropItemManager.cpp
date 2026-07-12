// Fill out your copyright notice in the Description page of Project Settings.


#include "DBDropItemManager.h"
#include "Interfaces/ItemInterface.h"
#include "../Framework/BFL/ItemLibrary.h"
#include "../Items/PDA_ItemSlot.h"
#include "../Inventory/ItemObject.h"

ADBDropItemManager::ADBDropItemManager()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
}

void ADBDropItemManager::BeginPlay()
{
	Super::BeginPlay();

}
/// <summary>
/// Creates array of FItem instances for inventory. 
/// Each FItem instance is assigned a unique stat named Effect.
///  
/// </summary>
/// <param name="RowName"> : Name of a monster from which to generate items. Name must match the row names of DT_DropRate data table.
/// </param>
/// <returns>
/// Returns instances of generated items. Returns empty if an error occured.
/// </returns>
TArray<UItemObject*> ADBDropItemManager::GenerateItems(FName MonsterName)
{
	TArray<UItemObject*> ItemsToGenerate;
	ItemsToGenerate.Empty();

	if (ensureAlways(DT_DropRate && !MonsterName.IsNone()))
	{
		FDropRate* dropRate = DT_DropRate->FindRow<FDropRate>(MonsterName, FString::Printf(TEXT("Context")));

		if (!ensureAlwaysMsgf(dropRate, TEXT("Could not find RowName")))
			return TArray<UItemObject*>();

		if (!FindCumulativeProbability(dropRate))
			return TArray<UItemObject*>();


		const int amount = dropRate->Amount;
		const TArray<FDroppedItem>& items = dropRate->Items;

		for (int i = 0; i < amount; ++i)
		{
			FDroppedItem itemDropped;
			float rate = FMath::RandRange(0.f, 1.f);

			for (int j = 0; j < CumulativeProbability.Num(); ++j)
			{
				if (rate <= CumulativeProbability[j])
				{
					itemDropped = items[j];
					break;
				}
			}

			if (ensure(ItemTableMap.Contains(itemDropped.ItemType)))
			{
				UDataTable* ItemTable = *ItemTableMap.Find(itemDropped.ItemType);
				TArray<FName> RowNames = ItemTable->GetRowNames();

				int rand = FMath::RandRange(0, RowNames.Num() - 1);

				FItem tempItem = CreateItemData(ItemTable, RowNames[rand]);
				if (tempItem.IsValid())
				{
					auto ItemObject = NewObject<UItemObject>(this);
					if (ItemObject)
					{
						ItemObject->Initialize(tempItem);
						ItemsToGenerate.Add(ItemObject);
					}
				}

				else
					UE_LOG(LogTemp, Warning, TEXT("Invalid Item generated. Generated Item will be discarded"));
			}
			else return TArray<UItemObject*>();
		}
	}
	else return TArray<UItemObject*>();

	return ItemsToGenerate;
}

UItemObject* ADBDropItemManager::GenerateItemByName(FName ItemName, EItemType Type)
{
	UDataTable* DT = *ItemTableMap.Find(Type);

	if (DT && DT->FindRow<FItem>(ItemName, FString::Printf(TEXT("Context"))))
	{
		FItem ItemData = CreateItemData(DT, ItemName);
		auto ItemObject = NewObject<UItemObject>(this);
		if (ItemObject)
		{
			ItemObject->Initialize(ItemData);
			return ItemObject;
		}
	}

	return nullptr;
}

FItem ADBDropItemManager::CreateItemData(UDataTable* Table, FName RowName)
{
	GetWorld();
	FItem* item = Table->FindRow<FItem>(RowName, FString::Printf(TEXT("Context")));
	if (!item)
		return FItem();

	FItem tempItem = *item;

	AssignRarity(tempItem);
	AssignSlotHolder(tempItem);
	AssignEnchantment(tempItem);

	return tempItem;
}

bool ADBDropItemManager::FindCumulativeProbability(const FDropRate* DropRate)
{
	CumulativeProbability.Empty();
	float sum = 0.f;

	for (auto item : DropRate->Items) {
		sum += item.Probability;
		CumulativeProbability.Add(sum);
	}

	if (!ensureAlwaysMsgf(sum == 1.f, TEXT("Total probability sum is not 100 percent"))) {
		CumulativeProbability.Empty();
		return false;
	}

	return true;
}

void ADBDropItemManager::AssignSlotHolder(FItem& Item)
{
	Item.Initialize();
}

void ADBDropItemManager::AssignRarity(FItem& Item)
{
	if (!ensureAlways(Item.IsValid() == false)) return;
	int max = Item.GetRaritiesFromItemSlot().Num() - 1;
	int rand = FMath::RandRange(0, max);

	FRarity Rarity = Item.GetRaritiesFromItemSlot()[rand];

	GenerateStatFromRange(Rarity.Range);
		
	Item.SetRarity(Rarity);
}

void ADBDropItemManager::AssignEnchantment(FItem& Item)
{
	if (!ensureAlways(Item.IsValid())) return;

	if (!ensureAlwaysMsgf(Item.GetRarity().GetRarityType() != (uint8)ERarityType::NONE, TEXT("Ensure AssignRarity() is called before AssignEnhancement()")))
		return;

	if (!ensureAlways(!Enchantments.IsEmpty()))
		return;

	if ((int)Item.GetSlotType() >= (int)ESlotType::_ENCHANTMENTMARK_)
		return;

	uint8 EnchantmentNum = 0;
	EnchantmentNum = Item.GetRarity().GetRarityType();

	std::vector<bool> attributeCheck;
	TArray<FAttribute> AttributesGenerated;

	TArray<FPhysicalDamage> PhysicalDamageGenerated;
	std::vector<bool> PhysicalDamageCheck;

	int counter = 0;

	//No enchantments for common. 1 enchantment for rare. 2 enchantments for epic;
	while (counter < EnchantmentNum - 1)
	{
		int DataTableIndex = FMath::RandRange(0, Enchantments.Num() - 1);
		FName SlotTypeName = FName(UEnum::GetValueAsString(Item.GetSlotType()));

		UDataTable* Enchantment = Enchantments[DataTableIndex];

		if (Enchantment) {
			switch ((EDarkBornStatType)DataTableIndex)
			{
			case EDarkBornStatType::ATTRIBUTE: {
				FAttributeHolder* Holder = Enchantment->FindRow<FAttributeHolder>(SlotTypeName, FString::Printf(TEXT("Enchantment")));

				if (!Holder || Holder->Attributes.IsEmpty())
					continue;

				ProcessEnchantment<FAttribute>(AttributesGenerated, Holder->Attributes, attributeCheck, counter);
				break;
			}
			case EDarkBornStatType::PHYSICALDAMAGE: {
				FPhysicalDamageHolder* Holder = Enchantment->FindRow<FPhysicalDamageHolder>(SlotTypeName, FString::Printf(TEXT("Enchantment")));

				if (!Holder || Holder->PhysicalDamages.IsEmpty())
					continue;
				ProcessEnchantment<FPhysicalDamage>(PhysicalDamageGenerated, Holder->PhysicalDamages, PhysicalDamageCheck, counter);
			}
			}
		}
	}

	FDarkBorneStats holder;

	holder.Attributes = AttributesGenerated;

	holder.PhysicalDamages = PhysicalDamageGenerated;

	Item.SetEnchantments(holder);
}

