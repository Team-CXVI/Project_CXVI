// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CXJustTimingComponent.generated.h"

class UAnimMontage;
class USkeletalMeshComponent;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class CXVI_API UCXJustTimingComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UCXJustTimingComponent();

	void BeginWindow(
		USkeletalMeshComponent* InMesh,
		UAnimMontage* InMontage,
		int32 InHitNumber,
		float InStartPosition,
		float InDuration,
		float InSuccessStartRatio,
		float InSuccessEndRatio);

	void EndWindow(int32 InHitNumber);

	void HandleInput();

	// 해당 타격의 결과를 가져오고 기록에서 제거
	bool ConsumeSuccess(int32 InHitNumber);

	// 공격 시작 종료 중단 시 초기화
	void ResetTiming();

private:
	TWeakObjectPtr<USkeletalMeshComponent> Mesh;
	TWeakObjectPtr<UAnimMontage> Montage;

	int32 CurrentHitNumber = INDEX_NONE;

	float StartPosition = 0.f;
	float Duration = 0.f;
	float SuccessStartRatio = 0.f;
	float SuccessEndRatio = 1.f;

	bool bWindowActive = false;
	bool bInputUsed = false;

	TMap<int32, bool> HitResults;

		
};
