add_llvm_library(MatfSimpleMem2Reg MODULE BUILDTREE_ONLY
  MatfSimpleMem2Reg.cpp

  PLUGIN_TOOL
  opt
  )