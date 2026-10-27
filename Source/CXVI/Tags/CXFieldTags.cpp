#include "CXFieldTags.h"

namespace CXVIFieldTag
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_MovingBlock, "State.MovingBlock", "플레이어 이동 제한 상태");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_CanGrapplingHook, "State.CanGrapplingHook","플레이어 그래플링 가능 상태");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_CanAssassination, "State.CanAssassination","플레이어 몬스터 기습 공격 가능 상태");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_CanUseExpeditionFlag, "State.CanUseExpeditionFlag","플레이어 원정대 깃발 사용 가능 상태");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Input_Field_Sprint, "Input.Field.Sprint","플레이어 스프린트 입력");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Input_Field_Attack, "Input.Field.Attack","플레이어 공격");


	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Field_EnterSlowMotion, "Event.Field.EnterSlowMotion","플레이어 및 몬스터의 공격 성공 시 슬로우모션 진입 이벤트");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Field_ShowCable, "Event.Field.ShowCable","그래플링 성공 시 케이블이 보이는 이벤트");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Field_StartGrapplingHook, "Event.Field.StartMove","그래플링 성공 시 플레이어 비행 시작을 알리는 이벤트");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Field_AttackHit, "Event.Field.AttackHit","필드 공격의 타격 판정 시점");
}
