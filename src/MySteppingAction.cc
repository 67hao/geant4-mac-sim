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
    if (fEventAction->IsFirstInteractionRecorded()) return;

    auto track = step->GetTrack();
    if (track->GetTrackID() != 1) return;
    if (track->GetDefinition()->GetParticleName() != "gamma") return;

    auto volume = step->GetPreStepPoint()->GetTouchableHandle()->GetVolume();
    if (!volume || volume->GetName() != "Sample") return;

    auto process = step->GetPostStepPoint()->GetProcessDefinedStep();
    if (!process) return;

    G4String name = process->GetProcessName();
    if (name == "Transportation" || name == "CoupledTransportation") return;

    fRunAction->AddProcessCount(name);
    fEventAction->SetFirstInteractionRecorded();
}
