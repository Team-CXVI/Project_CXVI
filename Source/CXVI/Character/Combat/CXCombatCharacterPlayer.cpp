// Fill out your copyright notice in the Description page of Project Settings.


#include "CXCombatCharacterPlayer.h"

#include "CXVI/Tags/CXGameplayTags.h"
#include "Foundation/Combat/CXCollisionChannels.h"

#include "CXVI/AbilitySystem/Attributes/Combat/CXCombatPlayerAttributeSet.h"
#include "Character/Combat/Components/CXJustTimingComponent.h"
#include "Foundation/GameplayData/CXGameplayDataSubsystem.h"
#include "AbilitySystem/Abilities/Combat/CXGameplayAbilityCombatBase.h"
#include "Foundation/Combat/CXCombatManagerSubsystem.h"
#include "Foundation/UserData/CXUserDataSubsystem.h"
#include "AbilitySystem/Attributes/Combat/CXCombatAttributeSetBase.h"

#include "GameplayEffect.h"
#include "Templates/UnrealTemplate.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbility.h"
#include "Components/PrimitiveComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(CXCombatCharacterPlayer)

ACXCombatCharacterPlayer::ACXCombatCharacterPlayer()
{
	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(RootComponent);
	SpringArm->SetRelativeLocation(FVector(0.f, 30.f, 50.f));
	SpringArm->TargetArmLength = 400.f;

	SpringArm->bUsePawnControlRotation = false;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(SpringArm);


	JustTimingComponent = CreateDefaultSubobject<UCXJustTimingComponent>(TEXT("JustTimingComponent"));

	AttributeSet = CreateDefaultSubobject<UCXCombatPlayerAttributeSet>(TEXT("AttributeSet"));
	ASC->AddAttributeSetSubobject<UCXCombatPlayerAttributeSet>(AttributeSet);
	
	// 카메라 표적
	ActionFocusPoint = CreateDefaultSubobject<USceneComponent>(TEXT("ActionMenuFocus"));
	ActionFocusPoint->SetupAttachment(RootComponent);
	ActionFocusPoint->SetRelativeLocation(FVector(0.f, 0.f, 44.f));
	ActionFocusPoint->ComponentTags.AddUnique(FName(TEXT("ActionMenuFocus")));

	ItemFocusPoint = CreateDefaultSubobject<USceneComponent>(TEXT("ItemMenuFocus"));
	ItemFocusPoint->SetupAttachment(RootComponent);
	ItemFocusPoint->SetRelativeLocation(FVector(0.f, 0.f, 44.f));
	ItemFocusPoint->ComponentTags.AddUnique(FName(TEXT("ItemMenuFocus")));

	AttackingFocus = CreateDefaultSubobject<USceneComponent>(TEXT("AttackingFocus"));
	AttackingFocus->SetupAttachment(RootComponent);
	AttackingFocus->SetRelativeLocation(FVector(0.f, 0.f, 44.f));
	AttackingFocus->ComponentTags.AddUnique(FName(TEXT("AttackingFocus")));

	SkillFocusPoint = CreateDefaultSubobject<USceneComponent>(TEXT("SkillFocus"));
	SkillFocusPoint->SetupAttachment(RootComponent);
	SkillFocusPoint->SetRelativeLocation(FVector(0.f, 0.f, 44.f));
	SkillFocusPoint->ComponentTags.AddUnique(FName(TEXT("SkillMenuFocus")));

}



void ACXCombatCharacterPlayer::Look(const FInputActionValue& Value)
{
	// 입력 값 읽어오기 ( 입력에 지정된 타입으로 변환 ).
	FVector2D RotationValue = Value.Get<FVector2D>();

	// 회전 처리 -> 컨트롤러에 회전 입력 전달.
	FRotator CharacterRotation = GetActorRotation();
	CharacterRotation.Yaw += RotationValue.X;
	//CharacterRotation.Pitch += RotationValue.Y;
	SetActorRotation(CharacterRotation);

	FRotator CameraRotation = SpringArm->GetRelativeRotation();
	CameraRotation.Pitch += RotationValue.Y;
	SpringArm->SetRelativeRotation(CameraRotation);
}

bool ACXCombatCharacterPlayer::PerformAimTrace(FHitResult& OutHit)
{
	OutHit = FHitResult();

	UWorld* World = GetWorld();

	if (!World)
	{
		return false;
	}


	const float TraceDistance = 10000.f;
	const FVector TraceStart = Camera->GetComponentLocation();
	const FVector TraceEnd = TraceStart + Camera->GetForwardVector() * TraceDistance;

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	const bool bHit = World->LineTraceSingleByChannel(
		OutHit,
		TraceStart,
		TraceEnd,
		CX_TRACE_COMBAT_AIM,
		QueryParams
	);

	const FVector DebugEnd = bHit ? OutHit.ImpactPoint : TraceEnd;

	return bHit;
}

bool ACXCombatCharacterPlayer::ToggleAimMode()
{
	bIsAimMode = !bIsAimMode;

	ReturnToOriginLocation();

	return bIsAimMode;
}

void ACXCombatCharacterPlayer::FireAbility()
{
	if (!ASC)
	{
		return;
	}

	// Ability 처리중 부여 목록이 변경되어도 안전하도록 보호합니다.
	FScopedAbilityListLock AbilityListLock(*ASC);

	FGameplayTagContainer AttackTags;
	AttackTags.AddTag(CXVIGameplayTags::Ability_Fire);

	TArray<FGameplayAbilitySpec*> MatchingAbilities;

	ASC->GetActivatableGameplayAbilitySpecsByAllMatchingTags(
		AttackTags,
		MatchingAbilities,
		false
	);

	for (FGameplayAbilitySpec* Spec : MatchingAbilities)
	{
		if (!Spec || !Spec->Ability || Spec->IsActive())
		{
			continue;
		}


		// 스킬 사용중인건 배열에 저장
		CurrentActivatingHandles.AddUnique(Spec->Handle);

		// 첫 클릭
		if (!ASC->TryActivateAbility(Spec->Handle))
		{
			CurrentActivatingHandles.Remove(Spec->Handle);

			UE_LOG(LogTemp, Display, TEXT("[자유조준] 발사 불가: AP 또는 사용 조건 확인"));
		}
		// 입력 한번에 발사 어빌리티 하나만 시도
		break;
	}
}

bool ACXCombatCharacterPlayer::CanActivateSelectSkill(int32 SlotIndex) const
{
	if (!IsValid(ASC) || !ASC->AbilityActorInfo.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: 스킬 슬롯 설정을 확인해주세요. 슬롯=%d"), *GetName(), SlotIndex);
		return false;
	}

	const FGameplayAbilitySpec* Spec = FindPreparedSkillSpec(SlotIndex);
	//const FGameplayAbilitySpec* Spec = ASC->FindAbilitySpecFromClass(SkillAbilities[SlotIndex]);

	if (!Spec || !Spec->Ability)
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: 선택한 스킬이 ASC에 부여되어있지 않음."), *GetName());
		return false;
	}

	if (Spec->IsActive())
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: 이미 실행중인 스킬입니다."), *GetName());
		return false;
	}

	const UGameplayAbility* Ability = Spec->Ability.Get();

	const FGameplayAbilityActorInfo* ActorInfo = ASC->AbilityActorInfo.Get();

	// 실제 차감에 사용할 비용 GE를 기준으로 검사
	// 이 호출은 마나를 소비하지 않음
	if (!Ability->CheckCost(Spec->Handle, ActorInfo))
	{
		UCXCombatPlayerAttributeSet* PlayerAttribute = Cast<UCXCombatPlayerAttributeSet>(AttributeSet);

		const float CurrentMana = PlayerAttribute ? PlayerAttribute->GetAP() : 0.f;

		UE_LOG(LogTemp, Display, TEXT("%s: 마나 부족으로 스킬을 사용할 수  없습니다. 현재 마나 =%.0f"), *GetName(), CurrentMana);
		
		return false;
	}

	// 비용 외에도 쿨다운 태그 등 GAS의 활성화 조건을 확인한다.
	if (!Ability->CanActivateAbility(Spec->Handle, ActorInfo))
	{
		UE_LOG(LogTemp, Display, TEXT("%s: 현재 상태에서는 스킬을 사용할수 없습니다."), *GetName());
		return false;
	}

	return true;
}

void ACXCombatCharacterPlayer::SkipTurn()
{
	EndTurnPhase();
}

void ACXCombatCharacterPlayer::TryRunaway()
{
	FCXCombatMessageData Response;
	Response.Character = this;
	Response.Phase = SavedMessage.Phase;
	Response.MessageType = ECXCombatMessageType::WantFlee;
	CombatMessageResponse(Response);
}

const FCXSkillRow* ACXCombatCharacterPlayer::GetSkillRowForSlot(int32 SlotIndex) const
{
	if (SlotIndex < 0 || SlotIndex >= 3 || SkillRowPrefix.IsNone() || !UCXGameplayDataSubsystem::HasInstance(this))
	{
		return nullptr;
	}

	const FName SkillName(*FString::Printf(TEXT("Skill%02d"), SlotIndex + 1));

	return UCXGameplayDataSubsystem::Get(this).FindSkillRow(SkillRowPrefix, SkillName);
}

FName ACXCombatCharacterPlayer::GetItemIdForSlot(int32 SlotIndex) const
{
	// 전투 아이템 메뉴 슬롯 순서. 입력(ACXCommandCharacter)과 메뉴 표시(UCXCombatCommandMenuWidget)가 이 목록을 함께 사용
	static const FName CombatItemSlots[] = { TEXT("HealingTint"), TEXT("EnergyTint") };

	if (SlotIndex < 0 || SlotIndex >= UE_ARRAY_COUNT(CombatItemSlots))
	{
		return NAME_None;
	}

	return CombatItemSlots[SlotIndex];
}

bool ACXCombatCharacterPlayer::PrepareSelectedSkill(int32 SlotIndex)
{
	if (!IsValid(ASC) || SlotIndex < 0 || SlotIndex >= 3)
	{
		return false;
	}

	// 같은 슬롯을 다시 선택하면 등록해둔 spec 을 재사용
	if (FindPreparedSkillSpec(SlotIndex))
	{
		return true;
	}

	if (!HasAuthority())
	{
		return false;
	}

	// 기존 Spec이 제거된 경우 캐시도 정리
	if (const FGameplayAbilitySpecHandle* OldHandle = PreparedSkillHandles.Find(SlotIndex))
	{
		PreparedSkillAPCosts.Remove(*OldHandle);
		PreparedSkillHandles.Remove(SlotIndex);
	}

	const FCXSkillRow* Row = GetSkillRowForSlot(SlotIndex);

	if (!Row || !FMath::IsFinite(Row->APCost) || Row->APCost < 0.f)
	{
		UE_LOG(LogTemp, Warning, TEXT("[스킬] 행 또는 AP 비용 확인. Prefix=%s 슬롯=%d"),
			*SkillRowPrefix.ToString(), SlotIndex + 1);
		return false;
	}

	const TSubclassOf<UGameplayAbility> SkillClass = Row->AbilityClass.LoadSynchronous();
	
	if (!SkillClass || SkillClass->HasAnyClassFlags(CLASS_Abstract))
	{
		UE_LOG(LogTemp, Warning, TEXT("[스킬] GA 클래스 확인. 슬롯=%d"), SlotIndex + 1);

		return false;
	}

	const FGameplayAbilitySpecHandle Handle = ASC->GiveAbility(FGameplayAbilitySpec(SkillClass));

	if (!Handle.IsValid())
	{
		return false;
	}

	PreparedSkillHandles.Add(SlotIndex, Handle);

	if (Row)
	{
		PreparedSkillAPCosts.Add(Handle, Row->APCost);
	}

	UE_LOG(LogTemp, Display, TEXT("[스킬 등록] %s 슬롯=%d GA=%s"),
		*GetName(), SlotIndex + 1, *GetNameSafe(SkillClass.Get()));

	return true;
}

void ACXCombatCharacterPlayer::PostInitializeComponents()
{
	CharacterAttributeSet = AttributeSet;

	Super::PostInitializeComponents();

	InitializeGAS();

	//ASC->SetNumericAttributeBase(AttributeSet->GetMaxHealthAttribute(), 200.f);
	//ASC->SetNumericAttributeBase(AttributeSet->GetHealthAttribute(), 200.f);
	const bool bHasInitialHealth = InitialAttributes.MaxHealth > 0.f;
	const float MaxHealth = bHasInitialHealth ? InitialAttributes.MaxHealth : 200.f;
	const float CurrentHealth = bHasInitialHealth ? InitialAttributes.CurrentHealth : 200.f;

	ASC->SetNumericAttributeBase(AttributeSet->GetMaxHealthAttribute(), MaxHealth);
	ASC->SetNumericAttributeBase(AttributeSet->GetHealthAttribute(), CurrentHealth);
	ASC->SetNumericAttributeBase(AttributeSet->GetCombatSpeedAttribute(), InitialAttributes.CombatSpeed);
	ASC->SetNumericAttributeBase(AttributeSet->GetAttackPowerAttribute(), InitialAttributes.AttackPower);
	ASC->SetNumericAttributeBase(AttributeSet->GetDefenseAttribute(), InitialAttributes.Defense);
	ASC->SetNumericAttributeBase(AttributeSet->GetCriticalChanceAttribute(), InitialAttributes.CriticalChance);
	ASC->SetNumericAttributeBase(AttributeSet->GetCriticalDamageMultiplierAttribute(), InitialAttributes.CriticalDamageMultiplier);

	if (CurrentHealth <= 0.f)
	{
		AttributeSet->OnOutOfHealth.Broadcast(this);
	}

	if (AttributeSet->GetHealth() <= 0.f)
	{
		OnOutOfHealthCpp(nullptr);
	}

	UE_LOG(LogTemp, Display, TEXT("[전투 HP 적용] %s HP=%.1f/%.1f"),
		*GetName(), AttributeSet->GetHealth(), AttributeSet->GetMaxHealth());


}

bool ACXCombatCharacterPlayer::ActivateSelectedSkill(int32 SlotIndex)
{
	const FGameplayAbilitySpec* Spec = FindPreparedSkillSpec(SlotIndex);
	if (!Spec || Spec->IsActive())
	{
		return false;
	}

	const FGameplayAbilitySpecHandle Handle = Spec->Handle;
	return TryStartCombatAbility(Handle);
}

bool ACXCombatCharacterPlayer::ExecuteCombatAbility()
{
	const FCXActionSelection Action = GetPendingAction();

	switch (Action.Type)
	{
	case ECXActionType::BasicAttack:
		return BasicAttackAbility();

	case ECXActionType::Skill:
		return ActivateSelectedSkill(Action.SlotIndex);

	case ECXActionType::CounterAttack:
		return CounterAbility();

	default:
		return false;
	}
}

void ACXCombatCharacterPlayer::BeginTurnPhase()
{
	if (HasAuthority() && IsValid(ASC) && ASC->GetSet<UCXCombatPlayerAttributeSet>())
	{
		const FGameplayAttribute APAttribute = UCXCombatPlayerAttributeSet::GetAPAttribute();
		const float BaseAP = ASC->GetNumericAttributeBase(APAttribute);

		ASC->SetNumericAttributeBase(APAttribute, BaseAP + 1.f);
	}

	Super::BeginTurnPhase();
}

void ACXCombatCharacterPlayer::SelectActionPhase()
{
	// 커맨드 폰의 SubmitAction() 기다리는 중
}

void ACXCombatCharacterPlayer::SelectTargetPhase()
{

	OnTargetSelectionRequested.Broadcast(this, GetTargetCandidates());

	// 대상 확정 전이므로 응답하지 않음.
}

bool ACXCombatCharacterPlayer::ActionPhase()
{
	if (!Super::ActionPhase())
	{
		return false;
	}

	const ECXActionType ActionType = GetPendingAction().Type;
	if (ActionType == ECXActionType::BasicAttack ||
		ActionType == ECXActionType::Skill)
	{
		if (const UCXCombatManagerSubsystem* CombatManager =
			UCXCombatManagerSubsystem::TryGet(this))
		{
			CombatManager->RequestCameraPreset(TEXT("Menu.Action"), this);
		}
	}

	return true;
}

void ACXCombatCharacterPlayer::StartHitReactMontage()
{
	UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr;
	if (nullptr == AnimInstance)
	{
		return;
	}
	AnimInstance->Montage_Play(HitReactMontage);
}

//bool ACXCombatCharacterPlayer::ShouldApproachTarget() const
//{
//	const FCXActionSelection& Action = GetPendingAction();
//
//	if (Action.Type == ECXActionType::Skill)
//	{
//		if (const FGameplayAbilitySpec* Spec = FindPreparedSkillSpec(Action.SlotIndex))
//		{
//			if (const auto* Skill = Cast<UCXGameplayAbilityCombatBase>(Spec->Ability))
//			{
//				return Skill->ShouldApproachTarget();
//			}
//		}
//	}
//
//	return Super::ShouldApproachTarget();
//}

const FGameplayAbilitySpec* ACXCombatCharacterPlayer::GetSelectedAttackSpec() const
{
	const FCXActionSelection& Action = GetPendingAction();

	if (Action.Type == ECXActionType::Skill)
	{
		return FindPreparedSkillSpec(Action.SlotIndex);
	}

	return nullptr;
}

FText ACXCombatCharacterPlayer::GetSelectedActionName() const
{
	const FCXActionSelection& Action = GetPendingAction();

	if (Action.Type == ECXActionType::Skill)
	{
		if (const FCXSkillRow* Row = GetSkillRowForSlot(Action.SlotIndex))
		{
			return Row->DisplayName;
		}
	}

	return FText::GetEmpty();
}

bool ACXCombatCharacterPlayer::UseItem(FName ItemId, ACXCombatCharacterBase* Target)
{
	if (bUsingItem || !HasAuthority() || !GetIsTurn()
		|| SavedMessage.Phase != ECXCombatPhase::SelectTargetPhase
		|| GetPendingAction().Type != ECXActionType::Item
		|| GetPendingAction().ItemId != ItemId)
	{
		return false;
	}

	auto* Ally = Cast<ACXCombatCharacterPlayer>(Target);
	if (!IsValid(Ally) || !Ally->GetIsCharacterAlive())
	{
		return false;
	}

	const bool bCandidate = GetTargetCandidates().ContainsByPredicate(
		[Target](const TWeakObjectPtr<APawn>& Candidate)
		{
			return Candidate.Get() == Target;
		}
	);

	if (!bCandidate)
	{
		return false;
	}

	UCXUserDataSubsystem* UserData = UCXUserDataSubsystem::TryGet(this);
	UAbilitySystemComponent* TargetASC = Target->GetAbilitySystemComponent();
	const TSubclassOf<UGameplayEffect>* EffectClass = ItemEffects.Find(ItemId);

	if (!IsValid(UserData) || !IsValid(ASC) || !IsValid(TargetASC) ||
		!EffectClass || !EffectClass->Get() || !UCXGameplayDataSubsystem::HasInstance(this))
	{
		return false;
	}

	const FCXItemRow* ItemRow = UCXGameplayDataSubsystem::Get(this).FindItemRow(ItemId);

	constexpr int32 ItemLevel = 1;
	const int32 RecoveryIndex = ItemLevel - 1;

	if (!ItemRow || !ItemRow->RecoveryValues.IsValidIndex(RecoveryIndex))
	{
		return false;
	}

	//const float Ratio = ItemRow->RecoveryValues[RecoveryIndex];
	//if (!FMath::IsFinite(Ratio) || Ratio <= 0.f || Ratio > 1.f)
	//{
	//	return false;
	//}

	//const auto* TargetAttributes = TargetASC->GetSet<UCXCombatAttributeSetBase>();

	//if (!TargetAttributes)
	//{
	//	return false;
	//}

	//if (TargetAttributes->GetHealth() >= TargetAttributes->GetMaxHealth())
	//{
	//	UE_LOG(LogTemp, Display, TEXT("[아이템] 이미 최대 체력입니다."));

	//	return false;
	//}

	//// 회복 대상의 최대 HP를 기준으로 계산한다.
	//const float HealAmount = TargetAttributes->GetMaxHealth() * Ratio;

	const float RecoveryValue = ItemRow->RecoveryValues[RecoveryIndex];

	if (!FMath::IsFinite(RecoveryValue) || RecoveryValue <= 0.f)
	{
		return false;
	}

	const auto* TargetAttributes = TargetASC->GetSet<UCXCombatPlayerAttributeSet>();

	if (!TargetAttributes)
	{
		return false;
	}

	float RecoveryAmount = 0.f;

	if (ItemRow->ItemType.MatchesTagExact(CXVIGameplayTags::Item_Type_HealingTint))
	{
		// HP회복량은 최대 HP에 대한 비율이다.
		if (RecoveryValue > 1.f)
		{
			return false;
		}

		if (TargetAttributes->GetHealth() >= TargetAttributes->GetMaxHealth())
		{
			UE_LOG(LogTemp, Display, TEXT("[아이템] 이미 최대 체력입니다."));

			return false;
		}

		RecoveryAmount = TargetAttributes->GetMaxHealth() * RecoveryValue;
	}
	else if (ItemRow->ItemType.MatchesTagExact(CXVIGameplayTags::Item_Type_EnergyTint))
	{
		// AP회복량은 정수 값.
		if (TargetAttributes->GetAP() >= TargetAttributes->GetMaxAP())
		{
			UE_LOG(LogTemp, Display, TEXT("[아이템] 이미 최대 AP입니다."));

			return false;
		}

		RecoveryAmount = RecoveryValue;
	}
	else
	{
		return false;
	}

	FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
	Context.AddSourceObject(this);

	FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(*EffectClass, 1.f, Context);

	if (!Spec.IsValid())
	{
		return false;
	}

	Spec.Data->SetSetByCallerMagnitude(CXVIGameplayTags::Data_ItemRecovery, RecoveryAmount);

	// 수량 변경 메시지에서 같은 사용요청이 들어와도 중복 실행을 막는다.
	TGuardValue<bool> UseGuard(bUsingItem, true);

	if (!UserData->RemoveItem(ItemId, 1))
	{
		UE_LOG(LogTemp, Display, TEXT("[아이템] 보유 수량이 부족합니다."));

		return false;
	}

	// 수량 변경 알림 이후에도 사용 주체와 대상이 유효한지 확인한다.
	if (!IsValid(ASC) || !IsValid(TargetASC))
	{
		UserData->AddItem(ItemId, 1);
		
		return false;
	}

	const FActiveGameplayEffectHandle AppliedEffect = ASC->ApplyGameplayEffectSpecToTarget(*Spec.Data.Get(), TargetASC);

	if (!AppliedEffect.WasSuccessfullyApplied())
	{
		UserData->AddItem(ItemId, 1);

		return false;
	}

	UE_LOG(LogTemp, Display, TEXT("[아이템] %s -> %s, 요청 회복량 %.1f"),
		*ItemId.ToString(), *GetNameSafe(Target), RecoveryAmount);

	EndTurnPhase();

	return true;

}

const FGameplayAbilitySpec* ACXCombatCharacterPlayer::FindPreparedSkillSpec(int32 SlotIndex) const
{
	const FGameplayAbilitySpecHandle* Handle = PreparedSkillHandles.Find(SlotIndex);

	return IsValid(ASC) && Handle ? ASC->FindAbilitySpecFromHandle(*Handle) : nullptr;
}

//void ACXCombatCharacterPlayer::ActionPhase()
//{
//	const FCXActionSelection Action = GetPendingAction();
//
//	switch (Action.Type)
//	{
//	case ECXActionType::BasicAttack:
//		if (!BasicAttackAbility())
//		{
//			FinishAction();
//		}
//		return;
//
//	case ECXActionType::Skill:
//		if (!ActivateSelectedSkill(Action.SlotIndex))
//		{
//			FinishAction();
//		}
//		return;
//
//	default:
//		return;
//	}
//}
