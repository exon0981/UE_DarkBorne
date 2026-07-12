// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "DBGameInstance.generated.h"

/**
 *
 */

DECLARE_DELEGATE_TwoParams(FFindCompleteDelegate, bool /*bWasSuccessful*/, bool /*bCanJoinSession*/);
DECLARE_DELEGATE_OneParam(FJoinSessionEventDelegate, bool /*bWasSuccessful*/);
DECLARE_DELEGATE_OneParam(FCreateCompleteDelegate, bool /*bWasSuccessful*/);

UCLASS()
class DARKBORNE_API UDBGameInstance : public UGameInstance
{
	GENERATED_BODY()
public:
	// �� �������� (���� �����, ���� �˻�, ���� ����)
	TSharedPtr<class IOnlineSession, ESPMode::ThreadSafe> sessionInterface;

	// ���� �˻��� �Ϸ�Ǹ� ȣ���ؾ� �ϴ� Delegate
	FFindCompleteDelegate OnFindComplete;
	FJoinSessionEventDelegate OnJoinSessionEvent;
	FCreateCompleteDelegate OnCreateComplete;

	// ���� ����� �Լ�
	UFUNCTION(BlueprintCallable)
	void CreateMySession(int32 PlayerCount, float CountdownTime);
			
	// ������ �˻� �Լ�
	UFUNCTION(BlueprintCallable)
	void FindOtherSession();

	void DestroyMySession();

protected:
	virtual void Init() override;

	void OnCreateSessionComplete(FName SessionName, bool bWasSuccessful);
	void OnDestroySessionComplete(FName SessionName, bool bWasSuccessful);

	void OnFindSessionComplete(bool bWasSuccessful);

	// ���� ���� �Լ�
	UFUNCTION(BlueprintCallable)
	void JoinOtherSession(int32 idx);
	void OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type result);

	virtual void Shutdown() override;
protected:

	// ���� �˻��� ���̴� Ŭ����
	TSharedPtr<class FOnlineSessionSearch> sessionSearch;

	// ���� �̸�
	FString mySessionName = TEXT("SessionFind");

	FString roomName;
	int32 maxPlayer;
	float CountdownTime;
};
