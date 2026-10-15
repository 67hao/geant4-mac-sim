// =========================================================================
// Header Definition
// =========================================================================

// 1. Include Local headers
#include "MyDetectorConstruction.hh" // Geometry definition class
#include "MySensitiveDetector.hh"    // Custom Sensitive Detector

// 2. Include Geant4 headers
#include "G4NistManager.hh"          // Material database
#include "G4Box.hh"                  // Box geometry
#include "G4Tubs.hh"                 // Cylinder geometry
#include "G4LogicalVolume.hh"        // Logical volume
#include "G4VPhysicalVolume.hh"
#include "G4PVPlacement.hh"          // Physical volume placement
#include "G4VisAttributes.hh"        // Visualization attributes
#include "G4Colour.hh"               // Colors
#include "G4Region.hh"               // Regions
#include "G4SDManager.hh"            // Sensitive detector manager
#include "G4SystemOfUnits.hh"        // Units
#include "G4RegionStore.hh"
#include "G4LogicalVolumeStore.hh"
#include "G4RunManager.hh"           // Run manager
#include "G4ProductionCuts.hh"
#include <map>

// =========================================================================
// Geometry Dimensions (Paper 10.1 - PNE 2022 Benchmark, Fig. 1 & Section 2.2)
// Setup:
//   - Point Source @ z = 0 cm (emission along +z axis)
//   - Pb Collimator @ z = [10, 50] cm (L = 40 cm, r_in = 0.5 cm, r_out = 3.0 cm)
//   - Glass Sample @ z = [50.0, 50.0 + thickness] cm (r = 5.0 cm)
//   - Detection Field (F4 equivalent) @ z = [70.0, 72.0] cm (r = 5.0 cm)
//   - DetPlane (Surface Crossing Tally, F2 / [T-Cross] equivalent) @ z = 70.0 cm (r = 5.0 cm)
//   - Outer Pb Shield @ z = [45.0, 75.0] cm (L = 30 cm, r_in = 8.0 cm, r_out = 10.0 cm)
// =========================================================================
namespace GEO {
	// 1. World Volume (Vacuum)
	const G4double WorldSize = 3.0 * m;

	// 2. Point Source
	const G4double Source_PosZ = 0.0 * cm;

	// 3. Pb Collimator
	const G4double Collimator_ZMin = 10.0 * cm;
	const G4double Collimator_ZMax = 50.0 * cm;
	const G4double Collimator_Length = Collimator_ZMax - Collimator_ZMin; // 40.0 cm
	const G4double Collimator_PosZ = (Collimator_ZMin + Collimator_ZMax) / 2.0; // 30.0 cm
	const G4double Collimator_InnerRadius = 0.5 * cm; // 5.0 mm (bore hole)
	const G4double Collimator_OuterRadius = 3.0 * cm; // 30.0 mm

	// 4. Sample
	const G4double Sample_ZFront = 50.0 * cm;
	const G4double Sample_Radius = 5.0 * cm; // 50.0 mm

	// 5. Detection Field / Detector Tally Cell (z = [70, 72] cm)
	const G4double Det_ZMin = 70.0 * cm;
	const G4double Det_ZMax = 72.0 * cm;
	const G4double Det_Length = Det_ZMax - Det_ZMin; // 2.0 cm
	const G4double Det_PosZ = (Det_ZMin + Det_ZMax) / 2.0; // 71.0 cm
	const G4double Det_Radius = 5.0 * cm; // 50.0 mm

	// 5b. Virtual Detector Plane (surface-crossing flux tally, right at z = 70.0 cm)
	const G4double DetPlane_Radius    = Det_Radius; // 5.0 cm -> Area = 78.5398 cm2
	const G4double DetPlane_Thickness = 0.001 * mm; // infinitesimally thin virtual surface
	const G4double DetPlane_PosZ      = Det_ZMin - DetPlane_Thickness / 2.0; // touches z = 70.0 cm front face

	// 6. Outer Pb Shield (z = [45, 75] cm)
	const G4double Shield_ZMin = 45.0 * cm;
	const G4double Shield_ZMax = 75.0 * cm;
	const G4double Shield_Length = Shield_ZMax - Shield_ZMin; // 30.0 cm
	const G4double Shield_PosZ = (Shield_ZMin + Shield_ZMax) / 2.0; // 60.0 cm
	const G4double Shield_InnerRadius = 8.0 * cm; // 80.0 mm
	const G4double Shield_OuterRadius = 10.0 * cm; // 100.0 mm
}

// =========================================================================
// Constructor / Destructor
// =========================================================================
MyDetectorConstruction::MyDetectorConstruction()
	: G4VUserDetectorConstruction(),
	fMessenger(nullptr),
	fSample_Thickness(0.5 * cm), // Default thickness = 0.5 cm (5.0 mm)
	fSampleMaterial("S1"),       // Default sample S1
	fCollimator2_Height(0.0)
{
	fMessenger = new G4GenericMessenger(this, "/DetectorConstruction/", "Detector Construction Control");
	fMessenger->DeclarePropertyWithUnit("Sample_Thickness", "cm", fSample_Thickness, "Set Sample_Thickness");
	fMessenger->DeclareProperty("Sample_Material", fSampleMaterial, "Set Sample material name (S0, S1, S2, ..., S8)");
	fMessenger->DeclareProperty("Collimator2_Height", fCollimator2_Height, "Unused (retained for backward compatibility)");
	fMessenger->DeclareMethod("update", &MyDetectorConstruction::UpdateGeometry, "Update geometry after parameter changes");
}

MyDetectorConstruction::~MyDetectorConstruction() {
	delete fMessenger;
}

// =========================================================================
// Define Materials (Paper 10.1 - PNE 2022, Table 1)
// =========================================================================
void MyDetectorConstruction::DefineMaterials() {
	G4NistManager* NIST = G4NistManager::Instance();

	// 1. NIST Materials
	NIST->FindOrBuildMaterial("G4_Galactic");
	NIST->FindOrBuildMaterial("G4_Pb");

	// NIST Elements
	G4Element* elLi = NIST->FindOrBuildElement("Li");
	G4Element* elB  = NIST->FindOrBuildElement("B");
	G4Element* elO  = NIST->FindOrBuildElement("O");
	G4Element* elAl = NIST->FindOrBuildElement("Al");
	G4Element* elSi = NIST->FindOrBuildElement("Si");
	G4Element* elCa = NIST->FindOrBuildElement("Ca");
	G4Element* elGd = NIST->FindOrBuildElement("Gd");

	// 2. Sample Materials (Paper 10.1, Table 1)
	// [0]. S0: Blank / Vacuum
	if (!G4Material::GetMaterial("S0", false)) {
		new G4Material("S0", 1, 1.008 * g / mole, 1e-25 * g / cm3,
			kStateGas, 2.73 * kelvin, 3e-18 * pascal);
	}

	// [1]. S1: 45 B2O3 - 10 SiO2 - 40 Li2O - 5 Al2O3 - 0 Gd2O3 (rho = 2.390 g/cm3)
	if (!G4Material::GetMaterial("S1", false)) {
		G4Material* S1 = new G4Material("S1", 2.390 * g / cm3, 5);
		S1->AddElement(elLi, 0.18583);
		S1->AddElement(elB,  0.13976);
		S1->AddElement(elO,  0.60120);
		S1->AddElement(elAl, 0.02646);
		S1->AddElement(elSi, 0.04674);
	}

	// [2]. S2: 40 B2O3 - 10 SiO2 - 40 Li2O - 5 Al2O3 - 5 Gd2O3 (rho = 2.725 g/cm3)
	if (!G4Material::GetMaterial("S2", false)) {
		G4Material* S2 = new G4Material("S2", 2.725 * g / cm3, 6);
		S2->AddElement(elLi, 0.18583);
		S2->AddElement(elB,  0.12423);
		S2->AddElement(elO,  0.57335);
		S2->AddElement(elAl, 0.02646);
		S2->AddElement(elSi, 0.04674);
		S2->AddElement(elGd, 0.04338);
	}

	// [3]. S3: 35 B2O3 - 10 SiO2 - 40 Li2O - 5 Al2O3 - 10 Gd2O3 (rho = 2.908 g/cm3)
	if (!G4Material::GetMaterial("S3", false)) {
		G4Material* S3 = new G4Material("S3", 2.908 * g / cm3, 6);
		S3->AddElement(elLi, 0.18583);
		S3->AddElement(elB,  0.10870);
		S3->AddElement(elO,  0.54550);
		S3->AddElement(elAl, 0.02646);
		S3->AddElement(elSi, 0.04674);
		S3->AddElement(elGd, 0.08676);
	}

	// [4]. S4: 30 B2O3 - 10 SiO2 - 40 Li2O - 5 Al2O3 - 15 Gd2O3 (rho = 3.103 g/cm3)
	if (!G4Material::GetMaterial("S4", false)) {
		G4Material* S4 = new G4Material("S4", 3.103 * g / cm3, 6);
		S4->AddElement(elLi, 0.18583);
		S4->AddElement(elB,  0.09317);
		S4->AddElement(elO,  0.51765);
		S4->AddElement(elAl, 0.02646);
		S4->AddElement(elSi, 0.04674);
		S4->AddElement(elGd, 0.13014);
	}

	// [5]. S5: 60 B2O3 - 10 SiO2 - 10 CaO - 20 Gd2O3 (rho = 3.836 g/cm3)
	if (!G4Material::GetMaterial("S5", false)) {
		G4Material* S5 = new G4Material("S5", 3.836 * g / cm3, 5);
		S5->AddElement(elB,  0.18635);
		S5->AddElement(elO,  0.52192);
		S5->AddElement(elSi, 0.04674);
		S5->AddElement(elCa, 0.07147);
		S5->AddElement(elGd, 0.17352);
	}

	// [6]. S6: 55 B2O3 - 10 SiO2 - 10 CaO - 25 Gd2O3 (rho = 3.909 g/cm3)
	if (!G4Material::GetMaterial("S6", false)) {
		G4Material* S6 = new G4Material("S6", 3.909 * g / cm3, 5);
		S6->AddElement(elB,  0.17082);
		S6->AddElement(elO,  0.49407);
		S6->AddElement(elSi, 0.04674);
		S6->AddElement(elCa, 0.07147);
		S6->AddElement(elGd, 0.21690);
	}

	// [7]. S7: 50 B2O3 - 10 SiO2 - 10 CaO - 30 Gd2O3 (rho = 4.181 g/cm3)
	if (!G4Material::GetMaterial("S7", false)) {
		G4Material* S7 = new G4Material("S7", 4.181 * g / cm3, 5);
		S7->AddElement(elB,  0.15529);
		S7->AddElement(elO,  0.46622);
		S7->AddElement(elSi, 0.04674);
		S7->AddElement(elCa, 0.07147);
		S7->AddElement(elGd, 0.26028);
	}

	// [8]. S8: 45 B2O3 - 10 SiO2 - 10 CaO - 35 Gd2O3 (rho = 4.411 g/cm3)
	if (!G4Material::GetMaterial("S8", false)) {
		G4Material* S8 = new G4Material("S8", 4.411 * g / cm3, 5);
		S8->AddElement(elB,  0.13976);
		S8->AddElement(elO,  0.43837);
		S8->AddElement(elSi, 0.04674);
		S8->AddElement(elCa, 0.07147);
		S8->AddElement(elGd, 0.30366);
	}
}

// =========================================================================
// Construct Geometry (Paper 10.1 - PNE 2022 Benchmark, Fig. 1 & Section 2.2)
// =========================================================================
G4VPhysicalVolume* MyDetectorConstruction::Construct() {
	DefineMaterials();
	G4NistManager* NIST = G4NistManager::Instance();
	G4Material* vacuumMat = NIST->FindOrBuildMaterial("G4_Galactic");
	G4Material* pbMat     = NIST->FindOrBuildMaterial("G4_Pb");

	// 1. World Volume (Vacuum)
	G4Box* solidWorld = new G4Box("World", GEO::WorldSize / 2, GEO::WorldSize / 2, GEO::WorldSize / 2);
	G4LogicalVolume* World_LV = new G4LogicalVolume(solidWorld, vacuumMat, "WorldLV");
	World_Phys = new G4PVPlacement(nullptr, {}, World_LV, "World", nullptr, false, 0, true);

	// 2. Pb Collimator (z = [10, 50] cm, r = [0.5, 3.0] cm)
	G4Tubs* solidCollimator = new G4Tubs("Collimator", GEO::Collimator_InnerRadius, GEO::Collimator_OuterRadius,
		GEO::Collimator_Length / 2, 0, 360 * deg);
	G4LogicalVolume* Collimator_LV = new G4LogicalVolume(solidCollimator, pbMat, "Collimator_LV");
	new G4PVPlacement(nullptr, G4ThreeVector(0, 0, GEO::Collimator_PosZ), Collimator_LV, "Collimator", World_LV, false, 0, true);

	// 3. Glass Sample Slot (z = [50.0, 50.0 + thickness] cm, r = 5.0 cm)
	G4double thickness = (fSample_Thickness > 0.0) ? fSample_Thickness : 0.001 * mm;
	G4Tubs* solidSample = new G4Tubs("Sample", 0, GEO::Sample_Radius, thickness / 2, 0, 360 * deg);

	G4Material* sampleMat = G4Material::GetMaterial(fSampleMaterial, false);
	if (!sampleMat) {
		G4cerr << "\n*** WARNING: Material \"" << fSampleMaterial << "\" not found. Defaulting to S1 ***\n" << G4endl;
		sampleMat = G4Material::GetMaterial("S1");
	}
	G4LogicalVolume* Sample_LV = new G4LogicalVolume(solidSample, sampleMat, "Sample_LV");
	G4double samplePosZ = GEO::Sample_ZFront + thickness / 2;
	new G4PVPlacement(nullptr, G4ThreeVector(0, 0, samplePosZ), Sample_LV, "Sample", World_LV, false, 0, true);

	// 4. Outer Pb Shield (z = [45, 75] cm, r = [8.0, 10.0] cm)
	G4Tubs* solidOuterShield = new G4Tubs("OuterShield", GEO::Shield_InnerRadius, GEO::Shield_OuterRadius,
		GEO::Shield_Length / 2, 0, 360 * deg);
	G4LogicalVolume* OuterShield_LV = new G4LogicalVolume(solidOuterShield, pbMat, "OuterShield_LV");
	new G4PVPlacement(nullptr, G4ThreeVector(0, 0, GEO::Shield_PosZ), OuterShield_LV, "OuterShield", World_LV, false, 0, true);

	// 5. Detection Field / Detector Tally Cell (z = [70, 72] cm, r = 5.0 cm)
	G4Tubs* solidDetector = new G4Tubs("Detector", 0, GEO::Det_Radius, GEO::Det_Length / 2, 0, 360 * deg);
	G4LogicalVolume* Detector_LV = new G4LogicalVolume(solidDetector, vacuumMat, "Detector_LV");
	new G4PVPlacement(nullptr, G4ThreeVector(0, 0, GEO::Det_PosZ), Detector_LV, "Detector", World_LV, false, 0, true);

	// 5b. Virtual Detector Plane (surface-crossing flux tally at z = 70.0 cm front face, r = 5.0 cm)
	G4Tubs* solidDetPlane = new G4Tubs("DetPlane", 0, GEO::DetPlane_Radius, GEO::DetPlane_Thickness / 2, 0, 360 * deg);
	G4LogicalVolume* DetPlane_LV = new G4LogicalVolume(solidDetPlane, vacuumMat, "DetPlane_LV");
	new G4PVPlacement(nullptr, G4ThreeVector(0, 0, GEO::DetPlane_PosZ), DetPlane_LV, "DetPlane", World_LV, false, 0, true);

	// ===============================================
	// VISUALIZATION ATTRIBUTES
	// ===============================================
	G4VisAttributes* World_VisAtt = new G4VisAttributes(G4Colour(1, 1, 1, 0.0));
	World_VisAtt->SetVisibility(false);
	World_LV->SetVisAttributes(World_VisAtt);

	// Collimator: Dark Grey
	G4VisAttributes* Collimator_VisAtt = new G4VisAttributes(G4Colour(0.5, 0.5, 0.5));
	Collimator_VisAtt->SetForceSolid(true);
	Collimator_LV->SetVisAttributes(Collimator_VisAtt);

	// Outer Shield: Dark Grey (semi-transparent)
	G4VisAttributes* Shield_VisAtt = new G4VisAttributes(G4Colour(0.3, 0.3, 0.3, 0.5));
	Shield_VisAtt->SetForceSolid(true);
	OuterShield_LV->SetVisAttributes(Shield_VisAtt);

	// Sample: Cyan
	G4VisAttributes* Sample_VisAtt = new G4VisAttributes(G4Colour(0.0, 1.0, 1.0));
	Sample_VisAtt->SetForceSolid(true);
	Sample_LV->SetVisAttributes(Sample_VisAtt);

	// Detector: Green
	G4VisAttributes* Det_VisAtt = new G4VisAttributes(G4Colour(0.0, 1.0, 0.0, 0.4));
	Det_VisAtt->SetForceSolid(true);
	Detector_LV->SetVisAttributes(Det_VisAtt);

	// DetPlane: Red line/plane
	G4VisAttributes* DetPlane_VisAtt = new G4VisAttributes(G4Colour(1.0, 0.0, 0.0, 0.7));
	DetPlane_VisAtt->SetForceSolid(true);
	DetPlane_LV->SetVisAttributes(DetPlane_VisAtt);

	// ===============================================
	// PRODUCTION CUTS (Sample Region)
	// ===============================================
	G4Region* Sample_Region = G4RegionStore::GetInstance()->FindOrCreateRegion("SampleRegion");
	Sample_Region->AddRootLogicalVolume(Sample_LV);
	G4ProductionCuts* sampleCuts = new G4ProductionCuts();
	sampleCuts->SetProductionCut(0.1 * mm, G4ProductionCuts::GetIndex("gamma"));
	sampleCuts->SetProductionCut(0.1 * mm, G4ProductionCuts::GetIndex("e-"));
	sampleCuts->SetProductionCut(0.1 * mm, G4ProductionCuts::GetIndex("e+"));
	Sample_Region->SetProductionCuts(sampleCuts);

	return World_Phys;
}

void MyDetectorConstruction::UpdateGeometry() {
	auto runManager = G4RunManager::GetRunManager();
	runManager->ReinitializeGeometry();
	ConstructSDandField();
	G4cout << "\n================ Updated Geometry ================\n";
	G4cout << " Sample Thickness: " << fSample_Thickness / cm << " cm (" << fSample_Thickness / mm << " mm)\n";
	G4cout << " Sample Material : " << fSampleMaterial << "\n";
	G4cout << "==================================================\n";
}

G4String MyDetectorConstruction::GetSampleMaterialName() const {
	return fSampleMaterial;
}

G4double MyDetectorConstruction::GetSampleThickness() const {
	return fSample_Thickness;
}

// =========================================================================
// Construct Sensitive Detectors
// =========================================================================
void MyDetectorConstruction::ConstructSDandField() {
	auto sdManager = G4SDManager::GetSDMpointer();
	auto LVS = G4LogicalVolumeStore::GetInstance();

	// DetPlane surface-crossing flux tally
	G4String SDname = "DetPlane_SD";
	auto DetPlane_Detector = dynamic_cast<MySensitiveDetector*>(sdManager->FindSensitiveDetector(SDname, false));
	if (!DetPlane_Detector) {
		DetPlane_Detector = new MySensitiveDetector(SDname, "DetPlaneHitsCollection");
		sdManager->AddNewDetector(DetPlane_Detector);
	}
	for (auto lv : *LVS) {
		if (lv->GetName() == "DetPlane_LV") {
			lv->SetSensitiveDetector(DetPlane_Detector);
		}
	}
}