// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CXVictoryStage.generated.h"

class UCameraComponent;
class ACharacter;

/**
 * 보스 처치 후 승리 춤 무대.
 * - 레벨에 하나 배치하고 카메라 위치를 잡는다. 춤출 캐릭터 3개(파티원 2 + 보스)는 레벨에 따로 배치해 지정한다.
 * - 춤 애니는 각 댄서 BP의 메시에서 지정한다. 평소에는 댄서를 숨기고 충돌을 꺼 둔다.
 * - Play()가 불리면 댄서를 보이고, 필드 플레이어를 숨기고 카메라를 무대 카메라로 넘긴다.
 */
UCLASS()
class CXVI_API ACXVictoryStage : public AActor
{
	GENERATED_BODY()

public:
	ACXVictoryStage();

	UFUNCTION(BlueprintCallable, Category = "CXVI|Field|Victory")
	void Play();

protected:
	virtual void BeginPlay() override;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Victory")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Victory")
	TObjectPtr<UCameraComponent> StageCamera;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Victory")
	TObjectPtr<ACharacter> DancerA;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Victory")
	TObjectPtr<ACharacter> DancerB;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Victory")
	TObjectPtr<ACharacter> BossDancer;

	// 필드 카메라에서 무대 카메라로 넘어가는 시간(초)
	UPROPERTY(EditAnywhere, Category = "Victory", meta = (ClampMin = "0.0"))
	float CameraBlendTime = 1.0f;

private:
	bool bPlaying = false;
};
