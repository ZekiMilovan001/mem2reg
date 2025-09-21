#include <vector>
#include <unordered_set>

#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/Pass.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/Constants.h" 
#include "llvm/IR/CFG.h"              
#include "llvm/Support/CommandLine.h" 



using namespace llvm;

static cl::opt<bool> MatfVerbose(
  "matf-verbose", cl::desc("Print brief logs for changed allocas"),
  cl::init(false)
);


namespace {

static void eraseLifetimesFor(AllocaInst *AI) {
  std::vector<Instruction*> Kill;
  for (User *U : AI->users()) {
    if (auto *CI = dyn_cast<CallInst>(U)) {
      if (Function *Callee = CI->getCalledFunction()) {
        auto IID = Callee->getIntrinsicID();
        if (IID == Intrinsic::lifetime_start || IID == Intrinsic::lifetime_end)
          Kill.push_back(CI);
      }
    }
  }
  for (Instruction *I : Kill) I->eraseFromParent();
}



struct MatfSimpleMem2Reg : public FunctionPass {
  static char ID;
  MatfSimpleMem2Reg() : FunctionPass(ID) {}

static bool collectUses(AllocaInst *AI,
                        std::vector<StoreInst*> &Stores,
                        std::vector<LoadInst*>  &Loads) {
  Stores.clear();
  Loads.clear();

  std::vector<Value*> WL{AI};
  std::unordered_set<Value*> Seen{AI};
  auto push = [&](Value *V){ if (Seen.insert(V).second) WL.push_back(V); };

  while (!WL.empty()) {
    Value *Ptr = WL.back(); WL.pop_back();

    for (User *U : Ptr->users()) {
      if (auto *LI = dyn_cast<LoadInst>(U)) {
        if (LI->isVolatile() || LI->isAtomic()) return false;
        if (LI->getPointerOperand() != Ptr) return false;
        Loads.push_back(LI);
        continue;
      }
      if (auto *SI = dyn_cast<StoreInst>(U)) {
        if (SI->isVolatile() || SI->isAtomic()) return false;
        if (SI->getPointerOperand() != Ptr) return false;
        Stores.push_back(SI);
        continue;
      }
      if (auto *CI = dyn_cast<CallInst>(U)) {
        if (Function *Callee = CI->getCalledFunction()) {
          switch (Callee->getIntrinsicID()) {
            case Intrinsic::lifetime_start:
            case Intrinsic::lifetime_end:
              continue;
            default: break;
          }
        }
        return false;
      }
      if (auto *BC  = dyn_cast<BitCastInst>(U))        { push(BC);  continue; }
      if (auto *ASC = dyn_cast<AddrSpaceCastInst>(U))  { push(ASC); continue; }
      if (auto *CE  = dyn_cast<ConstantExpr>(U)) {
        if (CE->isCast()) { push(CE); continue; }
      }

      return false;
    }
  }
  return true;
}

  bool runOnFunction(Function &F) override {
    bool Changed = false;

    DominatorTree &DT = getAnalysis<DominatorTreeWrapperPass>().getDomTree();

    std::vector<AllocaInst*> Allocas;
    for (BasicBlock &BB : F) {
      for (Instruction &I : BB) {
        if (auto *AI = dyn_cast<AllocaInst>(&I)) {
          Allocas.push_back(AI);
        }
      }
    }
    for (AllocaInst *AI : Allocas) {
      if (!AI->isStaticAlloca()) continue;

      std::vector<StoreInst*> Stores;
      std::vector<LoadInst*>  Loads;
      if (!collectUses(AI, Stores, Loads)) continue;

      if (Stores.size() == 1) {
        StoreInst *OnlyStore = Stores[0];
        bool Safe = true;
        for (LoadInst *LI : Loads) {
          if (!DT.dominates(OnlyStore, LI)) { Safe = false; break; }
        }
        if (!Safe) continue;

        Value *StoredVal = OnlyStore->getValueOperand();
        for (LoadInst *LI : Loads) {
          LI->replaceAllUsesWith(StoredVal);
          LI->eraseFromParent(); Changed = true;
        }
        OnlyStore->eraseFromParent(); Changed = true;
        eraseLifetimesFor(AI);
        if (AI->use_empty()) { AI->eraseFromParent(); Changed = true; }
        if (MatfVerbose) errs() << "[matf-mem2reg] single-store promoted " << (AI->hasName()?AI->getName():"<unnamed>") << "\n";
        continue;
      }

    }

    return Changed;
  }

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.addRequired<DominatorTreeWrapperPass>();
  }
};

} 

char MatfSimpleMem2Reg::ID = 0;

static RegisterPass<MatfSimpleMem2Reg>
X("matf-simple-mem2reg",
  "Simple mem2reg pass",
  false, false);
