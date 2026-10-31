// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SphereComponent.h"

#include "CXInteractionSenderComponent.generated.h"

/**
 * 캐릭터에 붙이는 상호작용 송신기. 상호작용 범위(구)를 겸한다.
 *
 * 흐름
 *  - BeginOverlap: 인터페이스 구현 액터를 후보(약참조)로 캐싱, Tick 활성화
 *  - Tick(저주기): 오너 조건 -> 후보별 기믹 조건 -> 최적 타겟 선정 -> 타겟이 바뀔 때만 UI on/off
 *  - EndOverlap: 후보 제거, 후보가 없으면 타겟 해제 + Tick 비활성화
 *  - TryInteract: 입력 시점에 전부 재검증 후 Interact 호출
 */
UCLASS( ClassGroup=(Interaction), meta=(BlueprintSpawnableComponent) )
class CXVI_API UCXInteractionSenderComponent : public USphereComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UCXInteractionSenderComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

public:
	/** 상호작용 키 입력 시 호출. 성공적으로 Interact까지 갔으면 true */
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	bool TryInteract();

	UFUNCTION(BlueprintPure, Category = "Interaction")
	AActor* GetCurrentTarget() const { return CurrentTarget.Get(); }

	/** 오너(플레이어) 쪽 조건. 스턴/공격 중/사망 등을 여기서 판단. BP에서 오버라이드 가능 */
	UFUNCTION(BlueprintNativeEvent, Category = "Interaction")
	bool CanOwnerInteract() const;
	virtual bool CanOwnerInteract_Implementation() const;

protected:
	UFUNCTION()
	void HandleBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void HandleEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex);

private:
	void TryAddCandidate(AActor* Actor);
	void UpdateInteraction();
	AActor* SelectBestTarget() const;

	/** 프롬프트 on/off를 전부 이 함수로만 처리 (이전 타겟 끄기 -> 교체 -> 새 타겟 켜기) */
	void SetCurrentTarget(AActor* NewTarget);

	bool IsLocalOwner() const;

public:
	/** 후보/타겟 재평가 주기(초) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	float UpdateInterval = 0.1f;

	/** 타겟 점수에서 시선 방향 일치도(내적)의 가중치. 클수록 시선 우선, 0이면 거리만 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Targeting")
	float AngleWeight = 1.0f;

	/** 현재 타겟에 주는 보너스 점수. 두 기믹 사이에서 UI가 깜빡이는 것을 막는 히스테리시스 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Targeting")
	float CurrentTargetBonus = 0.15f;

private:
	TArray<TWeakObjectPtr<AActor>> Candidates;

	TWeakObjectPtr<AActor> CurrentTarget;
};
