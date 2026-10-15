// ============================================================================
// MyPhysicsList.hh
// Definition of the MyPhysicsList class
//
// This class defines the physics processes to be used in the simulation.
// It inherits from G4VModularPhysicsList and is responsible for
// registering electromagnetic, hadronic, decay, and other physics
// constructors as needed.
// ============================================================================

#ifndef MYPHYSICSLIST_HH
#define MYPHYSICSLIST_HH

// ============================================================================
// Geant4 Includes
// ============================================================================
#include <G4VModularPhysicsList.hh>

// ============================================================================
// MyPhysicsList
// ============================================================================
// Inherits from G4VModularPhysicsList
//
// Responsibilities:
// - Define and register the physics processes used in the simulation.
// - Control cut values, step limits, and global physics parameters.
// ============================================================================
class MyPhysicsList : public G4VModularPhysicsList {
public:
	// ========================================================================
	// Constructor / Destructor
	// ========================================================================
	MyPhysicsList();
	~MyPhysicsList() override;
};

#endif // MYPHYSICSLIST_HH
