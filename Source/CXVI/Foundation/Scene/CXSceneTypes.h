#pragma once

#include "CoreMinimal.h"
#include <StructUtils/InstancedStruct.h>

#include "CXSceneTypes.generated.h"

class UInputMappingContext;

/**
 * Scene 간 전달 데이터
 * Push: 호출자 → 새 Scene의 OnEnter
 * Pop: 나가는 씬 → 아래 Scene의 OnResume
 * Scene 마다 필요한 데이터가 달라서 Payload에 Scene 별 USTRUCT를 넣음
 */
USTRUCT(BlueprintType)
struct CXVI_API FCXSceneContext
{
	GENERATED_BODY()

public:
	template <typename T>
	const T* GetPayload() const
	{
		return Payload.GetPtr<T>();
	}

	template <typename T>
	static FCXSceneContext Make(const T& InPayload)
	{
		FCXSceneContext Context;
		Context.Payload = FInstancedStruct::Make(InPayload);
		return Context;
	}

public:
	// FInstanceStruct -> 다양한 USTRUCT를 하나의 구조체에 담을 수 있는 컨테이너
	// 내부적으로 UScriptStruct*를 가짐
	// 타입을 제한하고 싶으면 meta = (BaseStruct = "Script/CXVI.BaseStructName") 메타 지정자를 추가
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scene")
	FInstancedStruct Payload;
};

/** Scene 활성 중에 적용할 입력 매핑*/
USTRUCT(BlueprintType)
struct CXVI_API FCXSceneInputMapping
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Input")
	TSoftObjectPtr<UInputMappingContext> IMC;

	UPROPERTY(EditAnywhere, Category = "Input")
	int32 Priority = 0;
};

/** Scene이 Exit(Pop)될 때 서브레벨 처리 방식 */
UENUM(BlueprintType)
enum class ECXSceneExitPolicy : uint8
{
	// 다른 Scene이 참조하지 않으면 언로드. 가끔 쓰는 큰 레벨
	Unload,
	// Hidden 상태로 메모리에 유지. 자주 쓰는 레벨
	KeepLoaded
};

// Message.Screen.Fade 태그로 전달하는 메시지 구조체
// 카메라 페이드는 3D 화면에만 적용되므로 UI가 같은 값으로 따라가기 위해 사용
USTRUCT(BlueprintType)
struct CXVI_API FCXScreenFadeMessage
{
	GENERATED_BODY()

public:
	// 0 = 화면이 보이는 상태, 1 = 완전히 가려진 상태
	UPROPERTY(BlueprintReadOnly, Category = "Scene")
	float FromAlpha = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Scene")
	float ToAlpha = 0.f;

	// 0이면 ToAlpha를 바로 적용
	UPROPERTY(BlueprintReadOnly, Category = "Scene")
	float Duration = 0.f;
};