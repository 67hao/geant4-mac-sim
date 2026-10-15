// ============================================================================
// MyActionInitialization.hh
// Definition of the MyActionInitialization class
//
// This class initializes all user-defined actions for the Geant4 run.
// It ensures that RunAction, EventAction, PrimaryGeneratorAction, etc.,
// are registered correctly for both sequential and multi-threaded runs.
// ============================================================================

#ifndef MYACTIONINITIALIZATION_HH
#define MYACTIONINITIALIZATION_HH

// ============================================================================
// Geant4 Includes
// ============================================================================
#include "G4VUserActionInitialization.hh"

// ============================================================================
// MyActionInitialization
// ============================================================================
// Inherits from G4VUserActionInitialization.
// Provides implementations of Build() and BuildForMaster().
// - Build(): called for worker threads or in sequential mode.
// - BuildForMaster(): called for the master thread in MT mode.
// ============================================================================
class MyActionInitialization : public G4VUserActionInitialization {
public:
	// ========================================================================
	// Constructor / Destructor
	// ========================================================================
	MyActionInitialization();
	~MyActionInitialization() override;

	// ========================================================================
	// Build(): Defines actions for sequential mode or worker threads
	// ========================================================================
	void Build() const override;

	// ========================================================================
	// BuildForMaster(): Defines actions for master thread in MT mode
	// ========================================================================
	void BuildForMaster() const override;

};

#endif // MYACTIONINITIALIZATION_HH
