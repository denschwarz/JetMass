#include "../include/CentralInclude.h"

using namespace std;

void fill_hists_top(TString dir, TString process);
void fill_hists_W(TString dir, TString process);
bool passN2ddt(double n2, double pt, double mass);

void read_grid(TString gfilename);

TFile* outputFile;
vector<TString> handlenames;
TH2F* ddtmap;



int main(int argc, char* argv[]){

  bool fill_top = true;
  bool fill_W = true;

  if(argc > 1){
    if(strcmp(argv[1], "top") == 0) fill_W=false;
    else if(strcmp(argv[1], "W") == 0) fill_top=false;
  }

  read_grid("../Histograms/grid.root");

  // read in ddtmap here
  TString ddtMapFile = "../macros/scripts/maps/fixedPU/QCD_2017_smooth_gaus4p00sigma.root";
  TString ddtMapName = "N2_v_pT_v_rho_0p05_smooth_gaus4p00sigma_maps_cleaner";
  TFile * mapFile = new TFile(ddtMapFile);
  ddtmap = (TH2F*)mapFile->Get(ddtMapName);
  
  TString outname = "Histograms.root";
  outputFile = new TFile(outname,"recreate");

  TString histdir_W = "/nfs/dust/cms/user/albrechs/UHH2/JetMassOutput/WMassTrees/merged/";
  TString histdir_top = "../Histograms/top/";

  vector<TString> processes_W = {"Data", "W", "QCD", "Pseudo"};
  vector<TString> processes_top = {"Data", "TTbar", "SingleTop", "WJets", "other", "Pseudo"};

  vector<TFile*> files_W, files_top;
  
  if(fill_top){
    for(auto process: processes_top) fill_hists_top(histdir_top, process);
  }
  if(fill_W){
    for(auto process: processes_W) fill_hists_W(histdir_W, process);
  }


  outputFile->Close();
  return 0;
}


//------------------------------------------------------
//------------------------------------------------------
//------------------------------------------------------

void fill_hists_top(TString dir, TString process){
  cout << "filling hists for " << process << " (top) ..." << endl;
  TFile *file = new TFile(dir+process+".root");
  TTree *tree = (TTree *) file->Get("AnalysisTree");

  // creat a dummy hist for mjet
  TString dummyname = "top_"+process+"__mjet";
  TH1F* h_mjet_dummy = new TH1F(dummyname, "m_{jet} [GeV]", 50, 0, 500);

  // define pt bins
  vector<int> ptbins = {-1, 200, 300, 400, 100000};
  vector<TString> binnames = {"inclusive", "200to300", "300to400", "400toInf"};

  // create vectors of mjet hists
  vector<TH1F*> h_mjet_nominal;
  vector<vector<vector<TH1F*>>> h_mjet_vars;
  h_mjet_vars.resize(ptbins.size()-1);

  for(int i=0; i<ptbins.size()-1; i++){
    TH1F* h = (TH1F*)h_mjet_dummy->Clone(dummyname+"_"+binnames[i]);
    h_mjet_nominal.push_back(h);
  }

  // declare variables
  double weight, mjet, pt;
  vector<vector<double>*> mjet_variations;
  mjet_variations.resize(handlenames.size());

  // assign to branches
  tree->ResetBranchAddresses();
  tree->SetBranchAddress("weight",&weight);
  tree->SetBranchAddress("mjet",&mjet);
  tree->SetBranchAddress("pt",&pt);
  if(process != "Data"){
    for(unsigned int i=0; i<handlenames.size(); i++){
      tree->SetBranchAddress(handlenames[i],&mjet_variations[i]);
      for(int ptbin=0; ptbin<ptbins.size()-1; ptbin++){
        TH1F* h_up = (TH1F*)h_mjet_nominal[ptbin]->Clone();
        TH1F* h_down = (TH1F*)h_mjet_nominal[ptbin]->Clone();
        TString oldtitle = h_mjet_nominal[ptbin]->GetName();
        TString newtitle = oldtitle.ReplaceAll("mjet", handlenames[i]);
        h_up->SetName(newtitle+"__up");
        h_down->SetName(newtitle+"__down");
        h_mjet_vars[ptbin].push_back({h_up, h_down});
      }
    }
  }

  tree->SetBranchStatus("*",1);

  // loop over tree
  for(Int_t ievent=0; ievent < tree->GetEntriesFast(); ievent++) {
    if(tree->GetEntry(ievent)<=0) break;

    // loop over pt bins
    for(int ptbin=0; ptbin<ptbins.size()-1; ptbin++){
      if(ptbins[ptbin] == -1 || (pt > ptbins[ptbin] && pt < ptbins[ptbin+1]) ){
        // fill nominal hists
        h_mjet_nominal[ptbin]->Fill(mjet, weight);
        // mjet variations
        if(process != "Data"){
          for(int i=0; i<h_mjet_vars[ptbin].size(); i++){
            for(int j=0; j<h_mjet_vars[ptbin][i].size(); j++){
              h_mjet_vars[ptbin][i][j]->Fill(mjet_variations[i]->at(j), weight);
            }
          }
        }
      }
    }
  }

  // write hists
  outputFile->cd();
  for(int ptbin=0; ptbin<ptbins.size()-1; ptbin++){
    h_mjet_nominal[ptbin]->Write();
    for(int i=0; i<h_mjet_vars[ptbin].size(); i++){
      for(int j=0; j<h_mjet_vars[ptbin][i].size(); j++){
        h_mjet_vars[ptbin][i][j]->Write();
      }
    }
  }


  return;
}

//------------------------------------------------------
//------------------------------------------------------
//------------------------------------------------------

void fill_hists_W(TString dir, TString process){
  cout << "filling hists for " << process << " (W) ..." << endl;
  TFile *file = new TFile(dir+process+".root");
  TTree *tree = (TTree *) file->Get("AnalysisTree");

  // creat a dummy hist for mjet
  TString dummyname = "W_"+process+"__mjet";
  TH1F* h_mjet_dummy = new TH1F(dummyname, "m_{jet} [GeV]", 50, 0, 500);

  // define pt bins
  vector<int> ptbins = {-1, 500, 550, 600, 675, 800, 1200, 100000};
  vector<TString> binnames = {"inclusive", "500to550", "550to600", "600to675", "675to800", "800to1200", "1200toInf"};

  // create vectors of mjet hists
  vector<TH1F*> h_mjet_nominal_pass, h_mjet_nominal_fail;
  vector<vector<vector<TH1F*>>> h_mjet_vars_pass, h_mjet_vars_fail;
  h_mjet_vars_pass.resize(ptbins.size()-1);
  h_mjet_vars_fail.resize(ptbins.size()-1);

  for(int i=0; i<ptbins.size()-1; i++){
    TH1F* h1 = (TH1F*)h_mjet_dummy->Clone(dummyname+"_"+binnames[i]+"_pass");
    TH1F* h2 = (TH1F*)h_mjet_dummy->Clone(dummyname+"_"+binnames[i]+"_fail");
    h_mjet_nominal_pass.push_back(h1);
    h_mjet_nominal_fail.push_back(h2);
  }

  // declare variables
  double weight, mjet, pt, n2;
  vector<vector<double>*> mjet_variations;
  mjet_variations.resize(handlenames.size());

  // assign to branches
  tree->ResetBranchAddresses();
  tree->SetBranchAddress("weight",&weight);
  tree->SetBranchAddress("mjet",&mjet);
  tree->SetBranchAddress("pt",&pt);
  tree->SetBranchAddress("N2",&n2);

  if(process != "Data"){
    for(unsigned int i=0; i<handlenames.size(); i++){
      tree->SetBranchAddress(handlenames[i],&mjet_variations[i]);
      for(int ptbin=0; ptbin<ptbins.size()-1; ptbin++){
        TH1F* h_up_pass = (TH1F*)h_mjet_nominal_pass[ptbin]->Clone();
        TH1F* h_down_pass = (TH1F*)h_mjet_nominal_pass[ptbin]->Clone();
        TH1F* h_up_fail = (TH1F*)h_mjet_nominal_fail[ptbin]->Clone();
        TH1F* h_down_fail = (TH1F*)h_mjet_nominal_fail[ptbin]->Clone();
        TString oldtitle_pass = h_mjet_nominal_pass[ptbin]->GetName();
        TString newtitle_pass = oldtitle_pass.ReplaceAll("mjet", handlenames[i]);
        TString oldtitle_fail = h_mjet_nominal_fail[ptbin]->GetName();
        TString newtitle_fail = oldtitle_fail.ReplaceAll("mjet", handlenames[i]);
        h_up_pass->SetName(newtitle_pass+"__up");
        h_down_pass->SetName(newtitle_pass+"__down");
        h_up_fail->SetName(newtitle_fail+"__up");
        h_down_fail->SetName(newtitle_fail+"__down");
        h_mjet_vars_pass[ptbin].push_back({h_up_pass, h_down_pass});
        h_mjet_vars_fail[ptbin].push_back({h_up_fail, h_down_fail});

      }
    }
  }

  tree->SetBranchStatus("*",1);

  // loop over tree
  for(Int_t ievent=0; ievent < tree->GetEntriesFast(); ievent++) {
    if(tree->GetEntry(ievent)<=0) break;
    if(ievent % 10000 == 0) cout << "\r processing Event ("<< ievent << "/" << tree->GetEntriesFast() << ")"<< flush;
    // loop over pt bins
    for(int ptbin=0; ptbin<ptbins.size()-1; ptbin++){
      if(ptbins[ptbin] == -1 || (pt > ptbins[ptbin] && pt < ptbins[ptbin+1]) ){
        // fill nominal hists
        if(passN2ddt(n2, pt, mjet)) h_mjet_nominal_pass[ptbin]->Fill(mjet, weight);
        else                        h_mjet_nominal_fail[ptbin]->Fill(mjet, weight);

        // mjet variations
        if(process != "Data"){
          for(int i=0; i<h_mjet_vars_pass[ptbin].size(); i++){
            for(int j=0; j<h_mjet_vars_pass[ptbin][i].size(); j++){
              if(passN2ddt(n2, pt, mjet)) h_mjet_vars_pass[ptbin][i][j]->Fill(mjet_variations[i]->at(j), weight);
              else                        h_mjet_vars_fail[ptbin][i][j]->Fill(mjet_variations[i]->at(j), weight);
            }
          }
        }
      }
    }
  }

  // write hists
  outputFile->cd();
  for(int ptbin=0; ptbin<ptbins.size()-1; ptbin++){
    h_mjet_nominal_pass[ptbin]->Write();
    h_mjet_nominal_fail[ptbin]->Write();

    for(int i=0; i<h_mjet_vars_pass[ptbin].size(); i++){
      for(int j=0; j<h_mjet_vars_pass[ptbin][i].size(); j++){
        h_mjet_vars_pass[ptbin][i][j]->Write();
        h_mjet_vars_fail[ptbin][i][j]->Write();
      }
    }
  }
  return;
}

//------------------------------------------------------
//------------------------------------------------------
//------------------------------------------------------

void read_grid(TString gfilename){
  // read configuration from root file
  TFile* gfile = new TFile(gfilename);
  TH2F* grid = (TH2F*) gfile->Get("grid");
  TH1F* h_cat = (TH1F*) gfile->Get("categories");
  vector<TString> categories;
  for(int bin=1; bin<=h_cat->GetXaxis()->GetNbins(); bin++) categories.push_back(h_cat->GetXaxis()->GetBinLabel(bin));

  // get binnings and size of variation
  int Nbins_pt =  grid->GetXaxis()->GetNbins();
  int Nbins_eta = grid->GetYaxis()->GetNbins();
  int Nbins_cat = categories.size();

  // name and create all handles for mjet variations
  for(int i=0; i<Nbins_pt; i++){
    for(int j=0; j<Nbins_eta; j++){
      for(int k=0; k<Nbins_cat; k++){
        TString bin_name = "_" + to_string(i) + "_" + to_string(j) + "_" + categories[k];
        TString handlename = "mjet"+bin_name;
        handlenames.push_back(handlename);
      }
    }
  }
  return;
}

//------------------------------------------------------
//------------------------------------------------------
//------------------------------------------------------

bool passN2ddt(double n2, double pt, double mass){
  bool pass = false;

// deriving bin for pt and rho
  int pt_bin = ddtmap->GetYaxis()->FindFixBin(pt);
  if(pt_bin > ddtmap->GetYaxis()->GetNbins()){
    pt_bin = ddtmap->GetYaxis()->GetNbins();
  }else if(pt_bin <=0){
    pt_bin = 1;
  }

  double rho = 2 * TMath::Log(mass/pt);
  int rho_bin = ddtmap->GetXaxis()->FindFixBin(rho);
  if(rho_bin > ddtmap->GetXaxis()->GetNbins()){
    rho_bin = ddtmap->GetXaxis()->GetNbins();
  }else if(rho_bin <= 0){
    rho_bin = 1;
  }

  double N2ddt_map_value = ddtmap->GetBinContent(rho_bin,pt_bin);

  if(n2 < N2ddt_map_value) pass = true;

  return pass;
}

// double derive_kfactor(double pt){

// }
