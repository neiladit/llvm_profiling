
#include "llvm/Pass.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/raw_ostream.h"

#include "llvm/Analysis/CallGraph.h"
#include "llvm/Analysis/CallGraphSCCPass.h"

#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include <string.h>
#include <vector>

using namespace std;
using namespace llvm;

namespace {
  struct SkeletonPass : public ModulePass {
      static char ID;
      SkeletonPass() : ModulePass(ID) {}
      
      virtual bool runOnModule(Module &M);
      virtual bool runOnFunction(Function &F, Module &M);
      virtual bool runOnBasicBlock(BasicBlock &BB, Module &M);
      
      bool initialize(Module &M);
      bool finialize(Module &M);
      void createInstr(BasicBlock &bb, Constant *counter_ptr, int num);
      
      vector<string> atomicCounter = {"llvmInstrAtomicCounter", "basicBlockAtomicCounter", "mulAtomicCounter", "memOpAtomicCounter", "branchAtomicCounter"};;
  };
}

bool SkeletonPass::runOnModule(Module &M)
{
    bool modified = initialize(M);
    
    for (auto it = M.begin(); it != M.end(); it++) {
        modified |= runOnFunction(*it, M);
    }
    modified |= finialize(M);
    //M.dump();
    
    return modified;
}
bool SkeletonPass::runOnFunction(Function &F, Module &M)
{
    bool modified = false;
    //errs() << F.getName() << "\n";
  
  /*
    BasicBlock &b = F.getEntryBlock();
    Type *I64Ty = Type::getInt64Ty(M.getContext());
    IRBuilder<> Builder(F.getContext());
    Twine s = F.getName()+".glob";
    Value *atomicCounter = M.getOrInsertGlobal(s.str(), I64Ty);
    Value *One = ConstantInt::get(Type::getInt64Ty(F.getContext()), 1);

    new AtomicRMWInst(AtomicRMWInst::Add,
                      atomicCounter,
                      ConstantInt::get(Type::getInt64Ty(F.getContext()), 1),
                      AtomicOrdering::SequentiallyConsistent,
                      SyncScope::System, b.getFirstNonPHI ());
 
    errs()<< b;
    free(&b);
 */
    for (auto it = F.begin(); it != F.end(); it++) {
    
    
        if (it==F.begin()){
            Type *I64Ty = Type::getInt64Ty(M.getContext());
            IRBuilder<> Builder(F.getContext());
            Twine s = F.getName()+".glob";
            Value *atomicCounter = M.getOrInsertGlobal(s.str(), I64Ty);
            Value *One = ConstantInt::get(Type::getInt64Ty(F.getContext()), 1);
            
            new AtomicRMWInst(AtomicRMWInst::Add,
                              atomicCounter,
                              ConstantInt::get(Type::getInt64Ty(F.getContext()), 1),
                              AtomicOrdering::SequentiallyConsistent,
                              SyncScope::System, it->getFirstNonPHI ());
                              
        }
    
        
        modified |= runOnBasicBlock(*it, M);
        
    }
    
    

    //Value *result = Builder.CreateAdd(atomicCounter,One,"func_add");
    
    return modified;
}

void SkeletonPass::createInstr(BasicBlock &bb, Constant *counter_ptr, int num){
    if(num){
        new AtomicRMWInst(AtomicRMWInst::Add,
                      counter_ptr,
                      ConstantInt::get(Type::getInt64Ty(bb.getContext()), num),
                      AtomicOrdering::SequentiallyConsistent,
                      SyncScope::System, bb.getTerminator());
    }
}

bool SkeletonPass::runOnBasicBlock(BasicBlock &bb, Module &M)
{
    // Get the global variable for atomic counter
    Type *I64Ty = Type::getInt64Ty(M.getContext());
    Constant *instrCounter = M.getOrInsertGlobal("llvmInstrAtomicCounter", I64Ty);
    assert(instrCounter && "Could not declare or find llvmInstrAtomicCounter global");
    Constant *bbCounter = M.getOrInsertGlobal("basicBlockAtomicCounter", I64Ty);
    assert(bbCounter && "Could not declare or find basicBlockAtomicCounter global");
    Constant *mulCounter = M.getOrInsertGlobal("mulAtomicCounter", I64Ty);
    assert(mulCounter && "Could not declare or find mulAtomicCounter global");
    Constant *brCounter = M.getOrInsertGlobal("branchAtomicCounter", I64Ty);
    assert(brCounter && "Could not declare or find branchAtomicCounter global");
    Constant *memCounter = M.getOrInsertGlobal("memOpAtomicCounter", I64Ty);
    assert(memCounter && "Could not declare or find memOpAtomicCounter global");
    
    int instr = 0;
    int basic_block = 1;
    int mul_instr = 0;
    int br_instr = 0;
    int mem_instr = 0;
    
    instr += bb.size();
    for (auto it = bb.begin(); it != bb.end(); it++) {
        //errs() << it->getOpcodeName() << "\n";
        switch (it->getOpcode()) {
            case Instruction::Mul:
                mul_instr++;
                continue;
            case Instruction::Br:
                br_instr++;
                continue;
            case Instruction::Store:
            case Instruction::Load:
                mem_instr++;
                continue;
            default:
                break;
        }
    }
    createInstr(bb, bbCounter, basic_block);
    createInstr(bb, instrCounter,instr);
    createInstr(bb, mulCounter,mul_instr);
    createInstr(bb, brCounter,br_instr);
    createInstr(bb, memCounter,mem_instr);
    
    return true;
}

bool SkeletonPass::initialize(Module &M)
{
    IRBuilder<> Builder(M.getContext());
    Function *mainFunc = M.getFunction("main");
        
    // not the main module
    if (!mainFunc)
        return false;
    
    Type *I64Ty = Type::getInt64Ty(M.getContext());
    // Create atomic counter global variable;
    
    for (int i = 0; i < atomicCounter.size(); i++){
        new GlobalVariable(M, I64Ty, false, GlobalValue::CommonLinkage, ConstantInt::get(I64Ty, 0), atomicCounter[i]);
        /*
    new GlobalVariable(M, I64Ty, false, GlobalValue::CommonLinkage, ConstantInt::get(I64Ty, 0), "basicBasicAtomicCounter");
    new GlobalVariable(M, I64Ty, false, GlobalValue::CommonLinkage, ConstantInt::get(I64Ty, 0), "MemOpAtomicCounter");
    new GlobalVariable(M, I64Ty, false, GlobalValue::CommonLinkage, ConstantInt::get(I64Ty, 0), "branchAtomicCounter");
    new GlobalVariable(M, I64Ty, false, GlobalValue::CommonLinkage, ConstantInt::get(I64Ty, 0), "mulAtomicCounter");*/
    }
    
    
    auto &functionList = M.getFunctionList();
    for (auto &function : functionList) {
        Value *atomic_counter = new GlobalVariable(M, I64Ty, false, GlobalValue::CommonLinkage, ConstantInt::get(I64Ty, 0), function.getName()+".glob");
    }
    
    return true;
}

bool SkeletonPass::finialize(Module &M){
    
    IRBuilder<> Builder(M.getContext());
    Function *mainFunc = M.getFunction("main");
    
    // not the main module
    if (!mainFunc)
        return false;
    // Build printf function handle
    std::vector<Type *> FTyArgs;
    FTyArgs.push_back(Type::getInt8PtrTy(M.getContext()));
    FunctionType *FTy = FunctionType::get(Type::getInt32Ty(M.getContext()), FTyArgs, true);
    FunctionCallee printF = M.getOrInsertFunction("printf", FTy);
    //assert(printF != NULL);
    
    for (auto bb = mainFunc->begin(); bb != mainFunc->end(); bb++) {
        for(auto it = bb->begin(); it != bb->end(); it++) {
            // insert at the end of main function
            if ((std::string)it->getOpcodeName() == "ret") {
                Builder.SetInsertPoint(&*bb, it);
                for(int i = 0; i < atomicCounter.size(); i++){
                    // Build Arguments
                    Value *format_long;
                    if (i == 0){
                    format_long = Builder.CreateGlobalStringPtr("\n\n"+atomicCounter[i]+": %ld\n", "formatLong");
                    }else{
                        format_long = Builder.CreateGlobalStringPtr(atomicCounter[i]+": %ld\n", "formatLong");
                    }
                    std::vector<Value *> argVec;
                    argVec.push_back(format_long);
                    
                    Value *atomic_counter = M.getGlobalVariable(atomicCounter[i]);
                    Value* finalVal = new LoadInst(atomic_counter, atomic_counter->getName()+".val", &*it);
                    argVec.push_back(finalVal);
                    CallInst::Create(printF, argVec, "printf", &*it);
                    
                }
                
                
                
                Value *format_long;
                
                
                auto &functionList = M.getFunctionList();
                Type *I64Ty = Type::getInt64Ty(M.getContext());
                for (auto &function : functionList) {
                    
                    format_long = Builder.CreateGlobalStringPtr(function.getName().str()+": %ld\n", "formatLong");
                    std::vector<Value *> argVec;
                    argVec.push_back(format_long);
                    Twine s = function.getName()+".glob";
                    Value *atomicCounter = M.getGlobalVariable(s.str(),I64Ty);

                    Value* finalVal = new LoadInst(atomicCounter, function.getName()+".val", &*it);
                    
                    argVec.push_back(finalVal);
                    CallInst::Create(printF, argVec, "printf", &*it);
                }

            }
        }
    }
    return true;
    
}

char SkeletonPass::ID = 0;

// Register the pass so `opt -skeleton` runs it.
static RegisterPass<SkeletonPass> X("skeleton", "a useless pass");
