// ============================================================================
// MySensitiveDetector.hh
// Definition of the MySensitiveDetector class
//
// This class implements a sensitive detector in Geant4. It is responsible
// for recording hits (energy deposition, particle info, transmission)
// whenever a step occurs inside volumes assigned to this detector.
//
// Responsibilities:
// - Create a new hits collection at the beginning of each event.
// - Record hits during each step, depending on the volume type
//   (e.g., Paraffin, AirLayer, PreAirLayer, PostAirLayer).
// - Store energy deposition and particle crossing information
//   for later processing in EventAction and RunAction.
// ============================================================================

#ifndef MYSENSITIVEDETECTOR_h
#define MYSENSITIVEDETECTOR_h

// ============================================================================
// Geant4 Includes
// ============================================================================
#include "G4VSensitiveDetector.hh"
#include "G4THitsCollection.hh"

// ============================================================================
// Local Includes
// ============================================================================
#include "MyHits.hh"

// ============================================================================
// MySensitiveDetector
// ============================================================================
class MySensitiveDetector : public G4VSensitiveDetector {
public:
	// Constructor & Destructor
	MySensitiveDetector(const G4String& name, const G4String& collectionName);
	~MySensitiveDetector() override;

	// ========================================================================
	// Geant4 Interface Methods
	// ========================================================================

	// Called at the beginning of each event
	// → Creates and registers a new hits collection
	void Initialize(G4HCofThisEvent* hce) override;

	// Called for every step inside the sensitive volume
	// → Records energy deposition and/or boundary crossing
	G4bool ProcessHits(G4Step* aStep, G4TouchableHistory*) override;

private:
	// Hits collection pointer (reset every event in Initialize)
	MyHitsCollection* fHitsCollection;
};

#endif // MySensitiveDetector_h
