#!/bin/bash
# run_all_isolated.sh
#
# Chay TUNG file run_S{mat}_D{thick}_E{energy}.mac trong 1 TIEN TRINH RIENG.
# Ho tro truong hop file .mac va file thuc thi (Sim.exe) nam O 2 THU MUC KHAC NHAU.

# ---- SUA 2 DUONG DAN NAY CHO DUNG VOI MAY BAN ----
# LUU Y: dang chay trong WSL nen phai dung duong dan kieu /mnt/c/... 
# (khong dung C:\... vi bash khong hieu duoc)
MAC_DIR="/mnt/c/2026.9.28_Paper10/Geant4_mac/build/macros"
EXE="/mnt/c/2026.9.28_Paper10/Geant4_mac/build/Release/Sim.exe"
# ---------------------------------------------------

cd "$MAC_DIR" || { echo "Khong tim thay thu muc MAC_DIR: $MAC_DIR"; exit 1; }
[ -x "$EXE" ] || { echo "Khong tim thay hoac khong chay duoc EXE: $EXE"; exit 1; }

mkdir -p tmp_wrap

for f in run_S[0-9]*_H*_E*.mac; do
    [ -e "$f" ] || continue

    base=$(basename "$f" .mac)
    wrap="tmp_wrap/wrap_${base}.mac"

    # cwd hien tai la MAC_DIR, nen "${f}" (ten file tuong doi) van dung binh thuong
    cat > "$wrap" << MAC
/run/initialize
/control/execute ${f}
MAC

    echo ">>> Running ${f} ..."
    # Goi EXE bang duong dan tuyet doi, nhung dang chay/cwd la MAC_DIR
    # -> moi file .root, .csv sinh ra se nam trong MAC_DIR (dung nhu truoc gio)
    "$EXE" "$wrap"
done

echo "Done. Xem ket qua trong ${MAC_DIR}/process_results.csv"