#!/usr/bin/env python3
"""
gen_mac.py - Tu dong tao file .mac cho Geant4 tu file Excel dang bang ngang:

    Energy | s1   | s2   | s3   | s4   | s5   | s6
    15     | 12.3 | 9.8  | ...
    20     | ...
    ...

Cot dau tien la Energy (keV), cac cot con lai la CHIEU CAO COLLIMATOR 2
(Collimator2_Height, don vi mm) can dung khi ban cua tung sample tai nang
luong tuong ung. O trong (khong co gia tri) se duoc bo qua.

LUU Y: Sample_Thickness KHONG con duoc ghi vao .mac nua. Voi cac sample
that (S1-S8), be day Sample gio da CO DINH cung theo tung vat lieu ben
trong code C++ (MyDetectorConstruction::kSampleDims), nen macro khong con
dieu khien duoc gia tri nay nua - chi con Collimator2_Height la thay doi
duoc tu Excel/macro.

Mot o co the chua NHIEU gia tri Collimator2_Height, cach nhau boi dau phay
',' hoac cham phay ';' (vi du: "1,2,3,4" hoac "1.5; 2.5; 3"). Khi do script
se tao mot file .mac rieng cho MOI gia tri trong o do, ung voi cung 1 energy.

QUAN TRONG - TOI UU HINH HOC:
Voi cung 1 to hop (sample, Collimator2_Height), hinh hoc KHONG doi giua
cac nang luong khac nhau. Vi vay Sample_Material / Collimator2_Height /
update CHI duoc ghi 1 LAN duy nhat cho ca nhom (trong file
"run_group_{sample}_H{height}.mac"), thay vi lap lai trong tung file nang
luong -> tranh goi lai ConstructSDandField()/Check overlaps nhieu lan mot
cach lang phi. Cac file "run_{sample}_H{height}_E{energy}.mac" gio chi con
gun/energy, setFileName, beamOn.

Cach dung:
    python3 gen_mac.py data.xlsx --outdir ./mac_output
    python3 gen_mac.py data.xlsx --sheet "Sheet2" --beamon 500000
"""
import argparse
import re
import sys
from pathlib import Path
from collections import defaultdict
import pandas as pd

COL_ENERGY = "Energy"  # ten cot nang luong, sua neu file cua m khac

# File nang luong: chi con phan "ban sung" (khong dong toi hinh hoc)
ENERGY_TEMPLATE = """/gun/energy {energy:.6f} keV
/analysis/setFileName {histname}
/run/beamOn {beamon}
"""

# File nhom: set hinh hoc DUY NHAT 1 LAN cho ca nhom (sample, height),
# roi execute lan luot cac file nang luong con cua nhom do.
GROUP_HEADER_TEMPLATE = """/control/verbose 2

# Set hinh hoc 1 lan duy nhat cho CA nhom (sample={material}, height={height:.9f} mm)
/DetectorConstruction/Sample_Material {material}
/DetectorConstruction/Collimator2_Height {height:.9f}
/DetectorConstruction/update

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


def parse_height_list(cell) -> list:
    """Tra ve list cac gia tri Collimator2_Height (float, mm) trong 1 o.

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
            print(f"[CANH BAO] Khong doc duoc gia tri Collimator2_Height '{t}' trong o '{s}' - bo qua.")
    return values


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("excel", help="Duong dan file Excel (Energy | s1 | s2 | ... )")
    ap.add_argument("--sheet", default=0, help="Ten hoac index sheet (mac dinh: sheet dau tien)")
    ap.add_argument("--outdir", default="./mac_output", help="Thu muc xuat file .mac")
    ap.add_argument("--beamon", type=int, default=100_000_000, help="So hat cho /run/beamOn (mac dinh 10^8)")
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

    # groups[(s_tag, h_tag)] = {"height": float, "energies": [(energy, mac_name), ...]}
    groups = {}
    # thu tu nhom xuat hien theo tung sample, de sau nay build run_all_{sample}.mac
    sample_group_order = defaultdict(list)  # s_tag -> list (s_tag, h_tag) theo thu tu gap

    df_sorted = df.sort_values(by=COL_ENERGY)
    for _, row in df_sorted.iterrows():
        if pd.isna(row[COL_ENERGY]):
            continue
        energy = float(row[COL_ENERGY])
        e_tag = fmt_tag(energy)

        for col in sample_cols:
            height_list = parse_height_list(row[col])
            if not height_list:
                continue
            s_tag = sample_tag(col)

            for height in height_list:
                h_tag = fmt_tag(height)
                key = (s_tag, h_tag)

                base = f"{s_tag}_H{h_tag}_E{e_tag}"
                mac_name = f"run_{base}.mac"
                hist_name = f"hist_{base}"

                content = ENERGY_TEMPLATE.format(
                    energy=energy, histname=hist_name, beamon=args.beamon
                )
                (outdir / mac_name).write_text(content)

                if key not in groups:
                    groups[key] = {"height": height, "energies": []}
                    sample_group_order[s_tag].append(key)
                groups[key]["energies"].append(mac_name)

    total = sum(len(g["energies"]) for g in groups.values())
    if total == 0:
        print("[LOI] Khong tao duoc file nao - kiem tra lai du lieu Excel.")
        sys.exit(1)

    # run_group_{sample}_H{height}.mac - set hinh hoc 1 lan, roi chay het cac nang luong cua nhom
    group_file_by_key = {}
    for (s_tag, h_tag), g in groups.items():
        group_name = f"run_group_{s_tag}_H{h_tag}.mac"
        with open(outdir / group_name, "w") as f:
            f.write(GROUP_HEADER_TEMPLATE.format(material=s_tag, height=g["height"]))
            for n in g["energies"]:
                f.write(f"/control/execute {n}\n")
        group_file_by_key[(s_tag, h_tag)] = group_name

    # run_all_{sample}.mac - goi lan luot cac run_group_* cua sample do (moi group tu update hinh hoc 1 lan)
    master_lines = ["/control/verbose 2\n"]
    for s_tag in sample_group_order:
        run_all_name = f"run_all_{s_tag}.mac"
        with open(outdir / run_all_name, "w") as f:
            f.write("/control/verbose 2\n")
            for key in sample_group_order[s_tag]:
                f.write(f"/control/execute {group_file_by_key[key]}\n")
        master_lines.append(f"/control/execute {run_all_name}\n")

    # run_all.mac tong: goi lan luot run_all cua tung sample
    with open(outdir / "run_all.mac", "w") as f:
        f.writelines(master_lines)

    n_groups = len(groups)
    print(f"[DONE] Da tao {total} file .mac nang luong, gom trong {n_groups} nhom (sample,height).")
    print(f"       Moi nhom chi set hinh hoc (Sample_Material/Collimator2_Height/update) 1 LAN DUY NHAT.")
    print(f"       run_all_S1.mac ... + run_all.mac (tong) trong: {outdir.resolve()}")


if __name__ == "__main__":
    main()
