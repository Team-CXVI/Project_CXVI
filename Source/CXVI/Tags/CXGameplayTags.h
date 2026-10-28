// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "NativeGameplayTags.h"

namespace CXVIGameplayTags
{
	// Input
	CXVI_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_Combat_Confirm);
	CXVI_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_Combat_Parry);
	CXVI_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_Combat_Dodge);

	// CombatInputMode
	CXVI_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputMode_Combat_Command);
	CXVI_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputMode_Combat_Reaction);
	// Input Mode (Enhanced Input 입력 모드에 넣는 태그. IMC의 Input Mode Query가 검사)
	// InputMode.Block.*   : IMC 쿼리에 "없음" 조건 -> 태그가 있으면 IMC 제외
	// InputMode.Context.* : IMC 쿼리에 "있음" 조건 -> 태그가 있으면 IMC 적용
	CXVI_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputMode_Block_All);

	// Status
	CXVI_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Combat_Dodging);
	CXVI_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Combat_Parrying);

	// Event
	CXVI_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Combat_ParrySucceeded);
	CXVI_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Combat_AttackHit);

	// Message
	CXVI_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Message_Combat_End);

	// Data
	CXVI_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_Damage);

	// UI Layer
	CXVI_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(UI_Layer_Game);
	CXVI_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(UI_Layer_GameMenu);
	CXVI_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(UI_Layer_Menu);
	CXVI_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(UI_Layer_Modal);
	
	// Ability
	CXVI_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Basic_Attack);
	CXVI_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Fire);
	CXVI_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Dodge);
	CXVI_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Parry);
}