// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MVVMViewModelBase.h"
#include "Types/MVVMViewModelContext.h"
#include "Foundation/GameplayMessage/CXGameplayMessageSubsystem.h"

#include "CXPotionViewModel.generated.h"

class UTexture2D;
class UCXUserDataSubsystem;
struct FCXInventoryChangedMessage;

/**
 * 필드 HUD 포션 칸 표시 데이터 (아이콘 / 보유 수량 / 사용 가능 여부)
 * - UCXUIManagerSubsystem이 한 번 생성해서 MVVM 전역 Viewmodel Collection에 등록 (보관은 Collection)
 *   → WBP Viewmodel 생성 방식 = Global Viewmodel Collection, Identifier = PotionViewModel
 * - Initialize : 포션 아이콘(DT_Item) + 보유 수량으로 초기화 + Message.UserData.InventoryChanged 구독
 * - 메시지의 ItemId가 None(새 게임 / 불러오기)이면 수량을 다시 읽고, 포션이면 NewCount로 갱신
 * - 홀드 게이지는 입력에 묶인 화면 상태라 여기 두지 않음 (UCXPotionSlotWidget)
 */
UCLASS(BlueprintType)
class CXVI_API UCXPotionViewModel : public UMVVMViewModelBase
{
	GENERATED_BODY()
	
public:
	// 전역 Viewmodel Collection 등록 정보 (ContextName은 WBP의 Global Viewmodel Identifier와 같아야 함)
	static FMVVMViewModelContext MakeGlobalContext();

	// LocalPlayer Subsystem의 Initialize 시점에는 월드가 없을 수 있어서 GameInstance를 직접 받음
	void Initialize(UGameInstance& InGameInstance);

	void Reset();

	// 수량 TextBlock 바인딩용
	UFUNCTION(BlueprintPure, FieldNotify, Category = "CXVI|Potion")
	FText GetCountText() const;

	// 보유 수량이 1개 이상 (비활성 표시 바인딩용)
	UFUNCTION(BlueprintPure, FieldNotify, Category = "CXVI|Potion")
	bool IsUsable() const;
	
private:
	void HandleInventoryChanged(const FGameplayTag& InChannel, const FCXInventoryChangedMessage& InMessage);

	void RefreshIcon(UGameInstance& InGameInstance);

	void RefreshCount();

	void SetIcon(const TSoftObjectPtr<UTexture2D>& InValue);

	void SetCount(int32 InValue);
	
private:
	// 다른 형태의 프로그레스 바를 표시
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "CXVI|Potion", meta = (AllowPrivateAccess = true))
	TSoftObjectPtr<UTexture2D> Icon;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "CXVI|Potion", meta = (AllowPrivateAccess = true))
	int32 Count = 0;

	TWeakObjectPtr<UCXUserDataSubsystem> UserDataSubsystem;

	FCXGameplayMessageListenerHandle InventoryChangedListener;

};
