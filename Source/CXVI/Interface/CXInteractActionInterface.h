// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include <StructUtils/InstancedStruct.h>

#include "CXInteractActionInterface.generated.h"

class AActor;

UENUM(BlueprintType)
enum class EInteractActionType : uint8
{
	None,
	GrapplingHook,
	RopeTraversal
};

USTRUCT(BlueprintType)
struct CXVI_API FCXInteractActionParam
{
	GENERATED_BODY()

public:
	template <typename T>
	const T* GetPayload() const
	{
		return Payload.GetPtr<T>();
	}

	template <typename T>
	static FCXInteractActionParam Make(const T& InPayload)
	{
		FCXInteractActionParam Param;
		Param.Payload = FInstancedStruct::Make(InPayload);
		return Param;
	}

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interact")
	FInstancedStruct Payload;
};

USTRUCT(BlueprintType)
struct CXVI_API FCXGrapplingHookActionParams
{
	GENERATED_BODY()

public:
	UPROPERTY()
	TWeakObjectPtr<AActor> GrapplingTarget;

	UPROPERTY()
	TWeakObjectPtr<AActor> LandingTarget;
};



// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UCXInteractActionInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class CXVI_API ICXInteractActionInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "InteractAction")
	bool CanInteract() const;
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "InteractAction")
	void  StartInteractAction(EInteractActionType InActionType, const FCXInteractActionParam& InActionParam);

};
