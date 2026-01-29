// Fill out your copyright notice in the Description page of Project Settings.


#include "GrappleSwing.h"

// Sets default values
AGrappleSwing::AGrappleSwing()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AGrappleSwing::BeginPlay()
{
	Super::BeginPlay();

	ACharacter* playerCharacterRef = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);

	if (playerCharacterRef)
	{
		playerCharacter = playerCharacterRef;
	}

}

// Called every frame
void AGrappleSwing::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (swinging && IsValid(playerCharacter))
	{
		grapplePhysics();
	}

}

//////////////////////////
// FUNCTION DEFINITIONS //
//////////////////////////

void AGrappleSwing::grapplePhysics()
{

	playerVelocity = UKismetMathLibrary::VSize(playerCharacter->GetVelocity());
	playerDistance = UKismetMathLibrary::Vector_Distance(playerCharacter->GetActorLocation(), GetActorLocation());
	forwardVector = UKismetMathLibrary::GetForwardVector(UKismetMathLibrary::FindLookAtRotation(GetActorLocation(), playerCharacter->GetActorLocation()));

	float const tensionAngleDivisor = 1000;
	float const centrifugalVelocityDivisor = 2000;


	// Preliminary tension calculations //


	// Base force that pulls the player toward the grapple anchor
	FVector baseTension = (forwardVector * baseTensionStrength);
	// Should rope pull player back? (0 if player is above rope length, 1 otherwise)
	float slack = (float(playerDistance > ropeLength));
	// Strengthen tension as the player pushes past base rope length
	float tensionStrength = (playerDistance / ropeLength * ((playerDistance + ropeLength) / ropeSlack));
	// Strengthen tension the closer player velocity is to the direction the rope is pointed
	float tensionAngle = float((FVector::DotProduct(playerCharacter->GetVelocity(), forwardVector) / tensionAngleDivisor));



	// Calculate finalized tension
	FVector tensionForce = baseTension * tensionStrength * slack * FMath::Lerp(0.75f, 8.0f, (FMath::Clamp((tensionAngle), 0.0f, 999.0f)));
	// Calculate centrifugal force
	FVector centrifugalForce = tensionForce * -0.9 * (FMath::Clamp((playerVelocity / centrifugalVelocityDivisor), 0.0f, 1.0f));
	// Add them together
	grappleForces = (tensionForce + centrifugalForce);


	// Apply forces to character movement
	playerCharacter->GetCharacterMovement()->AddForce(grappleForces);
}

void AGrappleSwing::tryReel()
{
	float const reelSpeed = 300.0f;

	if (ropeLength > 200 && swinging)
	{
		ropeLength = (ropeLength - (GetWorld()->GetDeltaSeconds() * reelSpeed));
	}
}