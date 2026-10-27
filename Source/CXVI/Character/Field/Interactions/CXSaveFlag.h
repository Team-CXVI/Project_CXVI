// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CXSaveFlag.generated.h"

class ACXFieldPlayerCharacter;
UCLASS()
class CXVI_API ACXSaveFlag : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ACXSaveFlag();

public:
	// 이 플레이어에게 휴식을 제공할 수 있는지 물어보는 함수.
	bool CanRest(const ACXFieldPlayerCharacter* Player)const;

	void CompleteRest();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

protected:

	// 세이브 깃발 메시.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SaveFlag")
	TObjectPtr<UStaticMeshComponent> ExpeditionFlag;

	// 상호작용 가능하게 할 트리거 박스.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SaveFlag")
	TObjectPtr<class UChildActorComponent> SaveFlagTrigger;
	
};
