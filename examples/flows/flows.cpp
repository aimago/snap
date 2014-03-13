#include "stdafx.h"
#include <omp.h>

int BuildCapacityNetwork(const TStr& InFNm, PNEANet &Net, const int& SrcColId = 0, const int& DstColId = 1, const int& CapColId = 2) {
  TSsParser Ss(InFNm, ssfWhiteSep, true, true, true);
  TRnd Random;
  Net.Clr();
  Net = TNEANet::New();;
  int SrcNId, DstNId, CapVal, EId;
  int MaxCap = 0;
  while (Ss.Next()) {
    if (! Ss.GetInt(SrcColId, SrcNId) || ! Ss.GetInt(DstColId, DstNId)) { continue; }
    //Ss.GetInt(CapColId, CapVal);
    CapVal = Random.GetUniDevInt(1, 10000);
    MaxCap = max(CapVal, MaxCap);
    if (! Net->IsNode(SrcNId)) {
      Net->AddNode(SrcNId);
    }
    if (! Net->IsNode(DstNId)) {
      Net->AddNode(DstNId);
    }
    EId = Net->AddEdge(SrcNId, DstNId);
    Net->AddIntAttrDatE(EId, CapVal, TSnap::CapAttrName);
  }
  Net->Defrag();
  return MaxCap;
}

double getcputime() {
  struct rusage rusage;
  double result;
  getrusage(RUSAGE_SELF, &rusage);
  result =
    ((double) (rusage.ru_utime.tv_usec + rusage.ru_stime.tv_usec) / 1000000) +
    ((double) (rusage.ru_utime.tv_sec + rusage.ru_stime.tv_sec));
  return result;
}

int main(int argc, char* argv[]) {
  Env = TEnv(argc, argv, TNotify::StdNotify);
  Env.PrepArgs(TStr::Fmt("Flow. build: %s, %s. Time: %s", __TIME__, __DATE__, TExeTm::GetCurTm()));
  double NetPRTimeSum = 0;
  double NetEKTimeSum = 0;
  int NumWins = 0;
  Try
  const TStr InFNm = Env.GetIfArgPrefixStr("-i:", "small_bin", "Input file");
  //const TStr OutFNm = Env.GetIfArgPrefixStr("-o:", "small_bin", "Output file");
  const int Iters = Env.GetIfArgPrefixInt("-n:", 10, "Number of runs per thread");
  const int Threads = Env.GetIfArgPrefixInt("-t:", 4, "Number of threads");
  printf("Integer Flow Test\n");
  printf("Filename: %s\n", InFNm.CStr());
  printf("Building Network...\n");
  TFIn InFile(InFNm);
  PNEANet Net = TNEANet::Load(InFile);
  //int MaxEdgeCap = BuildCapacityNetwork(InFNm, Net);
  //TFOut OutFile(OutFNm);
  //Net->Save(OutFile);
  printf("PNEANet Nodes: %d, Edges: %d\n\n", Net->GetNodes(), Net->GetEdges());
  #pragma omp parallel for reduction(+:NetEKTimeSum,NetPRTimeSum,NumWins) schedule(static, 1)
  for (int t = 0; t < Threads; t++) {
    TRnd Random(t);
    for (int i = 0; i < Iters; i++) {
      int SrcNId = Net->GetRndNId(Random);
      int SnkNId = Net->GetRndNId(Random);

      //double PRBeginTime = getcputime();
      //int NetMaxFlowPR = TSnap::GetMaxFlowIntPR(Net, SrcNId, SnkNId);
      //double PREndTime = getcputime();
      //double NetPRFlowRunTime = PREndTime - PRBeginTime;

      double EKBeginTime = getcputime();
      int NetMaxFlowEK = TSnap::GetMaxFlowIntEK(Net, SrcNId, SnkNId);
      double EKEndTime = getcputime();
      double NetEKFlowRunTime = EKEndTime - EKBeginTime;
      
      //IAssert(NetMaxFlowPR == NetMaxFlowEK);

      //if (NetPRFlowRunTime < NetEKFlowRunTime) { NumWins++; }

      //NetPRTimeSum += NetPRFlowRunTime;
      NetEKTimeSum += NetEKFlowRunTime;
      
      #pragma omp critical
      {
        printf("Thread: %d\n", omp_get_thread_num());
        printf("Source: %d, Sink %d\n", SrcNId, SnkNId);
        printf("Max Flow: %d\n", NetMaxFlowEK);
        //printf("PR CPU Time: %f\n", NetPRFlowRunTime);
        printf("EK CPU Time: %f\n", NetEKFlowRunTime);
        printf("\n");
      }
    }
  }
  int TotalRuns = Iters*Threads;
  //printf ("Avg PR PNEANet Time: %f\n", NetPRTimeSum/TotalRuns);
  printf ("Avg EK PNEANet Time: %f\n", NetEKTimeSum/TotalRuns);
  //printf ("%d out of %d PR was faster\n", NumWins, TotalRuns);
  Catch
  return 0;
}

