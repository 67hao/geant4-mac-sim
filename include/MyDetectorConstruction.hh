// ============================================================================
// MyDetectorConstruction.hh
// Definition of the MyDetectorConstruction class
// Paper 10.1 (PNE 2022) Benchmark Setup
// ============================================================================

#ifndef MyDetectorConstruction_h
#define MyDetectorConstruction_h

#include "G4VUserDetectorConstruction.hh"
#include "G4LogicalVolume.hh"
#include "G4VPhysicalVolume.hh"
#include "G4GenericMessenger.hh"
#include "G4SystemOfUnits.hh"
#include "G4String.hh"

class MyDetectorConstruction : public G4VUserDetectorConstruction {
public:
	MyDetectorConstruction();
	~MyDetectorConstruction() override;

	G4VPhysicalVolume* Construct() override;
	void DefineMaterials();
	void ConstructSDandField() override;

	G4double GetSampleDetectorDistance() const { return fSample_Thickness; }
	G4String GetSampleMaterialName() const;
	G4double GetSampleThickness() const;
	G4double GetCollimator2Height() const { return fCollimator2_Height; }

private:
	void UpdateGeometry();

	G4double fSample_Thickness;
	G4String fSampleMaterial;
	G4double fCollimator2_Height;

	G4VPhysicalVolume* World_Phys = nullptr;
	G4GenericMessenger* fMessenger = nullptr;
};

#endif // MyDetectorConstruction_h
