// ============================================================================
// MyPrimaryGeneratorAction.hh
// Definition of the MyPrimaryGeneratorAction class
//
// This class defines the primary particle source of the simulation.
// It uses G4ParticleGun to shoot particles with user-defined
// properties (particle type, energy, direction, position, etc.).
//
// Responsibilities:
// - Configure the particle gun in the constructor.
// - Generate primaries at the start of each event.
// - Provide getters for particle type and energy (used by RunAction).
// ============================================================================

#ifndef MYPRIMARYGENERATORACTION_HH
#define MYPRIMARYGENERATORACTION_HH

// ============================================================================
// Geant4 Includes
// ============================================================================
#include "MyDetectorConstruction.hh"
#include <G4VUserPrimaryGeneratorAction.hh>
#include <G4ParticleGun.hh>
#include <globals.hh>

// Forward Declarations
class G4Event;

// ============================================================================
// MyPrimaryGeneratorAction
// ============================================================================
class MyPrimaryGeneratorAction : public G4VUserPrimaryGeneratorAction {
public:
	// Constructor / Destructor
	MyPrimaryGeneratorAction(const MyDetectorConstruction* detector);
	virtual ~MyPrimaryGeneratorAction();

	// ========================================================================
	// Main Method
	// ========================================================================
	// Called by Geant4 at the beginning of each event to generate
	// the primary vertex and particle(s).
	void GeneratePrimaries(G4Event* anEvent) override;

	// ========================================================================
	// Getter Methods (used by RunAction)
	// ========================================================================
	G4String GetParticleName() const;
	G4double GetParticleEnergy() const;

private:
	// Particle gun to shoot primaries into the simulation
	G4ParticleGun* fParticleGun;
	const MyDetectorConstruction* fDetector; // Biến lưu trữ con trỏ
};

#endif // MYPRIMARYGENERATORACTION_HH
