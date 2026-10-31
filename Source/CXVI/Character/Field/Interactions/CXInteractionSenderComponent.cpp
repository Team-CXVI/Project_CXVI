// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/Field/Interactions/CXInteractionSenderComponent.h"

#include "Interface/CXInteractableInterface.h"
#include "Interface/CXInteractActionInterface.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(CXInteractionSenderComponent)

// Sets default values for this component's properties
UCXInteractionSenderComponent::UCXInteractionSenderComponent()
{
	// Tick은 후보가 생겼을 때만 켠다
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;

	InitSphereRadius(200.f);
	SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	SetCollisionObjectType(ECC_WorldDynamic);
	SetCollisionResponseToAllChannels(ECR_Ignore);
	SetCollisionResponseToChannel(INTERACTABLE_CHANNEL, ECR_Overlap);
	SetGenerateOverlapEvents(true);
	SetCanEverAffectNavigation(false);
}

// Called when the game starts
void UCXInteractionSenderComponent::BeginPlay()
{
	Super::BeginPlay();

	OnComponentBeginOverlap.AddUniqueDynamic(this, &UCXInteractionSenderComponent::HandleBeginOverlap);
	OnComponentEndOverlap.AddUniqueDynamic(this, &UCXInteractionSenderComponent::HandleEndOverlap);

	// 스폰/텔레포트 직후 이미 겹쳐 있는 경우 대비 초기 스캔
	TArray<AActor*> Overlapping;
	GetOverlappingActors(Overlapping);
	for (AActor* Actor : Overlapping)
	{
		TryAddCandidate(Actor);
	}
	
}

void UCXInteractionSenderComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	SetCurrentTarget(nullptr);
	Candidates.Reset();

	OnComponentBeginOverlap.RemoveAll(this);
	OnComponentEndOverlap.RemoveAll(this);

	Super::EndPlay(EndPlayReason);
}


// Called every frame
void UCXInteractionSenderComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	UpdateInteraction();
}

bool UCXInteractionSenderComponent::TryInteract()
{
	AActor* Target = CurrentTarget.Get();
	AActor* Owner = GetOwner();

	// 입력 시점 재검증: 캐시 유효성 -> 오너 조건 -> 기믹 조건
	if (!Target || !Owner || !IsLocalOwner())
	{
		return false;
	}
	if (!CanOwnerInteract() || !ICXInteractableInterface::Execute_CanInteract(Target, Owner))
	{
		UpdateInteraction(); // 상태가 어긋나 있었으니 즉시 갱신해서 UI 정리
		return false;
	}

	ICXInteractableInterface::Execute_Interact(Target, Owner);

	// Interact 결과로 기믹이 파괴/비활성화되었을 수 있으니 바로 갱신
	UpdateInteraction();
	return true;
}

bool UCXInteractionSenderComponent::CanOwnerInteract_Implementation() const
{
	AActor* Owner = GetOwner();
	if (nullptr == Owner)
	{
		return false;
	}

	return ICXInteractActionInterface::Execute_CanInteract(Owner);
}

void UCXInteractionSenderComponent::HandleBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	TryAddCandidate(OtherActor);
}

void UCXInteractionSenderComponent::HandleEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (!OtherActor)
	{
		return;
	}

	// 한 기믹이 여러 컴포넌트로 겹쳐 있을 수 있으므로, 완전히 벗어났을 때만 제거
	if (IsOverlappingActor(OtherActor))
	{
		return;
	}

	Candidates.RemoveAll([OtherActor](const TWeakObjectPtr<AActor>& Ptr) { return Ptr.Get() == OtherActor; });
	UpdateInteraction();
}

void UCXInteractionSenderComponent::TryAddCandidate(AActor* InActor)
{
	if (!IsValid(InActor) || InActor == GetOwner() || !IsLocalOwner())
	{
		return;
	}
	if (!InActor->Implements<UCXInteractableInterface>())
	{
		return;
	}

	Candidates.AddUnique(InActor);

	// 판단 결과와 무관하게 범위 안에 들어왔으면 Tick 시작
	SetComponentTickEnabled(true);
	UpdateInteraction();
}

void UCXInteractionSenderComponent::UpdateInteraction()
{
	// 파괴되어 EndOverlap이 안 온 기믹 정리
	Candidates.RemoveAll([](const TWeakObjectPtr<AActor>& Ptr) { return !Ptr.IsValid(); });

	if (Candidates.IsEmpty())
	{
		SetCurrentTarget(nullptr);
		SetComponentTickEnabled(false);
		return;
	}

	// 오너 조건이 false면 기믹에는 판단을 요청하지 않는다 (타겟 null -> 기존 프롬프트 끄기만 전달됨)
	AActor* Target = CanOwnerInteract() ? SelectBestTarget() : nullptr;
	SetCurrentTarget(Target);
}

AActor* UCXInteractionSenderComponent::SelectBestTarget() const
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return nullptr;
	}

	FVector ViewLoc = Owner->GetActorLocation();
	FRotator ViewRot = Owner->GetActorRotation();
	if (const APawn* Pawn = Cast<APawn>(Owner))
	{
		if (const AController* Controller = Pawn->GetController())
		{
			Controller->GetPlayerViewPoint(ViewLoc, ViewRot);
		}
	}
	const FVector ViewDir = ViewRot.Vector();
	const FVector OwnerLoc = Owner->GetActorLocation();
	const float Radius = FMath::Max(GetScaledSphereRadius(), 1.f);

	AActor* Best = nullptr;
	float BestScore = -TNumericLimits<float>::Max();

	for (const TWeakObjectPtr<AActor>& Ptr : Candidates)
	{
		AActor* Candidate = Ptr.Get();
		if (!Candidate)
		{
			continue;
		}

		// 기믹 쪽 조건 (여기서 true면 오너/기믹 양쪽 모두 true)
		if (!ICXInteractableInterface::Execute_CanInteract(Candidate, Owner))
		{
			continue;
		}

		const FVector ToTarget = Candidate->GetActorLocation() - OwnerLoc;
		const float Dist = ToTarget.Size();
		const float Dot = Dist > KINDA_SMALL_NUMBER ? FVector::DotProduct(ViewDir, ToTarget / Dist) : 1.f;

		float Score = Dot * AngleWeight - (Dist / Radius);
		if (Candidate == CurrentTarget.Get())
		{
			Score += CurrentTargetBonus;
		}

		if (Score > BestScore)
		{
			BestScore = Score;
			Best = Candidate;
		}
	}
	return Best;
}

void UCXInteractionSenderComponent::SetCurrentTarget(AActor* NewTarget)
{
	AActor* Prev = CurrentTarget.Get();
	if (Prev == NewTarget)
	{
		if (!NewTarget)
		{
			CurrentTarget.Reset(); // 파괴된 타겟의 stale 포인터 정리
		}
		return;
	}

	if (Prev)
	{
		ICXInteractableInterface::Execute_SetInteractionPromptVisible(Prev, false);
	}

	CurrentTarget = NewTarget;

	if (NewTarget)
	{
		ICXInteractableInterface::Execute_SetInteractionPromptVisible(NewTarget, true);
	}
}

bool UCXInteractionSenderComponent::IsLocalOwner() const
{
	// 폰이 아니면 항상 true, 폰이면 로컬 조작 중일 때만 (타 클라이언트의 프록시에서 UI가 뜨지 않도록)
	const APawn* Pawn = Cast<APawn>(GetOwner());
	return !Pawn || Pawn->IsLocallyControlled();
}

