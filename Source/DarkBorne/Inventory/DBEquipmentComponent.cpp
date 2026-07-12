// Fill out your copyright notice in the Description page of Project Settings.


#include "DBEquipmentComponent.h"
#include "../ItemTypes/ItemType.h"
#include "PlayerEquipmentComponent.h"
#include "ItemObject.h"
#include "../Framework/BFL/ItemLibrary.h"
#include "Net/UnrealNetwork.h"
#include "Engine/ActorChannel.h"
#include "../DBWeapon/DBRogueWeaponComponent.h"
#include "PlayerEquipmentComponent.h"
#include "../DBCharacters/DBRogueCharacter.h"
#include "../DBPlayerWidget/DBPlayerWidget.h"
#include "../Status/CharacterStatusComponent.h"
#include "Kismet/GameplayStatics.h"

UDBEquipmentComponent::UDBEquipmentComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	bInvalidSlot = false;
	bOccupiedSlot = false;
	bHasGameStarted = false;
}

void UDBEquipmentComponent::BeginPlay()
{
	Super::BeginPlay();

	if (GetOwner())
	{
		if (GetOwner()->HasAuthority())
		{
			ESlotType slotNum = ESlotType::NONE;
			int32 size = (int32)slotNum - 1;
			InventoryItems.Items.SetNum(size, EAllowShrinking::No);

			int val = 10;
		}
	}
}

void UDBEquipmentComponent::OnRep_Items(FInventoryItems Old)
{
	Super::OnRep_Items(Old);

	for (int i = 0; i < Old.Items.Num(); ++i)
	{
		if (Old.Items[i] != InventoryItems.Items[i])
		{
			if (Old.Items[i]) //Remove stats from player
			{
				if (GetOwner()->HasAuthority())
				{
					UCharacterStatusComponent::AdjustAddedStats(GetOwner(), Old.Items[i], false);
				}
			}

			if (InventoryItems.Items[i]) //Add stats to player
			{
				if (GetOwner()->HasAuthority())
				{
					UCharacterStatusComponent::AdjustAddedStats(GetOwner(), InventoryItems.Items[i], true);
				}

				if (bHasGameStarted)
				{
					auto Pawn = Cast<APawn>(GetOwner());
					if (Pawn && Pawn->IsLocallyControlled())
					{
						UGameplayStatics::PlaySound2D(GetOwner(), InventoryItems.Items[i]->GetEquipSound());
					}
					else
					{
						Pawn = Cast<APawn>(InventoryItems.InteractingActor);
						if (Pawn && Pawn->IsLocallyControlled())
						{
							UGameplayStatics::PlaySound2D(GetOwner(), InventoryItems.Items[i]->GetInventorySound());
						}
					}
				}
				else 
				{
					bHasGameStarted = true;
				}
			}
		}
	}

	ADBRogueCharacter* RoguePlayer = Cast<ADBRogueCharacter>(GetOwner());
	if (RoguePlayer->PlayerWidget)
		RoguePlayer->PlayerWidget->UpdateSlot(GetSlots());
}

void UDBEquipmentComponent::RemoveItem(UItemObject* ItemObject, AActor* InitiatedActor)
{
	if (!IsValid(ItemObject)) return;
	int32 index = UItemLibrary::GetSlotIndexByObject(ItemObject);
	if (ItemObject != InventoryItems.Items[index])
		return;

	/*if (!ensureAlwaysMsgf(TaxiToServer->GetOwner()->HasNetOwner() ||
		this->GetOwner()->HasNetOwner(), TEXT("ensure this function has a reference to object that has owning connection for RPC call")))
		return;*/

	if (this->GetOwner()->HasNetOwner())
	{
		UE_LOG(LogTemp, Warning, TEXT("Has NetOwner()"));
		Server_RemoveItem(ItemObject, InitiatedActor);
	}
	else {
		UE_LOG(LogTemp, Warning, TEXT("Has no NetOwner()"));
		auto EquipCompTaxi = InitiatedActor->GetComponentByClass<UDBEquipmentComponent>();
		EquipCompTaxi->Server_TaxiForRemoveItem(this, ItemObject, InitiatedActor);
	}
}

void UDBEquipmentComponent::Server_TaxiForRemoveItem_Implementation(UBaseInventoryComponent* TaxiedInventoryComp, UItemObject* ItemObject, AActor* InitiatedActor)
{
	auto TaxiedEquipComp = Cast<UDBEquipmentComponent>(TaxiedInventoryComp);
	if (ensureAlways(TaxiedEquipComp))
	{
		TaxiedEquipComp->Server_RemoveItem(ItemObject, InitiatedActor);
	}
}

void UDBEquipmentComponent::Server_RemoveItem_Implementation(UItemObject* ItemObject, AActor* InitiatedActor)
{
	if (!ensureAlways(ItemObject))
		return;

	FInventoryItems Old = InventoryItems;

	int32 index = UItemLibrary::GetSlotIndexByObject(ItemObject);
	UItemObject* TempItemObj = InventoryItems.Items[index];
	InventoryItems.Items[index] = nullptr;
	InventoryItems.InteractingActor = InitiatedActor;

	auto WeaponComp = GetOwner()->GetComponentByClass<UDBRogueWeaponComponent>();
	if (WeaponComp)
	{
		WeaponComp->TryRemoveRogueItem(TempItemObj);
	}

	OnRep_Items(Old);
}

bool UDBEquipmentComponent::TryAddItem(UItemObject* ItemObject, AActor* InitiatedActor)
{
	if (!IsValid(ItemObject) || InventoryItems.Items.IsEmpty())
		return false;
	if (!InitiatedActor)
		return false;

	int32 index = UItemLibrary::GetSlotIndexByObject(ItemObject);
	if (InventoryItems.Items[index])
	{
		auto PlayerInventoryComp = GetOwner()->GetComponentByClass<UPlayerEquipmentComponent>();
		if (PlayerInventoryComp && !PlayerInventoryComp->HasRoomFor(InventoryItems.Items[index]))
			return false;
	}

	AddItem(ItemObject, InitiatedActor);
	return true;
}

void UDBEquipmentComponent::AddItem(UItemObject* ItemObject, AActor* InitiatedActor)
{
	if (!IsValid(ItemObject)) return;
	if (!InitiatedActor)
		return;

	if (this->GetOwner()->HasNetOwner())
	{
		UE_LOG(LogTemp, Warning, TEXT("Has NetOwner()"));
		Server_AddItem(ItemObject, InitiatedActor);
	}
	else {
		UE_LOG(LogTemp, Warning, TEXT("Has no NetOwner()"));
		auto EquipCompTaxi = InitiatedActor->GetComponentByClass<UDBEquipmentComponent>();
		EquipCompTaxi->Server_TaxiForAddItem(this, ItemObject, InitiatedActor);
	}
}

void UDBEquipmentComponent::Server_TaxiForAddItem_Implementation(UBaseInventoryComponent* TaxiedInventoryComp, UItemObject* ItemObject, AActor* InitiatedActor)
{
	auto TaxiedEquipComp = Cast<UDBEquipmentComponent>(TaxiedInventoryComp);
	if (ensureAlways(TaxiedEquipComp))
	{
		TaxiedEquipComp->Server_AddItem(ItemObject, InitiatedActor);
	}
}

void UDBEquipmentComponent::Server_AddItem_Implementation(UItemObject* ItemObject, AActor* InitiatedActor)
{
	if (!ensureAlways(ItemObject))
		return;


	if (InventoryItems.Items.IsEmpty())
		return;

	FInventoryItems Old = InventoryItems;

	auto PlayerInventoryComp = GetOwner()->GetComponentByClass<UPlayerEquipmentComponent>();
	bool bInventoryHasItemObject = false;

	//if ItemObject is in inventory, remove ItemObject from inventory.
	if (PlayerInventoryComp->HasItem(ItemObject))
	{
		bInventoryHasItemObject = true;
		PlayerInventoryComp->RemoveItem(ItemObject, InitiatedActor);
	}

	int32 index = UItemLibrary::GetSlotIndexByObject(ItemObject);
	UItemObject* EquippedItem = InventoryItems.Items[index];
	if (EquippedItem)
	{
		if (PlayerInventoryComp && !PlayerInventoryComp->HasRoomFor(EquippedItem))
		{
			//Place back ItemObject into inventory
			if (bInventoryHasItemObject)
			{
				PlayerInventoryComp->TryAddItem(ItemObject, InitiatedActor);
			}
			return;
		}
		PlayerInventoryComp->TryAddItem(EquippedItem, InitiatedActor);
	}

	InventoryItems.Items[index] = ItemObject;
	InventoryItems.InteractingActor = InitiatedActor;

	ItemObject->TryDestroyItemActor();

	auto WeaponComp = GetOwner()->GetComponentByClass<UDBRogueWeaponComponent>();
	if (WeaponComp)
	{
		WeaponComp->PassItem(ItemObject);
	}

	OnRep_Items(Old);
}


void UDBEquipmentComponent::ProcessPressInput(UItemObject* ItemObject, AActor* InitiatedActor, FInventoryInput InventoryInput)
{
	if (!ItemObject)
		return;
	if (!(InitiatedActor && InitiatedActor->HasNetOwner()))
		return;
	if (!HasItem(ItemObject))
		return;

	if (GetOwner()->HasNetOwner())
	{
		Server_ProcessPressInput(ItemObject, InitiatedActor, InventoryInput);
	}
	else
	{
		auto TaxiToServer = InitiatedActor->GetComponentByClass<UDBEquipmentComponent>();
		TaxiToServer->Server_TaxiForProcessPressInput(this, ItemObject, InitiatedActor, InventoryInput);
	}
}


void UDBEquipmentComponent::Server_TaxiForProcessPressInput_Implementation(UBaseInventoryComponent* TaxiedInventoryComp, UItemObject* ItemObject, AActor* InitiatedActor, FInventoryInput InventoryInput)
{
	auto Taxied = Cast<UDBEquipmentComponent>(TaxiedInventoryComp);
	if (Taxied && ensureAlways(Taxied != this))
	{
		UE_LOG(LogTemp, Warning, TEXT("Taxied"));
		Taxied->Server_ProcessPressInput(ItemObject, InitiatedActor, InventoryInput);
	}
}

void UDBEquipmentComponent::Server_ProcessPressInput_Implementation(UItemObject* ItemObject, AActor* InitiatedActor, FInventoryInput InventoryInput)
{
	UE_LOG(LogTemp, Warning, TEXT("ProcessPressInput"));

	if (!ItemObject)
		return;
	if (!InitiatedActor)
		return;
	if (!HasItem(ItemObject))
		return;

	UPlayerEquipmentComponent* PlayerInventory = InitiatedActor->GetComponentByClass<UPlayerEquipmentComponent>();
	if (!PlayerInventory)
		return;

	UDBEquipmentComponent* From = this;

	if (InventoryInput.bHasRightClicked)
	{
		if (InventoryInput.bHasShiftClicked)
		{
			From->RemoveItem(ItemObject, InitiatedActor);
			Server_SpawnItem(InitiatedActor, ItemObject, false);
		}
		else
		{
			UDBEquipmentComponent* PlayerEquipment = InitiatedActor->GetComponentByClass<UDBEquipmentComponent>();

			if (PlayerEquipment)
			{
				if (PlayerEquipment != From) //If InitiatedActor is looting ItemObject
				{
					UItemObject* EquippedItem = PlayerEquipment->GetSlotItem(ItemObject->GetSlotType());

					if (!EquippedItem)
					{
						From->RemoveItem(ItemObject, InitiatedActor);
						PlayerEquipment->AddItem(ItemObject, InitiatedActor);
					}
					else if (PlayerInventory->TryAddItem(EquippedItem, InitiatedActor)) //if adding equipped item to player's inventory was successful
					{
						PlayerEquipment->RemoveItem(EquippedItem, InitiatedActor);
						From->RemoveItem(ItemObject, InitiatedActor);
						PlayerEquipment->AddItem(ItemObject, InitiatedActor); //Equip ItemObject 
					}
					else
					{
						From->TryAddItem(ItemObject, InitiatedActor); //Put ItemObject back
					}
				}
				else
				{
					UItemObject* EquippedItem = GetSlotItem(ItemObject->GetSlotType());
					if (PlayerInventory->TryAddItem(EquippedItem, InitiatedActor))
					{
						From->RemoveItem(EquippedItem, InitiatedActor);
					}
				}
			}
		}
	}
	else
	{
		UDBEquipmentComponent* PlayerEquipment = InitiatedActor->GetComponentByClass<UDBEquipmentComponent>();

		if (PlayerEquipment && PlayerEquipment != From) //If InitiatedActor is looting ItemObject
		{
			UItemObject* ItemLooting = From->GetSlotItem(ItemObject->GetSlotType());
			if (PlayerInventory->TryAddItem(ItemLooting, InitiatedActor))
			{
				From->RemoveItem(ItemLooting, InitiatedActor);
			}
		}
	}
}

const TArray<UItemObject*> UDBEquipmentComponent::GetSlots() const
{
	return InventoryItems.Items;
}

UItemObject* UDBEquipmentComponent::GetSlotItem(ESlotType SlotType) const
{
	int32 index = UItemLibrary::GetSlotIndexByEnum(SlotType);
	if (InventoryItems.Items.IsEmpty()) return nullptr;
	return InventoryItems.Items[index];
}

bool UDBEquipmentComponent::IsSlotVacant(UItemObject* ItemObject) const
{
	int32 index = UItemLibrary::GetSlotIndexByObject(ItemObject);
	if (InventoryItems.Items.IsValidIndex(index) && InventoryItems.Items[index])
		return false;

	return true;
}

bool UDBEquipmentComponent::HasItem(UItemObject* ItemObject) const
{
	for (int32 i = 0; i < InventoryItems.Items.Num(); ++i)
	{
		if (ItemObject == InventoryItems.Items[i])
			return true;
	}
	return false;
}