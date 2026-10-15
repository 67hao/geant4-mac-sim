#include "MyActionInitialization.hh"
#include "MyPrimaryGeneratorAction.hh"
#include "MyRunAction.hh"
#include "MyEventAction.hh"
#include "MySteppingAction.hh"
#include "MyDetectorConstruction.hh"
#include "G4RunManager.hh"

MyActionInitialization::MyActionInitialization() : G4VUserActionInitialization() {}
MyActionInitialization::~MyActionInitialization() = default;

void MyActionInitialization::Build() const {
    const MyDetectorConstruction* detector =
        static_cast<const MyDetectorConstruction*>(
            G4RunManager::GetRunManager()->GetUserDetectorConstruction());

    SetUserAction(new MyPrimaryGeneratorAction(detector));

    auto runAction = new MyRunAction();
    SetUserAction(runAction);

    auto eventAction = new MyEventAction(runAction);
    SetUserAction(eventAction);

    SetUserAction(new MySteppingAction(runAction, eventAction));  // NEW
}

void MyActionInitialization::BuildForMaster() const {
    auto runAction = new MyRunAction();
    SetUserAction(runAction);
}
