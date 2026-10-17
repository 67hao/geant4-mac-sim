"""
run_mac_g4_task.py - Parallel worker for Geant4 MAC Simulation (2026.10.15_MAC_G4)
Runs on GitHub Actions Swarm (18 active accounts x 10 runners = 180 runners).
Uploads raw 1-to-1 CSV results to Flat Common Pool (02_Flux_Data).
Uploads child outputs (.root files) directly to Flat Common Pool (03_Simulation_Outputs, NO ZIP).
"""

import argparse
import os
import sys
import time
import json
import re
import subprocess
from pathlib import Path
import pandas as pd

if hasattr(sys.stdout, "reconfigure"):
    sys.stdout.reconfigure(encoding="utf-8", errors="replace")
if hasattr(sys.stderr, "reconfigure"):
    sys.stderr.reconfigure(encoding="utf-8", errors="replace")

import gdrive_helper

def parse_args():
    parser = argparse.ArgumentParser(description="Geant4 Parallel Worker for MAC Simulation")
    parser.add_argument("--runner-id", type=int, required=True, help="ID runner (1..10)")
    parser.add_argument("--num-runners", type=int, default=10, help="Tong so runner tren account nay")
    parser.add_argument("--primaries", type=int, default=100000000, help="So hat moi job")
    parser.add_argument("--gdrive-folder", type=str, default="1ZvUozvCDo3itVIYcA0r-70CR92Ag8IRh")
    parser.add_argument("--plan-file", type=str, default="mac_g4_plan_13acc.json")
    parser.add_argument("--github-user", type=str, default=None)
    parser.add_argument("--sim-bin", type=str, default=None)
    parser.add_argument("--timeout-hours", type=float, default=4.0)
    return parser.parse_args()

def ensure_compiled():
    candidates = ["./build/Sim", "./Sim", "./code/build/Sim", "/tmp/geant4_build/Sim"]
    for c in candidates:
        if os.path.exists(c) and os.access(c, os.X_OK):
            return c
    
    print(">>> [BUILD] Bien dich ma nguon Geant4 Sim...")
    build_dir = Path("build")
    build_dir.mkdir(parents=True, exist_ok=True)
    res_cmake = subprocess.run(["cmake", "..", "-DCMAKE_BUILD_TYPE=Release"], cwd=str(build_dir))
    if res_cmake.returncode != 0:
        raise RuntimeError("CMake configure that bai!")
    res_make = subprocess.run(["make", f"-j{os.cpu_count() or 2}", "Sim"], cwd=str(build_dir))
    if res_make.returncode != 0:
        raise RuntimeError("Make Sim that bai!")
    
    sim_bin = "./build/Sim"
    if not os.path.exists(sim_bin):
        raise FileNotFoundError(f"Khong tim thay binary {sim_bin} sau khi build!")
    return sim_bin

def get_existing_drive_files(drive_service, flux_folder_id):
    existing = set()
    page_token = None
    while True:
        try:
            q = f"'{flux_folder_id}' in parents and trashed = false"
            res = drive_service.files().list(
                q=q, supportsAllDrives=True, includeItemsFromAllDrives=True,
                fields="nextPageToken, files(name)",
                pageSize=1000, pageToken=page_token
            ).execute()
            for f in res.get("files", []):
                existing.add(f["name"].lower().strip())
            page_token = res.get("nextPageToken")
            if not page_token:
                break
        except Exception as e:
            print(f">>> [WARNING] Loi doc danh sach drive: {e}")
            break
    return existing

def main():
    args = parse_args()
    acc_name = args.github_user or os.environ.get("GITHUB_REPOSITORY_OWNER") or "runner"

    print("=" * 80)
    print(f"🚀 STARTING GEANT4 MAC RUNNER #{args.runner_id} (Account: {acc_name})")
    print(f"Project: 2026.10.15_MAC_G4 | Plan file: {args.plan_file}")
    print("=" * 80)

    sim_bin = args.sim_bin or ensure_compiled()
    print(f">>> Binary Geant4 san sang: {sim_bin}")

    # 1. Connect Google Drive and locate 02_Flux_Data & 03_Simulation_Outputs
    drive_service = None
    flux_folder_id = None
    outputs_folder_id = None
    existing_files = set()
    existing_root_files = set()

    try:
        drive_service = gdrive_helper.get_drive_service()
        subfolders = gdrive_helper.get_or_create_subfolders(drive_service, parent_id=args.gdrive_folder)
        flux_folder_id = subfolders.get("02_Flux_Data")
        outputs_folder_id = subfolders.get("03_Simulation_Outputs")

        print(f">>> [DRIVE] Ket noi Google Drive thanh cong!")
        print(f"  • Flat Common Pool (02_Flux_Data):          {flux_folder_id}")
        print(f"  • Raw Archive      (03_Simulation_Outputs): {outputs_folder_id}")

        existing_files = get_existing_drive_files(drive_service, flux_folder_id)
        if outputs_folder_id:
            existing_root_files = get_existing_drive_files(drive_service, outputs_folder_id)
        print(f">>> [DRIVE] Tim thay {len(existing_files)} CSVs va {len(existing_root_files)} ROOT files tren Drive.")
    except Exception as e:
        print(f">>> [WARNING] Ket noi Drive that bai: {e}. Du lieu se duoc luu local.")

    # 2. Load execution plan
    with open(args.plan_file, "r", encoding="utf-8") as pf:
        full_plan = json.load(pf)

    matched_acc = None
    for acc in full_plan:
        if acc.lower() == acc_name.lower():
            matched_acc = acc
            break
    if not matched_acc:
        matched_acc = list(full_plan.keys())[0]

    runner_jobs = full_plan[matched_acc].get(str(args.runner_id), [])
    total_jobs = len(runner_jobs)
    print(f"\nRunner #{args.runner_id} duoc phan cong {total_jobs} jobs tu account '{matched_acc}'.")

    # 3. Execute Jobs
    for idx, job in enumerate(runner_jobs, 1):
        jname = job["job_name"]
        macro_name = job["macro_file"]
        sample = job.get("sample", jname.split("_")[0])
        thickness_cm = float(job.get("thickness_cm", 0.0))
        energy_kev = float(job.get("energy_kev", 0.0))
        target_primaries = int(args.primaries or job.get("primaries", 1000000))

        # Check candidate filenames
        target_csv_name = f"{jname}.csv"
        target_root_name = f"hist_{jname}.root"
        if target_csv_name.lower() in existing_files and target_root_name.lower() in existing_root_files:
            print(f"[{idx}/{total_jobs}] SKIP: [{jname}] da co du ca ROOT va CSV tren Drive.")
            continue

        # Locate macro file
        macro_candidates = [
            Path("macros") / macro_name,
            Path("code") / "macros" / macro_name,
            Path("build") / "macros" / macro_name,
            Path(macro_name)
        ]
        macro_path = None
        for mc in macro_candidates:
            if mc.exists():
                macro_path = mc
                break

        if not macro_path:
            print(f"[{idx}/{total_jobs}] ERR: Khong tim thay macro file {macro_name}!")
            continue

        # Check primaries
        run_macro_path = macro_path
        try:
            with open(macro_path, "r", encoding="utf-8") as mf:
                content = mf.read()
            if f"/run/beamOn {target_primaries}" not in content:
                new_content = re.sub(r"/run/beamOn\s+\d+", f"/run/beamOn {target_primaries}", content)
                tmp_macro = Path(f"tmp_{macro_name}")
                with open(tmp_macro, "w", encoding="utf-8") as tmf:
                    tmf.write(new_content)
                run_macro_path = tmp_macro
        except Exception as e:
            print(f"    ⚠️ Warning checking macro content: {e}")

        print(f"\n--- [{idx}/{total_jobs}] RUNNING: {jname} ({macro_name}) ---")
        print(f"    Sample: {sample} | Thickness: {thickness_cm} cm | Energy: {energy_kev} keV | Primaries: {target_primaries:,}")

        # Set thread environment variable
        env = os.environ.copy()
        env["G4FORCENUMBEROFTHREADS"] = "2"

        t0 = time.time()
        cmd = [sim_bin, str(run_macro_path.resolve())]
        res = subprocess.run(cmd, capture_output=True, text=True, env=env)
        elapsed = time.time() - t0

        # Clean tmp macro if created
        if run_macro_path != macro_path and run_macro_path.exists():
            try:
                run_macro_path.unlink()
            except Exception:
                pass

        if res.returncode != 0:
            print(f"    [SIM ERROR] code {res.returncode}: {(res.stderr or res.stdout)[-400:]}")
        else:
            print(f"    [SIM OK] Thoi gian chay: {elapsed:.2f}s")

        # Parse results from stdout
        flux_tot_match = re.search(r"Flux\s+total\s+\(crossing\)\s*=\s*([0-9.eE+-]+)", res.stdout)
        flux_unc_match = re.search(r"Flux\s+uncollided\s*=\s*([0-9.eE+-]+)", res.stdout)
        area_match     = re.search(r"DetPlane\s+area\s*=\s*([0-9.eE+-]+)", res.stdout)
        events_match   = re.search(r"Number of events:\s*(\d+)", res.stdout)

        compt_match    = re.search(r"Compton\s*=\s*([0-9.eE+-]+)", res.stdout)
        photo_match    = re.search(r"Photoelectric\s*=\s*([0-9.eE+-]+)", res.stdout)
        pair_match     = re.search(r"Pair\s+Production\s*=\s*([0-9.eE+-]+)", res.stdout)
        rayl_match     = re.search(r"Rayleigh\s*=\s*([0-9.eE+-]+)", res.stdout)
        other_match    = re.search(r"Other\s*=\s*([0-9.eE+-]+)", res.stdout)
        tot_match      = re.search(r"Total\s*=\s*([0-9.eE+-]+)", res.stdout)

        r_compt_match  = re.search(r"R_Compton\s*=\s*([0-9.eE+-]+)", res.stdout)
        r_photo_match  = re.search(r"R_Photoelectric\s*=\s*([0-9.eE+-]+)", res.stdout)
        r_pair_match   = re.search(r"R_PairProduction\s*=\s*([0-9.eE+-]+)", res.stdout)
        r_rayl_match   = re.search(r"R_Rayleigh\s*=\s*([0-9.eE+-]+)", res.stdout)

        flux_total = float(flux_tot_match.group(1)) if flux_tot_match else 0.0
        flux_uncollided = float(flux_unc_match.group(1)) if flux_unc_match else 0.0
        detplane_area = float(area_match.group(1)) if area_match else 0.0
        events_simulated = int(events_match.group(1)) if events_match else target_primaries

        n_compt = float(compt_match.group(1)) if compt_match else 0.0
        n_photo = float(photo_match.group(1)) if photo_match else 0.0
        n_pair  = float(pair_match.group(1)) if pair_match else 0.0
        n_rayl  = float(rayl_match.group(1)) if rayl_match else 0.0
        n_other = float(other_match.group(1)) if other_match else 0.0
        n_tot   = float(tot_match.group(1)) if tot_match else 0.0

        r_compt = float(r_compt_match.group(1)) if r_compt_match else 0.0
        r_photo = float(r_photo_match.group(1)) if r_photo_match else 0.0
        r_pair  = float(r_pair_match.group(1)) if r_pair_match else 0.0
        r_rayl  = float(r_rayl_match.group(1)) if r_rayl_match else 0.0

        print(f"    [PARSED] FluxTotal: {flux_total:.6e} | FluxUncollided: {flux_uncollided:.6e} | Events: {events_simulated:,}")

        row = {
            "job_name": jname,
            "sample": sample,
            "thickness_cm": thickness_cm,
            "energy_kev": energy_kev,
            "primaries": events_simulated,
            "detplane_area_cm2": detplane_area,
            "flux_total": flux_total,
            "flux_uncollided": flux_uncollided,
            "n_compton": n_compt,
            "n_photo": n_photo,
            "n_pair": n_pair,
            "n_rayleigh": n_rayl,
            "n_other": n_other,
            "n_total": n_tot,
            "r_compton": r_compt,
            "r_photo": r_photo,
            "r_pair": r_pair,
            "r_rayleigh": r_rayl,
            "sim_seconds": round(elapsed, 2)
        }
        pd.DataFrame([row]).to_csv(target_csv_name, index=False)
        print(f"    [OUTPUT] Ghi ket qua vao {target_csv_name}")

        # Upload CSV directly to Flat Common Pool (02_Flux_Data)
        if drive_service and flux_folder_id and os.path.exists(target_csv_name):
            try:
                gdrive_helper.upload_file_to_folder(drive_service, target_csv_name, flux_folder_id)
                existing_files.add(target_csv_name.lower())
                print(f"    ✅ Uploaded {target_csv_name} -> 02_Flux_Data (Flat Common Pool)")
            except Exception as e:
                print(f"    ⚠️ Loi upload {target_csv_name}: {e}")

        # Upload ROOT output directly to 03_Simulation_Outputs (Flat ROOT file, NO ZIP)
        root_candidates = [
            Path(f"hist_{jname}.root"),
            Path("build") / f"hist_{jname}.root",
            Path(f"{jname}.root"),
            Path("build") / f"{jname}.root"
        ]
        found_roots = []
        for rc in root_candidates:
            if rc.exists() and rc not in found_roots:
                found_roots.append(rc)
        for rf in list(Path.cwd().glob("*.root")) + list(Path("build").glob("*.root")):
            if (jname.lower() in rf.name.lower() or f"hist_{jname.lower()}" in rf.name.lower()) and rf not in found_roots:
                found_roots.append(rf)

        if not found_roots:
            print(f"    ⚠️ Warning: Khong tim thay file root cho {jname}! Danh sach root hien co: {[f.name for f in Path.cwd().glob('*.root')]}")

        for job_root_file in found_roots:
            if drive_service and outputs_folder_id:
                try:
                    gdrive_helper.upload_file_to_folder(drive_service, str(job_root_file), outputs_folder_id)
                    existing_root_files.add(job_root_file.name.lower())
                    print(f"    ✅ Uploaded {job_root_file.name} -> 03_Simulation_Outputs (Direct Flat ROOT)")
                    try:
                        job_root_file.unlink()
                    except Exception:
                        pass
                except Exception as e:
                    print(f"    ⚠️ Loi upload root file {job_root_file.name}: {e}")

    print("\n" + "=" * 80)
    print(f"🎉 RUNNER #{args.runner_id} HOAN TAT MO PHONG!")
    print("=" * 80)

if __name__ == "__main__":
    main()
