#include "MySteppingAction.hh"
#include "MyRunAction.hh"
#include "MyEventAction.hh"

#include "G4Step.hh"
#include "G4Track.hh"
#include "G4VProcess.hh"
#include "G4VPhysicalVolume.hh"

MySteppingAction::MySteppingAction(MyRunAction* runAction, MyEventAction* eventAction)
    : fRunAction(runAction), fEventAction(eventAction)
{}

void MySteppingAction::UserSteppingAction(const G4Step* step)
{
    auto track = step->GetTrack();

    // Kill secondary charged particles (electrons, positrons) immediately to optimize speed:
    // for narrow-beam MAC calculation, only uncollided photons reaching DetPlane are tallies.
    if (track->GetDefinition()->GetParticleName() != "gamma") {
        track->SetTrackStatus(fStopAndKill);
        return;
    }

    if (fEventAction->IsFirstInteractionRecorded()) return;
    if (track->GetTrackID() != 1) return;

    auto volume = step->GetPreStepPoint()->GetTouchableHandle()->GetVolume();
    if (!volume || volume->GetName() != "Sample") return;

    auto process = step->GetPostStepPoint()->GetProcessDefinedStep();
    if (!process) return;

    G4String name = process->GetProcessName();
    if (name == "Transportation" || name == "CoupledTransportation") return;

    fRunAction->AddProcessCount(name);
    fEventAction->SetFirstInteractionRecorded();
}
