#include "MyPhysicsList.hh"

#include "G4SystemOfUnits.hh"
#include "G4EmStandardPhysics.hh"
#include "G4DecayPhysics.hh"
#include "G4HadronPhysicsQGSP_BIC_HP.hh"
#include "G4RadioactiveDecayPhysics.hh"
#include "G4HadronElasticPhysics.hh"
#include "G4GenericBiasingPhysics.hh"
#include <G4HadronPhysicsFTFP_BERT.hh>
#include <G4EmLivermorePhysics.hh>
#include <G4IonPhysics.hh>
#include <G4StoppingPhysics.hh>
#include <G4EmPenelopePhysics.hh>
#include <G4EmStandardPhysics_option4.hh>


MyPhysicsList::MyPhysicsList()
    : G4VModularPhysicsList()
{
    SetDefaultCutValue(1 * mm);
    SetVerboseLevel(1);

    // ===== EM Standard =====
    // NEW: doi sang G4EmStandardPhysics_option4 - day la bo physics EM
    // duoc Geant4 khuyen nghi/kiem dinh tot tren dai rong (~100 eV den
    // hang tram GeV), khop XCOM tot hon Penelope o vung nang luong cao
    // (MeV+) - phu hop dai nang luong ban dang test (1 keV - 15 MeV).
    //RegisterPhysics(new G4EmStandardPhysics_option4());
    //RegisterPhysics(new G4EmPenelopePhysics()); //EM interaction at low energy, usually for gamma, electron, positron // Using NIST database for material properties, which is more accurate for low-energy interactions. Penelope models are optimized for energies down to a few hundred eV, making them suitable for our application. NOTE: lech XCOM dang ke o vung MeV+ (xem thao luan), nen chuyen sang option4 o tren.
    RegisterPhysics(new G4EmLivermorePhysics());//EM  interaction at low energey, usually for gamma, electron, positron // Using EPDL97 database for material properties, which is more accurate for low-energy interactions. Livermore models are optimized for energies down to a few hundred eV, making them suitable for our application.
    //RegisterPhysics(new G4EmStandardPhysics());//EM interaction at high energy

    // ===== Nuclear - Hadron =====
    //RegisterPhysics(new G4HadronElasticPhysics());//Hadron Elastic Scattering    
    //RegisterPhysics(new G4StoppingPhysics());//Stopping proccess
    //RegisterPhysics(new G4IonPhysics());//Ion interaction
    //RegisterPhysics(new G4HadronPhysicsFTFP_BERT());//Hadron Inelastic Scattering
    //RegisterPhysics(new G4HadronPhysicsQGSP_BIC_HP());

    // ===== Decay =====
    //RegisterPhysics(new G4DecayPhysics());// Elemental Particles Decay
    //RegisterPhysics(new G4RadioactiveDecayPhysics());//Nuclear Decay
}


MyPhysicsList::~MyPhysicsList() {}