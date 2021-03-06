/* tests/test-ntl-lzz_pEX.C
 * evolved by bds from tests/test-ntl-lzz_pX.C
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

/*! @file  tests/test-ntl-lzz_pEX.C
 * @ingroup tests
 * @brief no doc.
 * @test no doc.
 */


#include "linbox/linbox-config.h"

#include <iostream>
#include <fstream>


#include "linbox/ring/ntl.h"

#include "test-field.h"

using namespace LinBox;

int main (int argc, char **argv)
{
	static integer q = 3;
	static size_t n = 10000;
	static int iterations = 1;

	static Argument args[] = {
   		{ 'q', "-q Q", "Operate over the \"field\" GF(Q) [1].", TYPE_INTEGER, &q },
		{ 'n', "-n N", "Set dimension of test vectors to NxN.", TYPE_INT,     &n },
		{ 'i', "-i I", "Perform each test for I iterations.", TYPE_INT,     &iterations },
		END_OF_ARGUMENTS
   	};

   	parseArguments (argc, argv, args);

	commentator().start("NTL_zz_pEX field test suite", "NTL_zz_pEX");
	bool pass = true;

	NTL_zz_pEX R(q,3);

    integer ch;
	if (R.characteristic(ch) != q)
		exit(-1);

	// Make sure some more detailed messages get printed
	commentator().getMessageClass (INTERNAL_DESCRIPTION).setMaxDepth (2);

	if (!runBasicRingTests (R, "NTL_zz_pEX", (unsigned int)iterations, false)) pass = false;
	// needs PID tests as well...

#if 0
	FieldArchetype K(new NTL_zz_pEX(101));

	if (!testField<FieldArchetype> (K, "Testing archetype with envelope of UnField<NTL::zz_pEX> field"))
		pass = false;
#endif

	commentator().stop("NTL_zz_pEX field test suite");
	return pass ? 0 : -1;
}

// vim:sts=8:sw=8:ts=8:noet:sr:cino=>s,f0,{0,g0,(0,:0,t0,+0,=s
// Local Variables:
// mode: C++
// tab-width: 8
// indent-tabs-mode: nil
// c-basic-offset: 8
// End:

