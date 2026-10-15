// ============================================================================
// MyPrimaryGeneratorAction.cc
// Implementation of the MyPrimaryGeneratorAction class
//
// This class defines the primary particle source for the simulation.
// Responsibilities:
// - Configure the particle gun (type, position, energy).
// - Generate primary events with isotropic emission.
// - Provide getter functions for RunAction or analysis.
// ============================================================================

// ============================================================================
// Include headers
// ============================================================================
#include "MyPrimaryGeneratorAction.hh"

// Geant4 headers
#include <G4ParticleGun.hh>
#include <G4ParticleTable.hh>
#include <G4Gamma.hh>
#include <G4SystemOfUnits.hh>
#include <Randomize.hh>

// C++ headers
#include <cmath>

// ============================================================================
// Constructor
// ============================================================================
//
// - Creates a particle gun with one particle per event.
// - Sets the particle type (neutron), initial position, and energy.
//
MyPrimaryGeneratorAction::MyPrimaryGeneratorAction(const MyDetectorConstruction* detector)
    : G4VUserPrimaryGeneratorAction(),
    fParticleGun(nullptr),
    fDetector(detector)
{
    G4int n_particle = 1;
    fParticleGun = new G4ParticleGun(n_particle);

    // Particle definition
    auto particleTable = G4ParticleTable::GetParticleTable();
    auto particle = particleTable->FindParticle("gamma");

    // Set particle type, source position, and initial energy
    fParticleGun->SetParticleDefinition(particle);
    fParticleGun->SetParticleEnergy(662 * keV);
    fParticleGun->SetParticlePosition(G4ThreeVector(0, 0, 0));
    fParticleGun->SetParticleMomentumDirection(G4ThreeVector(0, 0, 1));

}

// ============================================================================
// Destructor
// ============================================================================
MyPrimaryGeneratorAction::~MyPrimaryGeneratorAction() {
    delete fParticleGun;
}

// ============================================================================
// GeneratePrimaries
// ============================================================================
//
// - Called at the beginning of each event to define the primary particle.
// - Current setup generates neutrons isotropically with fixed energy.
// - Energy binning code is provided but commented out for future use.
//
void MyPrimaryGeneratorAction::GeneratePrimaries(G4Event* anEvent) {    
    // Generate the primary vertex for this event
    fParticleGun->GeneratePrimaryVertex(anEvent);

}

// ============================================================================
// Getter Methods
// ============================================================================
//
// - Provide access to particle name and energy.
// - Useful for RunAction or output to analysis.
// ============================================================================
G4String MyPrimaryGeneratorAction::GetParticleName() const {
    return fParticleGun->GetParticleDefinition()->GetParticleName();
}

G4double MyPrimaryGeneratorAction::GetParticleEnergy() const {
    return fParticleGun->GetParticleEnergy();
}
