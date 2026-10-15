#include <TFile.h>
#include <TTree.h>
#include <TH1D.h>
#include <TRandom3.h>
#include <TMath.h>
#include <TString.h>
#include <TObjArray.h>
#include <TObjString.h>
#include <TSystem.h>
#include <TSystemDirectory.h>
#include <TList.h>
#include <TSystemFile.h>
#include <iostream>
#include <fstream>
#include <map>
#include <set>
#include <tuple>
#include <vector>
#include <utility>
#include <iomanip>

// ── CẤU HÌNH ─────────────────────────────────────────────────────────────────
const double ROI_NSIGMA = 3.0;   // N_peak: đếm trong ± ROI_NSIGMA * sigma

// ── Độ phân giải NaI(Tl): FWHM (keV) = A + B*sqrt(E) + C*E ─────────────────
double GetFWHM(double E_keV) {
    const double A = -3.3, B = 1.847, C = 0.0001911;
    double fwhm = A + B * TMath::Sqrt(E_keV) + C * E_keV;
    return (fwhm > 0.0) ? fwhm : 1.0;
}

// ── Parse tên file: hist_<Sample>_D<D_str>_E<E_str>.root ─────────────────────
// Vd: hist_S1_D0p01_E20.root, hist_S1_D0p002_E15.root, hist_S6_D0p01_E20.root
// Quy uoc 'p' = dau thap phan:  D0p01 -> D = 0.01,  E20 -> E = 20 (keV)
bool parse_filename(TString fname, TString& Sample, double& D_val, double& E_keV) {
    fname = gSystem->BaseName(fname);
    Ssiz_t posDot = fname.Last('.');
    if (posDot != kNPOS) fname = fname(0, posDot);  // bo extension (.root)

    TObjArray* parts = fname.Tokenize("_");
    int n = parts->GetEntries();
    if (n < 4) { delete parts; return false; }

    TString first = ((TObjString*)parts->At(0))->GetString();
    TString dTok  = ((TObjString*)parts->At(n - 2))->GetString();
    TString eTok  = ((TObjString*)parts->At(n - 1))->GetString();

    if (!first.EqualTo("hist") || !dTok.BeginsWith("D") || !eTok.BeginsWith("E")) {
        delete parts;
        return false;
    }

    // Sample = cac token o giua "hist" va "D..." (phong khi Sample co dau '_')
    TString sampleStr;
    for (int i = 1; i <= n - 3; ++i) {
        if (i > 1) sampleStr += "_";
        sampleStr += ((TObjString*)parts->At(i))->GetString();
    }
    delete parts;
    if (sampleStr.IsNull()) return false;
    Sample = sampleStr;

    TString D_str = dTok(1, dTok.Length() - 1);
    D_str.ReplaceAll("p", ".");
    TString E_str = eTok(1, eTok.Length() - 1);
    E_str.ReplaceAll("p", ".");

    D_val = D_str.Atof();
    E_keV = E_str.Atof();
    return (E_keV > 0.0);
}

// ── Xử lý một file ROOT ──────────────────────────────────────────────────────
struct Result {
    TString  Sample  = "";
    double   D_val   = 0;
    double   E_keV   = 0;
    Long64_t N_total = 0;  // tổng số hạt ghi nhận được (mọi entry trong tree)
    Long64_t N_peak  = 0;  // số hạt trong vùng ± ROI_NSIGMA*sigma quanh E_incident
                           // sau khi áp smearing độ phân giải detector
};

Result process_file(TString path) {
    Result r;
    if (!parse_filename(path, r.Sample, r.D_val, r.E_keV)) {
        std::cerr << "[SKIP] Cannot parse: " << path << "\n";
        return r;
    }

    TFile* f = TFile::Open(path);
    if (!f || f->IsZombie()) {
        std::cerr << "[SKIP] Cannot open: " << path << "\n";
        return r;
    }

    TTree* tree = (TTree*)f->Get("Hits");
    if (!tree) {
        std::cerr << "[SKIP] No 'Hits' tree in: " << path << "\n";
        f->Close(); delete f; return r;
    }

    Double_t edep = 0;
    if      (tree->GetBranch("Edep_keV"))      tree->SetBranchAddress("Edep_keV",      &edep);
    else if (tree->GetBranch("E_smeared_keV")) tree->SetBranchAddress("E_smeared_keV", &edep);
    else {
        std::cerr << "[SKIP] No energy branch in: " << path << "\n";
        f->Close(); delete f; return r;
    }

    // N_total = tổng số hạt detector ghi nhận
    r.N_total = tree->GetEntries();

    // Histogram để smear: phủ ± 50% quanh E_incident
    double histMin = r.E_keV * 0.5;
    double histMax = r.E_keV * 1.5;
    TH1D* h = new TH1D("hSmear", "", 2000, histMin, histMax);
    h->SetDirectory(0);

    // ROI của photopeak
    double sigma_peak = GetFWHM(r.E_keV) / 2.355;
    double minROI = r.E_keV - ROI_NSIGMA * sigma_peak;
    double maxROI = r.E_keV + ROI_NSIGMA * sigma_peak;

    TRandom3 rng(42);
    Long64_t nEntries = tree->GetEntries();
    for (Long64_t i = 0; i < nEntries; ++i) {
        tree->GetEntry(i);
        if (edep <= 0) continue;
        // Smear theo độ phân giải detector tại năng lượng deposit thực
        double sigma_hit = GetFWHM(edep) / 2.355;
        h->Fill(edep + rng.Gaus(0.0, sigma_hit));
    }

    // N_peak = integral trong cửa sổ ROI
    r.N_peak = (Long64_t)h->Integral(h->FindBin(minROI), h->FindBin(maxROI));

    delete h;
    f->Close(); delete f;
    return r;
}

// ── ENTRY POINT ──────────────────────────────────────────────────────────────
void Count() {
    std::cout << "=== MAC Count ===\n"
              << "N_total : tong so hat detector ghi nhan\n"
              << "N_peak  : so hat trong +/-" << ROI_NSIGMA
              << " sigma quanh E_incident (sau smearing)\n\n";

    // key = (Sample, D, E_keV) -> tu dong sort
    std::map<std::tuple<std::string,double,double>, Result> results;

    TSystemDirectory dir(".", ".");
    TList* files = dir.GetListOfFiles();
    if (!files) { std::cerr << "No files found.\n"; return; }

    TIter next(files);
    TSystemFile* sf;
    while ((sf = (TSystemFile*)next())) {
        TString fname = sf->GetName();
        if (sf->IsDirectory()) continue;
        if (!fname.EndsWith(".root")) continue;
        if (!fname.BeginsWith("hist_")) continue;

        Result r = process_file(fname);
        if (r.E_keV > 0)
            results[std::make_tuple(std::string(r.Sample.Data()), r.D_val, r.E_keV)] = r;
    }

    if (results.empty()) {
        std::cerr << "[ERROR] No valid files processed.\n";
        return;
    }

    // ── Gom danh sach Sample (cot) va (D, E_keV) (hang) de pivot bang ───────
    std::set<std::string>            sampleSet;
    std::set<std::pair<double,double>> rowSet;   // (D, E_keV)
    std::map<std::tuple<double,double,std::string>, Long64_t> npeakOf; // (D,E,Sample) -> N_peak

    for (auto& [key, r] : results) {
        std::string sName = std::get<0>(key);
        double      dVal  = std::get<1>(key);
        double      eVal  = std::get<2>(key);
        sampleSet.insert(sName);
        rowSet.insert(std::make_pair(dVal, eVal));
        npeakOf[std::make_tuple(dVal, eVal, sName)] = r.N_peak;
    }

    std::vector<std::string> samples(sampleSet.begin(), sampleSet.end());
    int colW = 12;

    // ── In ra man hinh: hang ngang la ten Sample, ben duoi la N_peak ────────
    std::cout << std::left << std::setw(10) << "D" << std::setw(10) << "E_keV";
    for (auto& s : samples) std::cout << std::right << std::setw(colW) << s;
    std::cout << "\n" << std::string(20 + colW * (int)samples.size(), '-') << "\n";

    // ── Ghi file CSV cung dinh dang ──────────────────────────────────────────
    std::ofstream out("MAC_Results.csv");
    out << "D,E_keV";
    for (auto& s : samples) out << "," << s;
    out << "\n";

    for (auto& [d, e] : rowSet) {
        std::cout << std::left << std::fixed << std::setprecision(6)
                   << std::setw(10) << d
                   << std::setprecision(4) << std::setw(10) << e;
        out << std::fixed << std::setprecision(6) << d << "," << std::setprecision(6) << e;

        for (auto& s : samples) {
            auto it = npeakOf.find(std::make_tuple(d, e, s));
            if (it != npeakOf.end()) {
                std::cout << std::right << std::setw(colW) << it->second;
                out << "," << it->second;
            } else {
                std::cout << std::right << std::setw(colW) << "-";
                out << ",";
            }
        }
        std::cout << "\n";
        out << "\n";
    }
    out.close();

    std::cout << "\n[DONE] " << results.size()
              << " files -> MAC_Results.csv\n";
}
