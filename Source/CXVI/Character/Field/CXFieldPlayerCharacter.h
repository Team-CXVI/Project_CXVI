// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CXFieldCharacterBase.h"
#include "CXVI/Foundation/Scene/CXSceneSuspendable.h"
#include "CXVI/Actor/Field/Interactions/CXRopeTraversalActor.h"

#include "CXFieldPlayerCharacter.generated.h"

class UInputMappingContext;
class UInputAction;
class UAnimInstance;
class UAnimMontage;
class UWidgetComponent;
class UCableComponent;
class ACXExpeditionFlag;
struct FInputActionValue;



UCLASS()
class CXVI_API ACXFieldPlayerCharacter : public ACXFieldCharacterBase, public ICXSceneSuspendable
{
	GENERATED_BODY()

public:
	ACXFieldPlayerCharacter();
	
	virtual void BeginPlay() override;
	

	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;


public:
	// Grappling Target 위치 확인.
	void SetGrapplingTarget(AActor* TargetActor, AActor* LandingActor = nullptr);

	AActor* GetGrapplingTargetActor() const { return CurrentGrapplingTarget.Get(); }
	AActor* GetLandingSurfaceActor() const { return CurrentLandingSurface.Get(); }

	void ShowCable(AActor* Target);
	void HideCable();

	// 내 상태로 휴식을 시작 할 수 있는지 알려주는 함수.
	bool CanStartFlag() const;

	// 접근한 깃발을 사용 후보에 추가.
	void RegisterNearByFlag(ACXExpeditionFlag* Flag);

	// 벗어난 깃발을 사용 후보에서 제거.
	void UnregisterNearByFlag(ACXExpeditionFlag* Flag);

	// 로프 섹션.
public:
	ACXRopeTraversalActor* GetRopeTraversalTarget() const { return CurrentRopeTraversalTarget.Get(); }

	ECXRopeTraversalDirection GetRopeTraversalDirection() const
	{
		return CurrentRopeTraversalTarget.IsValid() ? CurrentRopeTraversalDirection : ECXRopeTraversalDirection::None;
	}

	void SetRopeTraversalTarget(ACXRopeTraversalActor* Rope, ECXRopeTraversalDirection Direction);

	void ClearRopeTraversalTarget(ACXRopeTraversalActor* Rope, ECXRopeTraversalDirection Direction);


	// 필드 씬 인터페이스 섹션.
public:
	void OnSceneSuspended_Implementation();

	void OnSceneResumed_Implementation();

	// 키 바인드할 때 기능을 나눠 줄 함수.
protected:
	// 상호 작용 키.
	void Interactive();

	// 공격 키.
	void AttackFunc();

protected:
	void Move(const FInputActionValue& InValue);
	void Look(const FInputActionValue& InValue);

	// 스프린트 어빌리티.
	void StartSprint();
	void StopSprint();

	// 그래플링 어빌리티.
	void GrapplingHook();

	// 태그가 있고/없을 때 UI의 상태 변경을 위한 함수.
	void RefreshGrapplingPrompt();

	// 그래플링 태그가 들어왔을 때 새로고침 하기위한 함수.
	void OnCanGrapplingHookChanged(const FGameplayTag, int32 NewCount);

	void StartUseRope();

	// 파쿠르 샘플.
	void StartJumpOrParkour();
	void ReleaseJumpOrParkour();
	void AlignParkourMontageEntry(UAnimInstance* AnimInstance, UAnimMontage* Montage);

	// BP에서 샘플 파쿠르를 호출.
	// 처리 했으면 true, 못했으면 false 반환.
	UFUNCTION(BlueprintImplementableEvent, Category = "Traversal")
	bool TryParkour();

	// 키를 떼도 진행 중인 파쿠르의 입력 잠금은 몽타주 종료까지 유지한다.
	TWeakObjectPtr<UAnimMontage> ActiveParkourMontage;
	bool bJumpOrParkourInputHeld = false;

	// 뒤잡 실행.
	void Assassination();

	// 원정대 깃발 사용 시작.
	void StartUseFlag();

	void OnFlagMontageChanged(class UAnimMontage* Montage, FName SectionName, bool bLooped);

	// 플레이어 공격.
	void Attack();

	

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Input)
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Input)
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Input)
	TObjectPtr<UInputAction> JumpAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Input)
	TObjectPtr<UInputAction> SprintAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Input)
	TObjectPtr<UInputAction> AttackAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Input)
	TObjectPtr<UInputAction> InteractiveAction;

	// 게임플레이 어빌리티 - 스프린트
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = GameplayAbilities)
	TSubclassOf<UGameplayAbility> SprintAbility;

	// ASC에 등록한 스프린트 Ability를 찾는 번호표.
	FGameplayAbilitySpecHandle SprintAbilityHandle;

	// 게임플레이 어빌리티 - 그래플링 훅
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = GameplayAbilities)
	TSubclassOf<UGameplayAbility> GrapplingAbility;

	// 그래플링 타겟 액터.
	UPROPERTY()
	TWeakObjectPtr<AActor> CurrentGrapplingTarget;

	// 그래플링 착지 액터.
	UPROPERTY()
	TWeakObjectPtr<AActor> CurrentLandingSurface;

	// 그래플링 훅 스펙을 관리할 핸들.
	FGameplayAbilitySpecHandle GrapplingHookHandle;

	// 태그가 달리고 없어졌을때 델리게이트 함수를 저장할 변수.
	FDelegateHandle GrapplingTagDelegateHandle;

	// 그래플링 UI
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Grappling")
	TObjectPtr<UWidgetComponent> GrapplingPrompt;

	// 그래플링 훅 케이블.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Grappling")
	TObjectPtr<UCableComponent> GrapplingCable;

	// 케이블 소켓(손목).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grappling")
	FName GrapplingHandSoket = TEXT("CableSocket");

	// 선제공격.
	// 게임플레이 어빌리티 - 선제공격.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = GameplayAbilities)
	TSubclassOf<UGameplayAbility> PreemptiveAttackAbility;

	// 태그가 달리고 없어졌을때 델리게이트 함수를 저장할 변수.
	FDelegateHandle PreemptiveAttackTagDelegateHandle;

	// 선제 공격 스펙을 관리할 핸들.
	FGameplayAbilitySpecHandle PreemptiveAttackHandle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ExpeditionFlag")
	TObjectPtr<class UAnimMontage> FlagMontage;

	// 플래그에서 추가한 이동 제한을 관리.
	bool bFlagMovementBlocked = false;

	// 플레이어 공격.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = GameplayAbilities)
	TSubclassOf<UGameplayAbility> AttackAbility;

	// 공격 스펙을 관리할 핸들.
	FGameplayAbilitySpecHandle AttackAbilityHandle;

	// 이번 휴식을 시작한 깃발.
	// 현재 사용 범위 안에 있는 깃발.
	UPROPERTY(Transient)
	TWeakObjectPtr<class ACXExpeditionFlag> NearByActiveExpeditionFlag;

	// 이번 휴식을 시작할 깃발.
	UPROPERTY(Transient)
	TWeakObjectPtr<class ACXExpeditionFlag> ActiveExpeditionFlag;


	// 플레이어 로프 어빌리티.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = GameplayAbilities)
	TSubclassOf<UGameplayAbility> RopeTraversalAbility;

	// 로프 어빌리티 스펙을 관리할 핸들.
	FGameplayAbilitySpecHandle RopeTraversalAbilityHandle;

	// 지금 타겟한 로프.
	UPROPERTY(Transient)
	TWeakObjectPtr<ACXRopeTraversalActor> CurrentRopeTraversalTarget;

	// 현재 로프 관련 상태.
	UPROPERTY(Transient)
	ECXRopeTraversalDirection CurrentRopeTraversalDirection = ECXRopeTraversalDirection::None;
};

