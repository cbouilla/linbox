
/* tests/test-minpoly.C
 * Copyright (C) 2001, 2002 Bradford Hovinen
 *
 * Written by Bradford Hovinen <hovinen@cis.udel.edu>
 *
 * --------------------------------------------------------
 * 2002-04-03: William J. Turner <wjturner@acm.org>
 *
 * changed name of sparse-matrix file.
 *
 * --------------------------------------------------------
 *
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 * ========LICENCE========
 *.
 */


/*! @file  tests/test-minpoly.C
 * @ingroup tests
 * @brief  no doc
 * @test NO DOC
 */



#include "linbox/linbox-config.h"

#include <iostream>
#include <fstream>

#include <cstdio>

#include "givaro/modular.h"
#include "givaro/gfq.h"
#include "linbox/matrix/sparse-matrix.h"
#include "linbox/blackbox/scalar-matrix.h"
#include "linbox/util/commentator.h"
#include "linbox/solutions/minpoly.h"
#include "linbox/vector/stream.h"

#include "linbox/vector/blas-vector.h"

#include "test-common.h"

using namespace LinBox;

/* Test 0: Minimal polynomial of the zero matrix
*/
template <class Field, class Meth>
static bool testZeroMinpoly (Field &F, size_t n, const Meth& M)
{
	ostream &report = commentator().report (Commentator::LEVEL_IMPORTANT, INTERNAL_DESCRIPTION);
	commentator().start ("Testing zero minpoly", "testZeroMinpoly");
	typedef BlasVector<Field> Polynomial;
	Polynomial phi(F);
	SparseMatrix<Field> A(F, n, n);
	minpoly(phi, A, M);

        A.write(report, Tag::FileFormat::Maple);
	report << "Minimal polynomial is: ";

	printPolynomial<Field, Polynomial> (F, report, phi);

	bool ret;
	if (phi.size () == 2 && F.isZero (phi[0]) && F.isOne(phi[1]) )
		ret = true;
	else
	{
		report << "ERROR: A = 0, should get x, got ";
		printPolynomial<Field, Polynomial> (F, report, phi);
		report << endl;
		ret = false;
	}
	commentator().stop (MSG_STATUS (ret), (const char *) 0, "testZeroMinpoly");
	return ret;
}

/* Test 1: Minimal polynomial of the identity matrix
 *
 * Construct the identity matrix and compute its minimal polynomial. Confirm
 * that the result is x-1
 *
 * F - Field over which to perform computations
 * n - Dimension to which to make matrix
 *
 * Return true on success and false on failure
 */

template <class Field, class Meth>
static bool testIdentityMinpoly (Field &F, size_t n, const Meth& M)
{
	typedef BlasVector<Field> Polynomial;
	typedef ScalarMatrix<Field> Blackbox;

	commentator().start ("Testing identity minpoly", "testIdentityMinpoly");

	typename Field::Element c0, c1;

	//StandardBasisStream<Field, Row> stream (F, n);
	Blackbox A (F, n, n, F.one);

	Polynomial phi(F);

	minpoly (phi, A, M );

	ostream &report = commentator().report (Commentator::LEVEL_IMPORTANT, INTERNAL_DESCRIPTION);
	report << "Minimal polynomial is: ";
	printPolynomial<Field, Polynomial> (F, report, phi);

	bool ret;

	F.assign(c0, F.mOne);
	F.assign(c1, F.one);

	if (phi.size () == 2 && F.areEqual (phi[0], c0) && F.areEqual (phi[1], c1))
		ret = true;
	else {
		ret = false;
		commentator().report (Commentator::LEVEL_IMPORTANT, INTERNAL_ERROR)
			<< "ERROR: A = I, should get x-1, got ";
		printPolynomial<Field, Polynomial> (F, report, phi);
	}

	commentator().stop (MSG_STATUS (ret), (const char *) 0, "testIdentityMinpoly");

	return ret;
}

/* Test 2: Minimal polynomial of a nilpotent matrix
 *
 * Construct an index-n nilpotent matrix and compute its minimal
 * polynomial. Confirm that the result is x^n
 *
 * F - Field over which to perform computations
 * n - Dimension to which to make matrix
 *
 * Return true on success and false on failure
 */

template <class Field, class Meth>
static bool testNilpotentMinpoly (Field &F, size_t n, const Meth& M)
{
	typedef BlasVector<Field> Polynomial;
	typedef SparseMatrix<Field> Blackbox;
	typedef typename Blackbox::Row Row;

	commentator().start ("Testing nilpotent minpoly", "testNilpotentMinpoly");

	bool ret;
	bool lowerTermsCorrect = true;

	size_t i;

	StandardBasisStream<Field, Row> stream (F, n);
	Row v;
	stream.next (v);
	Blackbox A (F, stream); // first subdiagonal is 1's.

	Polynomial phi(F,n+1);

	minpoly (phi, A, M);

	ostream &report = commentator().report (Commentator::LEVEL_IMPORTANT, INTERNAL_DESCRIPTION);
        A.write (report, Tag::FileFormat::Maple);
	report << "Minimal polynomial is: ";
	printPolynomial (F, report, phi);

	for (i = 0; i < n - 1; i++)
		if (!F.isZero (phi[i]))
			lowerTermsCorrect = false;

	if (phi.size () == n + 1 && F.isOne (phi[n]) && lowerTermsCorrect)
		ret = true;
	else {
		ret = false;
		commentator().report (Commentator::LEVEL_IMPORTANT, INTERNAL_ERROR)
			<< "ERROR: A^n = 0, should get x^" << n <<", got ";
		printPolynomial (F, report, phi);
	}

	commentator().stop (MSG_STATUS (ret), (const char *) 0, "testNilpotentMinpoly");

	return ret;
}

/* Test 3: Random minpoly of sparse matrix
 *
 * Generates a random sparse matrix with K nonzero elements per row and computes
 * its minimal polynomial. Then computes random vectors and applies the
 * polynomial to them in Horner style, checking whether the result is 0.
 *
 * F - Field over which to perform computations
 * n - Dimension to which to make matrix
 * K - Number of nonzero elements per row
 * numVectors - Number of random vectors to which to apply the minimal polynomial
 *
 * Return true on success and false on failure
 */

template <class Field, class BBStream, class VectStream, class Meth>
bool testRandomMinpoly (Field                 &F,
			int                    iterations,
			BBStream    &A_stream,
			VectStream &v_stream,
                        const Meth& M)
{
	typedef BlasVector<Field> Polynomial;
	typedef SparseMatrix<Field> Blackbox;
        typedef typename VectStream::Vector Vector;

        commentator().start ("Testing sparse random minpoly", "testRandomMinpoly", (unsigned int)iterations);

	bool ret = true;

	VectorDomain<Field> VD (F);

	Vector v, w;

	VectorWrapper::ensureDim (v, v_stream.n ());
	VectorWrapper::ensureDim (w, v_stream.n ());

	for (int i = 0; i < iterations; i++) {
		commentator().startIteration ((unsigned int)i);

		bool iter_passed = true;

		A_stream.reset ();
		Blackbox A (F, A_stream);

		ostream &report = commentator().report (Commentator::LEVEL_UNIMPORTANT, INTERNAL_DESCRIPTION);
		report << "Matrix:" << endl;
		A.write (report, Tag::FileFormat::Maple);

		Polynomial phi(F);

		minpoly (phi, A, M );

		report << "Minimal polynomial is: ";
		printPolynomial (F, report, phi);

		commentator().report (Commentator::LEVEL_IMPORTANT, INTERNAL_DESCRIPTION)
			<< "deg minpoly (A) = " << phi.size () - 1 << endl;

		v_stream.reset ();

		while (iter_passed && v_stream) {
			v_stream.next (v);

			report << "Input vector  " << v_stream.j () << ": ";
			VD.write (report, v);
			report << endl;

			applyPoly (F, w, A, phi, v);

			report << "Output vector " << v_stream.j () << ": ";
			VD.write (report, w);
			report << endl;

			if (!VD.isZero (w))
				ret = iter_passed = false;
		}

		if (!iter_passed)
			commentator().report (Commentator::LEVEL_IMPORTANT, INTERNAL_ERROR)
				<< "ERROR: A = rand, purported-minpoly(A) is not zero." << endl;
		commentator().stop ("done");
		commentator().progress ();
	}

	commentator().stop (MSG_STATUS (ret), (const char *) 0, "testRandomMinpoly");

	return ret;
}

/* Test 4: test gram matrix.
     A test of behaviour with self-orthogonal rows and cols.

	 Gram matrix is 1's offdiagonal and 0's on diagonal.  Self orthogonality when characteristic | n-1.

	 Arg m is ignored.
	 if p := characteristic is small then size is p+1 and minpoly is x^2 + x
	 (because A^2 = (p-1)A).
	 if p is large, size is 2 and minpoly is x^2 -1.
*/
template <class Field, class Meth>
static bool testGramMinpoly (Field &F, size_t m, const Meth& M)
{
	commentator().start ("Testing gram minpoly", "testGramMinpoly");
	typedef BlasVector<Field> Polynomial;
	integer n;
	F.characteristic(n); n += 1;
	if (n > 30) n = 2;
	Polynomial phi(F);
	BlasMatrix<Field> A(F, n, n);
	for (size_t i = 0; i < n; ++i) for (size_t j = 0; j < n; ++j) A.setEntry(i, j, F.one);
	for (size_t i = 0; i < n; ++i) A.setEntry(i, i, F.zero);
	minpoly(phi, A, M);

	ostream &report = commentator().report (Commentator::LEVEL_IMPORTANT, INTERNAL_DESCRIPTION);
        A.write (report);
	report << "Minimal polynomial is: ";
	printPolynomial<Field, Polynomial> (F, report, phi);

	bool ret;
	if (n == 2)
		if ( phi.size() == 3 && F.areEqual(phi[0], F.mOne) && F.isZero(phi[1]) && F.isOne(phi[2]))
			ret = true;
		else
		{
			commentator().report (Commentator::LEVEL_IMPORTANT, INTERNAL_ERROR)
				<< "ERROR: A = gram, should get x^2 - x, got ";
			printPolynomial<Field, Polynomial> (F, report, phi);
			ret = false;
		}
	else
		if (phi.size() == 3 && F.isZero(phi[0]) && F.isOne(phi[1]) && F.isOne(phi[2]))
			ret = true;
		else
		{
			commentator().report (Commentator::LEVEL_IMPORTANT, INTERNAL_ERROR)
				<< "ERROR: A = gram, should get x^2 + x, got ";
			printPolynomial<Field, Polynomial> (F, report, phi);
			ret = false;
		}
		commentator().stop (MSG_STATUS (ret), (const char *) 0, "testGramMinpoly");
		return ret;
}

int main (int argc, char **argv)
{
	commentator().setMaxDetailLevel (-1);

	commentator().setMaxDepth (-1);
	bool pass = true;

	static size_t n = 10;
	//static integer q = 65521U;
	static integer q = 1000003; // ok for both Givaro::Modular<int> and Givaro::Modular<double>
	static int e = 1; // exponent for field characteristic
	static int iterations = 1;
	static int numVectors = 1;
	static int k = 3;

	static Argument args[] = {
		{ 'n', "-n N", "Set dimension of test matrices to NxN.", TYPE_INT,     &n },
		{ 'q', "-q Q", "Operate over the \"field\" GF(Q^E) [1].", TYPE_INTEGER, &q },
		{ 'e', "-e E", "Operate over the \"field\" GF(Q^E) [1].", TYPE_INT, &e },
		{ 'i', "-i I", "Perform each test for I iterations.", TYPE_INT,     &iterations },
		{ 'v', "-v V", "Use V test vectors for the random minpoly tests.", TYPE_INT,     &numVectors },
		{ 'k', "-k K", "K nonzero Elements per row in sparse random apply test.", TYPE_INT,     &k },
		END_OF_ARGUMENTS
	};


	parseArguments (argc, argv, args);
	commentator().getMessageClass (TIMING_MEASURE).setMaxDepth (10);
	commentator().getMessageClass (INTERNAL_DESCRIPTION).setMaxDepth (10);
	commentator().getMessageClass (INTERNAL_DESCRIPTION).setMaxDetailLevel (Commentator::LEVEL_UNIMPORTANT);

	ostream &report = commentator().report (Commentator::LEVEL_IMPORTANT, INTERNAL_DESCRIPTION);
	// /////////////// finite field part //////////////////
	Method::Blackbox MB;
	//if (e == 1)
	{
		//typedef Givaro::Modular<uint32_t> Field;
		//typedef Givaro::Modular<int> Field;
		typedef Givaro::Modular<double> Field;
		Field F (q);
		srand ((unsigned)time (NULL));

		commentator().start("Blackbox prime field minpoly test suite", "Wminpoly");

		if (!testZeroMinpoly  	  (F, n, MB)) pass = false;
		if (!testIdentityMinpoly  (F, n, MB)) pass = false;
		if (!testNilpotentMinpoly (F, n, MB)) pass = false;
		//if (!testRandomMinpoly    (F, n)) pass = false;
		if (!testGramMinpoly      (F, n, MB)) pass = false;
		//need other tests...

		commentator().stop("Blackbox prime field minpoly test suite");
	}
	//else
	{	q = 3; e = 10;

		typedef Givaro::GFqDom<int64_t> Field;
		Field F (q, e);
		srand ((unsigned)time (NULL));

		commentator().start("Blackbox gfq field minpoly test suite", "Wminpoly");
		F.write(report);

		if (!testZeroMinpoly  	  (F, n, MB)) pass = false;
		if (!testIdentityMinpoly  (F, n, MB)) pass = false;
		if (!testNilpotentMinpoly (F, n, MB)) pass = false;
		//if (!testRandomMinpoly    (F, n)) pass = false;
		if (!testGramMinpoly      (F, n, MB)) pass = false;
		//need other tests...

		commentator().stop("Blackbox gfq minpoly test suite");
	}

#if 0
// this test can only work if Extension of Extension is compiling. -bds

	{	q = 3; e = 2;
		typedef Givaro::Modular<int32_t> GroundField;
		typedef Givaro::Extension<GroundField> Field;
		GroundField K(q);
		Field F (K, e);
		srand ((unsigned)time (NULL));
		MB.certificate(false);

		commentator().start("Blackbox givaro extension field minpoly test suite", "Wminpoly");
		F.write(report);

		if (!testZeroMinpoly  	  (F, n, MB)) pass = false;
		if (!testIdentityMinpoly  (F, n, MB)) pass = false;
		if (!testNilpotentMinpoly (F, n, MB)) pass = false;
		//if (!testRandomMinpoly    (F, n)) pass = false;
		if (!testGramMinpoly      (F, n, MB)) pass = false;
		//need other tests...

		commentator().stop("Blackbox givaro extension field minpoly test suite");
	}
#endif

#if 1

	Givaro::Modular<uint32_t> F (q);


	commentator().start("Hybrid prime field minpoly test suite", "Hminpoly");
	if (!testIdentityMinpoly  (F, n, Method::Hybrid())) pass = false;
	if (!testNilpotentMinpoly (F, n, Method::Hybrid())) pass = false;
	commentator().stop("Hybrid prime field minpoly test suite");
#elif 0
	commentator().start("Blackbox prime field minpoly test suite", "Bminpoly");
	if (!testIdentityMinpoly  (F, n, false,  Method::Blackbox())) pass = false;
	if (!testNilpotentMinpoly (F, n, Method::Blackbox())) pass = false;
	commentator().stop("Blackbox prime field minpoly test suite");

	commentator().start("Elimination prime field minpoly test suite", "Eminpoly");
	if (!testIdentityMinpoly  (F, n, false,  Method::Elimination())) pass = false;
	if (!testNilpotentMinpoly (F, n, Method::Elimination())) pass = false;
	commentator().stop("Elimination prime field minpoly test suite");

	// /////////////// integer part //////////////////
	typedef vector<Givaro::ZRing<Integer>::Element> ZDenseVector;
	typedef SparseMatrix<Givaro::ZRing<Integer> >::Row ZSparseVector;
	//typedef pair<vector<size_t>, vector<Field::Element> > SparseVector;
	Givaro::ZRing<Integer> Z;
	srand ((unsigned)time (NULL));

	commentator().getMessageClass (TIMING_MEASURE).setMaxDepth (10);
	commentator().getMessageClass (INTERNAL_DESCRIPTION).setMaxDepth (10);
	commentator().getMessageClass (INTERNAL_DESCRIPTION).setMaxDetailLevel (Commentator::LEVEL_UNIMPORTANT);

	commentator().start("Blackbox integer minpoly test suite", "WIminpoly");

	RandomDenseStream<Givaro::ZRing<Integer>, ZDenseVector, NonzeroRandIter<Givaro::ZRing<Integer> > >
	zv_stream (Z, NonzeroRandIter<Givaro::ZRing<Integer> > (Z, Givaro::ZRing<Integer>::RandIter (Z)), n, numVectors);
	RandomSparseStream<Givaro::ZRing<Integer>, ZSparseVector, NonzeroRandIter<Givaro::ZRing<Integer> > >
	zA_stream (Z, NonzeroRandIter<Givaro::ZRing<Integer> > (Z, Givaro::ZRing<Integer>::RandIter (Z)), (double) k / (double) n, n, n);

	if (!testIdentityMinpoly  (Z, n, MB)) pass = false;
	if (!testNilpotentMinpoly (Z, n, MB)) pass = false;

	if (!testRandomMinpoly    (Z, iterations, zA_stream, zv_stream)) pass = false;

	commentator().stop("Blackbox integer minpoly test suite");

	commentator().start("Hybrid integer minpoly test suite", "HIminpoly");
	if (!testIdentityMinpoly  (Z, n, Method::Hybrid())) pass = false;
	if (!testNilpotentMinpoly (Z, n, Method::Hybrid())) pass = false;
	commentator().stop("Hybrid integer minpoly test suite");

	commentator().start("Blackbox integer minpoly test suite", "BIminpoly");
	if (!testIdentityMinpoly  (Z, n, Method::Blackbox())) pass = false;
	if (!testNilpotentMinpoly (Z, n, Method::Blackbox())) pass = false;
	commentator().stop("Blackbox integer minpoly test suite");

	commentator().start("Elimination integer minpoly test suite", "EIminpoly");
	if (!testIdentityMinpoly  (Z, n, Method::Elimination())) pass = false;
	if (!testNilpotentMinpoly (Z, n, Method::Elimination())) pass = false;
	commentator().stop("Elimination integer minpoly test suite");

#endif
	return pass ? 0 : -1;
}

// Local Variables:
// mode: C++
// tab-width: 8
// indent-tabs-mode: nil
// c-basic-offset: 8
// End:
// vim:sts=8:sw=8:ts=8:noet:sr:cino=>s,f0,{0,g0,(0,\:0,t0,+0,=s
