// =========================================================================
// MyHits.cc
// =========================================================================

// 1. Include Local headers
#include "MyHits.hh"

// 2. Include Geant4 headers
#include "G4VVisManager.hh"
#include "G4Circle.hh"
#include "G4Colour.hh"
#include "G4VisAttributes.hh"
#include "G4ios.hh"

// ====================================================================
// DRAW METHOD IMPLEMENTATION
// ====================================================================
void MyHit::Draw()
{
    G4VVisManager* pVVisManager = G4VVisManager::GetConcreteInstance();
    if (pVVisManager)
    {
        // Nếu muốn, vẽ điểm hit, hiện tại để trống
    }
}

// ====================================================================
// PRINT METHOD IMPLEMENTATION
// ====================================================================
// NEW: fEdep đã bị xóa khỏi MyHit (bỏ logic pulse/edep cũ).
// Print() giờ in track-length + kinetic energy, phục vụ debug track-length tally.
// ====================================================================
void MyHit::Print()
{
    G4cout
        << "  --- Hit ---"
        << "  Volume: " << fVolumeName
        << "  Particle: " << fParticleName
        << "  CrossingFlux: " << fCrossingFlux
        << "  Ekin: " << G4BestUnit(fKineticEnergy, "Energy")
        << "  Weight (VR): " << fWeight
        << G4endl;
}
