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
#pragma once

#include "common/common.h"
#include "Mesh.h"

const E_Int normalIn_H[6] = {1, 0, 1, 0, 1, 0};

void H27_refine(E_Int hexa, Mesh *M);

void H27_reorder(E_Int hexa, Mesh *M);

void H18_refine(E_Int hexa, Mesh *M);

void H18_reorder(E_Int hexa, Mesh *M);

E_Int check_canon_hexa(E_Int hexa, Mesh *M);

inline
void update_shell_pe(E_Int hexa, Mesh *M)
{
    const auto &children = M->cchildren.at(hexa);

    for (E_Int cid : children) {
        E_Int *child = Mesh_get_cell(M, cid);

        for (E_Int j = 0; j < 6; j++) {
            E_Int face = child[4*j];
            
            if      (M->owner[face] == hexa) M->owner[face] = cid;
            else if (M->neigh[face] == hexa) M->neigh[face] = cid;
        }
    }
}

inline
void update_range_and_stride(Mesh *M, E_Int hexa, E_Int cpos, E_Int nchildren)
{
    E_Int *crange = Mesh_get_crange(M, hexa);
    for (E_Int i = 0; i < M->cstride[hexa]; i++) {
        crange[i] = 1;
    }

    for (E_Int i = 0; i < nchildren; i++) {
        E_Int child = cpos + i;

        M->cstride[child] = M->cstride[hexa];

        crange = Mesh_get_crange(M, child);
        for (E_Int j = 0; j < M->cstride[child]; j++) {
            crange[j] = 1;
        }
    }
}