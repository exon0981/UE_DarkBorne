// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "DBPlayerController.generated.h"

/**
 *
 */
class UGameEndWidget;
UCLASS()
class DARKBORNE_API ADBPlayerController : public APlayerController
{
	GENERATED_BODY()
public:
	ADBPlayerController();
public:
	UFUNCTION(Client, Reliable, BlueprintCallable)
	void Client_DisplayGameResult(bool bHasWon);

	UFUNCTION()
	void ReturnToMenu();
protected:
	virtual void BeginPlay() override;
	virtual void OnPossess(APawn* aPawn) override;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Settings")
	TSubclassOf<UGameEndWidget> GameEndWidgetClass;
	TObjectPtr<UGameEndWidget> GameEndWidget;

public:
	// ������ ��� �Ǳ�
	void ChangeToSpectator();
	UFUNCTION(Server, Reliable)
	void ServerRPC_ChangeToSpectator();
};
