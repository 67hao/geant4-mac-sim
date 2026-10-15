#!/usr/bin/env python3
"""
gen_mac.py - Tu dong tao file .mac cho Geant4 tu file Excel dang bang ngang:

    Energy | s1   | s2   | s3   | s4   | s5   | s6
    15     | 12.3 | 9.8  | ...
    20     | ...
    ...

Cot dau tien la Energy (keV), cac cot con lai la thickness (mm) cua tung sample
tai nang luong tuong ung. O trong (khong co gia tri) se duoc bo qua.

Mot o co the chua NHIEU gia tri thickness, cach nhau boi dau phay ',' hoac
cham phay ';' (vi du: "1,2,3,4" hoac "1.5; 2.5; 3"). Khi do script se tao
mot file .mac rieng cho MOI thickness trong o do, ung voi cung 1 energy.

Cach dung:
    python3 gen_mac.py data.xlsx --outdir ./mac_output
    python3 gen_mac.py data.xlsx --sheet "Sheet2" --beamon 500000
"""
import argparse
import re
import sys
from pathlib import Path
import pandas as pd

COL_ENERGY = "Energy"  # ten cot nang luong, sua neu file cua m khac

MAC_TEMPLATE = """/DetectorConstruction/Sample_Material {material}
/DetectorConstruction/Sample_Thickness {thickness:.9f}
/DetectorConstruction/update
/gun/energy {energy:.6f} keV
/analysis/setFileName {histname}
/run/beamOn {beamon}
"""


def fmt_tag(x: float) -> str:
    """So nguyen -> '10'. So le -> '10p5' (dung quy uoc parse cua Count.cc)."""
    if float(x).is_integer():
        return str(int(x))
    s = f"{x:.6f}".rstrip("0").rstrip(".")
    return s.replace(".", "p")


def sample_tag(col_name: str) -> str:
    s = str(col_name).strip()
    return f"S{s[1:]}" if s.upper().startswith("S") else f"S{s}"


def parse_thickness_list(cell) -> list:
    """Tra ve list cac gia tri thickness (float) trong 1 o.

    Ho tro:
      - O trong / NaN                -> []
      - So don (12.3)                -> [12.3]
      - Chuoi nhieu gia tri, cach     -> [1.0, 2.0, 3.0, 4.0]
        boi ',' hoac ';' (vi du "1,2,3,4" hoac "1.5; 2.5")
    Cac token rong hoac khong parse duoc thanh so se bi bo qua (co canh bao).
    """
    if pd.isna(cell):
        return []

    if isinstance(cell, (int, float)):
        return [float(cell)]

    s = str(cell).strip()
    if not s:
        return []

    # tach theo ',' hoac ';'
    tokens = [t.strip() for t in re.split(r"[;,]", s)]
    values = []
    for t in tokens:
        if not t:
            continue
        try:
            values.append(float(t))
        except ValueError:
            print(f"[CANH BAO] Khong doc duoc gia tri thickness '{t}' trong o '{s}' - bo qua.")
    return values


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("excel", help="Duong dan file Excel (Energy | s1 | s2 | ... )")
    ap.add_argument("--sheet", default=0, help="Ten hoac index sheet (mac dinh: sheet dau tien)")
    ap.add_argument("--outdir", default="./mac_output", help="Thu muc xuat file .mac")
    ap.add_argument("--beamon", type=int, default=1_000_000, help="So hat cho /run/beamOn")
    args = ap.parse_args()

    df = pd.read_excel(args.excel, sheet_name=args.sheet, dtype=object)
    df.columns = [str(c).strip() for c in df.columns]

    if COL_ENERGY not in df.columns:
        print(f"[LOI] Khong thay cot '{COL_ENERGY}'. Cac cot hien co: {list(df.columns)}")
        print("      -> Sua COL_ENERGY trong file script cho khop.")
        sys.exit(1)

    sample_cols = [c for c in df.columns if c != COL_ENERGY]
    if not sample_cols:
        print("[LOI] Khong tim thay cot sample nao (s1, s2, ...).")
        sys.exit(1)

    outdir = Path(args.outdir)
    outdir.mkdir(parents=True, exist_ok=True)

    # per_sample[sample_tag] = list ten file .mac, theo thu tu energy tang dan
    per_sample = {sample_tag(c): [] for c in sample_cols}

    df_sorted = df.sort_values(by=COL_ENERGY)
    for _, row in df_sorted.iterrows():
        if pd.isna(row[COL_ENERGY]):
            continue
        energy = float(row[COL_ENERGY])
        e_tag = fmt_tag(energy)

        for col in sample_cols:
            thickness_list = parse_thickness_list(row[col])
            if not thickness_list:
                continue
            s_tag = sample_tag(col)

            for thickness in thickness_list:
                d_tag = fmt_tag(thickness)

                base = f"{s_tag}_D{d_tag}_E{e_tag}"
                mac_name = f"run_{base}.mac"
                hist_name = f"hist_{base}"

                content = MAC_TEMPLATE.format(
                    material=s_tag, thickness=thickness, energy=energy,
                    histname=hist_name, beamon=args.beamon
                )
                (outdir / mac_name).write_text(content)
                per_sample[s_tag].append(mac_name)

    total = sum(len(v) for v in per_sample.values())
    if total == 0:
        print("[LOI] Khong tao duoc file nao - kiem tra lai du lieu Excel.")
        sys.exit(1)

    # run_all_{sample}.mac - rieng cho tung sample, theo thu tu energy tang dan
    master_lines = ["/control/verbose 2\n"]
    for s_tag, mac_list in per_sample.items():
        if not mac_list:
            continue
        run_all_name = f"run_all_{s_tag}.mac"
        with open(outdir / run_all_name, "w") as f:
            f.write("/control/verbose 2\n")
            for n in mac_list:
                f.write(f"/control/execute {n}\n")
        master_lines.append(f"/control/execute {run_all_name}\n")

    # run_all.mac tong: goi lan luot run_all cua tung sample
    with open(outdir / "run_all.mac", "w") as f:
        f.writelines(master_lines)

    print(f"[DONE] Da tao {total} file .mac cho {len(sample_cols)} sample.")
    print(f"       run_all_S1.mac ... run_all_S6.mac + run_all.mac (tong) trong: {outdir.resolve()}")


if __name__ == "__main__":
    main()
