// =========================================================================
// MySensitiveDetector.cc - TRACK-LENGTH TALLY ONLY (pulse/edep logic removed)
// =========================================================================

#include "MySensitiveDetector.hh"
#include "MyHits.hh"

#include "G4Step.hh"
#include "G4HCofThisEvent.hh"
#include "G4SDManager.hh"
#include "G4SystemOfUnits.hh"
#include "G4VTouchable.hh"
#include "G4ios.hh"

#include <cmath>

// =========================================================================
// Constructor
// =========================================================================
MySensitiveDetector::MySensitiveDetector(const G4String& name, const G4String& collectionName)
    : G4VSensitiveDetector(name) {
    this->collectionName.push_back(collectionName);
}

// =========================================================================
// Destructor
// =========================================================================
MySensitiveDetector::~MySensitiveDetector() = default;

// =========================================================================
// Initialize
// =========================================================================
void MySensitiveDetector::Initialize(G4HCofThisEvent* hce) {
    fHitsCollection = new MyHitsCollection(SensitiveDetectorName, collectionName[0]);
    G4int hcID = G4SDManager::GetSDMpointer()->GetCollectionID(fHitsCollection);
    hce->AddHitsCollection(hcID, fHitsCollection);
}

// =========================================================================
// ProcessHits
// =========================================================================
// NEW: logic track-length tally (F4-like) o "NaI" da bi BO HOAN TOAN.
// Instance nay gio chi con xu ly 1 loai tally duy nhat:
//
//  DetPlane -> surface-crossing flux tally kieu MCNP F2 / PHITS [T-Cross]:
//              chi tinh khi track VUA buoc vao volume mong nay
//              (StepStatus == fGeomBoundary), dong gop weight/|cosTheta|
//              (dung cach PHITS lam ben trong: current += 1,
//              flux += 1/cos(theta) moi lan cat mat).
// =========================================================================
G4bool MySensitiveDetector::ProcessHits(G4Step* aStep, G4TouchableHistory*) {

    const G4String& volName = aStep->GetPreStepPoint()->GetTouchableHandle()
        ->GetVolume()->GetName();

    // ---------------------------------------------------------------
    // DetPlane: surface flux tally (T-Cross-like)
    // ---------------------------------------------------------------
    if (volName == "DetPlane") {
        // Chi tinh khi track VUA di vao volume nay (cat bien tu ngoai vao),
        // de khong dem trung "vao" + "ra" trong 1 volume rat mong.
        if (aStep->GetPreStepPoint()->GetStepStatus() != fGeomBoundary) return false;

        G4ThreeVector dir = aStep->GetPreStepPoint()->GetMomentumDirection();
        G4double cosTheta = std::fabs(dir.z());  // mat phang DetPlane vuong goc truc Z
        if (cosTheta < 1e-6) return false;         // track bay gan song song mat -> bo qua

        G4double weight = aStep->GetPreStepPoint()->GetWeight();
        G4double ekin   = aStep->GetPreStepPoint()->GetKineticEnergy();

        MyHit* hit = new MyHit();
        hit->SetCrossingFlux(weight / cosTheta);   // dong gop kieu flux (giong PHITS [T-Cross])
        hit->SetCosTheta(cosTheta);
        hit->SetKineticEnergy(ekin);                // dung cho energy-window filter
        hit->SetVolumeName(volName);
        hit->SetWeight(weight);
        hit->SetParticleName(aStep->GetTrack()->GetDefinition()->GetParticleName());
        hit->SetTrackID(aStep->GetTrack()->GetTrackID());

        fHitsCollection->insert(hit);
        return true;
    }

    return false;
}
