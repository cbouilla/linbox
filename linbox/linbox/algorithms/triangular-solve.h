/* ===================================================================
 * (C) LinBox 2008
 * Triangular Solve
 * See COPYING for license information.
 * Time-stamp: <15 Sep 08 14:30:09 Jean-Guillaume.Dumas@imag.fr> 
 * ===================================================================
 */
#ifndef __TRI_SOLVE_INL
#define __TRI_SOLVE_INL

#include "linbox/vector/vector-domain.h"

namespace LinBox 
{
    template <class _Matrix, class Vector1, class Vector2> Vector1&
    upperTriangularSolve (Vector1& x,
                          const _Matrix  &U,
                          const Vector2& b)
    {
        linbox_check( x.size() == U.coldim() );
        linbox_check( b.size() == U.rowdim() );
        typedef _Matrix Matrix;
        typedef typename Matrix::Field Field;
        const Field& F = U.field();

        commentator.start ("Sparse Elimination Upper Triangular Solve", "utrsm");

        typename Vector2::const_iterator vec=b.begin();
        typename Vector1::iterator res=x.begin();
        typename Matrix::ConstRowIterator row=U.rowBegin();

            // Find last constrained values of x, U and b
//         for( ; (res != x.end()) && (row != U.rowEnd()); ++res, ++row, ++vec) { }
        size_t last = U.coldim();
        if( b.size() < last ) last = b.size();
        res += last;
        row += last;
        vec += last;
        

        bool consistant = true;
        for(typename Vector2::const_iterator bcheck=vec; bcheck != b.end(); ++bcheck) {
            if( ! F.isZero(*bcheck) ) {
                consistant = false;
                break;
            }
        }
        if (consistant) {
            --vec; --res; --row;
            
            VectorDomain<Field> VD(F);
            for( ; row != U.rowBegin(); --row, --vec, --res) {
                F.init(*res, 0UL);
                if (row->size()) {
                    typename Field::Element tmp;
                    VD.dot(tmp, *row, x);
                    F.negin(tmp);
                    F.addin(tmp,*vec);
                    F.divin(tmp,row->front().second);
                    F.assign(*res,tmp);
                } else {
                        // Consistency check
                    if( ! F.isZero(*vec) ) {
                        consistant = false;
                        break;
                    }
                }
            }
            
            F.init(*res, 0UL);
            if (row->size()) {
                typename Field::Element tmp;
                VD.dot(tmp, *row, x);
                F.negin(tmp);
                F.addin(tmp,*vec);
                F.divin(tmp,row->front().second);
                F.assign(*res,tmp);
            } else {
                    // Consistency check
                if( ! F.isZero(*vec) ) consistant = false;
            }
        }
        if (! consistant) throw LinboxError ("upperTriangularSolve returned INCONSISTENT");
        
        commentator.stop ("done", NULL, "utrsm");
        return x;
    }

    template <class _Matrix, class Vector1, class Vector2> Vector1&
    lowerTriangularUnitarySolve (Vector1& x,
                                 const _Matrix  &L,
                                 const Vector2& b)
    {
        linbox_check( b.size() == U.coldim() );
        typedef _Matrix Matrix;
        typedef typename Matrix::Field Field;
        const Field& F = L.field();

        commentator.start ("Sparse Elimination Lower Triangular Unitary Solve", "ltrsm");

        typename Vector2::const_iterator vec=b.begin();
        typename Vector1::iterator res=x.begin();
        typename Matrix::ConstRowIterator row=L.rowBegin();

        VectorDomain<Field> VD(F);
        for( ; row != L.rowEnd(); ++row, ++vec, ++res) {
            F.init(*res, 0UL);
            typename Field::Element tmp;
            VD.dot(tmp, *row, x);
            F.negin(tmp);
            F.addin(tmp,*vec);
            F.assign(*res,tmp);
        }
        
        commentator.stop ("done", NULL, "ltrsm");
        return x;
    }

}
#endif