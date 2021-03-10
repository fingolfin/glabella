/*
 * glabella: Low level interface to graph automorphism canonical labelling tools
 */

#include "compiled.h" // GAP headers

#include "../extern/bliss-0.73/graph.hh" /* for bliss graph classes and namespaces */
#include "../extern/nauty27r1/nautinv.h"
#include "../extern/nauty27r1/naututil.h"

#include "../extern/nauty27r1/traces.h"
#include "../extern/nauty27r1/nauty.h"
#include "../extern/nauty27r1/naurng.h"
#include "../extern/nauty27r1/schreier.h"
#include "../extern/nauty27r1/nausparse.h"

/***************** THINGS *********************/

static Obj automorphism_list;

static Obj PermToGAP(int *perm, int n) {
  Obj p = NEW_PERM4(n);
  UInt4 *ptr = ADDR_PERM4(p);

  for (int v = 0; v < n; v++) {
    ptr[v] = perm[v];
  }

  return p;
}

static void userautomproc_traces(int count, int *perm,  int n) {
  Obj p = PermToGAP(perm, n);
  AddList(automorphism_list, p);
}

static void userautomproc(int count, int *perm, int *orbits, int numorbits,
                          int stabvertex, int n) {
  Obj p = PermToGAP(perm, n);
  AddList(automorphism_list, p);
}

/***************** NAUTY STARTS *********************/

/*
 * The following code is a derivative work of the code from the GAP package
 * NautyTracesInterface 0.2 which is licensed GPLv2.
 *
 * Based on "userautomproc()" and "FuncNAUTY_DENSE()" of the GAP package
 * NautyTracesInterface 0.2.
 *
 * Modified by G.P. Nagy, 19/02/2021
 */

Obj FuncNAUTY_GRAPH_CANONICAL_LABELING(Obj self, Obj nr_vert, Obj outneigh,
                                       Obj vertices, Obj stops,
                                       Obj isdirected) {
  // allocate the graph
  DYNALLSTAT(graph, g, g_sz);
  size_t n = INT_INTOBJ(nr_vert);
  size_t m = SETWORDSNEEDED(n);
  g_sz = m * n;
  g = (graph *)calloc(g_sz, sizeof(graph));

  // set the edges
  UInt i, j, b_size;
  Obj block;
  for (i = 1; i <= n; i++) {
    block = ELM_PLIST(outneigh, i);
    b_size = LEN_PLIST(block);
    for (j = 1; j <= b_size; j++) {
      if (isdirected == True) {
        ADDONEARC(g, i - 1, INT_INTOBJ(ELM_PLIST(block, j)) - 1, m);
      } else {
        ADDONEEDGE(g, i - 1, INT_INTOBJ(ELM_PLIST(block, j)) - 1, m);
      }
    }
  }

  // set the coloring
  DYNALLSTAT(int, lab, lab_sz);
  DYNALLSTAT(int, ptn, ptn_sz);
  DYNALLSTAT(int, orbits, orbits_sz);
  DYNALLOC1(int, lab, lab_sz, n, "malloc");
  DYNALLOC1(int, ptn, ptn_sz, n, "malloc");
  DYNALLOC1(int, orbits, orbits_sz, n, "malloc");

  if (IS_LIST(vertices) && (LEN_LIST(vertices) == n)) {
    for (int i = 0; i < n; i++) {
      lab[i] = INT_INTOBJ(ELM_PLIST(vertices, i + 1)) - 1;
      ptn[i] = INT_INTOBJ(ELM_PLIST(stops, i + 1));
    }
  }

  // set nauty's parameters
  static optionblk options;
  if (isdirected == True) {
    static DEFAULTOPTIONS_DIGRAPH(temp_options);
    options = temp_options;
  } else {
    static DEFAULTOPTIONS_GRAPH(temp_options2);
    options = temp_options2;
  }
  options.getcanon = TRUE; // FALSO == no canonically labelled graph to return
  if (IS_LIST(vertices) && (LEN_LIST(vertices) == n)) {
    options.defaultptn = FALSE; // lab, ptn are used
  } else {
    options.defaultptn = TRUE; // lab, ptn are ignored
  }
  options.userautomproc = userautomproc;

  nauty_check(WORDSIZE, m, n, NAUTYVERSIONID);

  // call nauty
  statsblk stats;
  automorphism_list = NEW_PLIST(T_PLIST, 0);
  DYNALLSTAT(graph, cg, cg_sz);
  DYNALLOC2(graph, cg, cg_sz, m, n, "malloc");
  options.schreier = TRUE;
  options.tc_level = 100;
  densenauty(g, lab, ptn, orbits, &options, &stats, m, n, cg);

  // compute 32-bit hashvalue
  long hash = hashgraph(cg, m, n, 255);

  // collect results
  Obj return_list = NEW_PLIST(T_PLIST, 0);
  AddList(return_list, automorphism_list);
  AddList(return_list, PermToGAP(lab, n));
  AddList(return_list, INTOBJ_INT(hash));
  automorphism_list = 0;

  // free pointers
  DYNFREE(g, g_sz);
  DYNFREE(cg, cg_sz);
  DYNFREE(lab, lab_sz);
  DYNFREE(ptn, ptn_sz);
  DYNFREE(orbits, orbits_sz);

  return return_list;
}

/*****************  NAUTY ENDS  *********************/

/***************** SPARSE NAUTY STARTS *********************/

Obj FuncNAUTY_SPARSEGRAPH_CANONICAL_LABELING(Obj self, Obj nr_vert, Obj outneigh,
                                       Obj vertices, Obj stops,
                                       Obj isdirected) {

  // allocate, declare, initialize 
  DYNALLSTAT(int,lab,lab_sz);
  DYNALLSTAT(int,ptn,ptn_sz);
  DYNALLSTAT(int,orbits,orbits_sz);
  DYNALLSTAT(int,map,map_sz);
  static DEFAULTOPTIONS_SPARSEGRAPH(options);
  statsblk stats;

  SG_DECL(sg); 
  SG_DECL(cg);

  options.getcanon = TRUE;
  size_t n = INT_INTOBJ(nr_vert);
  size_t m = SETWORDSNEEDED(n);

  m = SETWORDSNEEDED(n);
  nauty_check(WORDSIZE,m,n,NAUTYVERSIONID);

  DYNALLOC1(int,lab,lab_sz,n,"malloc");
  DYNALLOC1(int,ptn,ptn_sz,n,"malloc");
  DYNALLOC1(int,orbits,orbits_sz,n,"malloc");
  DYNALLOC1(int,map,map_sz,n,"malloc");

  // set the edges
  size_t nredges = 0;
  size_t p = 0;
  for (UInt i = 1; i <= n; i++) {
    nredges += LEN_PLIST( ELM_PLIST( outneigh, i ) );
  }

  SG_ALLOC(sg,n,nredges,"malloc");
  sg.nv = n;              /* Number of vertices */
  sg.nde = nredges;       /* Number of directed edges */

  for (size_t i = 0; i < n; i++) {
    Obj block = ELM_PLIST(outneigh, i+1 );
    UInt b_size = LEN_PLIST(block);

    sg.v[i] = p;
    sg.d[i] = b_size;

    for (size_t j = 0; j < b_size; j++) {
      sg.e[p] = INT_INTOBJ(ELM_PLIST(block, j + 1)) - 1;
      ++p;
    }
  }

  // set nauty's parameters
  if (IS_LIST(vertices) && (LEN_LIST(vertices) == n)) {
    options.defaultptn = FALSE; // lab, ptn are used
    for (int i = 0; i < n; i++) {
      lab[i] = INT_INTOBJ(ELM_PLIST(vertices, i + 1)) - 1;
      ptn[i] = INT_INTOBJ(ELM_PLIST(stops, i + 1));
    }
  } else {
    options.defaultptn = TRUE; // lab, ptn are ignored
  }
  options.userautomproc = userautomproc;

  // call nauty
  automorphism_list = NEW_PLIST(T_PLIST, 0);
  sparsenauty(&sg,lab,ptn,orbits,&options,&stats,&cg);
  //densenauty(g, lab, ptn, orbits, &options, &stats, m, n, cg);

  // compute 32-bit hashvalue
  long hash = hashgraph_sg(&cg, 255);

  // collect results
  Obj return_list = NEW_PLIST(T_PLIST, 0);
  AddList(return_list, automorphism_list);
  AddList(return_list, PermToGAP(lab, n));
  AddList(return_list, INTOBJ_INT(hash));
  automorphism_list = 0;

  // free pointers
  SG_FREE(sg);
  SG_FREE(cg);
  DYNFREE(lab,lab_sz);
  DYNFREE(ptn,ptn_sz);
  DYNFREE(orbits,orbits_sz);
  
  return return_list;
}
/*****************  SPARSE NAUTY ENDS  *********************/

/***************** TRACES STARTS *********************/

Obj FuncTRACES_GRAPH_CANONICAL_LABELING(Obj self, Obj nr_vert, Obj outneigh,
                                       Obj vertices, Obj stops,
                                       Obj isdirected) {

  // allocate, declare, initialize 
  DYNALLSTAT(int,lab,lab_sz);
  DYNALLSTAT(int,ptn,ptn_sz);
  DYNALLSTAT(int,orbits,orbits_sz);
  DYNALLSTAT(int,map,map_sz);
  static DEFAULTOPTIONS_TRACES(options);
  TracesStats stats;

  SG_DECL(sg); 
  SG_DECL(cg);

  options.getcanon = TRUE;
  size_t n = INT_INTOBJ(nr_vert);
  size_t m = SETWORDSNEEDED(n);

  m = SETWORDSNEEDED(n);
  nauty_check(WORDSIZE,m,n,NAUTYVERSIONID);

  DYNALLOC1(int,lab,lab_sz,n,"malloc");
  DYNALLOC1(int,ptn,ptn_sz,n,"malloc");
  DYNALLOC1(int,orbits,orbits_sz,n,"malloc");
  DYNALLOC1(int,map,map_sz,n,"malloc");

  // set the edges
  size_t nredges = 0;
  size_t p = 0;
  for (UInt i = 1; i <= n; i++) {
    nredges += LEN_PLIST( ELM_PLIST( outneigh, i ) );
  }

  SG_ALLOC(sg,n,nredges,"malloc");
  sg.nv = n;              /* Number of vertices */
  sg.nde = nredges;       /* Number of directed edges */

  for (size_t i = 0; i < n; i++) {
    Obj block = ELM_PLIST(outneigh, i+1 );
    UInt b_size = LEN_PLIST(block);

    sg.v[i] = p;
    sg.d[i] = b_size;

    for (size_t j = 0; j < b_size; j++) {
      sg.e[p] = INT_INTOBJ(ELM_PLIST(block, j + 1)) - 1;
      ++p;
    }
  }

  // set nauty's parameters
  if (IS_LIST(vertices) && (LEN_LIST(vertices) == n)) {
    options.defaultptn = FALSE; // lab, ptn are used
    for (int i = 0; i < n; i++) {
      lab[i] = INT_INTOBJ(ELM_PLIST(vertices, i + 1)) - 1;
      ptn[i] = INT_INTOBJ(ELM_PLIST(stops, i + 1));
    }
  } else {
    options.defaultptn = TRUE; // lab, ptn are ignored
  }
  options.userautomproc = userautomproc_traces;
  options.verbosity = 1;

  // call Traces
  automorphism_list = NEW_PLIST(T_PLIST, 0);
  Traces(&sg,lab,ptn,orbits,&options,&stats,&cg);

  // compute 32-bit hashvalue
  long hash = hashgraph_sg(&cg, 255);

  // collect results
  Obj return_list = NEW_PLIST(T_PLIST, 0);
  AddList(return_list, automorphism_list);
  AddList(return_list, PermToGAP(lab, n));
  AddList(return_list, INTOBJ_INT(hash));
  automorphism_list = 0;

  // free pointers
  SG_FREE(sg);
  SG_FREE(cg);
  DYNFREE(lab,lab_sz);
  DYNFREE(ptn,ptn_sz);
  DYNFREE(orbits,orbits_sz);
  
  return return_list;
}

/*****************  TRACES ENDS  *********************/

/***************** BLISS STARTS *********************/

/*
 * The following code is a derivative work of the code from the GAP package
 * Digraphs which is licensed GPLv3. This code therefore is also licensed under
 * the terms of the GNU Public License, version 3.
 *
 * Based on "digraphs_hook_function()" of the GAP package Digraphs 0.15.2
 * Modified by G.P. Nagy, 21/08/2019
 */

void glabella_hook_function(void *user_param_v, unsigned int N,
                             const unsigned int *aut) {
  UInt4 *ptr;
  Obj p, gens, user_param;
  UInt i, n;

  // one needs this in C++ to avoid "invalid conversion" error
  user_param = static_cast<Obj>(user_param_v);
  n = INT_INTOBJ(ELM_PLIST(user_param, 2)); // the degree
  p = PermToGAP((int *)aut, n);

  gens = ELM_PLIST(user_param, 1);
  AssPlist(gens, LEN_PLIST(gens) + 1, p);
}

/*
 * The following code is a derivative work of the code from the GAP package
 * Digraphs which is licensed GPLv3. This code therefore is also licensed under
 * the terms of the GNU Public License, version 3.
 *
 * Based on "FuncDIGRAPH_AUTOMORPHISMS" of the GAP package Digraphs 0.15.2
 * Modified by G.P. Nagy, 21/08/2019
 */

/*
 * Returns: a GAP object list [gens,cl], where <gens> are generators
 * of the aut group of the bipartite digraph, associated to the Bipartite
 * system, and <cl> is a canonical labelling of the digraph.
 */

static Obj glabella_autgr_canlab(bliss::AbstractGraph *graph) {
  Obj autos, p, n;
  UInt4 *ptr;
  const unsigned int *canon;
  Int i;
  bliss::Stats stats;

  autos = NEW_PLIST(T_PLIST, 3);
  n = INTOBJ_INT(graph->get_nof_vertices());

  SET_ELM_PLIST(autos, 1, NEW_PLIST(T_PLIST, 0)); // perms of the vertices
  CHANGED_BAG(autos);
  SET_ELM_PLIST(autos, 2, n);
  SET_LEN_PLIST(autos, 2);

  canon = graph->canonical_form(stats, glabella_hook_function, autos);

  p = PermToGAP((int *)canon, graph->get_nof_vertices());
  SET_ELM_PLIST(autos, 2, p);
  CHANGED_BAG(autos);

  bliss::AbstractGraph *gg;
  gg = graph->permute(canon);
  Obj hash = INTOBJ_INT(gg->get_hash());
  delete gg;

  SET_ELM_PLIST(autos, 3, hash);
  SET_LEN_PLIST(autos, 3);
  CHANGED_BAG(autos);

  if (LEN_PLIST(ELM_PLIST(autos, 1)) != 0) {
    SortDensePlist(ELM_PLIST(autos, 1));
    RemoveDupsDensePlist(ELM_PLIST(autos, 1));
  }

  return autos;
}

/*
 * We construct the coloured graph <C>G</C> on <A>n</A>
 * vertices <C>[1..n]</C>. If <A>isdirected</A> is <C>true</C> the <C>G</C>
 * is a directed graph.
 *
 * The graph is given by the list <C>[N_1,...,N_n]</C>,
 * where <C>N_i</C> is the list of (out)neighbors of the vertex <C>i</C>.
 * Duplicate edges between vertices are ignored.
 *
 * If <A>colours</A> is a list of length <C>n</C> then its elements are used to
 * define a vertex coloring of <C>G</C>.
 *
 *
 * Returns: The pair <C>[gens,cl]</C> as GAP object, where <C>gens</C> is a list
 * of generators for <C>Aut(G)</C> and <C>cl</C> is a canonical labelling of
 * <C>G</C>.
 *
 * Nonchecking version.
 */

Obj FuncBLISS_GRAPH_CANONICAL_LABELING(Obj self, Obj n, Obj outneigh,
                                       Obj colours, Obj isdirected) {
  bliss::AbstractGraph *g;
  UInt nn, i, j, b_size;
  Obj block;

  nn = INT_INTOBJ(n);
  if (isdirected == True) {
    g = new bliss::Digraph(nn);
  } else {
    g = new bliss::Graph(nn);
  }

  if (IS_LIST(colours) && (LEN_LIST(colours) == nn)) {
    for (i = 1; i <= nn; i++) {
      g->change_color(i - 1, INT_INTOBJ(ELM_PLIST(colours, i)));
    }
  }

  for (i = 1; i <= nn; i++) {
    block = ELM_PLIST(outneigh, i);
    b_size = LEN_PLIST(block);
    for (j = 1; j <= b_size; j++) {
      g->add_edge(i - 1, INT_INTOBJ(ELM_PLIST(block, j)) - 1);
    }
  }
  Obj ret = glabella_autgr_canlab(g);
  delete g;
  return ret;
}

/*****************  BLISS ENDS  *********************/

/***************************************************************************/

// Table of functions to export

/*
 * "GVarFuncs" is not workin in C++, due to
 * "invalid conversion from ‘void*’ to ‘Obj‘" error
 */

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
    GVAR_FUNC_TABLE_ENTRY(TRACES_GRAPH_CANONICAL_LABELING, 5,
                          "n, outneigh, vertices, stops, isdirected"),
    // { "TRACES_GRAPH_CANONICAL_LABELING", 
    //   5, 
    //   "n, outneigh, vertices, stops, isdirected", 
    //   (GVarFuncTypeDef)FuncTRACES_GRAPH_CANONICAL_LABELING, 
    //   //__FILE__ ":" "TRACES_GRAPH_CANONICAL_LABELING" 
    //    "src/glabella_traces.c:FuncTRACES_GRAPH_CANONICAL_LABELING" 
    // },
    GVAR_FUNC_TABLE_ENTRY(NAUTY_SPARSEGRAPH_CANONICAL_LABELING, 5,
                          "n, outneigh, vertices, stops, isdirected"),
    GVAR_FUNC_TABLE_ENTRY(NAUTY_GRAPH_CANONICAL_LABELING, 5,
                          "n, outneigh, vertices, stops, isdirected"),
    GVAR_FUNC_TABLE_ENTRY(BLISS_GRAPH_CANONICAL_LABELING, 4,
                          "n, outneigh, colours, isdirected"),

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
    /* name        = */ "glabella",
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
};

extern "C" StructInitInfo *Init__Dynamic(void) { return &module; }
