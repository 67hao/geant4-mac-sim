#ifndef MYSTEPPINGACTION_HH
#define MYSTEPPINGACTION_HH

#include "G4UserSteppingAction.hh"
#include "globals.hh"

class MyRunAction;
class MyEventAction;

class MySteppingAction : public G4UserSteppingAction {
public:
    MySteppingAction(MyRunAction* runAction, MyEventAction* eventAction);
    ~MySteppingAction() override = default;
    void UserSteppingAction(const G4Step* step) override;

private:
    MyRunAction*   fRunAction;
    MyEventAction* fEventAction;
};

#endif
