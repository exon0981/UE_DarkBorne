// Fill out your copyright notice in the Description page of Project Settings.


#include "DBPlayerController.h"
#include "../UI/GameEndWidget.h"
#include <../../../../../../../Source/Runtime/UMG/Public/Blueprint/WidgetBlueprintLibrary.h>
#include "DBDropItemManager.h"
#include "../ItemTypes/EnchantmentTypes.h"
#include "../ItemTypes/ItemType.h"
#include "../Inventory/ItemObject.h"
#include "../DBCharacters/DBCharacter.h"
#include "../Inventory/DBEquipmentComponent.h"
#include "../TP_ThirdPerson/TP_ThirdPersonGameMode.h"
#include <../../../../../../../Source/Runtime/Engine/Classes/GameFramework/SpectatorPawn.h>
#include <../../../../../../../Source/Runtime/Engine/Classes/Kismet/KismetSystemLibrary.h>
#include "Kismet/GameplayStatics.h"

ADBPlayerController::ADBPlayerController()
{
	// �⺻ ���콺 Ŀ�� ����
	DefaultMouseCursor = EMouseCursor::Default;
	// ���� ���콺 Ŀ�� ����
	CurrentMouseCursor = EMouseCursor::Default;
}

void ADBPlayerController::Client_DisplayGameResult_Implementation(bool bHasWon)
{
	if (ensureAlways(GameEndWidgetClass) && !GameEndWidget)
	{
		GameEndWidget = CreateWidget<UGameEndWidget>(this, GameEndWidgetClass);
	}

	if (GameEndWidget)
	{
		if (bHasWon) {
			GameEndWidget->SetGameStateText(FText::FromString(FString::Printf(TEXT("WON"))));
		}
		else
			GameEndWidget->SetGameStateText(FText::FromString(FString::Printf(TEXT("LOST"))));

		GameEndWidget->AddToViewport();
	}

	FTimerHandle Handle;
	GetWorld()->GetTimerManager().SetTimer(Handle, this, &ADBPlayerController::ReturnToMenu, 7.f, false);
}

void ADBPlayerController::ReturnToMenu()
{
	UGameplayStatics::OpenLevel(GetWorld(), "MainMenuMap", true);
}


void ADBPlayerController::BeginPlay()
{
	Super::BeginPlay();
	
	if (IsLocalPlayerController()) 
	{
		UWidgetBlueprintLibrary::SetInputMode_GameOnly(this);
		SetShowMouseCursor(false);
	}
	else {
		UE_LOG(LogTemp, Warning, TEXT("NOOE"));
	}
}

void ADBPlayerController::OnPossess(APawn* aPawn)
{
	Super::OnPossess(aPawn);
}

void ADBPlayerController::ChangeToSpectator()
{
	ServerRPC_ChangeToSpectator();
}

void ADBPlayerController::ServerRPC_ChangeToSpectator_Implementation()
{
	APawn* player = GetPawn();
	if (player)
	{
		// �÷��� ���� ����� ���Ӹ�带 ��������
		ATP_ThirdPersonGameMode* gm = Cast<ATP_ThirdPersonGameMode>(GetWorld()->GetAuthGameMode());

		// ������ Pawn ����
		ASpectatorPawn* spectator = GetWorld()->SpawnActor<ASpectatorPawn>(gm->SpectatorClass, player->GetActorTransform());

		// ���� �÷��̾� Possess ����
		UnPossess();

		// ������ Pawn �� Possess
		Possess(spectator);
		//UKismetSystemLibrary::K2_SetTimer(this, TEXT("ChangeToSpectator"), 5, false);
	}
}
