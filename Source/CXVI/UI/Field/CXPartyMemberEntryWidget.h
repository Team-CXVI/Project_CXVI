// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"

#include "CXPartyMemberEntryWidget.generated.h"

class UImage;
class UTexture2D;


/**
 * 필드 HUD 파티 상태 한 칸 (WBP_PartyMemberEntry의 부모)
 * - 파티 목록의 Viewmodel Extension(Set Items)이 생성하고 Entry Viewmodel(CXPartyMemberViewModel)을 연결
 * - HP 바 / HP 수치 / 레벨은 WBP의 View Bindings로 직접 바인딩
 * - 초상화는 Soft 참조라 C++에서 비동기 로드 : WBP에서 Portrait → SetPortrait 바인딩
 */
UCLASS(Abstract)
class CXVI_API UCXPartyMemberEntryWidget : public UCommonUserWidget
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintCallable, Category = "CXVI|Party")
	void SetPortrait(TSoftObjectPtr<UTexture2D> InPortrait);

private:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> PortraitImage;
};
