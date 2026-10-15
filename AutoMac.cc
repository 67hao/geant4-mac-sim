#include <fstream>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <cmath>

// ----------------------------------------------------------------------------
// Chuyển số thực thành chuỗi dùng 'p' thay dấu thập phân, bỏ trailing zeros.
//
// precision: số chữ số sau dấu thập phân tối đa khi cần.
//   Dùng 6 cho energy (keV): đủ cho 0.001 eV
//   Dùng 9 cho distance/thickness (mm): đủ cho 0.001 nm (= 1e-9 mm / 1e-6 mm)
//
// Ví dụ (precision=9):
//   300.0       -> "300"
//   0.000005    -> "0p000005"     (5 nm)
//   0.001       -> "0p001"        (1 um)
//   0.5         -> "0p5"          (0.5 mm)
//   1.5         -> "1p5"
// ----------------------------------------------------------------------------
std::string formatValue(double value, int precision = 9) {
    std::ostringstream oss;

    double intpart;
    if (std::modf(value, &intpart) == 0.0) {
        oss << static_cast<long long>(value);
    }
    else {
        oss << std::fixed << std::setprecision(precision) << value;
        std::string s = oss.str();
        s.erase(s.find_last_not_of('0') + 1);
        if (s.back() == '.') s.pop_back();
        std::replace(s.begin(), s.end(), '.', 'p');
        return s;
    }

    std::string s = oss.str();
    std::replace(s.begin(), s.end(), '.', 'p');
    return s;
}

// ----------------------------------------------------------------------------
// Chọn bề dày mẫu tối ưu theo năng lượng photon (nhắm T ≈ 0.1, ~2 MFP).
// Bảng tham chiếu dựa trên MFP của Ti2AlC (vật liệu nhẹ nhất trong bộ 8).
//
// Kết quả:    D (mm)     = bề dày mẫu ghi vào tên file và macro
// Khoảng cách mẫu–detector: hằng số DETECTOR_DISTANCE_MM (không đưa vào tên file)
// ----------------------------------------------------------------------------
double getSampleThickness_mm(double energy_keV) {
    if (energy_keV < 2.0) return 0.0001;   // 100 nm  — E < 2 keV
    if (energy_keV < 5.0) return 0.0005;   // 500 nm  — 2–5 keV
    if (energy_keV < 10.0) return 0.002;    //   2 um  — 5–10 keV
    if (energy_keV < 20.0) return 0.02;     //  20 um  — 10–20 keV
    if (energy_keV < 50.0) return 0.1;      // 0.1 mm  — 20–50 keV
    if (energy_keV < 150.0) return 0.5;      // 0.5 mm  — 50–150 keV
    if (energy_keV < 500.0) return 2.0;      //   2 mm  — 150–500 keV
    if (energy_keV < 2000.0) return 5.0;      //   5 mm  — 500–2000 keV
    return 10.0;                               //  10 mm  — >= 2000 keV
}

int main() {
    // ------------------------------------------------------------------------
    // Khoảng cách mẫu–detector: cố định, không đưa vào tên file.
    // D trong tên file = bề dày mẫu, thay đổi theo năng lượng.
    // ------------------------------------------------------------------------
    const double DETECTOR_DISTANCE_MM = 300.0;

    // ------------------------------------------------------------------------
    // Danh sách năng lượng (keV) — bao phủ toàn dải từ 1 keV đến 4 MeV
    // ------------------------------------------------------------------------
    const std::vector<double> energies = {
        // Vùng thấp (1–10 keV): bề dày nm–um, MFP rất nhỏ
        1.0, 1.5, 2.0, 3.0, 4.0, 5.0, 6.0, 8.0,
        // Vùng trung thấp (10–100 keV)
        10.0, 15.0, 20.0, 30.0, 40.0, 50.0, 60.0, 80.0,
        // Vùng trung (100–500 keV)
        100.0, 150.0, 200.0, 300.0, 400.0, 500.0,
        // Vùng cao (500 keV–4 MeV)
		600.0, 800.0, 1000.0, 1500.0, 2000.0, 3000.0, 4000.0, 5000.0, 6000.0, 8000.0, 10000.0, 15000.0
    };

    // ------------------------------------------------------------------------
    // Tạo file macro chủ
    // ------------------------------------------------------------------------
    std::ofstream master(std::string(MACROPATH) + "/run_all.mac");
    if (!master) {
        std::cerr << "=== Failed to create master macro file ===" << std::endl;
        return 1;
    }
    master << "/control/verbose 2\n";

    // ------------------------------------------------------------------------
    // Sinh macro riêng cho từng năng lượng
    // ------------------------------------------------------------------------
    for (double energy : energies) {
        double thickness = getSampleThickness_mm(energy);

        // D_str: bề dày mẫu (mm), precision=9 để biểu diễn đến ~0.001 nm
        // E_str: năng lượng (keV), precision=6 để biểu diễn đến 0.001 eV
        std::string D_str = formatValue(thickness, 9);
        std::string E_str = formatValue(energy, 6);

        std::string filename = "run_D" + D_str + "_E" + E_str + ".mac";

        std::ofstream macfile(std::string(MACROPATH) + "/" + filename);
        if (!macfile) {
            std::cerr << "=== File Generating Failed === " << filename << std::endl;
            continue;
        }

        // Cập nhật bề dày mẫu và khoảng cách mẫu–detector, sau đó rebuild geometry
        macfile << "/DetectorConstruction/Sample_Thickness "
            << std::fixed << std::setprecision(9) << thickness << "\n";
        macfile << "/DetectorConstruction/update\n";

        // Thiết lập súng phát
        macfile << "/gun/energy "
            << std::fixed << std::setprecision(6) << energy << " keV\n";

        // Tên file histogram (D = bề dày mẫu)
        macfile << "/analysis/setFileName hist_D" << D_str << "_E" << E_str << "\n";

        // Chạy sự kiện (10^8 hạt)
        macfile << "/run/beamOn 100000000\n";

        macfile.close();

        master << "/control/execute " << filename << "\n";

        std::cout << "== Generated: " << filename
            << "  (D = " << thickness << " mm, E = " << energy << " keV)"
            << std::endl;
    }

    // ------------------------------------------------------------------------
    // Kết thúc
    // ------------------------------------------------------------------------
    master.close();
    std::cout << "\nAll macros generated at: " << MACROPATH << std::endl;
    std::cout << "Master macro:  run_all.mac" << std::endl;
    std::cout << "Total macros:  " << energies.size() << std::endl;
    std::cout << "Detector dist: " << DETECTOR_DISTANCE_MM << " mm (fixed)" << std::endl;

    std::cout << "\nPress Enter to exit...";
    std::cin.get();

    return 0;
}
