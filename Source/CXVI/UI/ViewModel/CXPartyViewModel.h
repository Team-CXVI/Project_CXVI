// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MVVMViewModelBase.h"
#include "Types/MVVMViewModelContext.h"
#include "Foundation/GameplayMessage/CXGameplayMessageSubsystem.h"

#include "CXPartyViewModel.generated.h"

class UCXUserDataSubsystem;
class UCXPartyMemberViewModel;
struct FCXPartyChangedMessage;

/**
 * 파티 전체 표시 데이터 (필드 HUD 파티원 상태 목록)
 * - UCXUIManagerSubsystem이 한 번 생성해서 MVVM 전역 Viewmodel Collection에 등록 (보관은 Collection)
 *   → WBP Viewmodel 생성 방식 = Global Viewmodel Collection, Identifier = PartyViewModel
 * - Initialize : UserData의 Party로 Members 구성 + Message.UserData.PartyChanged 구독
 * - MemberIndex가 INDEX_NONE이거나 인원 수가 바뀌면 Members 재구성, 아니면 해당 파티원만 갱신
 * - 화면이 없는 동안(전투 중 등)에도 구독을 유지하므로 다시 바인딩될 때 최신 값이 표시됨
 */
UCLASS(BlueprintType)
class CXVI_API UCXPartyViewModel : public UMVVMViewModelBase
{
	GENERATED_BODY()
	
public:
	// 전역 Viewmodel Collection 등록 정보 (ContextName은 WBP의 Global Viewmodel Identifier와 같아야 함)
	static FMVVMViewModelContext MakeGlobalContext();

	// LocalPlayer Subsystem의 Initialize 시점에는 월드가 없을 수 있어서 GameInstance를 직접 받음
	void Initialize(UGameInstance& InGameInstance);

	void Reset();

private:
	void HandlePartyChanged(const FGameplayTag& InChannel, const FCXPartyChangedMessage& InMessage);

	void RebuildMembers();

	void RefreshMember(int32 InMemberIndex);
	
private:
	// 파티 상태 목록 (Set Items 바인딩)
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "CXVI|Party", meta = (AllowPrivateAccess = true))
	TArray<TObjectPtr<UCXPartyMemberViewModel>> Members;

	TWeakObjectPtr<UCXUserDataSubsystem> UserDataSubsystem;

	FCXGameplayMessageListenerHandle PartyChangedListener;

};
