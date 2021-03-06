/* linbox/algorithms/echelon-form.h
 * Copyright (C) 2006 Pascal Giorgi
 *
 * Written by Pascal Giorgi pascal.giorgi@univ-perp.fr
 *
 * ========LICENCE========
 * This file is part of the library LinBox.
 *
  * LinBox is free software: you can redistribute it and/or modify
 * it under the terms of the  GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 * ========LICENCE========
 */

/** @file algorithms/echelon-form.h
 * @ingroup algorithms
 * @brief NO DOC
 */

#ifndef __LINBOX_echelon_form_H
#define __LINBOX_echelon_form_H

#include "linbox/matrix/dense-matrix.h"
#include "linbox/matrix/matrix-domain.h"

#include "linbox/matrix/matrix-domain.h"
#include "linbox/matrix/factorized-matrix.h"

namespace LinBox
{

	//! Echelon form domain
	template<class Field>
	class EchelonFormDomain{

	private:

		const Field                      *_field;
		BlasMatrixDomain<Field>  _BMD;
		MatrixDomain<Field>       _MD;

	public:
		typedef typename Field::Element Element;

		// constructor
		EchelonFormDomain(const Field &F) :
			_field(&F), _BMD(F), _MD(F)
		{}


		/*!  row echelon form
		 * @warning E is supposed to be the zero matrix
		 */
		template<class Matrix>
		int rowEchelon(Matrix &E, const Matrix& A)
		{

			size_t m,n, rank;
			m = A.rowdim();
			n = A.coldim();

			// get the transposed of A
			BlasMatrix<Field> At(field(),n, m);
			for (size_t i=0;i<m;++i)
				for (size_t j=0;j<n;++j)
					At.setEntry(j,i,A.getEntry(i,j));


			rank = (size_t)columnEchelon(At);

			// read the transpose of the echelon form from the rank 1st column of L
			for (size_t i=0; i<rank;++i)
				for (size_t j=0;j<n;++j){
					E.setEntry(i,j, At.getEntry(j,i));
				}
			return (int)rank;
		}




		//! row reduced echelon form (using copy)
		template<class Matrix>
		int rowReducedEchelon(Matrix &E, const Matrix& A)
		{

			size_t m,n, rank;
			m = A.rowdim();
			n = A.coldim();

			// get the transposed of A
			BlasMatrix<Field> At(field(),n, m);
			for (size_t i=0;i<m;++i)
				for (size_t j=0;j<n;++j)
					At.setEntry(j,i,A.getEntry(i,j));

			rank = (size_t)columnReducedEchelon(At);

			// read the transpose of the echelon form from the rank 1st column of At
			for (size_t i=0; i<rank;++i)
				for (size_t j=0;j<n;++j)
					E.setEntry(i,j, At.getEntry(j,i));
			return (int)rank;
		}


		//!  column echelon form (using copy)
		template<class Matrix>
		int columnEchelon(Matrix &E, const Matrix& A)
		{

			size_t m,n;
			m = A.rowdim();
			n = A.coldim();

			// copy  A in E
			for (size_t i=0;i<m;++i)
				for (size_t j=0;j<n;++j)
					E.setEntry(i,j,A.getEntry(i,j));

			return columnEchelon(E);
		}


		//! column reduced echelon form (using copy)
		template<class Matrix>
		int columnReducedEchelon(Matrix &E, const Matrix& A)
		{

			size_t m,n;

			m = A.rowdim();
			n = A.coldim();

			// copy  A in E
			for (size_t i=0;i<m;++i)
				for (size_t j=0;j<n;++j)
					E.setEntry(i,j,A.getEntry(i,j));

			return columnReducedEchelon(E);
		}

		//! column echelon form (IN-PLACE VERSION)
		template<class Matrix>
		int columnEchelon(Matrix &E)
		{

			size_t m,n, rank;
			m = E.rowdim();
			n = E.coldim();

			BlasPermutation<size_t> P(E.coldim());
			BlasPermutation<size_t> Qt(E.rowdim());
			// compute the LQUP of E
			LQUPMatrix<Field> LQUP(E,P,Qt);

			// get the rank
			rank = LQUP.getRank();

			// get permutation Qt
			// BlasPermutation<size_t> Qt = LQUP.getQ();

			// Zero out upper triangular part of E
			for (size_t i=0;i<m;++i)
				for (size_t j=i;j<n;++j)
					E.setEntry(i,j,field().zero);

			// put one inplace of pivot
			for (size_t i=0;i<rank;++i){
				E.setEntry(*(Qt.getPointer()+i),i,field().one);
			}

			return (int)rank;
		}

		//! column reduced echelon form (IN-PLACE VERSION)
		template<class Matrix>
		int columnReducedEchelon(Matrix &E)
		{
			size_t m,n, rank;

			m = E.rowdim();
			n = E.coldim();

			// compute the LQUP of E
			BlasPermutation<size_t> P(E.coldim());
			BlasPermutation<size_t> Qt(E.rowdim());

			LQUPMatrix<Field> LQUP( E, P, Qt);

			// get the rank
			rank = LQUP.getRank();

			// BlasPermutation<size_t> Qt = LQUP.getQ();
			TransposedBlasMatrix<BlasPermutation<size_t> > Q(Qt);

			// Zero out upper triangular part of E
			for (size_t i=0;i<m;++i)
				for (size_t j=i;j<n;++j)
					E.setEntry(i,j,field().zero);

			// permute E with Qt
			_BMD.mulin_right(Qt,E);

			// put one inplace of pivot
			for (size_t i=0;i<rank;++i)
				E.setEntry(i,i, field().one);//*(Qt.getPointer()+i),one);

			// Update the first r columns of E by Err^(-1)
			// BlasSubmatrix<Field> Er(E,0,0,rank,rank);
			BlasMatrix<Field> Er(E,0,0,rank,rank);
			TriangularBlasMatrix<Field> Err(Er, Tag::Shape::Lower, Tag::Diag::Unit);
			BlasSubmatrix<BlasMatrix<Field> > En(E,rank,0,m-rank,rank);

			_BMD.right_solve(Err, En);

			for (size_t i=0;i<rank;++i){
				for (size_t j=0;j<i;++j)
					E.setEntry(i,j,field().zero);
			}

			// permute L such that L<-Q.E
			_BMD.mulin_right(Q,E);

			return (int)rank;
		}

		//! writer/helper to maple format
		template<class Matrix>
		void write_maple(const char* name, const Matrix& A)
		{

			size_t m,n;
			m = A.rowdim();
			n = A.coldim();
			std::cout<<name<<":= Matrix([";

			for (size_t i=0;i<m-1;++i){
				std::cout<<"[";
				for (size_t j=0;j<n-1;++j)
					field().write(std::cout,A.getEntry(i,j))<<",";
				field().write(std::cout, A.getEntry(i,n-1))<<"] , ";
			}
			std::cout<<"[";
			for (size_t j=0;j<n-1;++j)
				field().write(std::cout,A.getEntry(m-1,j))<<",";
			field().write(std::cout, A.getEntry(m-1,n-1))<<"]]);\n ";
		}

		const Field & field() { return *_field; }
	};

}

#endif

// Local Variables:
// mode: C++
// tab-width: 8
// indent-tabs-mode: nil
// c-basic-offset: 8
// End:
// vim:sts=8:sw=8:ts=8:noet:sr:cino=>s,f0,{0,g0,(0,\:0,t0,+0,=s

