// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"

#include "CXVI/Foundation/Combat/CXCombatMessageInterface.h"
#include "AbilitySystemInterface.h"
#include "GameplayAbilitySpecHandle.h"

#include "CXCombatCharacterBase.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FCXOnCombatAbilityFinished, bool);
DECLARE_MULTICAST_DELEGATE_OneParam(FCXOnCanStartReturnPhase, bool);

struct FGameplayAbilitySpec;
struct FCXDamageAppliedData;
class USoundBase;
class UTexture2D;

UCLASS()
class CXVI_API ACXCombatCharacterBase : public ACharacter, public IAbilitySystemInterface, public ICXCombatMessageInterface
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	ACXCombatCharacterBase();
	
	// 가스 초기화 함수
	void InitializeGAS();

	bool BasicAttackAbility();
	bool DodgeAbility();
	bool ParryAbility();
	bool CounterAbility();

	// 어빌리티 끝났다고 알리는 함수
	void NotifyBattleAbilityEnded(FGameplayAbilitySpecHandle Handle, bool bWasCancelled);
	void NotifyAttackStarted(int32 TotalHitCount);

	void FinishAction();

	virtual void CombatMessageRequest(const FCXCombatMessageData& MessageRequest) override;
	void SubmitAction(const FCXActionSelection& Selection);
	void SubmitTarget(APawn* Target);
	void CancelSelection();

	//
	FCXOnCombatAbilityFinished OnCombatAbilityFinished;
	FCXOnCanStartReturnPhase OnCanStartReturnPhase;

	virtual bool ExecuteCombatAbility();

	void CompleteCombatBehavior();
	void ReturnToOriginTransform();
	void CancelCombatAbility();

	UAbilitySystemComponent* GetAbilitySystemComponent() const override;


	// Getter / Setter
	FORCEINLINE bool GetIsCharacterAlive() const { return bIsCharacterAlive; }
	UFUNCTION(BlueprintCallable)
	FORCEINLINE bool GetIsTurn() const { return bIsTurn; }
	UFUNCTION(BlueprintCallable)
	FORCEINLINE bool GetIsTurnPresentationActive() const { return bIsTurnPresentationActive; }
	FORCEINLINE void SetOriginTransform(const FTransform& NewTransform) { OriginTransform = NewTransform; }
	FORCEINLINE const FTransform& GetOriginTransform() const { return OriginTransform; }
	FORCEINLINE bool GetDoingAction() const { return bDoingAction; }
	FORCEINLINE void SetDoingAction(bool NewValue) { bDoingAction = NewValue; }
	FORCEINLINE bool GetIsDefenseTarget() const { return bIsDefenseTarget; }
	FORCEINLINE void SetIsDefenseTarget(bool NewValue, bool bAllowParry = true)
	{ 
		bIsDefenseTarget = NewValue; 
		bCanParryIncomingAttack = NewValue && bAllowParry; 
		bIncomingAttackInProgress = false;
	}
	FORCEINLINE void SetIncomingAttackInProgress(bool bInProgress)
	{
		bIncomingAttackInProgress = bInProgress;
	}
	FORCEINLINE bool GetCounterAttackPending() const { return bCounterAttackPending; }
	FORCEINLINE void SetCounterAttackPending(bool NewValue) { bCounterAttackPending = NewValue; }
	FORCEINLINE bool GetWaitingForReturn() const { return bWaitingForReturn; }
	FORCEINLINE bool CanParryIncomingAttack() const{return bIsDefenseTarget && bCanParryIncomingAttack;	}
	FORCEINLINE TObjectPtr<class UAnimMontage> GetDashMontage() const { return DashMontage; }
	FORCEINLINE bool GetIsReturningToOrigin() const{ return bIsReturningToOrigin; }



	FORCEINLINE FPrimaryAssetId GetCharacterId() const { return CharacterId; }

	// UI 초상화 (전투 매니저가 정의 에셋에서 읽어 스폰 시 지정. 경로만 보관)
	FORCEINLINE void SetPortrait(const TSoftObjectPtr<UTexture2D>& InPortrait) { Portrait = InPortrait; }
	FORCEINLINE const TSoftObjectPtr<UTexture2D>& GetPortrait() const { return Portrait; }

	FORCEINLINE const TArray<TWeakObjectPtr<ACXCombatCharacterBase>>& GetSelectedCombatTargets() const { return SelectedCombatTargets; }

	float GetCombatSpeed() const;

	APawn* GetSelectedCombatTarget() const { return CurrentTargetPawn.Get(); }

	// 현재 선택한 공격이 패링 가능한 공격인지 확인.
	bool CanSelectedAttackBeParried() const;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void PostInitializeComponents() override;

	virtual FOnTurnPhaseFinished& GetPhaseFinishedDelegate() override;

	virtual void StartDodgeMontage() {}
	virtual void StartHitReactMontage() {}
	virtual void PlayDeathPresentation() {}
	// Inherited via IAbilitySystemInterface
	
	UFUNCTION(BlueprintImplementableEvent)
	void CXPostInitializeComponents();

	UFUNCTION()
	void OnOutOfHealthCpp(AActor* InInstigator);

	// 피격 처리 모음 (OnDamageApplied). 회피 / 패링 성공 시에는 호출 안 됨
	void HandleDamageApplied(const FCXDamageAppliedData& InDamageData);

	UFUNCTION()
	void HandleParrySucceeded(AActor* InInstigator);

	// 회피 성공 처리 모음 (OnDodgeSucceeded)
	void HandleDodgeSucceeded(AActor* InInstigator);

	// 피해 결과를 다른 영역에 알림 (Message.Combat.DamageResult)
	void BroadcastDamageResult(AActor* InInstigator, ECXDamageResult InResult, float InAmount) const;

	void TeleportToAttackLocation();
	void ReturnToOriginLocation();

	virtual const FGameplayAbilitySpec* GetSelectedAttackSpec() const
	{
		return nullptr;
	}

	bool TryStartCombatAbility(FGameplayAbilitySpecHandle Handle);

	// 이번 행동의 종류 / 이름 (Message.Combat.ActionStarted용). 이름이 없는 행동이면 빈 FText
	virtual ECXActionType GetSelectedActionType() const { return PendingAction.Type; }
	virtual FText GetSelectedActionName() const { return FText::GetEmpty(); }

	// 행동 시작을 다른 영역에 알림 (Message.Combat.ActionStarted)
	void BroadcastActionStarted() const;

	virtual bool ShouldApproachTarget() const;
	

	void ProcessMessage(const ECXCombatPhase& CurPhase);

	// 명령 실행 함수.
	virtual void BeginTurnPhase();
	void BeforeSelectPhase();
	virtual void SelectActionPhase();
	virtual void SelectTargetPhase();
	virtual bool ActionPhase();
	void ReturnPhase();
	void EndTurnPhase();

	const TArray<TWeakObjectPtr<APawn>>& GetTargetCandidates() const { return SavedMessage.Targets; }
	const FCXActionSelection& GetPendingAction() const { return PendingAction; }

private:
	void TryGainBasicAttackMana();

	
	
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "GAS")
	TObjectPtr<class UAbilitySystemComponent> ASC;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAS")
	TArray<TSubclassOf<class UGameplayAbility>> Abilities;

	// 현재 실행중인 스킬
	UPROPERTY(VisibleAnywhere, Category = "GAS")
	TArray<FGameplayAbilitySpecHandle> CurrentActivatingHandles;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "UI")
	TObjectPtr<class UWidgetComponent> HpBar;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	TObjectPtr<class USceneComponent> DamageNumberAnchor;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Attribute")
	TObjectPtr<class UCXCombatAttributeSetBase> CharacterAttributeSet;

	// Gameplay Effect
	UPROPERTY(EditDefaultsOnly, Category = "Combat|Mana")
	TSubclassOf<class UGameplayEffect> BasicAttackManaGainEffect;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Focus")
	TObjectPtr<class USceneComponent> TargetFocusPoint;

protected:
	// 페이즈 끝남 델리게이트
	FOnTurnPhaseFinished PhaseFinishedEvent;

	// 턴매니저에 보낼 메시지
	FCXCombatMessageData SavedMessage;

	FCXActionSelection PendingAction;

	FGameplayAbilitySpecHandle ExecutingCombatAbility;

	FTransform OriginTransform;
	TWeakObjectPtr<APawn> CurrentTargetPawn;

	// 이번 행동에서 실제로 효과를 적용할 대상들.
	UPROPERTY(Transient)
	TArray<TWeakObjectPtr<ACXCombatCharacterBase>> SelectedCombatTargets;

	TWeakObjectPtr<ACXCombatCharacterBase> CounterAttackTarget;

	FGameplayAbilitySpecHandle SelectedAttackHandle;

	// 어빌리티 초기화 플래그
	bool bIsInitializedAbility = false;

	// 생존 플래그
	bool bIsCharacterAlive = false;

	// 현재 자기 턴인지
	bool bIsTurn = false;

	// 마나 획득 플래그
	bool bBasicAttackManaGranted = false;

	bool bDoingAction = false;

	// 현재 공격받고있는 상태인지
	bool bIsDefenseTarget = false;

	bool bCounterAttackPending = false;

	bool bWaitingForReturn = false;

	bool bCanParryIncomingAttack = false;

	// 공격자의 공격상태
	bool bIncomingAttackInProgress = false;

	// 방어 입력가능 시간
	double NextDefenseAllowedTime = 0.f;

	bool bIsTurnPresentationActive = false;

	bool bIsReturningToOrigin = false;


protected:
	/*UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Attribute")
	TObjectPtr<class UBaseAttributeSet> CharacterAttributeSet;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Attribute")
	TObjectPtr<class UGASCombatAttributeSet> CombatAttributeSet;*/

	//////////////////////////

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadWrite, Category = "Id")
	FPrimaryAssetId CharacterId;

	// 스폰 시 FCXCombatSpawnEntry에서 받은 초상화. 비어 있을 수 있음
	UPROPERTY(Transient)
	TSoftObjectPtr<UTexture2D> Portrait;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Audio")
	TArray<TObjectPtr<USoundBase>> MoveStartSounds;

	UPROPERTY(EditDefaultsOnly, Category = "Montage|Dash")
	TObjectPtr<class UAnimMontage> DashMontage;
};
