// ============================================================================
// MyHits.hh
// Definition of the MyHit class and its collection
//
// MyHit stores information about a single hit recorded in a sensitive
// detector: surface-crossing flux (T-Cross-like tally at DetPlane), kinetic
// energy (for the uncollided energy-window cut), and identification info.
//
// NEW: pulse/energy-deposit counting logic has been REMOVED entirely
// (fEdep, SetEdep/GetEdep), and the track-length tally logic (fTrackLength,
// SetTrackLength/GetTrackLength) has also been REMOVED entirely — the
// analysis now relies only on the surface-crossing flux tally.
// ============================================================================

#ifndef MYHITS_HH
#define MYHITS_HH

// ============================================================================
// Geant4 Includes
// ============================================================================
#include "G4VHit.hh"
#include "G4THitsCollection.hh"
#include "G4UnitsTable.hh"
#include "G4SystemOfUnits.hh"
#include "G4ThreeVector.hh"

// C++ Includes
#include <string>

// ============================================================================
// MyHit
// ============================================================================
class MyHit : public G4VHit {
public:
	// ========================================================================
	// Constructor / Destructor
	// ========================================================================
	MyHit() : fWeight(0.), fTrackID(-1),
	          fKineticEnergy(0.),
	          fCrossingFlux(0.), fCosTheta(1.) {}
	~MyHit() override = default;

	// ========================================================================
	// Setters
	// ========================================================================
	void SetWeight(G4double w) { fWeight = w; }
	void SetTrackID(G4int id) { fTrackID = id; }
	void SetParticleName(const G4String& name) { fParticleName = name; }
	void SetVolumeName(const G4String& name) { fVolumeName = name; }

	void SetKineticEnergy(G4double e) { fKineticEnergy = e; }    ///< pre-step kinetic energy, used for uncollided energy-window cut

	// NEW: surface-crossing (T-Cross-like) tally quantities, recorded when
	// a track enters the thin "DetPlane" volume placed in front of the NaI.
	void SetCrossingFlux(G4double f) { fCrossingFlux = f; }       ///< weight / |cosTheta| contribution at the crossing
	void SetCosTheta(G4double c) { fCosTheta = c; }                ///< |cos(angle to surface normal)| at the crossing

	// ========================================================================
	// Getters
	// ========================================================================
	G4double GetWeight() const { return fWeight; }
	G4int GetTrackID() const { return fTrackID; }
	G4String GetParticleName() const { return fParticleName; }
	G4String GetVolumeName() const { return fVolumeName; }

	G4double GetKineticEnergy() const { return fKineticEnergy; }

	G4double GetCrossingFlux() const { return fCrossingFlux; }
	G4double GetCosTheta() const { return fCosTheta; }

	// ========================================================================
	// Geant4 interface
	// ========================================================================
	void Draw() override;   ///< Visualization hook (declared only)
	void Print() override;  ///< Debug print (declared only)

private:
	// ========================================================================
	// Data Members
	// ========================================================================
	G4double fWeight;        ///< Statistical weight of the hit
	G4int fTrackID;          ///< Track ID associated with this hit
	G4String fParticleName;  ///< Name of the particle
	G4String fVolumeName;    ///< Logical volume name where hit occurred

	G4double fKineticEnergy;  ///< Pre-step kinetic energy (for uncollided energy-window filtering)

	G4double fCrossingFlux;   ///< weight/|cosTheta| contribution, for T-Cross-like surface flux tally
	G4double fCosTheta;       ///< |cos(angle to surface normal)| at the crossing (debug/QA)
};

// ============================================================================
// Typedef for hits collection
// ============================================================================
typedef G4THitsCollection<MyHit> MyHitsCollection;

#endif // MYHITS_HH
