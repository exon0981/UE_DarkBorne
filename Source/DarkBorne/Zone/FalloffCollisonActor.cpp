// Fill out your copyright notice in the Description page of Project Settings.

#include "DarkBorne/Zone/FalloffCollisonActor.h"
#include "DarkBorne/Status/CharacterStatusComponent.h"
#include "Net/UnrealNetwork.h"
#include "Components/BoxComponent.h"
#include "DarkBorne/DBCharacters/DBRogueCharacter.h"
#include <DarkBorne/TP_ThirdPerson/TP_ThirdPersonGameMode.h>

// Sets default values
AFalloffCollisonActor::AFalloffCollisonActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	FallBoxCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("FallBox"));
	FallBoxCollision->SetBoxExtent(FVector(300,300,30));
	FallBoxCollision->SetCollisionProfileName(TEXT("WeaponCapColl"));



}

// Called when the game starts or when spawned
void AFalloffCollisonActor::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		FallBoxCollision->OnComponentBeginOverlap.AddDynamic(this, &AFalloffCollisonActor::OnOverlapBegin);
	}
	
}

// Called every frame
void AFalloffCollisonActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AFalloffCollisonActor::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	ACharacter* OtherPlayer = Cast<ACharacter>(OtherActor);
	//UE_LOG(LogTemp, Warning, TEXT("Testing here: %s"), *GetNameSafe(GetOwner()));
	//UDBRogueAnimInstance* OtherPlayerAnim = Cast<UDBRogueAnimInstance>(OtherPlayer->GetMesh()->GetAnimInstance());

	// ĳ������ GetOnwer�� �ν��Ͻ��� ������ ���� �÷��̾� �ִ� �ν��Ͻ��� �����´�
	ServerRPC_OnOverlapBegin(OtherActor);
	
}

void AFalloffCollisonActor::ServerRPC_OnOverlapBegin_Implementation(AActor* OtherActor)
{
	//���� �ƴ� �ٸ� �α� �÷��̾ otherActor�� ĳ����
	ADBRogueCharacter* OtherPlayer = Cast<ADBRogueCharacter>(OtherActor);
	if (OtherPlayer)
	{
		FString Level = GetWorld()->GetMapName();
		Level.RemoveFromStart(GetWorld()->StreamingLevelsPrefix);

		if (Level != TEXT("ThirdPersonMap"))
		{
			UCharacterStatusComponent* StatusComponent = OtherActor->GetComponentByClass<UCharacterStatusComponent>();
			//���� �ƴ� �ٸ� �α� �÷��̾ otherActor�� ĳ����
			//�κ� üũ
			StatusComponent->DamageProcess(FallOffDamage, GetOwner());
			//�÷��̾��� ���� ü�¿��� ���ⵥ������ŭ �������� �ش�
			//onRep �Լ��� Ŭ�󿡼��� ȣ��Ǿ ���������� �ѹ� ȣ��������Ѵ�
			//StatusComponent->OnRep_CurrHP();

			//UE_LOG(LogTemp, Warning, TEXT("%s : %.f"),
			//	GetWorld()->GetNetMode() == ENetMode::NM_Client ? TEXT("Client") : TEXT("Server"), OtherPlayer->CurrHP);
			auto GM = GetWorld()->GetAuthGameMode<ATP_ThirdPersonGameMode>();
			if (ensure(GM) && StatusComponent->CurrHP <= 0.f && OtherPlayer != nullptr)
			{
				auto PC = OtherPlayer->GetOwner<APlayerController>();
				if (PC)
				{
					GM->OnPlayerDead(PC);
				}
			}
		}
	}
}

