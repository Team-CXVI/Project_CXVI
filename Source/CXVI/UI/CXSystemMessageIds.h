// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

/**
 * 팝업 본문 ID (시스템 메시지 String Table의 키)
 * - UCXUIManagerSubsystem::ShowConfirmPopup / ShowConfirmCancelPopup의 InBodyTextId로 전달
 * - UCXPopupWidget이 WBP_Popup에 지정된 SystemMessageTable(ST_UI_SystemMessage)에서 이 키로 본문을 찾음
 * - 키를 추가할 때는 ST_UI_SystemMessage에도 같은 키를 추가 (없으면 본문이 비고 LogCXUI 경고)
 * - 호출부에 문자열을 직접 쓰지 않고 이 상수를 사용 (오타 방지)
 */
namespace CXSystemMessageIds
{
	// 저장 슬롯 목록 : 슬롯 불러오기 확인
	inline const FName Save_LoadConfirm = TEXT("Save.LoadConfirm");

	// 저장 슬롯 목록 : 기존 슬롯 덮어쓰기 확인
	inline const FName Save_OverwriteConfirm = TEXT("Save.OverwriteConfirm");

	// 저장 슬롯 목록 : 저장 실패 알림
	inline const FName Save_Failed = TEXT("Save.Failed");

	// 타이틀 / 필드 메뉴 : 게임 종료 확인
	inline const FName Game_QuitConfirm = TEXT("Game.QuitConfirm");

	// 필드 메뉴 : 타이틀로 돌아가기 확인
	inline const FName Game_ReturnToTitleConfirm = TEXT("Game.ReturnToTitleConfirm");

	// 깃발 메뉴 : 휴식 완료 알림
	inline const FName Flag_RestComplete = TEXT("Flag.RestComplete");
}
