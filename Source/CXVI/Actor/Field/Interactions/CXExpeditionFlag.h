// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CXExpeditionFlag.generated.h"

class ACXFieldPlayerCharacter;
UCLASS()
class CXVI_API ACXExpeditionFlag : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ACXExpeditionFlag();

public:
	// 이 플레이어에게 플래그를 사용할 수 있는지 물어보는 함수.
	bool CanFlag(const ACXFieldPlayerCharacter* Player)const;

	void CompleteFlag();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

protected:

	// 원정대 깃발 메시.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ExpeditionFlag")
	TObjectPtr<UStaticMeshComponent> ExpeditionFlag;

	// 상호작용 가능하게 할 트리거 박스.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ExpeditionFlag")
	TObjectPtr<class UChildActorComponent> ExpeditionFlagTrigger;
	
};
