/*    
    Copyright 2013-2024 Onera.

    This file is part of Cassiopee.

    Cassiopee is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Cassiopee is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Cassiopee.  If not, see <http://www.gnu.org/licenses/>.
*/
// Fill hole from edges
#include "occ.h"

#include "TopoDS.hxx"
#include "BRep_Tool.hxx"
#include "TopExp.hxx"
#include "TopExp_Explorer.hxx"
#include "TopTools_IndexedMapOfShape.hxx"
#include "ShapeBuild_ReShape.hxx"
#include "BRep_Builder.hxx"
#include "BRepBuilderAPI_MakeWire.hxx"
#include "BRepBuilderAPI_MakeFace.hxx"
#include "StdFail_NotDone.hxx"
#include "BRepFill_Filling.hxx"

//=====================================================================
// Remove some faces and rebuild compound
//=====================================================================
PyObject* K_OCC::fillHole(PyObject* self, PyObject* args)
{
  PyObject* hook; PyObject* listEdges;
  if (!PYPARSETUPLE_(args, OO_, &hook, &listEdges)) return NULL;

  void** packet = NULL;
#if (PY_MAJOR_VERSION == 2 && PY_MINOR_VERSION < 7) || (PY_MAJOR_VERSION == 3 && PY_MINOR_VERSION < 1)
  packet = (void**) PyCObject_AsVoidPtr(hook);
#else
  packet = (void**) PyCapsule_GetPointer(hook, NULL);
#endif

  // get top shape
  TopoDS_Shape* shp = (TopoDS_Shape*)packet[0];
  TopTools_IndexedMapOfShape& edges = *(TopTools_IndexedMapOfShape*)packet[2];
  //TopTools_IndexedMapOfShape& surfaces = *(TopTools_IndexedMapOfShape*)packet[1];
  E_Int nEdges = edges.Extent();

  // get edges and make a wire
  BRepBuilderAPI_MakeWire wireMaker;
  for (E_Int no = 0; no < PyList_Size(listEdges); no++)
  {
    PyObject* noEdgeO = PyList_GetItem(listEdges, no);
    E_Int noEdge = PyInt_AsLong(noEdgeO);
    if (noEdge < 0 && noEdge >= -nEdges)
    {
      const TopoDS_Edge& E = TopoDS::Edge(edges(-noEdge));
      E.Reversed();  
      wireMaker.Add(E);
    }
    else if (noEdge > 0 && noEdge <= nEdges)
    {
      const TopoDS_Edge& E = TopoDS::Edge(edges(noEdge));
      wireMaker.Add(E);
    }
    else printf("Warning: fillHole: invalid edge.\n");
  }

  // Build wire
  TopoDS_Wire myWire;
  bool fail = false;
  try {
    myWire = wireMaker.Wire();
    //myWire.Reversed();
  } catch (StdFail_NotDone& e) { fail = true; }
  if (fail) 
  {
    PyErr_SetString(PyExc_TypeError, "fillHole: input is not a wire.");  
    return NULL;
  }
  
  // Build face on wire
  TopoDS_Face F;
  try {
    BRepBuilderAPI_MakeFace faceMaker(myWire, Standard_False);
    //faceMaker.Add(myWire);
    F = faceMaker.Face();

    if (not faceMaker.IsDone())
    {
      fail = true;
      //PyErr_SetString(PyExc_TypeError, "fillHole: fail to generate face (isDone).");  
      //return NULL;
    }

  } catch (StdFail_NotDone& e) { fail = true; }
  if (fail) 
  {
    //PyErr_SetString(PyExc_TypeError, "fillHole: fail to generate face (notDone).");  
    //return NULL;
  }
  
  // try with brepfill
  if (fail)
  {
    BRepFill_Filling filler;
    for (E_Int no = 0; no < PyList_Size(listEdges); no++)
    {
      PyObject* noEdgeO = PyList_GetItem(listEdges, no);
      E_Int noEdge = PyInt_AsLong(noEdgeO);
      if (noEdge < 0 && noEdge >= -nEdges)
      {
        const TopoDS_Edge& E = TopoDS::Edge(edges(-noEdge));
        E.Reversed();  
        filler.Add(E, GeomAbs_C1, true);
      }
      else if (noEdge > 0 && noEdge <= nEdges)
      {
        const TopoDS_Edge& E = TopoDS::Edge(edges(noEdge));
        filler.Add(E, GeomAbs_C1, true);
      }
      else printf("Warning: fillHole: invalid edge.\n");
    }
    filler.Build();
    F = filler.Face();
  }
  // Add face to shape
  //ShapeBuild_ReShape reshaper;
  //reshaper.Add(F); // no add
  //TopoDS_Shape shc = reshaper.Apply(*shp);

  TopoDS_Compound shc;
  BRep_Builder aBuilder;
  aBuilder.MakeCompound(shc);
  aBuilder.Add(shc, *shp);
  aBuilder.Add(shc, F); // How can I check face orientation?
  
  // export
  delete shp;
  TopoDS_Shape* newshp = new TopoDS_Shape(shc);

  // Export
  packet[0] = newshp;
  TopTools_IndexedMapOfShape* ptr = (TopTools_IndexedMapOfShape*)packet[1];
  delete ptr;
  TopTools_IndexedMapOfShape* sf = new TopTools_IndexedMapOfShape();
  TopExp::MapShapes(*newshp, TopAbs_FACE, *sf);
  packet[1] = sf;
  TopTools_IndexedMapOfShape* ptr2 = (TopTools_IndexedMapOfShape*)packet[2];
  delete ptr2;
  TopTools_IndexedMapOfShape* se = new TopTools_IndexedMapOfShape();
  TopExp::MapShapes(*newshp, TopAbs_EDGE, *se);
  packet[2] = se;
  printf("INFO: after fillHole: Nb edges=%d\n", se->Extent());
  printf("INFO: after fillHole: Nb faces=%d\n", sf->Extent());

  Py_INCREF(Py_None);
  return Py_None;
}