// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/CXActivatableWidget.h"
#include "Foundation/GameplayMessage/CXGameplayMessageSubsystem.h"

#include "CXFieldHUDWidget.generated.h"

class UInputAction;
class UCXFieldMenuWidget;
class UCommonActivatableWidget;
class UCommonActivatableWidgetContainerBase;
class UBorder;
class UTextBlock;
class UWidgetAnimation;
struct FCXItemAcquiredMessage;

/**
 * 필드 HUD 베이스 (WBP_FieldHUD의 부모)
 * - 필드 Scene 정의의 HUDClass로 지정 → Scene이 UI.Layer.Game에 Push
 * - 입력 모드 Game
 * - 메뉴 열기 : OpenMenuAction을 Game 모드 UI 액션으로 바인딩 → UI.Layer.GameMenu에 MenuClass Push
 *   (OpenMenuAction의 키는 IMC_UI에 매핑해야 키 조회 / 입력 처리가 됨)
 * - 버튼 안내(메뉴 / 이동 / 시점 / 공격)는 WBP에서 WBP_InputHint로 배치하고 키를 지정
 * - 메뉴가 열린 동안 숨김 : UI.Layer.GameMenu의 표시 위젯 변경을 구독해서 루트 위젯을 Collapsed / 복원
 *   (HUD 자신을 숨기면 CommonUI가 입력 불가로 처리하므로 루트 위젯만 숨김)
 * - 시스템 메시지 : Message.Field.ItemAcquired를 받아 습득 아이템을 잠깐 표시 후 숨김 (전투 HUD와 같은 방식)
 *   - 문구는 아이템마다 ItemAcquiredFormat으로 만들고, 여러 개면 줄바꿈으로 이어 한 번에 표시
 *   - 아이템 이름은 DT_Item의 DisplayName. Row가 없는 아이템은 건너뜀
 *   - 표시 / 숨김은 프레임(SystemMessageBox) 단위. 문구는 그 안의 SystemMessageText
 *   - 표시 시간 / 연출은 SystemMessageAnim이 담당. 끝나면 SystemMessageBox를 숨김
 *   - 표시 중 새 메시지가 오면 문구를 바꾸고 애니메이션을 처음부터 다시 재생
 *   - 구독 기간은 Activated ~ Deactivated (메뉴 레이어 구독과 같음)
 */
UCLASS(Abstract)
class CXVI_API UCXFieldHUDWidget : public UCXActivatableWidget
{
	GENERATED_BODY()
	
public:
	UCXFieldHUDWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	virtual void NativeOnInitialized() override;

	virtual void NativeOnActivated() override;

	virtual void NativeOnDeactivated() override;

	virtual void OnAnimationFinished_Implementation(const UWidgetAnimation* InAnimation) override;

private:
	void HandleOpenMenu();

	// GameMenu 레이어의 표시 위젯이 바뀔 때 호출. 표시 중인 위젯이 없으면 nullptr
	void HandleGameMenuLayerChanged(UCommonActivatableWidget* InDisplayedWidget);

	void SetContentHidden(bool bInHidden);

	void HandleItemAcquired(const FGameplayTag& InChannel, const FCXItemAcquiredMessage& InMessage);

	void ShowSystemMessage(const FText& InText);

	void HideSystemMessage();

protected:
	// 메뉴 열기 입력
	UPROPERTY(EditDefaultsOnly, Category = "Menu")
	TObjectPtr<UInputAction> OpenMenuAction;

	// 메뉴 위젯 클래스
	UPROPERTY(EditDefaultsOnly, Category = "Menu")
	TSubclassOf<UCXFieldMenuWidget> MenuClass;

	// 아이템 습득 문구 형식 (아이템 1개당 1줄). {Name} = 아이템 이름, {Count} = 습득 수량
	UPROPERTY(EditDefaultsOnly, Category = "System Message")
	FText ItemAcquiredFormat = NSLOCTEXT("CXFieldHUD", "ItemAcquiredFormat", "{Name} x{Count} 획득");

private:
	// 시스템 메시지 프레임 (SystemMessageText를 감쌈). 표시 / 숨김 대상
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> SystemMessageBox;

	// 시스템 메시지 문구. 프레임 / 문구 중 하나라도 없으면 시스템 메시지를 표시하지 않음
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SystemMessageText;

	// 시스템 메시지 표시 연출 (나타남 → 유지 → 사라짐). 길이 = 표시 시간. SystemMessageBox에 걸 것
	UPROPERTY(Transient, meta = (BindWidgetAnimOptional))
	TObjectPtr<UWidgetAnimation> SystemMessageAnim;

	FCXGameplayMessageListenerHandle ItemAcquiredListener;


private:
	// 구독 중인 GameMenu 레이어
	TWeakObjectPtr<UCommonActivatableWidgetContainerBase> GameMenuLayer;

	FDelegateHandle GameMenuLayerChangedHandle;

	// 숨기기 전 루트 위젯의 Visibility (복원용)
	ESlateVisibility CachedRootVisibility = ESlateVisibility::Visible;

	bool bContentHidden = false;
};
