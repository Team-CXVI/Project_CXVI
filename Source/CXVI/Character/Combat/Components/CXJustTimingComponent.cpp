// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/Combat/Components/CXJustTimingComponent.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(CXJustTimingComponent)

// Sets default values for this component's properties
UCXJustTimingComponent::UCXJustTimingComponent()
{
	
}

void UCXJustTimingComponent::BeginWindow(USkeletalMeshComponent* InMesh, UAnimMontage* InMontage, int32 InHitNumber, float InStartPosition, float InDuration, float InSuccessStartRatio, float InSuccessEndRatio)
{
	if (!IsValid(InMesh) || !IsValid(InMontage) || InHitNumber < 1 || InDuration <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	// 앞구간이 남아 있다면 먼저 닫는다.
	if (bWindowActive)
	{
		EndWindow(CurrentHitNumber);
	}

	Mesh = InMesh;
	Montage = InMontage;
	CurrentHitNumber = InHitNumber;

	StartPosition = InStartPosition;
	Duration = InDuration;

	SuccessStartRatio = FMath::Clamp(InSuccessStartRatio, 0.f, 1.f);

	SuccessEndRatio = FMath::Clamp(InSuccessEndRatio, SuccessStartRatio, 1.f);

	bWindowActive = true;
	bInputUsed = false;

	HitResults.Remove(InHitNumber);

	UE_LOG(LogTemp, Display, TEXT("[저스트] %d타 구간 시작: 성공 범위 %.2f ~ %.2f"),
		CurrentHitNumber,
		SuccessStartRatio,
		SuccessEndRatio);

}

void UCXJustTimingComponent::EndWindow(int32 InHitNumber)
{
	if (!bWindowActive || CurrentHitNumber != InHitNumber)
	{
		return;
	}

	if (!bInputUsed)
	{
		HitResults.Add(CurrentHitNumber, false);

		UE_LOG(LogTemp, Display, TEXT("[저스트] %d타 미입력 실패"), CurrentHitNumber);
	}

	// 결과는 남겨두고 입력 구간만 닫는다.
	bWindowActive = false;

}

void UCXJustTimingComponent::HandleInput()
{
	if (!bWindowActive || bInputUsed)
	{
		return;
	}

	UAnimInstance* AnimInstance = Mesh.IsValid() ? Mesh->GetAnimInstance() : nullptr;

	if (!IsValid(AnimInstance) || !Montage.IsValid() || !AnimInstance->Montage_IsActive(Montage.Get()))
	{
		return;
	}

	const float Position = AnimInstance->Montage_GetPosition(Montage.Get());

	const float Progress = (Position - StartPosition) / Duration;

	// 해당 타격의 첫 입력만 사용한다.
	bInputUsed = true;

	const bool bSuccess = Progress >= SuccessStartRatio && Progress <= SuccessEndRatio;

	HitResults.Add(CurrentHitNumber, bSuccess);

	UE_LOG(LogTemp, Display, TEXT("[저스트] %d타 입력: 진행도 %.2f / %s"),
		CurrentHitNumber,
		Progress,
		bSuccess ? TEXT("성공") : TEXT("실패"));

}

bool UCXJustTimingComponent::ConsumeSuccess(int32 InHitNumber)
{
	bool bSuccess = false;
	HitResults.RemoveAndCopyValue(InHitNumber, bSuccess);
	return bSuccess;
}

void UCXJustTimingComponent::ResetTiming()
{
	bWindowActive = false;
	bInputUsed = false;
	CurrentHitNumber = INDEX_NONE;

	Mesh.Reset();
	Montage.Reset();
	HitResults.Reset();
}

