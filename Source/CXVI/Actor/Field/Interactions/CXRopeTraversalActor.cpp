// Fill out your copyright notice in the Description page of Project Settings.


#include "CXRopeTraversalActor.h"
#include "CXVI/Character/Field/CXFieldPlayerCharacter.h"

#include "Components/SceneComponent.h"
#include "Components/BoxComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/CapsuleComponent.h"

// Sets default values
ACXRopeTraversalActor::ACXRopeTraversalActor()
{
 	
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneComponent"));
	SetRootComponent(SceneRoot);

	BottomRopePoint = CreateDefaultSubobject<USceneComponent>(TEXT("BottomRopePoint"));
	BottomRopePoint->SetupAttachment(SceneRoot);

	TopRopePoint = CreateDefaultSubobject<USceneComponent>(TEXT("TopRopePoint"));
	TopRopePoint->SetupAttachment(SceneRoot);

	BottomExitPoint = CreateDefaultSubobject<USceneComponent>(TEXT("BottomExitPoint"));
	BottomExitPoint->SetupAttachment(SceneRoot);

	TopExitPoint = CreateDefaultSubobject<USceneComponent>(TEXT("TopExitPoint"));
	TopExitPoint->SetupAttachment(SceneRoot);

	BottomInteractionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("BottomInteractionBox"));
	BottomInteractionBox->SetupAttachment(BottomExitPoint);

	TopInteractionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("TopInteractionBox"));
	TopInteractionBox->SetupAttachment(TopExitPoint);

	BottomInteractionBox->SetRelativeLocation(FVector(0.0f, 0.0f, 90.0f));
	BottomInteractionBox->SetBoxExtent(FVector(80.0f, 80.0f, 90.0f));

	BottomInteractionBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	BottomInteractionBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	BottomInteractionBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	BottomInteractionBox->SetGenerateOverlapEvents(true);

	TopInteractionBox->SetRelativeLocation(FVector(0.0f, 0.0f, 90.0f));
	TopInteractionBox->SetBoxExtent(FVector(80.0f, 80.0f, 90.0f));

	TopInteractionBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	TopInteractionBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	TopInteractionBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	TopInteractionBox->SetGenerateOverlapEvents(true);

	
}

bool ACXRopeTraversalActor::GetTraversalPoints(ECXRopeTraversalDirection Direction, FVector& OutEntry, FVector& OutDestination, FVector& OutExit) const
{
	OutEntry = FVector::ZeroVector;
	OutDestination = FVector::ZeroVector;
	OutExit = FVector::ZeroVector;

	USceneComponent* Entry = nullptr;
	USceneComponent* Destination = nullptr;
	USceneComponent* Exit = nullptr;

	if (Direction == ECXRopeTraversalDirection::Up)
	{
		Entry = BottomRopePoint;
		Destination = TopRopePoint;
		Exit = TopExitPoint;
	}
	else if (Direction == ECXRopeTraversalDirection::Down)
	{
		Entry = TopRopePoint;
		Destination = BottomRopePoint;
		Exit = BottomExitPoint;
	}
	else
	{
		return false;
	}

	if (!IsValid(Entry) || !IsValid(Destination) || !IsValid(Exit))
	{
		return false;
	}

	OutEntry = Entry->GetComponentLocation();
	OutDestination = Destination->GetComponentLocation();
	OutExit = Exit->GetComponentLocation();

	return true;
}

// Called when the game starts or when spawned
void ACXRopeTraversalActor::BeginPlay()
{
	Super::BeginPlay();

	BottomInteractionBox->OnComponentBeginOverlap.AddUniqueDynamic(this, &ThisClass::HandleInteractionBeginOverlap);
	BottomInteractionBox->OnComponentEndOverlap.AddUniqueDynamic(this, &ThisClass::HandleInteractionEndOverlap);

	TopInteractionBox->OnComponentBeginOverlap.AddUniqueDynamic(this, &ThisClass::HandleInteractionBeginOverlap);
	TopInteractionBox->OnComponentEndOverlap.AddUniqueDynamic(this, &ThisClass::HandleInteractionEndOverlap);
}

void ACXRopeTraversalActor::HandleInteractionBeginOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	ACXFieldPlayerCharacter* Player = Cast<ACXFieldPlayerCharacter>(OtherActor);

	if (!IsValid(Player) || OtherComp != Player->GetCapsuleComponent())
	{
		return;
	}

	const ECXRopeTraversalDirection Direction = OverlappedComponent == BottomInteractionBox.Get() ? ECXRopeTraversalDirection::Up : ECXRopeTraversalDirection::Down;
	Player->SetRopeTraversalTarget(this, Direction);

	const TCHAR* Side = OverlappedComponent == BottomInteractionBox.Get() ? TEXT("하단") : TEXT("상단");

	UE_LOG(LogTemp, Log, TEXT("[Rope] %s 진입: %s"), Side, *GetNameSafe(Player));

}

void ACXRopeTraversalActor::HandleInteractionEndOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex)
{
	ACXFieldPlayerCharacter* Player = Cast<ACXFieldPlayerCharacter>(OtherActor);

	if (!IsValid(Player) || OtherComp != Player->GetCapsuleComponent())
	{
		return;
	}

	const ECXRopeTraversalDirection Direction = OverlappedComponent == BottomInteractionBox.Get() ? ECXRopeTraversalDirection::Up : ECXRopeTraversalDirection::Down;
	Player->ClearRopeTraversalTarget(this, Direction);

	const TCHAR* Side = OverlappedComponent == BottomInteractionBox.Get() ? TEXT("하단") : TEXT("상단");

	UE_LOG(LogTemp, Log, TEXT("[Rope] %s 이탈: %s"), Side, *GetNameSafe(Player));
}




