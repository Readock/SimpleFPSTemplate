// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "FPSCharacter.h"
#include "FPSProjectile.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Animation/AnimSequence.h"


AFPSCharacter::AFPSCharacter() {
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(55.f, 89.f);

	// Create a CameraComponent	
	FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCameraComponent->SetupAttachment(GetCapsuleComponent());
	FirstPersonCameraComponent->SetRelativeLocation(FVector(0.f, 0.f, BaseEyeHeight)); // Position the camera
	FirstPersonCameraComponent->SetRelativeScale3D(FVector(0.4, 0.4, 0.4)); // Scale of the camera
	FirstPersonCameraComponent->bUsePawnControlRotation = true;

	// Create a mesh component that will be used when being viewed from a '1st person' view (when controlling this pawn)
	Mesh1P = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("CharacterMesh1P"));
	Mesh1P->SetOnlyOwnerSee(true);
	Mesh1P->SetupAttachment(FirstPersonCameraComponent);
	Mesh1P->bCastDynamicShadow = false;
	Mesh1P->CastShadow = false;
	Mesh1P->SetRelativeRotation(FRotator(-.5, 6.6, -93.));
	Mesh1P->SetRelativeLocation(FVector(-6.5, -2.1, -125.9f));

	// Create the '3rd person' body mesh
	Mesh3P = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("CharacterMesh3P"));
	Mesh3P->SetupAttachment(GetCapsuleComponent());
	Mesh3P->SetOwnerNoSee(true);
	Mesh3P->SetRelativeLocation(FVector(3.8f, -2.93f, -89.f));
	Mesh3P->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));

	CrouchingTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("CrouchingTimeline"));

	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
	GetCharacterMovement()->NavAgentProps.bCanCrouch = true;

	// Create a gun mesh component
	GunMesh1P = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("GunMesh1P"));
	GunMesh1P->CastShadow = false;
	GunMesh1P->SetOnlyOwnerSee(true);
	GunMesh1P->SetupAttachment(Mesh1P, "GripPoint");

	// Create a gun mesh component
	GunMesh3P = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("GunMesh3P"));
	GunMesh3P->CastShadow = true;
	GunMesh3P->SetOwnerNoSee(true);
	GunMesh3P->SetupAttachment(Mesh3P, "GripPoint");
}

void AFPSCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) {
	// set up gameplay key bindings
	check(PlayerInputComponent);

	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &AFPSCharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);

	PlayerInputComponent->BindAction("Crouch", IE_Pressed, this, &AFPSCharacter::ToggleCrouching);

	PlayerInputComponent->BindAction("Sprint", IE_Pressed, this, &AFPSCharacter::EnableSprint);
	PlayerInputComponent->BindAction("Sprint", IE_Released, this, &AFPSCharacter::DisableSprint);

	PlayerInputComponent->BindAction("Fire", IE_Pressed, this, &AFPSCharacter::Fire);

	PlayerInputComponent->BindAxis("MoveForward", this, &AFPSCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AFPSCharacter::MoveRight);

	PlayerInputComponent->BindAxis("Turn", this, &AFPSCharacter::TurnAtRate);
	PlayerInputComponent->BindAxis("LookUp", this, &AFPSCharacter::LookUpAtRate);
}

void AFPSCharacter::BeginPlay() {
	// Call the base class  
	Super::BeginPlay();

	// Show or hide the two versions of the gun based on whether or not we're using motion controllers.
	Mesh1P->SetHiddenInGame(false, true);

	AssertNotNull(CrouchingCurve, GetWorld(), __FILE__, __LINE__, "No crouching curve assigned");

	FOnTimelineFloat callback{};
	callback.BindUFunction(this, FName{ TEXT("CrouchingCallback") });
	CrouchingTimeline->AddInterpFloat(CrouchingCurve, callback, FName{ TEXT("CrouchingTimelineAnimation") });

	CamMiddle = FirstPersonCameraComponent->GetRelativeLocation();
	CrouchHeightDiff = GetCapsuleComponent()->GetScaledCapsuleHalfHeight() - GetCharacterMovement()->CrouchedHalfHeight;
	CamTop = FVector(CamMiddle.X, CamMiddle.Y, CamMiddle.Z + CrouchHeightDiff);
	CamBottom = FVector(CamMiddle.X, CamMiddle.Y, CamMiddle.Z - CrouchHeightDiff);
}


void AFPSCharacter::Tick(float DeltaTime) {
	Super::Tick(DeltaTime);

	if (wantsToCrouch && GetCharacterMovement()->IsMovingOnGround()) {
		wantsToCrouch = false;
		Crouch();
	}
}


void AFPSCharacter::Fire() {
	// try and fire a projectile
	if (ProjectileClass) {
		// Grabs location from the mesh that must have a socket called "Muzzle" in his skeleton
		FVector MuzzleLocation = GunMesh1P->GetSocketLocation("Muzzle");
		// Use controller rotation which is our view direction in first person
		FRotator MuzzleRotation = GetControlRotation();

		//Set Spawn Collision Handling Override
		FActorSpawnParameters ActorSpawnParams;
		ActorSpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding;

		// spawn the projectile at the muzzle
		GetWorld()->SpawnActor<AFPSProjectile>(ProjectileClass, MuzzleLocation, MuzzleRotation, ActorSpawnParams);
	}

	// try and play the sound if specified
	if (FireSound) {
		UGameplayStatics::PlaySoundAtLocation(this, FireSound, GetActorLocation());
	}

	// try and play a firing animation if specified
	if (FireAnimation) {
		// Get the animation object for the arms mesh
		UAnimInstance* AnimInstance = Mesh1P->GetAnimInstance();
		if (AnimInstance) {
			AnimInstance->PlaySlotAnimationAsDynamicMontage(FireAnimation, "Arms", 0.0f);
		}
	}
}

void AFPSCharacter::MoveForward(float value) {
	if (value != 0.0f) {
		AddMovementInput(GetActorForwardVector(), value);
	}

	if (ShouldStartSprinting(value)) {
		StartSprinting();
		//ServerStartSprinting();
	}
	else if (ShouldStopSprinting(value)) {
		StopSprinting();
		//ServerStopSprinting();
	}
}

void AFPSCharacter::MoveRight(float value) {
	if (value != 0.0f) {
		AddMovementInput(GetActorRightVector(), value);
	}
}

void AFPSCharacter::TurnAtRate(float rate) {
	// calculate delta for this frame from the rate information
	AddControllerYawInput(rate);
}

void AFPSCharacter::LookUpAtRate(float rate) {
	// calculate delta for this frame from the rate information
	AddControllerPitchInput(rate);
	//ServerUpdateCameraPitch(FirstPersonCameraComponent->GetComponentRotation().Pitch);
	//MulticastUpdateCameraPitch(FirstPersonCameraComponent->GetComponentRotation().Pitch);
}

void AFPSCharacter::EnableSprint() {
	sprintEnabled = true;
	if (bIsCrouched) {
		UnCrouch();
	}
}

void AFPSCharacter::DisableSprint() {
	sprintEnabled = false;
}

bool AFPSCharacter::ShouldStartSprinting(float ForwardMovement) {
	return sprintAllowed && sprintEnabled && ForwardMovement > 0 && GetVelocity().Size() <= WalkSpeed;
}

bool AFPSCharacter::ShouldStopSprinting(float ForwardMovement) {
	if (GetVelocity().Size() <= WalkSpeed) {
		return false;
	}
	return !sprintAllowed || !sprintEnabled || ForwardMovement <= 0;
}

void AFPSCharacter::StartSprinting() {
	GetCharacterMovement()->MaxWalkSpeed = SprintSpeed;
}

void AFPSCharacter::StopSprinting() {
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
}

void AFPSCharacter::Jump() {
	if (bIsCrouched) {
		UnCrouch();
	}
	else {
		Super::Jump();
	}
}

void AFPSCharacter::ToggleCrouching() {
	if (!bIsCrouched) {
		wantsToCrouch = true;
	}
	else {
		UnCrouch();
	}
}

void AFPSCharacter::OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) {
	FVector loc = Mesh3P->GetRelativeLocation();
	Mesh3P->SetRelativeLocation(FVector(loc.X, loc.Y, loc.Z + CrouchHeightDiff - 7));
	CamStart = CamTop;
	CamFinish = CamMiddle;
	CrouchingTimeline->Play();
}

void AFPSCharacter::OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) {
	FVector loc = Mesh3P->GetRelativeLocation();
	Mesh3P->SetRelativeLocation(FVector(loc.X, loc.Y, loc.Z - CrouchHeightDiff + 7));
	CamStart = CamMiddle;
	CamFinish = CamBottom;
	CrouchingTimeline->Reverse();
}

void AFPSCharacter::CrouchingCallback(float value) {
	FirstPersonCameraComponent->SetRelativeLocation(FMath::Lerp(CamFinish, CamStart, value));
}

