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

void UCXJustTimingComponent::BeginWindow(
	USkeletalMeshComponent* InMesh, UAnimMontage* InMontage, 
	int32 InHitNumber, int32 InTimingIndex, float InStartPosition, 
	float InDuration, float InSuccessStartRatio, float InSuccessEndRatio)
{
	if (!IsValid(InMesh) || !IsValid(InMontage) || InHitNumber < 1 || InTimingIndex < 1 || InDuration <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	const FIntPoint Key(InHitNumber, InTimingIndex);

	// 같은 타격안에서는 판정 번호가 중복되면 안된다.
	if (Windows.Contains(Key))
	{
		UE_LOG(LogTemp, Warning, TEXT("[저스트] 중복구간: %d타 / 판정%d"), InHitNumber, InTimingIndex);
		return;
	}

	FCXJustTimingWindow Window;
	Window.Mesh = InMesh;
	Window.Montage = InMontage;
	Window.StartPosition = InStartPosition;
	Window.Duration = InDuration;
	Window.SuccessStartRatio = FMath::Clamp(InSuccessStartRatio, 0.f, 1.f);
	Window.SuccessEndRatio = FMath::Clamp(InSuccessEndRatio, Window.SuccessStartRatio, 1.f);

	Windows.Add(Key, Window);

	UE_LOG(LogTemp, Display, TEXT("[저스트] %d타 / 판정 %d 시작: 성공 범위 %.2f ~ %.2f"),
		InHitNumber,
		InTimingIndex,
		Window.SuccessStartRatio,
		Window.SuccessEndRatio);


}

void UCXJustTimingComponent::EndWindow(int32 InHitNumber, int32 InTimingIndex)
{
	FCXJustTimingWindow* Window = Windows.Find(FIntPoint(InHitNumber, InTimingIndex));

	if (!Window || !Window->bActive)
	{
		return;
	}

	Window->bActive = false;

	if (!Window->bInputUsed)
	{
		Window->bSuccess = false;

		UE_LOG(LogTemp, Display, TEXT("[저스트] %d타 / 판정 %d 미입력 실패"), InHitNumber, InTimingIndex);
	}
	
	// 종료된 구간도 결과 확인 전까지 기록을 유지

}

void UCXJustTimingComponent::HandleInput()
{
	FCXJustTimingWindow* SelectedWindow = nullptr;
	FIntPoint SelectedKey(INDEX_NONE, INDEX_NONE);

	float SelectedProgress = 0.f;
	float BestRemaining = TNumericLimits<float>::Max();

	for (auto& Pair : Windows)
	{
		FCXJustTimingWindow& Window = Pair.Value;

		if (!Window.bActive || Window.bInputUsed || !Window.Mesh.IsValid() || !Window.Montage.IsValid())
		{
			continue;
		}

		UAnimInstance* AnimInstance = Window.Mesh->GetAnimInstance();

		if (!IsValid(AnimInstance) || !AnimInstance->Montage_IsActive(Window.Montage.Get()))
		{
			continue;
		}

		const float Position = AnimInstance->Montage_GetPosition(Window.Montage.Get());

		const float EndPosition = Window.StartPosition + Window.Duration;

		if (Position < Window.StartPosition || Position > EndPosition)
		{
			continue;
		}

		const float Remaining = EndPosition - Position;

		// 종료 시점이 같으면 타격번호, 판정번호 순서로 선택
		const bool bEarlierKey = Pair.Key.X < SelectedKey.X || (Pair.Key.X == SelectedKey.X && Pair.Key.Y < SelectedKey.Y);

		if (!SelectedWindow || Remaining < BestRemaining || (Remaining == BestRemaining && bEarlierKey))
		{
			SelectedWindow = &Window;
			SelectedKey = Pair.Key;
			BestRemaining = Remaining;

			SelectedProgress = (Position - Window.StartPosition) / Window.Duration;
		}
	}

	if (!SelectedWindow)
	{
		return;
	}

	// 입력 한번은 선택된 판정 하나에만 사용
	SelectedWindow->bInputUsed = true;

	SelectedWindow->bSuccess = SelectedProgress >= SelectedWindow->SuccessStartRatio && SelectedProgress <= SelectedWindow->SuccessEndRatio;

	UE_LOG(LogTemp, Display, TEXT("[저스트] %d타 / 판정 %d 입력: 진행도 %.2f / %s"),
		SelectedKey.X,
		SelectedKey.Y,
		SelectedProgress,
		SelectedWindow->bSuccess ? TEXT("성공") : TEXT("실패"));

}

bool UCXJustTimingComponent::ConsumeSuccess(int32 InHitNumber, int32 RequiredCount)
{
	RequiredCount = FMath::Max(1, RequiredCount);

	bool bAllSuccess = true;
	int32 SuccessCount = 0;


	for (int32 Index = 1; Index <= RequiredCount; ++Index)
	{
		const FCXJustTimingWindow* Window =	Windows.Find(FIntPoint(InHitNumber, Index));

		// 판정이 누락됐거나 아직 종료되지 않았어도 실패
		const bool bSuccess = Window && !Window->bActive && Window->bInputUsed && Window->bSuccess;

		if (bSuccess)
		{
			++SuccessCount;
		}
		else
		{
			bAllSuccess = false;
		}
	}

	int32 RegisteredCount = 0;

	for (auto It = Windows.CreateIterator(); It; ++It)
	{
		if (It.Key().X == InHitNumber)
		{
			++RegisteredCount;
			It.RemoveCurrent();
		}
	}

	// 데이터에 설정한 수와 실제 배치된 구간 수가 일치해야 한다.
	bAllSuccess = bAllSuccess && RegisteredCount == RequiredCount;

	UE_LOG(LogTemp, Display,
		TEXT("[저스트] %d타 최종: %d/%d 성공 / %s"),
		InHitNumber,
		SuccessCount,
		RequiredCount,
		bAllSuccess ? TEXT("전체 성공") : TEXT("실패"));

	return bAllSuccess;
}

void UCXJustTimingComponent::ResetTiming()
{
	Windows.Reset();
}

