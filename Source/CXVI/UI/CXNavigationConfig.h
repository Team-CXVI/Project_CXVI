// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include <Framework/Application/NavigationConfig.h>

/**
 * 게임 UI 내비게이션 키 설정
 * 엔진 기본(방향키, D-Pad, 왼쪽 스틱)에 WASD 추가
 * UCXUIManagerSubsystem이 로컬 플레이어 생성 시 등록
 * 해제 시 이전 설정으로 복구
 * (Slate 전역 설정이라 PIE 중에는 에디터 패널에도 적용됨)
 */
struct CXVI_API FCXNavigationConfig : public FNavigationConfig 
{
public:
	FCXNavigationConfig();
};
