#ifndef MYRUNACTION_HH
#define MYRUNACTION_HH

#include "G4UserRunAction.hh"
#include "G4Accumulable.hh"
#include "G4Timer.hh"
#include "globals.hh"

class G4Run;

class MyRunAction : public G4UserRunAction {
public:
	MyRunAction();
	~MyRunAction() override;

	void BeginOfRunAction(const G4Run* run) override;
	void EndOfRunAction(const G4Run* run) override;

	// --- process counters (Sample, khong doi) ---
	void AddProcessCount(const G4String& processName);

	// --- surface-crossing flux tally (T-Cross-like) tai DetPlane, duy nhat con lai ---
	void AddCrossingFluxNaIDet(G4double flux);            ///< flux toan phan (co buildup)
	void AddCrossingFluxUncollidedNaIDet(G4double flux);  ///< flux "uncollided" (energy window quanh E0)

	G4double GetPrimaryEnergy() const;
	G4double GetEnergyWindowFraction() const { return fEnergyWindowFrac; }
	void     SetEnergyWindowFraction(G4double frac) { fEnergyWindowFrac = frac; }

private:
	// --- process counters ---
	G4Accumulable<G4double> fNCompton;
	G4Accumulable<G4double> fNPhotoelectric;
	G4Accumulable<G4double> fNPairProduction;
	G4Accumulable<G4double> fNRayleigh;
	G4Accumulable<G4double> fNOther;
	G4Accumulable<G4double> fNTotal;

	// --- surface-crossing flux tally accumulables (T-Cross-like) tai DetPlane ---
	G4Accumulable<G4double> fFluxCrossNaI{0.0};
	G4Accumulable<G4double> fFluxCrossUncollidedNaI{0.0};

	// --- energy window (%) dung de xac dinh "uncollided", default +-1% ---
	G4double fEnergyWindowFrac = 0.01;

	G4Timer fTimer;
};

#endif
