// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"

#include "CXLevelTransitionSubsystem.generated.h"

class UCXLevelDefinition;

DECLARE_LOG_CATEGORY_EXTERN(LogCXLevelTransition, Log, All);

DECLARE_MULTICAST_DELEGATE_OneParam(FCXOnLevelTransition, const UCXLevelDefinition* /*TargetLevel*/);

/**
 * 레벨 간 전환 관리
 * - 타이틀 이동은 TravelToTitle, 인게임 레벨 이동은 TravelToLevel 호출 (두 함수는 내부에서 같은 이동 경로를 사용)
 * - 전환 = 이동 요청부터 도착한 레벨의 준비 완료(NotifyLevelReady)까지
 * - 전환 중에는 다른 이동 요청과 게임 종료를 거부
 * - 진입 태그를 보관해서 도착한 GameMode가 스폰 위치를 고를 수 있게 함
 * - 세션 데이터는 다루지 않음
 * 
 * *도착한 레벨의 GameMode는 준비가 끝나면 반드시 NotifyLevelReady()를 호출해서 레벨 준비가 끝났다는걸 알려야 함
 */
UCLASS()
class CXVI_API UCXLevelTransitionSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
public:
	static UCXLevelTransitionSubsystem& Get(const UObject* WorldContextObject);

	static UCXLevelTransitionSubsystem* TryGet(const UObject* WorldContextObject);

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	virtual void Deinitialize() override;

public:
	// 타이틀 레벨(Settings.TitleLevel에 설정된 레벨 정의)로 이동
	UFUNCTION(BlueprintCallable, Category = "CXVI|LevelTransition")
	bool TravelToTitle();

	// 인게임 레벨로 이동. InEntryTag가 None이면 도착 레벨의 기본 PlayerStart 사용
	// 타이틀 레벨 ID는 거부 (TravelToTitle 사용)
	UFUNCTION(BlueprintCallable, Category = "CXVI|LevelTransition")
	bool TravelToLevel(UPARAM(meta = (AllowedTypes = "CXLevelDefinition")) FPrimaryAssetId InLevelId, FName InEntryTag);

	UFUNCTION(BlueprintCallable, Category = "CXVI|LevelTransition")
	bool QuitGame();

	// 레벨 전이 중인지 체크
	UFUNCTION(BlueprintPure, Category = "CXVI|LevelTransition")
	bool IsTransitioning() const { return bIsTransitioning; }

	// 현재 레벨의 레벨 정의 (정의가 없는 레벨이면 nullptr)
	const UCXLevelDefinition* GetCurrentLevelDefinition() const { return CurrentDefinition; }

	// 현재 레벨의 GameMode가 스폰할 위치 Tag
	FName GetPendingEntryTag() const { return PendingEntryTag; }

	// 도착한 레벨의 GameMode가 레벨 준비 완료 시 호출. 레벨 전환 상태에서만 처리
	void NotifyLevelReady();

private:
	// TravelToTitle / TravelToLevel 공통 이동 경로
	bool BeginTransition(UCXLevelDefinition* InDefinition, FName InEntryTag);

	UCXLevelDefinition* LoadLevelDefinition(const FPrimaryAssetId& InLevelId) const;

	// 요청 없이 시작된 월드(부트 맵, 에디터에서 바로 PIE)의 레벨 정의 검색
	UCXLevelDefinition* FindLevelDefinitionForWorld(const UWorld* InWorld) const;

	void HandleWorldActorsInitialized(const UWorld::FActorsInitializedParams& InParams);

	// Return Value
	// true = 페이드 대기 시작 (완료 시 OpenPendingLevel 호출)
	// false = 즉시 완료
	bool StartFadeOut(float InDuration, const FLinearColor& InColor);

	void OpenPendingLevel();

	void HandleTravelFailure(UWorld* InWorld, ETravelFailure::Type InFailureType, const FString& InErrorString);

	void ResetTransition();

#if !UE_BUILD_SHIPPING
	void ValidateSettings() const;
#endif // !UE_BUILD_SHIPPING

public:
	FCXOnLevelTransition OnTransitionStarted;

	FCXOnLevelTransition OnTransitionFinished;

private:
	// 이동 중인 목표 레벨 정의 (레벨 이동 중에도 GC되지 않도록 UPROPERTY)
	UPROPERTY()
	TObjectPtr<UCXLevelDefinition> PendingDefinition;

	// 현재 레벨 정의
	UPROPERTY()
	TObjectPtr<UCXLevelDefinition> CurrentDefinition;

	bool bIsTransitioning = false;

	// EntryTag : PlayerStart 지정을 위한 Tag
	FName PendingEntryTag;

	// 페이드 시간 제어를 위한 타이머 핸들
	FTimerHandle FadeHandle;

	// 레벨 로드 실패 시 엔진으로부터 호출되는 델리게이트 핸들
	FDelegateHandle TravelFailureHandle;

	// 새 레벨(월드)의 액터 초기화 직후(BeginPlay 전)에 호출되는 델리게이트 핸들
	FDelegateHandle WorldActorsInitializedHandle;
};
