// Fill out your copyright notice in the Description page of Project Settings.


#include "Subsystems/CXInputControlSubsystem.h"

#include <EnhancedInputSubsystems.h>
#include <Engine/LocalPlayer.h>


#include UE_INLINE_GENERATED_CPP_BY_NAME(CXInputControlSubsystem)

DEFINE_LOG_CATEGORY(LogCXInput);

UCXInputControlSubsystem& UCXInputControlSubsystem::Get(const UObject* WorldContextObject)
{
	UCXInputControlSubsystem* Subsystem = TryGet(WorldContextObject);
	check(Subsystem);
	return *Subsystem;
}

UCXInputControlSubsystem* UCXInputControlSubsystem::TryGet(const UObject* WorldContextObject)
{
	const UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
	if (nullptr == World)
	{
		return nullptr;
	}

	const ULocalPlayer* LocalPlayer = World->GetFirstLocalPlayerFromController();
	if (nullptr == LocalPlayer)
	{
		return nullptr;
	}

	return LocalPlayer->GetSubsystem<UCXInputControlSubsystem>();
}

void UCXInputControlSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// 레벨 이동 시 이전 월드의 요청 정리
	WorldCleanupHandle = FWorldDelegates::OnWorldCleanup.AddUObject(this, &ThisClass::HandleWorldCleanup);
}

void UCXInputControlSubsystem::Deinitialize()
{
	FWorldDelegates::OnWorldCleanup.Remove(WorldCleanupHandle);
	WorldCleanupHandle.Reset();

	Requests.Reset();
	TagCounts.Reset();

	Super::Deinitialize();
}

FCXInputModeHandle UCXInputControlSubsystem::AddInputModeTag(const FGameplayTag& InTag, FName InReason)
{
	FCXInputModeHandle Handle;

	if (!InTag.IsValid())
	{
		UE_LOG(LogCXInput, Warning, TEXT("AddInputModeTag 거부: 태그가 비어 있습니다. (%s)"), *InReason.ToString());
		return Handle;
	}

	if (IsReservedTag(InTag))
	{
		UE_LOG(LogCXInput, Warning, TEXT("AddInputModeTag 거부: 다른 곳이 관리하는 태그입니다. [%s] (%s)"), *InTag.ToString(), *InReason.ToString());
		return Handle;
	}

	int32& Count = TagCounts.FindOrAdd(InTag);

	// 첫 요청일 때만 입력 모드 변경 (중복 요청은 카운트만 증가)
	if (0 == Count && !ApplyInputModeTag(InTag, true))
	{
		TagCounts.Remove(InTag);
		UE_LOG(LogCXInput, Warning, TEXT("AddInputModeTag 거부: PlayerInput이 없습니다. [%s] (%s)"), *InTag.ToString(), *InReason.ToString());
		return Handle;
	}

	++Count;

	Handle.Id = NextHandleId++;
	Requests.Add(Handle.Id, FTagRequest{ InTag, InReason });

	UE_LOG(LogCXInput, Log, TEXT("Input mode tag added: %s (%s), Count=%d"), *InTag.ToString(), *InReason.ToString(), Count);
	return Handle;

}

void UCXInputControlSubsystem::RemoveInputModeTag(FCXInputModeHandle& InOutHandle)
{
	FTagRequest Request;
	if (!InOutHandle.IsValid() || !Requests.RemoveAndCopyValue(InOutHandle.Id, Request))
	{
		InOutHandle.Reset();
		return;
	}

	InOutHandle.Reset();

	int32* Count = TagCounts.Find(Request.Tag);
	if (nullptr == Count)
	{
		return;
	}

	// 마지막 요청이 반납될 때만 입력 모드에서 태그 제거
	if (--(*Count) > 0)
	{
		UE_LOG(LogCXInput, Log, TEXT("Input mode tag released: %s (%s), Count=%d"), *Request.Tag.ToString(), *Request.Reason.ToString(), *Count);
		return;
	}

	TagCounts.Remove(Request.Tag);
	ApplyInputModeTag(Request.Tag, false);

	UE_LOG(LogCXInput, Log, TEXT("Input mode tag removed: %s (%s)"), *Request.Tag.ToString(), *Request.Reason.ToString());
}

bool UCXInputControlSubsystem::HasInputModeTag(const FGameplayTag& InTag) const
{
	return TagCounts.Contains(InTag);
}

bool UCXInputControlSubsystem::IsReservedTag(const FGameplayTag& InTag) const
{
	// 여기서 관리할 대상이 아닌 태그
	// CommonUI가 입력 설정(Game / Menu)에 따라 직접 관리하는 태그 (UCommonUIActionRouterBase)
	static const FGameplayTag CommonUIGameTag = FGameplayTag::RequestGameplayTag(TEXT("InputMode.Game"), false);
	static const FGameplayTag CommonUIMenuTag = FGameplayTag::RequestGameplayTag(TEXT("InputMode.Menu"), false);

	// 시작 입력 모드 (Project Settings > Enhanced Input > Default Input Mode). 
	// 이게 빠지면 기본 쿼리 IMC가 전부 비활성화 됨
	static const FGameplayTag DefaultModeTag = FGameplayTag::RequestGameplayTag(TEXT("EnhancedInput.Modes.Default"), false);

	// 이 중에 하나라도 걸리면 여기서 관리하지 않음
	return InTag.MatchesTagExact(CommonUIGameTag)
		|| InTag.MatchesTagExact(CommonUIMenuTag)
		|| InTag.MatchesTagExact(DefaultModeTag);
}

bool UCXInputControlSubsystem::ApplyInputModeTag(const FGameplayTag& InTag, bool bInAdd) const
{
	UEnhancedInputLocalPlayerSubsystem* InputSubsystem = GetInputSubsystem();

	// 입력 모드는 PlayerController의 PlayerInput이 들고 있음 (PC 생성 전 / 정리 후에는 없음)
	if (nullptr == InputSubsystem || nullptr == InputSubsystem->GetPlayerInput())
	{
		return false;
	}

	// 태그 변경 후 다음 처리 시점에 매핑 재구성 (같은 프레임의 변경은 한 번으로 묶임)
	if (bInAdd)
	{
		InputSubsystem->AddTagToInputMode(InTag);
	}
	else
	{
		InputSubsystem->RemoveTagFromInputMode(InTag);
	}

	return true;
}

UEnhancedInputLocalPlayerSubsystem* UCXInputControlSubsystem::GetInputSubsystem() const
{
	const ULocalPlayer* LocalPlayer = GetLocalPlayer();
	return LocalPlayer ? LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>() : nullptr;
}

void UCXInputControlSubsystem::HandleWorldCleanup(UWorld* InWorld, bool bInSessionEnded, bool bInCleanupResources)
{
	const ULocalPlayer* LocalPlayer = GetLocalPlayer();
	if (nullptr == InWorld || nullptr == LocalPlayer || !InWorld->IsGameWorld() || InWorld->GetGameInstance() != LocalPlayer->GetGameInstance())
	{
		return;
	}

	if (Requests.IsEmpty())
	{
		return;
	}

	// 요청자는 이전 월드와 함께 사라지므로 반납되지 않은 요청 폐기
	for (const TPair<int32, FTagRequest>& Pair : Requests)
	{
		UE_LOG(LogCXInput, Warning, TEXT("월드 정리: 반납되지 않은 입력 모드 태그 폐기 [%s] (%s)"), *Pair.Value.Tag.ToString(), *Pair.Value.Reason.ToString());
	}

	// PlayerInput이 아직 남아 있으면 태그도 제거 (새 PlayerInput은 기본 입력 모드로 시작)
	for (const TPair<FGameplayTag, int32>& Pair : TagCounts)
	{
		ApplyInputModeTag(Pair.Key, false);
	}

	Requests.Reset();
	TagCounts.Reset();
}
