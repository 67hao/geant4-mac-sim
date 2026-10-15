#!/usr/bin/env python3
"""
generate_geant4_macros.py
Sinh tu dong 1.711 file macro (.mac) va checklist_geant4_paper10_1.csv
cho Geant4 theo chuan hinh hoc & vat lieu Paper 10.1 (PNE 2022).
"""

import os
from pathlib import Path
import pandas as pd

ENERGIES_KEV = [
    15, 20, 30, 40, 50, 60, 80, 100, 200, 300, 400, 500,
    600, 800, 1000, 2000, 3000, 4000, 5000, 6000, 8000, 10000, 15000
]

SAMPLES = ["S1", "S2", "S3", "S4", "S5", "S6", "S7", "S8"]

def get_thicknesses(energy_kev: int):
    # Voi dải 15 - 40 keV co them be day mong 0.005 cm
    if energy_kev in [15, 20, 30, 40]:
        return [0.005, 0.1, 0.3, 0.5, 0.8, 1.0, 1.5, 2.0, 2.5, 3.0]
    else:
        return [0.1, 0.3, 0.5, 0.8, 1.0, 1.5, 2.0, 2.5, 3.0]

def main():
    base_dir = Path(__file__).resolve().parent
    macro_dir = base_dir / "macros"
    macro_dir.mkdir(parents=True, exist_ok=True)

    checklist_rows = []
    
    # 1. Blank (S0) cho 23 muc nang luong (be day mac dinh 0.005 cm)
    for e in ENERGIES_KEV:
        th = 0.005
        job_name = f"BLANK_{th}cm_{e}keV"
        mac_name = f"run_BLANK_{th}cm_{e}keV.mac"
        mac_path = macro_dir / mac_name
        
        content = f"""# Paper 10.1 Benchmark - S0 (Blank) @ {e} keV
/control/verbose 0
/run/verbose 0
/event/verbose 0
/tracking/verbose 0

/DetectorConstruction/Sample_Material S0
/DetectorConstruction/Sample_Thickness {th:.4f} cm
/DetectorConstruction/update

/gun/particle gamma
/gun/energy {e} keV
/gun/position 0 0 0 cm
/gun/direction 0 0 1

/analysis/setFileName hist_{job_name}
/run/beamOn 1000000
"""
        mac_path.write_text(content, encoding="utf-8")
        
        checklist_rows.append({
            "job_name": job_name,
            "sample": "BLANK",
            "thickness_cm": th,
            "energy_kev": e,
            "macro_file": mac_name,
            "output_csv": f"BLANK_{th}cm_{e}keV.csv"
        })

    # 2. 8 mau thuy tinh (S1 - S8)
    for s in SAMPLES:
        for e in ENERGIES_KEV:
            th_list = get_thicknesses(e)
            for th in th_list:
                th_str = f"{th:g}"
                job_name = f"{s}_{th_str}cm_{e}keV"
                mac_name = f"run_{s}_{th_str}cm_{e}keV.mac"
                mac_path = macro_dir / mac_name
                
                content = f"""# Paper 10.1 Benchmark - {s} @ {e} keV, th={th_str} cm
/control/verbose 0
/run/verbose 0
/event/verbose 0
/tracking/verbose 0

/DetectorConstruction/Sample_Material {s}
/DetectorConstruction/Sample_Thickness {th:.4f} cm
/DetectorConstruction/update

/gun/particle gamma
/gun/energy {e} keV
/gun/position 0 0 0 cm
/gun/direction 0 0 1

/analysis/setFileName hist_{job_name}
/run/beamOn 1000000
"""
                mac_path.write_text(content, encoding="utf-8")
                
                checklist_rows.append({
                    "job_name": job_name,
                    "sample": s,
                    "thickness_cm": th,
                    "energy_kev": e,
                    "macro_file": mac_name,
                    "output_csv": f"{s}_{th_str}cm_{e}keV.csv"
                })

    df = pd.DataFrame(checklist_rows)
    checklist_path = base_dir / "checklist_geant4_paper10_1.csv"
    df.to_csv(checklist_path, index=False)
    print(f"-> Da sinh thanh cong {len(df)} files macro tai {macro_dir}")
    print(f"-> Da luu checklist tai {checklist_path}")

if __name__ == "__main__":
    main()
