// Fill out your copyright notice in the Description page of Project Settings.


#include "DBGameInstance.h"
#include "OnlineSubsystem.h"
#include "OnlineSessionSettings.h"
#include "Online/OnlineSessionNames.h"
#include "Misc/Guid.h"
#include <../../../../../../../Source/Runtime/Engine/Classes/Kismet/GameplayStatics.h>
#include <../../../../../../../Source/Runtime/Engine/Classes/GameFramework/PlayerController.h>
#include <../../../../../../../Source/Runtime/Engine/Classes/GameFramework/PlayerState.h>

void UDBGameInstance::Init()
{
	Super::Init();


	// �¶��� ���� �ý��� ��������
	IOnlineSubsystem* subsys = IOnlineSubsystem::Get();
	if (subsys)
	{
		// ���� �������̽� ��������
		sessionInterface = subsys->GetSessionInterface();
		sessionInterface->OnCreateSessionCompleteDelegates.AddUObject(this, &UDBGameInstance::OnCreateSessionComplete);
		sessionInterface->OnFindSessionsCompleteDelegates.AddUObject(this, &UDBGameInstance::OnFindSessionComplete);
		sessionInterface->OnJoinSessionCompleteDelegates.AddUObject(this, &UDBGameInstance::OnJoinSessionComplete);
	}
	FGuid guid;
	roomName = guid.NewGuid().ToString();
	maxPlayer = 3;

	DestroyMySession();

}

void UDBGameInstance::CreateMySession(int32 PlayerCount, float _CountdownTime)
{
	FOnlineSessionSettings sessionSettings;

	// true ������ �˻� �ȴ�.
	sessionSettings.bShouldAdvertise = true;

	// steam ����ϸ� �ش� �ɼ��� true ������ ���� �� �ִ�.
	sessionSettings.bUseLobbiesIfAvailable = true;

	// ���� �������� �ƴ����� �����ٰ���
	sessionSettings.bUsesPresence = true;
	// ���� �÷��� �߿� ������ �� �ְ�
	sessionSettings.bAllowJoinInProgress = true;
	sessionSettings.bAllowJoinViaPresence = true;

	// �ο� �� 
	sessionSettings.NumPublicConnections = PlayerCount;

	sessionSettings.Set(FName("ROOM_NAME"), roomName, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);


	// ���� ���� ��û
	FUniqueNetIdPtr netID = GetWorld()->GetFirstLocalPlayerFromController()->GetUniqueNetIdForPlatformUser().GetUniqueNetId();

	int32 rand = FMath::RandRange(1, 100000);
	mySessionName += FString::Printf(TEXT("%d"), rand);
	maxPlayer = PlayerCount;
	CountdownTime = _CountdownTime;
	sessionInterface->CreateSession(*netID, FName(mySessionName), sessionSettings);
}

void UDBGameInstance::OnCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
	if (bWasSuccessful)
	{
		OnCreateComplete.ExecuteIfBound(true);

		UE_LOG(LogTemp, Warning, TEXT("OnCreateSessionComplete Success -- %s"), *SessionName.ToString());
		// Battle Map ���� �̵�����
		FString Option = FString::Printf(TEXT("/Game/DBMaps/LobbyMap?listen?MaxPlayers=%d?CountdownTime=%f"), maxPlayer, CountdownTime);
		GetWorld()->ServerTravel(Option);
	}
	else
	{
		OnCreateComplete.ExecuteIfBound(false);
		UE_LOG(LogTemp, Warning, TEXT("OnCreateSessionComplete Fail"));
	}
}

void UDBGameInstance::DestroyMySession()
{
	sessionInterface->DestroySession(FName(mySessionName));
}

void UDBGameInstance::OnDestroySessionComplete(FName SessionName, bool bWasSuccessful)
{
	if (bWasSuccessful)
	{
		UE_LOG(LogTemp, Warning, TEXT("OnDestroySessionComplete Success -- %s"), *SessionName.ToString());
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("OnDestroySessionComplete Fail"));
	}
}

void UDBGameInstance::FindOtherSession()
{
	sessionSearch = MakeShared<FOnlineSessionSearch>();

	sessionSearch->QuerySettings.Set(SEARCH_LOBBIES, true, EOnlineComparisonOp::Equals);

	sessionSearch->MaxSearchResults = 10;

	auto PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	UE_LOG(LogTemp, Warning, TEXT("NetId:%s"), *PC->PlayerState->GetUniqueId().ToString());

	// ���� �˻� ��û
	sessionInterface->FindSessions(0, sessionSearch.ToSharedRef());
	UE_LOG(LogTemp, Warning, TEXT("Searching session"));
}

void UDBGameInstance::OnFindSessionComplete(bool bWasSuccessful)
{
	if (bWasSuccessful)
	{
		auto results = sessionSearch->SearchResults;
		UE_LOG(LogTemp, Warning, TEXT("OnFindSessionComplete Success - count : %d"), results.Num());

		int roomIndex = -1;

		for (int32 i = 0; i < results.Num(); i++)
		{
			FOnlineSessionSearchResult si = results[i];
			si.Session.SessionSettings.Get(FName("ROOM_NAME"), roomName);

			// ���� ���� ---> String ���� 
			int32 max = si.Session.SessionSettings.NumPublicConnections;

			int32 currPlayer = max - si.Session.NumOpenPublicConnections;
			//room is full
			if (currPlayer >= max)
				continue;

			//
			roomIndex = i;
			break;
		}

		if (roomIndex != -1)
		{
			OnFindComplete.ExecuteIfBound(true, true);
			JoinOtherSession(roomIndex);
		}
		else {
			OnFindComplete.ExecuteIfBound(true, false);
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("OnFindSessionComplete Fail"));
		OnFindComplete.ExecuteIfBound(false, false);
	}
}

void UDBGameInstance::JoinOtherSession(int32 idx)
{
	//TArray<FOnlineSessionSearchResult> 
	auto results = sessionSearch->SearchResults;
	if (sessionInterface == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("sessionInterface is null"));
	}
	if (results.Num() <= 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("results Zero"));
	}
	UE_LOG(LogTemp, Warning, TEXT("results count : %d, idx : %d"), results.Num(), idx);
	sessionInterface->JoinSession(0, FName(mySessionName), results[idx]);
}

void UDBGameInstance::OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type result)
{
	if (result == EOnJoinSessionCompleteResult::Success)
	{
		OnJoinSessionEvent.ExecuteIfBound(true);

		UE_LOG(LogTemp, Warning, TEXT("OnJoinSessionComplete Success : %s"), *SessionName.ToString());
		FString url;
		// �����ؾ� �ϴ� Listen ���� URL�� �޾� ����
		sessionInterface->GetResolvedConnectString(SessionName, url);
		UE_LOG(LogTemp, Warning, TEXT("Join session URL : %s"), *url);

		if (!url.IsEmpty())
		{
			// �ش� URL �� ��������
			APlayerController* pc = GetWorld()->GetFirstPlayerController();
			pc->ClientTravel(url, ETravelType::TRAVEL_Absolute);
		}
	}
	else
	{
		OnJoinSessionEvent.ExecuteIfBound(false);
		UE_LOG(LogTemp, Warning, TEXT("OnJoinSessionComplete Fail : %d"), result);
	}
}

void UDBGameInstance::Shutdown()
{
	Super::Shutdown();

	DestroyMySession();
}