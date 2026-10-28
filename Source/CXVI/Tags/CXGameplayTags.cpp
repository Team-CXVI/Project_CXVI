// Fill out your copyright notice in the Description page of Project Settings.


#include "CXGameplayTags.h"

namespace CXVIGameplayTags
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Input_Combat_Confirm, "Input.Combat.Confirm", "전투 선택 확정 입력");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Input_Combat_Parry, "Input.Combat.Parry", "적 공격 패링 입력");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Input_Combat_Dodge, "Input.Combat.Dodge", "적 공격 회피 입력");


	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputMode_Combat_Command, "InputMode.Combat.Command", "플레이어 차례 입력모드");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputMode_Combat_Reaction, "InputMode.Combat.Reaction", "적 차례 입력모드");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputMode_Block_All, "InputMode.Block.All", "플레이어 게임 입력 전체 차단 (프로젝트 기본 Input Mode Query를 쓰는 IMC 제외)");


	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Status_Stunned, "Status.Stunned", "행동 불가 상태");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Status_Dead, "Status.Dead", "사망 상태");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Combat_ParrySucceeded, "Event.Combat.ParrySucceeded", "패링 성공 시 GAS 이벤트");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Combat_AttackHit, "Event.Combat.AttackHit", "공격 타격시 GAS 이벤트");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Message_Combat_End, "Message.Combat.End", "전투 종료 알림 채널");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Data_Damage, "Data.Damage", "GE에 전달하는 데미지 값(SetByCaller)");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(UI_Layer_Game, "UI.Layer.Game", "HUD 레이어 (필드/전투 HUD)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(UI_Layer_GameMenu, "UI.Layer.GameMenu", "게임 내 메뉴 레이어 (인벤토리, 장비, 스킬)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(UI_Layer_Menu, "UI.Layer.Menu", "시스템 메뉴 레이어 (일시정지, 옵션)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(UI_Layer_Modal, "UI.Layer.Modal", "모달 레이어 (확인 팝업, 게임오버)");
	
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Basic_Attack, "Character.Ability.BasicAttack", "캐릭터 기본 공격");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Fire, "Character.Ability.Fire", "캐릭터 레이 발사");
}