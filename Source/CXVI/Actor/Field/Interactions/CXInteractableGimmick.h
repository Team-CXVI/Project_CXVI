// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interface/CXInteractableInterface.h"

#include "CXInteractableGimmick.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogCXGimmic, Log, All);

class UBoxComponent;
class UWidgetComponent;

UCLASS(Abstract)
class CXVI_API ACXInteractableGimmick : public AActor, public ICXInteractableInterface
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ACXInteractableGimmick();

	virtual bool CanInteract_Implementation(AActor* Interactor) const override;
	virtual void Interact_Implementation(AActor* Interactor) override;
	virtual void SetInteractionPromptVisible_Implementation(bool bVisible) override;

	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void SetInteractionEnabled(bool bEnabled) { bInteractionEnabled = bEnabled; }

protected:
	/** BP용 구현 동작 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Interaction")
	void K2_OnInteracted(AActor* Interactor);

protected:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USceneComponent> Root;

	/** 송신기에 감지되기 위한 트리거. Object Type = Interactable */
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UBoxComponent> InteractionTrigger;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UWidgetComponent> PromptWidget;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	bool bInteractionEnabled = true;

	/** true면 상호작용자가 이 기믹 쪽을 보고 있을 때만 가능 (뒤돌아 있으면 불가) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Facing")
	bool bRequireInteractorFacing = true;

	/** 상호작용자 정면 방향 기준 허용 반각(도). 90이면 앞쪽 반평면, 작을수록 정면에 가까워야 함 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|Facing", meta = (ClampMin = "0", ClampMax = "180", EditCondition = "bRequireInteractorFacing"))
	float InteractorFacingHalfAngle = 90.f;
	
};
