/* -*- mode: C++; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */

/* linbox/blackbox/scalar.h
 * Copyright (C) 2002 by -bds  
 * evolved from diagonal.h written by William J Turner and Bradford Hovinen 
 *
 * -------------------------------
 * Modified by Dmitriy Morozov <linbox@foxcub.org>. May 28, 2002.
 *
 * Added parametrization of VectorCategory tags by VectorTraits. See
 * vector-traits.h for more details.
 *
 * -------------------------------
 */

#ifndef __SCALAR_H
#define __SCALAR_H

#include <algorithm>
#include "linbox/vector/vector-traits.h"
#include "linbox/util/debug.h"
#include "linbox-config.h"
#include <linbox/blackbox/blackbox-interface.h>

namespace LinBox
{

	/** \brief Blackbox for aI.  Use particularly for representing 0 and I.

	 * This is a class of blackbox square scalar matrices.
	 * Each scalar matrix occupies O(scalar-size) memory.
	 * The matrix itself is not stored in memory, just the scalar and the dimensions.
	 * \ingroup blackbox
	 */
	template <class _Field>
	class ScalarMatrix : public  BlackboxInterface 
	{
	    public:
		
		typedef _Field Field;
		typedef typename Field::Element        Element;

		/*  In each specialization, I must define suitable constructor(s) and
		BlackboxArchetype<Vector> * clone() const;
		Vector& apply(Vector& y, const Vector& x) const;
		Vector& applyTranspose(Vector& y, const Vector& x) const;
		size_t rowdim(void) const;
		size_t coldim(void) const;
		*/

		/// Constructs an initially 0 by 0 matrix.
		ScalarMatrix ()	:  _n(0) {}

		/** Scalar matrix Constructor from an element.
		 * @param F	field in which to do arithmetic.
		 * @param n	size of the matrix.
		 * @param s	scalar, a field element, to be used as the diagonal of the matrix.
		 */
		ScalarMatrix (const Field &F, const size_t n, const Element &s)
			: _F(F), _n(n), _v(s) {}

		/** Constructor from a random element.
		 * @param F    field in which to do arithmetic.
		 * @param n    size of the matrix.
		 * @param iter Random iterator from which to get the diagonal scalar element.
		 */
		ScalarMatrix (const Field &F, const size_t n, const typename Field::RandIter& iter)
			: _F(F), _n(n) { iter.random(_v); }

		ScalarMatrix(const ScalarMatrix<Field> &M) : _F(M._F)
		{
			_n = M._n;
			_v = M._v;
		}


		/** Application of BlackBox matrix.
		 * y= A*x.
		 * Requires time linear in n, the size of the matrix.
		 */
                template<class OutVector, class InVector>
		OutVector& apply(OutVector &y, const InVector &x) const 
		{
			typename VectorTraits<InVector>::VectorCategory t;
			return _app (y, x, t);
		}

		/** Application of BlackBox matrix transpose.
		 * y= transpose(A)*x.
		 * Requires time linear in n, the size of the matrix.
		 */
		template<class OutVector, class InVector>
		OutVector& applyTranspose(OutVector &y, const InVector &x) const
			{ return apply(y, x); }  // symmetric matrix.


		template<typename _Tp1> 
		struct rebind 
		{ typedef ScalarMatrix<_Tp1> other; };


		size_t rowdim(void) const { return _n; }
    
		size_t coldim(void) const { return _n; }

		const Field& field() const {return _F;}

		// for a specialization in solutions
		Element& trace(Element& t) const
		{	Element n; _F.init(n, _n); 
		return _F.mul(t, _v, n);  
		}

		

	    protected:

		Field _F;   // Field for arithmetic

		size_t _n;  // Number of rows and columns of square matrix.

		Element _v; // the scalar used in applying matrix.

		// dense vector _app for apply
		template<class OutVector, class InVector>
		OutVector& _app (OutVector &y, const InVector &x, VectorCategories::DenseVectorTag) const;
		// The third argument is just a device to let overloading determine the method.

		// sparse sequence vector _app for apply

		
		template <class OutVector, class InVector>
		OutVector& _app (OutVector &y, const InVector &x, VectorCategories::SparseSequenceVectorTag) const;

		// sparse associative vector _app for apply
		template<class OutVector, class InVector>
		OutVector& _app (OutVector &y, const InVector &x, VectorCategories::SparseAssociativeVectorTag) const;

	}; // template <Field> class ScalarMatrix

	// dense vector _app
	template <class Field>
	template <class OutVector, class InVector>
	inline OutVector &ScalarMatrix<Field>
		::_app(OutVector& y, const InVector& x, VectorCategories::DenseVectorTag t) const
		{   
		    linbox_check (x.size() >= _n);
		    linbox_check (y.size() >= _n);
		    typename OutVector::iterator y_iter = y.begin ();

		    if (_F.isZero(_v)) // just write zeroes
		        for ( ; y_iter != y.end ();  ++y_iter) *y_iter = _v;
                    else if (_F.isOne(_v) ) // just copy 
			copy(x.begin(), x.end(), y.begin());
		    else // use actual muls
		    {   typename InVector::const_iterator x_iter = x.begin ();
		            for (  ; y_iter != y.end () ; ++y_iter, ++x_iter )
		                _F.mul (*y_iter, _v, *x_iter);
		    }
		    return y;

		} // dense vector _app

		
	// sparse sequence vector _app
	template <class Field>
	template <class OutVector, class InVector>
	inline OutVector &ScalarMatrix<Field>
		::_app(OutVector& y, const InVector& x, VectorCategories::SparseSequenceVectorTag t) const
	{
		//linbox_check ((!x.empty ()) && (_n < x.back ().first));
		// neither is required of x ?

		y.clear (); // we'll overwrite using push_backs.

		// field element to be used in calculations
		Element entry;
		_F.init (entry, 0); // needed?

		// For each element, multiply input element with corresponding element
		// of stored scalar and insert non-zero elements into output vector
		for ( typename InVector::const_iterator x_iter = x.begin (); x_iter != x.end (); ++x_iter) 
		{	_F.mul (entry, _v, x_iter->second);
			if (!_F.isZero (entry)) y.push_back (make_pair (x_iter->first, entry));
		} 

		return y;
	} // sparse sequence vector _app

	// sparse associative vector _app
	template <class Field>
		template <class OutVector, class InVector>
	inline OutVector& ScalarMatrix<Field>
	::_app(OutVector& y, const InVector& x, VectorCategories::SparseAssociativeVectorTag t) const
	{
		y.clear (); // we'll overwrite using inserts

		// create field elements and size_t to be used in calculations
		Element entry;
		_F.init (entry, 0);

		// Iterator over indices of input vector.
		// For each element, multiply input element with 
		// stored scalar and insert non-zero elements into output vector
		for ( typename InVector::const_iterator x_iter = x.begin (); x_iter != x.end (); ++x_iter)
		{	_F.mul (entry, _v, x_iter->second);
			if (!_F.isZero (entry)) y.insert (y.end (), make_pair (x_iter->first, entry));
		}

		return y;
	} // sparse associative vector _app

} // namespace LinBox
#endif // __ScalarMatrix
