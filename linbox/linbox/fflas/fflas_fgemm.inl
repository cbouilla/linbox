/* -*- mode: C++; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/* linbox/fflas/fflas_fgemm.inl
 * Copyright (C) 2003 Clement Pernet
 *
 * Written by Clement Pernet <Clement.Pernet@imag.fr>
 *
 * Warning,
 * TRANSPOSE option will force classic matrix multiplication
 *
 * See COPYING for license information.
 */
#ifndef MAX
#define MAX(a,b) (a<b)?b:a
#endif
#ifndef MIN
#define MIN(a,b) (a>b)?b:a
#endif

#include <linbox/field/modular-double.h>
// Note:
// The domain is supposed to be a field since some divisions are required for efficiency purposes
// An alternative has to written for finite rings if necessary

// Borne pour une représentation modulaire
template <class Field>
inline size_t LinBox::FFLAS::FflasKmax( size_t& kmax, const Field& F, const size_t w,
					const typename Field::Element beta ){

	static typename Field::Element mone;
	F.init( mone, -1 );
	integer charac;
	F.characteristic(charac);		
	if ( w>0 ){
		size_t ex=1;
		for ( size_t i=0; i < w; ++i ) 	ex *= 3;
		long long c = (charac-1)*(ex)/2;
//		long long c = (charac-1)*(1+ex)/2;
		kmax = ( (( (long long) 1<<53) )/(c*c) + 1)*(1<<w);
		if ( kmax == (size_t) (1<<w) )
			kmax=2;
	}
	else{
		long long c = charac-1;
		long long cplt=0;
		if ( !F.isZero( beta ) )
			if ( F.isOne( beta ) || F.areEqual( beta, mone ) )
				cplt = c;
			else cplt = c*c;
		kmax = ( ( (long long) 1<<53) - cplt ) /(c*c);
	}
	return kmax;
}


// Classic Multiplication over double
template <>
inline  void LinBox::FFLAS::ClassicMatmul(const DoubleDomain& F, 
					  const enum FFLAS_TRANSPOSE ta,
					  const enum FFLAS_TRANSPOSE tb,
					  const size_t m, const size_t n,const size_t k,
					  const DoubleDomain::Element alpha,
					  const DoubleDomain::Element * Ad, const size_t lda,
					  const DoubleDomain::Element * Bd, const size_t ldb,
					  const DoubleDomain::Element beta,
					  DoubleDomain::Element * Cd, const size_t ldc, 
					  const size_t kmax){
	
	// Call to the blas multiplication
	cblas_dgemm(CblasRowMajor, (enum CBLAS_TRANSPOSE) ta, 
		    (enum CBLAS_TRANSPOSE) tb,
		    m, n, k, (DoubleDomain::Element) alpha,
		    Ad, lda, Bd, ldb, (DoubleDomain::Element) beta,Cd, ldc);
}


// Classic multiplication over a finite Field
template <class Field>
void LinBox::FFLAS::ClassicMatmul( const Field& F,  
				   const enum FFLAS_TRANSPOSE ta,
				   const enum FFLAS_TRANSPOSE tb,
				   const size_t m, const size_t n,const size_t k,
				   const typename Field::Element alpha,
				   const typename Field::Element * A, const size_t lda,
				   const typename Field::Element * B, const size_t ldb,
				   const typename Field::Element beta,
				   typename Field::Element* C, const size_t ldc,
				   const size_t kmax ){
	static  typename Field::Element Mone;
	static  typename Field::Element one;
	F.init(Mone, -1);
	F.init(one, 1);
	typename Field::Element tmp;
	DoubleDomain::Element alphad, betad;
	size_t k2 = MIN(k,kmax-1); // Size of the blocks
	DoubleDomain::Element * Add = new DoubleDomain::Element[m*k2];
	DoubleDomain::Element * Bdd = new DoubleDomain::Element[k2*n];
	DoubleDomain::Element * Cd = new DoubleDomain::Element[m*n];
	
	// To ensure the initial computation with beta
	size_t nblock = k / (kmax-1);
	size_t remblock = k % (kmax-1);
	if (!remblock){
		remblock = kmax - 1 ;
		--nblock;
	}
	if ( F.areEqual( Mone, beta ) )
		betad = -1.0;
	else
		F.convert( betad, beta );
	
	if ( F.areEqual( Mone, alpha ) )
		alphad = -1.0;
	else{
		alphad = 1.0;
		if (! F.areEqual( one, alpha) ){
			// Compute y = A*x + beta/alpha.y
			// and after y *= alpha
			F.div( tmp, beta, alpha );
			F.convert( betad, tmp );
		}
	}
	
	size_t dlda, dldb;
	if (!F.isZero(beta))
		MatF2MatD( F, Cd, n, C, ldc, m, n ); 

	if (ta == FflasTrans){ 
		dlda = m; 
		MatF2MatD( F, Add, dlda, A+k2*nblock*lda, lda, remblock, m ); 
	}
	else{ 
		dlda = k2; 
		MatF2MatD( F, Add, dlda, A+k2*nblock, lda, m, remblock);	
	}
	if (tb == FflasTrans){ 
		dldb = k2; 
		MatF2MatD( F, Bdd, k2, B+k2*nblock, ldb, n, remblock ); 
	}
	else{ 
		dldb = n; 
		MatF2MatD( F, Bdd, dldb, B+k2*nblock*ldb, ldb, remblock, n ); 
	}
	
// 	std::cout<<"m,n,remblock,alphad,betad,k2,nblock"<<m<<" "<<n<<" "
//	 <<remblock<<" "<<alphad<<" "<<betad<<" "<<k2<<" "<<nblock<<std::endl;
// 	std::cout<<"A="<<std::endl;
// 	write_field(F, std::cout, A, m,k,lda);
// 	std::cout<<"Add="<<std::endl;
// 	write_field(DoubleDomain(), std::cout, Add, m,remblock,k2);
// 	std::cout<<"B="<<std::endl;
// 	write_field(F, std::cout, B, k,n,ldb);
// 	std::cout<<"Bdd="<<std::endl;
// 	write_field(DoubleDomain(), std::cout, Bdd, remblock,n,n);
// 	std::cout<<"C="<<std::endl;
// 	write_field(F, std::cout,C, m,n,ldc);
// 	std::cout<<"Cd="<<std::endl;
// 	write_field(DoubleDomain(), std::cout,Cd, m,n,n);

	ClassicMatmul( DoubleDomain(), ta, tb, m, n, remblock, alphad, Add, dlda,
		       Bdd, dldb, betad, Cd, n, kmax );
// 	std::cout<<"Avant conversion:"<<std::endl;
// 	std::cout<<"C="<<std::endl;
// 	write_field(F, std::cout,C, m,n,ldc);
// 	std::cout<<"Cd="<<std::endl;
// 	write_field(DoubleDomain(), std::cout,Cd, m,n,n);
	MatD2MatF( F, C, ldc, Cd, n, m, n );
	MatF2MatD( F, Cd, n, C, ldc, m, n );
// 	std::cout<<"Apres conversion:"<<std::endl;
// 	std::cout<<"C="<<std::endl;
// 	write_field(F, std::cout,C, m,n,ldc);
// 	std::cout<<"Cd="<<std::endl;
// 	write_field(DoubleDomain(), std::cout,Cd, m,n,n);

	for ( size_t i = 0; i < nblock; ++i ){

		if (ta == FflasTrans){ 
			dlda = m; 
			MatF2MatD( F, Add, dlda, A+k2*i*lda, lda, k2, m ); 
		}
		else{ 
			dlda = k2; 
			MatF2MatD( F, Add, dlda,  A+k2*i, lda, m, k2); 
		}
		if (tb == FflasTrans){ 
			dldb = k2; 
			MatF2MatD( F, Bdd, dldb, B+k2*i, ldb, n, k2 ); 
		}
		else{ 
			dldb = n; 
			MatF2MatD( F, Bdd, dldb, B+k2*i*ldb, ldb, k2, n );	
		}
		
// 		std::cout<<"Add="<<std::endl;
// 		write_field(DoubleDomain(), std::cout, Add, m,k2, k2);
// 		std::cout<<"Bdd="<<std::endl;
// 		write_field(DoubleDomain(), std::cout, Bdd, k2,n,n);
// 		std::cout<<"C="<<std::endl;
// 		write_field(F, std::cout,C, m,n,ldc);
// 		std::cout<<"Cd="<<std::endl;
// 		write_field(DoubleDomain(), std::cout,Cd, m,n,n);
		ClassicMatmul( DoubleDomain(), ta, tb, m, n, k2, alphad, Add, dlda,
			       Bdd, dldb, 1.0, Cd, n, kmax );

// 	std::cout<<"Avant conversion:"<<std::endl;
// 	std::cout<<"C="<<std::endl;
// 	write_field(F, std::cout,C, m,n,ldc);
// 	std::cout<<"Cd="<<std::endl;
// 	write_field(DoubleDomain(), std::cout,Cd, m,n,n);
	
	MatD2MatF( F, C, ldc, Cd, n, m, n );
	MatF2MatD( F, Cd, n, C, ldc, m, n );
	
// 	std::cout<<"Apres conversion:"<<std::endl;
// 	std::cout<<"C="<<std::endl;
// 	write_field(F, std::cout,C, m,n,ldc);
// 	std::cout<<"Cd="<<std::endl;
// 	write_field(DoubleDomain(), std::cout,Cd, m,n,n);
	}
	
	if ( (!F.areEqual( one, alpha )) && (!F.areEqual( Mone, alpha)) ){
		for ( typename Field::Element * Ci = C; Ci < C+m*ldc; Ci += ldc)
			for ( size_t j = 0; j < n; ++j ) 
				F.mulin( *( Ci + j ), alpha );
	}
	
	// Conversion double => Finite Field
	delete[] Add;
	delete[] Bdd;
// if ( !F.areEqual(alpha, Mone) && !F.isOne(alpha)){
// 		// Cd may contain approximations of C entries:
// 		// Perturbation of Cd to get C from truncation.
		
// 		for ( double* Cdi = Cd; ; Cdi<Cd+m*ldc; Cdi+=ldc)
// 			for(size_t j=0; j<n; ++j)
// 				if (*(Cdi+j)>=0){
// 					*(Cdi+j) += 0.1;
// 				}
// 				else
// 					*(Cdi+j) -= 0.1;
// 	}
	delete[] Cd;
}


template <>
void LinBox::FFLAS::ClassicMatmul( const Modular<double>& F,  
				   const enum FFLAS_TRANSPOSE ta,
				   const enum FFLAS_TRANSPOSE tb,
				   const size_t m, const size_t n,const size_t k,
				   const double alpha, 
				   const double * A, const size_t lda,
				   const double * B, const size_t ldb,
				   const double beta,
				   double* C, const size_t ldc, 
				   const size_t kmax ){
	static  double Mone, one;
	F.init(Mone, -1);
	F.init(one, 1);
	double _alpha, _beta;
	// To ensure the initial computation with beta
	size_t k2 = MIN(k,kmax-1);
	size_t nblock = k / (kmax-1);
	size_t remblock = k % (kmax-1);
	if (!remblock){
		remblock = kmax-1;
		--nblock;
	}
	if ( F.areEqual( Mone, beta ) ) 
		_beta = -1.0;
	else
		_beta = beta;

	if ( F.areEqual( Mone, alpha ) )
		_alpha = -1.0;
	else{
		_alpha = 1.0;
		if (! F.areEqual( one, alpha) ){
			// Compute y = A*x + beta/alpha.y
			// and after y *= alpha
			F.divin( _beta, alpha );
		}
	}
	size_t shiftA, shiftB;
	if (ta == FflasTrans){ 
		shiftA = k2*lda;
	}
	else{ 
		shiftA = k2;
	}
	if (tb == FflasTrans){ 
		shiftB = k2;
	}
	else{ 
		shiftB = k2*ldb;
	}
	
	ClassicMatmul( DoubleDomain(), ta, tb, m, n, remblock, _alpha, A+nblock*shiftA, lda,
		       B+nblock*shiftB, ldb, _beta, C, ldc, kmax );
	for ( double * Ci = C; Ci != C+m*ldc; Ci += ldc)
		for ( size_t j=0; j<n;++j)
			F.init(*(Ci+j),*(Ci+j));
	for ( size_t i = 0; i < nblock; ++i ){
		ClassicMatmul( DoubleDomain(), ta, tb, m, n, k2, _alpha, A+i*shiftA, lda,
			       B+i*shiftB, ldb, one, C, ldc, kmax );
		for ( double * Ci = C; Ci != C+m*ldc; Ci += ldc)
			for ( size_t j=0; j<n;++j)
				F.init(*(Ci+j),*(Ci+j));
	}
	
	if ( (!F.areEqual( one, alpha )) && (!F.areEqual( Mone, alpha)) ){
		for ( double * Ci = C; Ci < C+m*ldc; Ci += ldc)
			for ( size_t j = 0; j < n; ++j ) 
				F.mulin( *( Ci + j ), alpha );
	}
}

// Winograd Multiplication  A(n*k) * B(k*m) in C(n*m)

// Computation of the 22 Winograd's operations
template<class Field>
inline  void LinBox::FFLAS::WinoCalc( const Field& F, 
				      const enum FFLAS_TRANSPOSE ta,
				      const enum FFLAS_TRANSPOSE tb,
				      const size_t mr, const size_t nr, const size_t kr,
				      const typename Field::Element alpha,
				      const typename Field::Element* A,const size_t lda,
				      const typename Field::Element* B,const size_t ldb,
				      const typename Field::Element beta,
				      typename Field::Element * C, const size_t ldc,
				      long long kmax, size_t w){
	static typename Field::Element zero, one;
	F.init ( zero, 0 );
	F.init ( one, 1 );

	typename Field::Element mbeta;
	F.neg(mbeta,beta);
	size_t i,j, imaxb, jmaxb, imaxa, jmaxa, ldx2, ldx3;
	size_t x3rd = MAX(mr,kr);
	typename Field::Element tmpU2, tmpU3;
	const typename Field::Element* d11,*d12,*d21,*d22;
	typename Field::Element* d11c,*d12c,*d21c,*d22c,*dx1,*dx2,*dx3;
	const typename Field::Element * A11=A, *A12, *A21, *A22;
	const typename Field::Element * B11=B, *B12, *B21, *B22;
	typename Field::Element * C11=C, *C12=C+nr, *C21=C+mr*ldc, *C22=C+nr+mr*ldc;
	
	if ( ta == FflasTrans ){
		A21 = A + mr;
		A12 = A + kr*lda;
		A22 = A12 + mr;
		imaxa = kr;
		ldx2 = jmaxa = mr;
	}
	else{
		A12 = A + kr;
		A21 = A + mr*lda;
		A22 = A21 + kr;
		imaxa = mr;
		ldx2 = jmaxa = kr;
	}
	if ( tb == FflasTrans ){
		B21 = B + kr;
		B12 = B + nr*ldb;
		B22 = B12 + kr;
		imaxb = nr;
		jmaxb = kr;
		ldx3 = x3rd;
	}
	else{
		B12 = B + nr;
		B21 = B + kr*ldb;
		B22 = B21 + nr;
		imaxb = kr;
		ldx3 = jmaxb = nr;
	}
	// Three temporary submatrices are required
	typename Field::Element* X1 = new typename Field::Element[mr*nr];
	typename Field::Element* X2 = new typename Field::Element[mr*kr];
	typename Field::Element* X3 = new typename Field::Element[x3rd*nr];

	// P2 = alpha . A12 * B21 + beta . C11  in C11
	WinoMain( F, ta, tb, mr, nr, kr, alpha, A12, lda, B21, ldb, beta, C11, ldc, kmax,w-1);
	
	// T3 = B22 - B12 in X3
	d12 = B12; d22 = B22; dx3 = X3;
	for ( i=0; i<imaxb; ++i, d12+=ldb, d22+=ldb, dx3+=ldx3){
		for (j=0;j<jmaxb;++j)
			F.sub( *(dx3+j), *(d22 + j), *(d12 + j) );
		
	}

	// S3 = A11 - A21 in X2 
	d11 = A11; d21 = A21; dx2 = X2; 
	for ( size_t i = 0; i < imaxa; ++i, d11 += lda, d21 += lda, dx2 += ldx2 )
		for ( size_t j = 0; j < jmaxa; ++j )
			F.sub( *(dx2+j), *(d11 + j), *(d21 + j) );

	if ( !F.isZero(beta) ){
		// C22 = C22 - C12 if beta != 0
		d12c = C12;
		d22c = C22;
		for ( size_t i = 0; i< mr; ++i, d12c += ldc, d22c += ldc )
			for ( size_t j = 0; j < nr; ++j )
				F.subin( *(d22c + j), *(d12c + j) );

		// C21 = C21 - C22
		d21c = C21;
		d22c = C22;
		for ( size_t i = 0; i< mr; ++i, d22c += ldc, d21c += ldc )
			for ( size_t j = 0; j < nr; ++j )
				F.subin( *(d21c + j), *(d22c + j) );
	}

	// P7 = alpha . S3 * T3 + beta . C22 in C22
	WinoMain( F, ta, tb, mr, nr, kr, alpha, X2, ldx2, X3, ldx3, beta, C22, ldc, kmax, w-1 );

	// T1 = B12 - B11 in X3
	d11 = B11; d12 = B12; dx3 = X3;
	for ( size_t i = 0; i < imaxb; ++i, d11 += ldb, d12 += ldb, dx3 += ldx3 ){
		for ( size_t j = 0; j < jmaxb; ++j )
			F.sub( *(dx3 + j), *(d12 + j), *(d11 + j) );
	}

	// S1 = A21 + A22 in X2
	d21 = A21; d22 = A22; dx2 = X2;
	for ( size_t i = 0; i < imaxa; ++i, d21+=lda, d22+=lda, dx2+=ldx2){
		for (size_t j=0;j<jmaxa;++j)
			F.add(*(dx2+j),*( d21 + j),*(d22 + j));
	}

	// P5 = alpha . S1*T1 + beta . C12 in C12
	WinoMain( F, ta, tb, mr, nr, kr, alpha, X2, ldx2, X3, ldx3, beta, C12, ldc, kmax, w-1 );

	// T2 = B22 - T1 in X3
	d22 = B22; dx3 = X3;
	for ( size_t i = 0; i < imaxb; ++i, d22+=ldb, dx3+=ldx3 ){
		for ( size_t j = 0; j < jmaxb; ++j )
			F.sub( *(dx3+j), *(d22 + j), *(dx3+j) );
	}
	
	// S2 = S1 - A11 in X2
	d11 = A11; dx2 = X2;
	for ( size_t i = 0; i < imaxa; ++i, d11+=lda, dx2+=ldx2 ){
		for (size_t j = 0; j < jmaxa; ++j)
			F.subin( *(dx2+j), *(d11 + j) );
	}

	// P6 = alpha . S2 * T2 in X1
	WinoMain( F, ta, tb, mr, nr, kr, alpha, X2, ldx2, X3, ldx3, zero, X1, nr, kmax, w-1 );

	// T4 = T2 - B21 in X3
	d21 = B21;dx3=X3;
	for ( size_t i = 0; i < imaxb; ++i, d21+=ldb, dx3+=ldx3 ){
		for ( size_t j = 0; j < jmaxb; ++j )
			F.subin( *(dx3+j),*( d21 + j) );
	}
	
	// S4 = A12 -S2 in X2 
	d12 = A12; dx2 = X2;
	for ( size_t i = 0; i < imaxa; ++i, d12 += lda, dx2 += ldx2 ){
		for ( size_t j = 0; j<jmaxa; ++j )
			F.sub( *(dx2+j), *(d12 + j), *(dx2+j) );
	}

	// P4 = alpha . A22 * T4 - beta . C21 in C21
	WinoMain( F, ta, tb, mr, nr, kr, alpha, A22, lda, X3, ldx3, mbeta, C21, ldc, kmax, w-1 );

	// P1 = alpha . A11 * B11 in X3
	WinoMain( F, ta, tb, mr, nr, kr, alpha, A11, lda, B11, ldb, zero, X3, nr, kmax, w-1);

	//  U1 = P2 + P1 in C11	
	d11c = C11; dx3 = X3; 
	for ( size_t i = 0; i < mr; ++i, d11c += ldc, dx3 += nr )
		for ( size_t j = 0; j < nr; ++j )
			F.addin( *(d11c + j), *(dx3 + j) );

	// U2 = P1 + P6 in tmpU2  and
	// U3 = P7 + U2 in tmpU3  and 
	// U7 = P5 + U3 in C22    and
	// U4 = P5 + U2 in C12    and
	// U6 = U3 - P4 in C21    and
	d12c = C12; dx1=X1; dx3=X3; d21c = C21; d22c = C22; 
	for ( size_t i = 0; i < mr; 
	      ++i, d12c += ldc, dx1 += nr, dx3 += nr, d22c+=ldc, d21c += ldc ){
		for (size_t j=0;j<nr;++j){
			F.add( tmpU2, *(dx3 + j), *(dx1 + j));    // temporary U2 = P1 + P6
			F.add( tmpU3, tmpU2, *(d22c + j) );      // temporary U3 = U2 + P7
			F.add( *(d22c + j), *(d12c + j), tmpU3);  // U7 = P5 + U3 in C22
			F.addin( *(d12c + j), tmpU2);             // U4 = P5 + U2 in C12
			F.sub( *(d21c + j), tmpU3, *(d21c + j) ); // U6 = U3 - P4 in C21
		}
	}
	// P3 = alpha . S4*B22 in X1
	WinoMain( F, ta, tb, mr, nr, kr, alpha, X2, ldx2, B22, ldb, zero, X1, nr, kmax, w-1 );

	// U5 = P3 + U4 in C12
	d12c = C12; dx1 = X1; 
	for ( size_t i = 0; i < mr; ++i, d12c += ldc, dx1 += nr )
		for ( size_t j = 0; j < nr; ++j )
			F.addin( *(d12c + j), *(dx1 + j) );

	delete[] X1;
	delete[] X2;
	delete[] X3;
} 


// Control the switch with classic multiplication
// Fix-up for odd-sized matrices using dynamic pealing
// for matrices over double
template<>
inline  void LinBox::FFLAS::WinoMain( const DoubleDomain& D, 
				      const enum FFLAS_TRANSPOSE ta,
				      const enum FFLAS_TRANSPOSE tb,
				      const size_t m, const size_t n, const size_t k,
				      const DoubleDomain::Element alpha,
				      const DoubleDomain::Element * A, const size_t lda,
				      const DoubleDomain::Element * B, const size_t ldb,
				      const DoubleDomain::Element beta,
				      DoubleDomain::Element * C, const size_t ldc,
				      long long kmax, size_t w ){

	if (w<=0) 
		ClassicMatmul( D, ta, tb, m, n, k, alpha, A, lda, B, ldb, beta, C, ldc, kmax );
	else{
		WinoCalc( D, ta, tb, m/2, n/2, k/2, alpha, A, lda, B, ldb, beta, C, ldc, kmax, w); 
		DynamicPealing( D, ta, tb, m, n, k, alpha, A, lda, B, ldb, beta, C, ldc, kmax );
	}
}

template <class Field>
inline  void LinBox::FFLAS::WinoMain( const Field& F, 
				      const enum FFLAS_TRANSPOSE ta,
				      const enum FFLAS_TRANSPOSE tb,
				      const size_t m, const size_t n, const size_t k,
				      const typename Field::Element alpha,
				      const typename Field::Element* A,const size_t lda,
				      const typename Field::Element* B,const size_t ldb,
				      const typename Field::Element beta,
				      typename Field::Element * C, const size_t ldc,
				      long long kmax, size_t w){
	static typename Field::Element one,zero,mone;
	F.init(one, 1);
	F.init(zero, 0);
	F.init(mone, -1);
	
	if (w<=0) // Winograd -> Classic
		ClassicMatmul( F, ta, tb, m, n, k, alpha, A, lda, B, ldb, beta, C, ldc, kmax );
	else{
		if ( k < kmax ){ // switch on double
			// Temporary double matrices
			DoubleDomain::Element alphad, betad;
			typename Field::Element _betabis;
			
 			if (F.areEqual( mone, alpha )){
				alphad = -1.0;
				F.convert( betad, beta );
			}
 			else{
				if (! F.areEqual( one, alpha) ){
					// Compute C = A*B + beta/alpha.C
					// and after C *= alpha
					F.div( _betabis, beta, alpha );
					F.convert( betad, _betabis );
				}
				else
					F.convert( betad, beta );
				alphad = 1.0;
			}
				
			DoubleDomain::Element * Ad = new DoubleDomain::Element[m*k];
			DoubleDomain::Element * Bd = new DoubleDomain::Element[k*n];
			DoubleDomain::Element * Cd = new DoubleDomain::Element[m*n];
			// Conversion GFq => double
			size_t ma, ka, kb, nb; //mb, na
			if ( ta == FflasTrans ){ ma = k; ka = m; }
			else { ma = m; ka = k; }
			if ( tb == FflasTrans ){ kb = n; nb = k; }
			else {  kb = k; nb = n; }
			
			MatF2MatD( F, Ad, ka, A, lda, ma, ka);
			MatF2MatD( F, Bd, nb, B, ldb, kb, nb); 
			if ( !F.isZero(beta) )
				MatF2MatD( F, Cd, n, C, ldc, m, n ); 
			// recursive call
			WinoMain( DoubleDomain(), m, n, k, 
				  alphad, Ad, k, Bd, n, betad, Cd, n, kmax, w);
			// Conversion double => GFq
			MatD2MatF( F, C, ldc, Cd, n, m, n );
			
			if ( !F.areEqual( one, alpha ) && !F.areEqual( mone, alpha) ){
				// Fix-up: compute C *= alpha
				for (typename Field::Element* Ci=C; Ci<C+m*ldc; Ci+=ldc)
					for ( size_t j=0; j<n; ++j ) 
						F.mulin( *( Ci + j ), alpha );
			}
			// Temporary double matrices destruction
			delete[] Ad;
			delete[] Bd;
			delete[] Cd;
		}
		else{
			WinoCalc( F, ta, tb, m/2, n/2, k/2, alpha, A, lda, B, ldb, beta, C, ldc, kmax,w);
			DynamicPealing( F, ta, tb, m, n, k, alpha, A, lda, B, ldb, beta, C, ldc, kmax );
		}
	}
}

template <>
inline  void LinBox::FFLAS::WinoMain( const Modular<double>& F, 
				      const enum FFLAS_TRANSPOSE ta,
				      const enum FFLAS_TRANSPOSE tb,
				      const size_t m, const size_t n, const size_t k,
				      const double alpha,
				      const double* A,const size_t lda,
				      const double* B,const size_t ldb,
				      const double beta,
				      double * C, const size_t ldc,
				      long long kmax, size_t w){
	if ( w <= 0 ) 
		ClassicMatmul( F, ta, tb, m, n, k, alpha, A, lda, B, ldb, beta, C, ldc, kmax );
	else{
		if (k < kmax){ // switch on double
			// Temporary double matrices
			DoubleDomain::Element _alpha, _beta;
			
			_beta = beta;
 			if (F.areEqual( -1.0, alpha )){
				_alpha = -1.0;
			}
 			else{
				// Compute C = A*B + beta/alpha.C and then C *= alpha
				if (! F.areEqual( 1.0, alpha) ){
					F.divin( _beta, alpha );
				}
				_alpha = 1.0;
			}
			
			size_t ma, ka, kb, nb; //mb, na;
			if ( ta == FflasTrans ){ ma = k; ka = m; }
			else { ma = m; ka = k; }
			if ( tb == FflasTrans ){ kb = n; nb = k; }
			else {  kb = k; nb = n; }
		
			// recursive call
			WinoMain( DoubleDomain(), ta, tb, m, n, k, 
				  _alpha, A, lda, B, ldb, _beta, C, ldc, kmax, w);
			// Conversion double => GFq
			for ( double * Ci = C; Ci != C+m*ldc; Ci+=ldc )
				for (size_t j = 0; j < n; ++j )
					F.init( *(Ci + j), *(Ci + j) );
			
			if ( !F.areEqual( 1.0, alpha ) && !F.areEqual( -1.0, alpha) ){
				// Fix-up: compute C *= alpha
				for ( double* Ci=C; Ci<C+m*ldc; Ci+=ldc)
					for ( size_t j=0; j<n; ++j ) 
						F.mulin( *( Ci + j ), alpha );
			}
			// Temporary double matrices destruction
		}
		else{
			WinoCalc( F, ta, tb, m/2, n/2, k/2, alpha, A, lda, B, ldb, beta, C, ldc, kmax,w);
			DynamicPealing( F, ta, tb, m, n, k, alpha, A, lda, B, ldb, beta, C, ldc, kmax );
		}
	}
}


template <class Field>
inline void
LinBox::FFLAS::DynamicPealing( const Field& F, 
			       const enum FFLAS_TRANSPOSE ta,
			       const enum FFLAS_TRANSPOSE tb,
			       const size_t m, const size_t n, const size_t k,
			       const typename Field::Element alpha, 
			       const typename Field::Element* A, const size_t lda,
			       const typename Field::Element* B, const size_t ldb, 
			       const typename Field::Element beta,
			       typename Field::Element* C, const size_t ldc, 
			       const size_t kmax ){

	//typename Field::Element tmp;
	const typename Field::Element *a12, *a21, *b12, *b21;
	size_t inca12, inca21, incb12, incb21, ma, na, mb, nb;
	size_t mkn = (n & 0x1)+( (k & 0x1)<<1 )+ ( (m & 0x1)<<2 ); 

	if ( ta == FflasTrans ){
 		ma = k;
 		na = m;
		a12 = A+(k-1)*lda; 
		inca12 = 1;
		a21 = A+m-1;
		inca21 = lda;
	}
	else{
 		ma = m;
 		na = k;
		a12 = A+k-1;
		inca12 = lda;
		a21 = A+(m-1)*lda;
		inca21 = 1;
	}
	if ( tb == FflasTrans ){
 		mb = n;
 		nb = k;
		b12 = B+(n-1)*ldb; 
		incb12 = 1;
		b21 = B+k-1;
		incb21 = ldb;
	}
	else{
 		mb = k;
 		nb = n;
		b12 = B+n-1;
		incb12 = ldb;
		b21 = B+(k-1)*ldb;
		incb21 = 1;
	}

	switch(mkn) { 
	case 1: // n oddsized
		fgemv( F, ta, ma, na, alpha, A, lda, b12, incb12, beta, C+n-1,ldc);
		break;
      
	case 2: // k oddsized
		fger( F, m, n, alpha, a12, inca12, b21, incb21, C, ldc);
		break;
			
	case 3: // n, k oddsized
		fgemv( F, ta, ma, na, alpha, A, lda, b12, incb12, beta, C+n-1,ldc);
		fger( F, m, n-1, alpha, a12, inca12, b21, incb21, C, ldc);
		break;
			
	case 4: // m oddsized
		fgemv( F, (tb==FflasTrans)?FflasNoTrans:FflasTrans, mb, nb,
		       alpha, B, ldb, a21, inca21, beta, C+(m-1)*ldc, 1);
		break;
			
	case 5: // m, n oddsized
		if ( tb == FflasTrans )
			mb--;
		else
			nb--;

		fgemv( F, ta, ma, na, alpha, A, lda, b12, incb12, beta, C+n-1, ldc);
		//a simplifier
		fgemv( F, (tb==FflasTrans)?FflasNoTrans:FflasTrans, mb, nb,
		       alpha, B, ldb, a21, inca21, beta, C+(m-1)*ldc, 1);
		break;
      
	case 6: // m, k oddsized
		fger( F, m-1, n, alpha, a12, inca12, b21, incb21, C, ldc);
		fgemv( F, (tb==FflasTrans)?FflasNoTrans:FflasTrans, mb, nb,
		       alpha, B, ldb, a21, inca21, beta, C+(m-1)*ldc, 1);
		break;
      
	case 7: // m, k, n oddsized
		if (tb == FflasTrans)
			mb--;
		else
			nb--;
		// Block NW
		fger( F, m-1, n-1, alpha, a12, inca12, b21, incb21, C, ldc);
		// Block SW
		fgemv( F, (tb==FflasTrans)?FflasNoTrans:FflasTrans, mb, nb,
		       alpha, B, ldb, a21, inca21, beta, C+(m-1)*ldc, 1 );
		// Block NE
		fgemv( F, ta, ma, na, alpha, A, lda, b12, incb12, beta, C+n-1, ldc );
		break;
	}
}

//-------------------------------------------------------------------
// visible function: operates C = A*B over double or a finite Field
//-------------------------------------------------------------------
template<class Field>
inline  typename Field::Element* 
LinBox::FFLAS::fgemm( const Field& F,
		      const enum FFLAS_TRANSPOSE ta,
		      const enum FFLAS_TRANSPOSE tb,
		      const size_t m, const size_t n, const size_t k,
		      const typename Field::Element alpha,
		      const typename Field::Element* A, const size_t lda,
		      const typename Field::Element* B, const size_t ldb, 
		      const typename Field::Element beta,
		      typename Field::Element* C, const size_t ldc,
		      const size_t w){
	if (!(m && n && k)) return C;
	
	size_t kmax;
	FflasKmax( kmax, F, w, beta );
	WinoMain( F, ta, tb, m, n, k, alpha, A, lda, B, ldb, beta, C, ldc, kmax, w);
	return C;
}

template<>
inline double*
LinBox::FFLAS::fsquare( const Modular<double>& F,
			const enum FFLAS_TRANSPOSE ta,
			const size_t n, const double alpha,
			const double* A, const size_t lda,
			const double beta,
			double* C, const size_t ldc){
	
	
	if ( C==A ){
		double * Ad = new double[n*n];
		for ( size_t i=0; i<n; ++i)
			fcopy( F, n,Ad+i*n, 1, A+i*lda, 1);
		fgemm( F, ta, ta, n, n, n, alpha, Ad, n, Ad, n,
		       beta, C, ldc );
		
// 		cblas_dgemm( CblasRowMajor, (enum CBLAS_TRANSPOSE)ta,
// 			     (enum CBLAS_TRANSPOSE)ta, n, n, n, 
//			     alpha, Ad, n, Ad, n, beta, C, ldc);
	}
	else
		fgemm( F, ta, ta, n, n, n, alpha, A, lda, A, lda,
		       beta, C, ldc );		
// 	cblas_dgemm( CblasRowMajor, (enum CBLAS_TRANSPOSE)ta,
// 			     (enum CBLAS_TRANSPOSE)ta, n, n, n, 
// 			     alpha, A, lda, A, lda, beta, C, ldc);
	// Conversion double => Finite Field
	size_t i;
	double *Ci;
	for ( i=0, Ci=C ; i<n;++i, Ci+=ldc)
		for ( size_t j=0; j<n;++j)
			F.init(*(Ci+j),*(Ci+j));
	return C;
}

template<class Field>
inline  typename Field::Element*
LinBox::FFLAS::fsquare( const Field& F,
			const enum FFLAS_TRANSPOSE ta,
			const size_t n, const typename Field::Element alpha,
			const typename Field::Element* A, const size_t lda,
			const typename Field::Element beta,
			typename Field::Element* C, const size_t ldc){
	static typename Field::Element mone;
	F.init( mone, -1 );
	double alphad, betad;
	F.convert( alphad, alpha );
	if ( F.areEqual( beta, mone ) )
		betad = -1.0;
	else
		F.convert( betad, beta );

	// Double  matrices initialisation
	DoubleDomain::Element * Ad = new DoubleDomain::Element[n*n];
	DoubleDomain::Element * Cd = new DoubleDomain::Element[n*n];
	// Conversion finite Field => double
	MatF2MatD( F, Ad, n, A, lda, n, n);
	if (!F.isZero(beta))
		MatF2MatD( F, Cd, n, C, ldc, n, n); 
	
	// Call to the blas Multiplication 
	cblas_dgemm( CblasRowMajor, (enum CBLAS_TRANSPOSE)ta,
		     (enum CBLAS_TRANSPOSE)ta, n, n, n, 
		     (DoubleDomain::Element) alphad, Ad, n, Ad, n,
		     (DoubleDomain::Element) betad, Cd, n);
	// Conversion double => Finite Field
	delete[] Ad;
	MatD2MatF( F, C, ldc, Cd, n, n, n);
	delete[] Cd;
	return C;
}
