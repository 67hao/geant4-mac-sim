#ifndef MYEVENTACTION_HH
#define MYEVENTACTION_HH

#include "G4UserEventAction.hh"
#include "globals.hh"

class G4Event;
class MyRunAction;

class MyEventAction : public G4UserEventAction {
public:
	explicit MyEventAction(MyRunAction* runAction);
	~MyEventAction() override;

	void BeginOfEventAction(const G4Event*) override;
	void EndOfEventAction(const G4Event* event) override;

	// --- first interaction tracking (dung boi MySteppingAction, khong doi) ---
	G4bool IsFirstInteractionRecorded() const { return fFirstInteractionRecorded; }
	void SetFirstInteractionRecorded() { fFirstInteractionRecorded = true; }

private:
	MyRunAction* fRunAction;
	G4bool fFirstInteractionRecorded = false;

	// NEW: fEdepEvent va fCountedNaIDet (std::set<G4int>) da bi XOA -
	// day la tan du cua logic dem pulse/edep cu, khong con duoc doc/ghi
	// o dau trong MyEventAction.cc / MySensitiveDetector.cc nua.
};

#endif
