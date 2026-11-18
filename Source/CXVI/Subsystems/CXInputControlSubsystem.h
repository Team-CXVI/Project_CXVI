// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "GameplayTagContainer.h"

#include "CXInputControlSubsystem.generated.h"

class UEnhancedInputLocalPlayerSubsystem;

DECLARE_LOG_CATEGORY_EXTERN(LogCXInput, Log, All);

/** 입력 모드 태그 요청 핸들 (AddInputModeTag가 발급, RemoveInputModeTag로 반납) */
USTRUCT(BlueprintType)
struct CXVI_API FCXInputModeHandle
{
	GENERATED_BODY()

public:
	bool IsValid() const { return INDEX_NONE != Id; }

	void Reset() { Id = INDEX_NONE; }

private:
	friend class UCXInputControlSubsystem;

	int32 Id = INDEX_NONE;
};

/**
 * 입력 제어 매니저 (로컬 플레이어마다)
 * - Enhanced Input 입력 모드(태그 컨테이너)에 상황 태그를 추가/제거
 *   -> 매핑 재구성 때 각 IMC의 Input Mode Query가 현재 태그로 적용 여부를 결정
 *   - 막기 : IMC 쿼리에 "태그 없음" 조건 (프로젝트 기본 쿼리 = Default 있음 AND InputMode.Block.All 없음)
 *   - 켜기 : IMC 쿼리에 "태그 있음" 조건
 *   - 전환 : 두 IMC에 "있음" / "없음"을 나눠 지정
 * - 해당 매니저는 IMC 등록/해제를 하지 않음
 * - 같은 태그를 여러 곳에서 요청할 수 있으므로 태그별 요청 수를 세고, 마지막 요청이 반납될 때만 태그 제거
 * - SetInputMode(통째 교체)는 쓰지 않음. 다른 곳이 관리하는 태그(CommonUI의 InputMode.Game / Menu, 시작 입력 모드)는 요청 거부
 * - 월드 정리 시 남은 요청 폐기 (요청자는 이전 월드와 함께 사라지고, 새 PlayerInput은 기본 입력 모드로 시작)
 * - 이벤트 단위 전환용 (태그 변경마다 매핑 재구성). 짧고 잦은 차단은 GAS 태그로 처리
 */
UCLASS()
class CXVI_API UCXInputControlSubsystem : public ULocalPlayerSubsystem
{
	GENERATED_BODY()
	
public:
	// 싱글 플레이 기준: 첫 번째 로컬 플레이어의 Input Control
	static UCXInputControlSubsystem& Get(const UObject* WorldContextObject);

	static UCXInputControlSubsystem* TryGet(const UObject* WorldContextObject);

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	virtual void Deinitialize() override;

public:
	// InTag : InputMode.Block.* / InputMode.Context.* / InReason : 요청자 구분용 (로그)
	// 예약 태그이거나 PlayerInput이 없으면(PlayerController 생성 전 등) 거부하고 무효 핸들 반환
	FCXInputModeHandle AddInputModeTag(const FGameplayTag& InTag, FName InReason);

	// 반납 후 핸들은 무효화. 이미 반납했거나 월드 정리로 폐기된 핸들은 무시
	void RemoveInputModeTag(FCXInputModeHandle& InOutHandle);

	// 이 매니저가 넣은 태그 기준 (CommonUI 등 다른 곳이 넣은 태그는 포함하지 않음)
	bool HasInputModeTag(const FGameplayTag& InTag) const;

private:
	// 다른 곳이 관리하는 입력 모드 태그인지
	bool IsReservedTag(const FGameplayTag& InTag) const;

	// true = 입력 모드에 태그 추가, false = 제거. PlayerInput이 없으면 false 반환
	bool ApplyInputModeTag(const FGameplayTag& InTag, bool bInAdd) const;

	UEnhancedInputLocalPlayerSubsystem* GetInputSubsystem() const;

	void HandleWorldCleanup(UWorld* InWorld, bool bInSessionEnded, bool bInCleanupResources);

private:
	struct FTagRequest
	{
		FGameplayTag Tag;
		FName Reason;
	};

	// <핸들 Id, 요청>
	TMap<int32, FTagRequest> Requests;

	// <태그, 요청 수>
	TMap<FGameplayTag, int32> TagCounts;
	
	int32 NextHandleId = 0;

	FDelegateHandle WorldCleanupHandle;
};
