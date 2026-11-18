// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "CXAbilityData.generated.h"

class UNiagaraSystem;

UENUM(BlueprintType)
enum class ECXHitFXLocation : uint8
{
	Body,
	Ground
};

USTRUCT(BlueprintType)
struct FCXAttackHitData
{
	GENERATED_BODY()

	// 해당 타격의 데미지 배율
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float DamageMultiplier = 1.f;

	// 해당 타격이 대상에게 재생할 이펙트
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UNiagaraSystem> HitFX;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UParticleSystem> HitCascadeFX;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hit FX")
	ECXHitFXLocation HitFXLocation = ECXHitFXLocation::Body;

	// 몸에서 재생할 때 사용할 소켓. 비워두면 캐릭터 중심을 사용한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hit FX")
	FName HitFXSocket = NAME_None;

	// 캐릭터 방향을 기준으로 적용하는 위치 보정값
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hit FX")
	FVector HitFXOffset = FVector::ZeroVector;

	// 이 타격에서 재생하는 FX의 균일 스케일
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hit FX", meta = (ClampMin = "0.01"))
	float HitFXScale = 1.0f;
};

UENUM(BlueprintType)
enum class ECXJustTimingType : uint8
{
	None,
	DamageBoost,
	ContinueAttack
};

UENUM(BlueprintType)
enum class ECXAttackTargetMode : uint8
{
	Single       UMETA(DisplayName = "단일 대상"),
	AllOpponents UMETA(DisplayName = "상대 전원")
};




/**
 * 
 */
UCLASS()
class CXVI_API UCXAbilityData : public UPrimaryDataAsset
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditDefaultsOnly, Category = "Montage")
	TObjectPtr<class UAnimMontage> AnimMontage;

	// 타격 시 데미지 배율 몽타주에 이 개수 만큼 애님노티파이를 배치해 피격 이벤트 쏴줘야함
	// 실제 타격 순서대로 설정한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack")
	TArray<FCXAttackHitData> HitData;

	UPROPERTY(EditDefaultsOnly, Category = "Cost")
	float APCost = 0.f;

	// true면 대상에게 접근한 뒤 시전하고, false면 제자리에서 시전한다.
	UPROPERTY(EditDefaultsOnly, Category = "Combat")
	bool bShouldApproach = true;

	// 저스트 타이밍 결과와 관계없이 모든 타격에 회피 불가를 적용한다.
	UPROPERTY(EditDefaultsOnly, Category = "Combat")
	bool bAlwaysUndodgeable = false;

	// 이번 공격이 적용될 대상 범위.
	UPROPERTY(EditDefaultsOnly, Category = "Combat")
	ECXAttackTargetMode AttackTargetMode = ECXAttackTargetMode::Single;

	UPROPERTY(EditDefaultsOnly, Category = "JustTiming")
	ECXJustTimingType JustTimingType = ECXJustTimingType::None;

	UPROPERTY(EditDefaultsOnly, Category = "JustTiming",
		meta = (ClampMin = "1.0"))
	float JustTimingDamageMultiplier = 1.5f;

	UPROPERTY(EditDefaultsOnly, Category = "JustTiming")
	bool bJustTimingCannotBeDodged = false;

	// 배열 0번은 1타, 1번은 2타.
	// 항목이 없으면 필요한 판정 수는 1개로 처리.
	UPROPERTY(EditDefaultsOnly, Category = "JustTiming", meta = (ClampMin = "0"))
	TArray<int32> RequiredTimingCounts;

	// 배열 0번은 1타 섹션, 1번은 2타 섹션.
	// 판정 성공 시 다음 배열 항목의 섹션으로 연결.
	UPROPERTY(EditDefaultsOnly, Category = "JustTiming|Combo")
	TArray<FName> AttackSectionNames;

	// None이면 현재 공격 섹션이 끝난뒤 몽타주 종료.
	// 마무리 동작이 있는 스킬은 해당 섹션이름을 지정
	UPROPERTY(EditDefaultsOnly, Category = "JustTiming|Combo")
	FName EndSectionName = NAME_None;

};
