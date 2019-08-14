/*
 * BlissInterface: Low level interface to the bliss graph automorphism tool
 */

#include "BlissInterface.hh"
#include "src/compiled.h" /* GAP headers */

Obj FuncTestCommand(Obj self) { return INTOBJ_INT(42); }

Obj FuncTestCommandWithParams(Obj self, Obj param, Obj param2) {
  /* simply return the first parameter */
  return param;
}

// TEST_BIT_BLIST( <list>, <pos> ) . . . . . .  test a bit of a boolean list

Obj FuncUseBliss(Obj self, Obj mat, Obj m, Obj n) {
  bliss::AbstractGraph *g;
  // It compiles if we add the neccessary .cc files to Makefile,
  // but fails to load the package with "undefined symbol" error message:
  g = new bliss::Graph(5);
  UInt k, mm, nn;
  UInt i, j;
  mm = INT_INTOBJ(m);
  nn = INT_INTOBJ(n);
  k = 0;
  for (i = 1; i <= mm; i++) {
    for (j = 1; j <= nn; j++) {
      if (TEST_BIT_BLIST(ELM_LIST(mat, i), j)) {
        k++;
      }
    }
  }
  return INTOBJ_INT(k * g->get_nof_vertices());
}

/***************************************************************************/

// Table of functions to export

// static StructGVarFunc GVarFuncs [] = {
//    GVAR_FUNC(TestCommand, 0, ""),
//    GVAR_FUNC(TestCommandWithParams, 2, "param, param2"),
//
//    { 0 } /* Finish with an empty entry */
//};

typedef Obj (*GVarFuncTypeDef)(/*arguments*/);

#define GVAR_FUNC_TABLE_ENTRY(name, nparam, params)                            \
  { #name, nparam, params, (GVarFuncTypeDef)Func##name, __FILE__ ":" #name }

// Table of functions to export
static StructGVarFunc GVarFuncs[] = {
    GVAR_FUNC_TABLE_ENTRY(TestCommand, 0, ""),
    GVAR_FUNC_TABLE_ENTRY(TestCommandWithParams, 2, "param, param2"),
    GVAR_FUNC_TABLE_ENTRY(UseBliss, 3, "mat, m, n"),

    {0} /* Finish with an empty entry */

};

/****************************************************************************
**
*F  InitKernel( <module> ) . . . . . . . .  initialise kernel data structures
*/
static Int InitKernel(StructInitInfo *module) {
  /* init filters and functions */
  InitHdlrFuncsFromTable(GVarFuncs);

  /* return success */
  return 0;
}

/****************************************************************************
**
*F  InitLibrary( <module> ) . . . . . . .  initialise library data structures
*/
static Int InitLibrary(StructInitInfo *module) {
  /* init filters and functions */
  InitGVarFuncsFromTable(GVarFuncs);

  /* return success */
  return 0;
}

/****************************************************************************
**
*F  Init__Dynamic() . . . . . . . . . . . . . . . . . table of init functions
*/
static StructInitInfo module = {
    /* type        = */ MODULE_DYNAMIC,
    /* name        = */ "BlissInterface",
    /* revision_c  = */ 0,
    /* revision_h  = */ 0,
    /* version     = */ 0,
    /* crc         = */ 0,
    /* initKernel  = */ InitKernel,
    /* initLibrary = */ InitLibrary,
    /* checkInit   = */ 0,
    /* preSave     = */ 0,
    /* postSave    = */ 0,
    /* postRestore = */ 0,
    /* moduleStateSize      = */ 0,
    /* moduleStateOffsetPtr = */ 0,
    /* initModuleState      = */ 0,
    /* destroyModuleState   = */ 0,
};

extern "C" StructInitInfo *Init__Dynamic(void) { return &module; }
