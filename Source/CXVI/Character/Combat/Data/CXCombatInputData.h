// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "InputTriggers.h"
#include "CXCombatInputData.generated.h"

UENUM()
enum class ECXInputCommand : uint8
{
    OpenItemMenu,
    OpenSkillMenu,
    BasicAttack,
    Skill1,
    Skill2,
    Skill3,
    PreviousTarget,
    NextTarget,
    Confirm,
    Cancel,
    ToggleAim,
    Look,
    Fire
};

UENUM()
enum class ECXInputContext : uint8
{
    Unassigned,
    Common,
    ActionMenu,
    ItemMenu,
    SkillMenu,
    TargetSelect,
    Reaction
};



//USTRUCT()
//struct FInputData
//{
//	GENERATED_BODY()
//
//	UPROPERTY(EditAnywhere, Category = "Input")
//    ECXInputContext Context = ECXInputContext::Unassigned;
//
//	UPROPERTY(EditAnywhere, Category = "IMC")
//    TObjectPtr<class UInputMappingContext> IMC = nullptr;
//
//	UPROPERTY(EditAnywhere, Category = "IA")
//	TArray<FCXInputActionData> Actions;
//};

/**
 * 
 */
UCLASS()
class CXVI_API UCXCombatInputData : public UPrimaryDataAsset
{
	GENERATED_BODY()


public:
    /*const TArray<FInputData>& GetInputData() const { return InputData; }

	UPROPERTY(EditAnywhere, Category = "Input")
	TArray<FInputData> InputData;*/

};
