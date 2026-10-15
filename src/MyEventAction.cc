#include "MyEventAction.hh"
#include "MyRunAction.hh"
#include "MyHits.hh"

#include "G4Event.hh"
#include "G4HCofThisEvent.hh"

#include <cmath>

MyEventAction::MyEventAction(MyRunAction* runAction)
    : G4UserEventAction(), fRunAction(runAction) {}

MyEventAction::~MyEventAction() = default;

void MyEventAction::BeginOfEventAction(const G4Event*) {
    fFirstInteractionRecorded = false;  // dung cho logic dem process o Sample (khong doi)
}

// =========================================================================
// EndOfEventAction - SURFACE-CROSSING TALLY (DetPlane) ONLY
// =========================================================================
// NEW: logic track-length tally (F4-like) o "NaI" da bi BO HOAN TOAN, cung
// nhu logic dem pulse/edep cu (fEdepEvent, AddEdepNaIDet, AddCountedNaIDet).
// Chi con lai 1 loai tally: surface-crossing flux (T-Cross-like) tai
// "DetPlane" + ban "uncollided" (loc theo cua so nang luong +/- windowFrac
// quanh E0).
// =========================================================================
void MyEventAction::EndOfEventAction(const G4Event* event) {
    if (!fRunAction) return;
    auto hce = event->GetHCofThisEvent();
    if (!hce) return;

    G4double crossFluxEvent            = 0.;
    G4double crossFluxUncollidedEvent  = 0.;

    G4double E0         = fRunAction->GetPrimaryEnergy();
    G4double windowFrac = fRunAction->GetEnergyWindowFraction();

    for (int i = 0; i < hce->GetNumberOfCollections(); i++) {
        auto hc = dynamic_cast<MyHitsCollection*>(hce->GetHC(i));
        if (!hc) continue;
        for (unsigned int j = 0; j < hc->entries(); j++) {
            auto hit = (*hc)[j];
            const G4String& volName = hit->GetVolumeName();
            if (volName != "DetPlane") continue;

            G4bool inWindow = (E0 > 0. &&
                std::fabs(hit->GetKineticEnergy() - E0) <= windowFrac * E0);

            crossFluxEvent += hit->GetCrossingFlux();
            if (inWindow) crossFluxUncollidedEvent += hit->GetCrossingFlux();
        }
    }

    if (crossFluxEvent > 0.) {
        fRunAction->AddCrossingFluxNaIDet(crossFluxEvent);
    }
    if (crossFluxUncollidedEvent > 0.) {
        fRunAction->AddCrossingFluxUncollidedNaIDet(crossFluxUncollidedEvent);
    }
}
