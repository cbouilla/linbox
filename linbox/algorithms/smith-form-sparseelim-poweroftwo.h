/* algorithms/smith-form-sparseelim-poweroftwo.h
 * Copyright (C) LinBox
 * Written by JG Dumas
 * Time-stamp: <03 Jul 15 13:59:19 Jean-Guillaume.Dumas@imag.fr>
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


#ifndef __LINBOX_pp_gauss_poweroftwo_H
#define __LINBOX_pp_gauss_poweroftwo_H
#include <map>
#include <givaro/givconfig.h> // for Signed_Trait
#include "linbox/algorithms/smith-form-sparseelim-local.h"

// LINBOX_pp_gauss_intermediate_OUT outputs intermediate matrices
#ifdef LINBOX_pp_gauss_intermediate_OUT

// LINBOX_pp_gauss_steps_OUT outputs elimination steps
#  ifndef LINBOX_pp_gauss_steps_OUT
#  define LINBOX_pp_gauss_steps_OUT
#  endif

#endif

// LINBOX_pp_gauss_steps_OUT outputs elimination steps
#ifdef LINBOX_pp_gauss_steps_OUT

// LINBOX_PRANK_OUT outputs intermediate ranks
#  ifndef LINBOX_PRANK_OUT
#  define LINBOX_PRANK_OUT
#  endif

#endif


namespace LinBox
{

        /** \brief Repository of functions for rank modulo a prime power by elimination
         * on sparse matrices.
         */
    template<typename UnsignedIntType>
    class PowerGaussDomainPowerOfTwo  {
        typedef UnsignedIntType UInt_t;
        typedef UnsignedIntType Element;
        const Element zero;
        const Element one;
    public:

            /** \brief The field parameter is the domain
             * over which to perform computations
             */
        PowerGaussDomainPowerOfTwo () : zero(0U), one(1U) {}

            //Copy constructor
            ///
        PowerGaussDomainPowerOfTwo (const PowerGaussDomainPowerOfTwo &M) {}



            // --------------------------------------------
            // Modulo operators
        bool isNZero(const UInt_t& a ) const { return (bool)a ;}
        bool isZero(const UInt_t& a ) const { return a == 0U;}
        bool isOne(const UInt_t& a ) const { return a == 1U;}
	/// @todo use Givaro isOdd
        bool isOdd(const UInt_t& b) const {
            return (bool)(b & 1U);
        }

        bool MY_divides(const UInt_t& a, const UInt_t& b) const {
            return (!(b%a));
        }

            // [On Newton-Raphson iteration for multiplicative 
            //  inverses modulo prime powers. J-G. Dumas. 
            //  IEEE Transactions on Computers, 2013]  
            // http://doi.ieeecomputersociety.org/10.1109/TC.2013.94
        UInt_t& MY_Zpz_inv (UInt_t& u1, const UInt_t& a, const size_t exponent, const UInt_t& TWOTOEXPMONE) const {
            static const UInt_t ttep2(TWOTOEXPMONE+3U);
            if (this->isOne(a)) return u1=this->one;
            REQUIRE( (one<<exponent) == (TWOTOEXPMONE+1U) );
            REQUIRE( a <= TWOTOEXPMONE );
            REQUIRE( (a & 1U) );
            u1=ttep2-a; // 2-a
            UInt_t xmone(a-1);
            for(size_t i=2; i<exponent; i<<=1) {
                UInt_t tmp(xmone); xmone *= tmp;
                xmone &= TWOTOEXPMONE;
                u1 *= ++xmone; --xmone;
                u1 &= TWOTOEXPMONE;
            }
            ENSURE( ((a * u1) & TWOTOEXPMONE) == 1U );
            return u1;
        }

//         UInt_t& MY_Zpz_inv (UInt_t& u1, const UInt_t a, const UInt_t _p) const {
//             u1 = 1U;
//             UInt_t r0(_p), r1(a);
//             UInt_t q(r0/r1);

//             r0 -= q * r1;
//             if ( this->isZero(r0) ) return u1;
//             UInt_t u0 = q;

//             q = r1/r0;
//             r1 -= q * r0;

//             while ( this->isNZero(r1) ) {
//                 u1 += q * u0;

//                 q = r0/r1;
//                 r0 -= q * r1;
//                 if ( this->isZero(r0) ) return u1;
//                 u0 += q * u1;

//                 q = r1/r0;
//                 r1 -= q * r0;

//             }

//             return u1=_p-u0;
//         }

//         UInt_t MY_Zpz_inv (const UInt_t a, const UInt_t _p) const {
//             UInt_t u1; return MY_Zpz_inv(u1,a,_p);
//         }


            // ------------------------------------------------
            // Pivot Searchers and column strategy
            // ------------------------------------------------
        template<class Vecteur>
        void SameColumnPivoting(const Vecteur& lignepivot, unsigned long& indcol, long& indpermut, Boolean_Trait<false>::BooleanType ) {}


        template<class Vecteur>
        void SameColumnPivoting(const Vecteur& lignepivot, unsigned long& indcol, long& indpermut, Boolean_Trait<true>::BooleanType ) {
                // Try first in the same column
            unsigned long nj = (unsigned long) lignepivot.size() ;
            if (nj && (indcol == lignepivot[0].first) && (this->isOdd((UInt_t)lignepivot[0].second) ) ) {
                indpermut = (long)indcol;
                ++indcol;
            }
        }

        template<class BB, class Mmap>
        bool SameColumnPivotingTrait(unsigned long& p, const BB& LigneA, const Mmap& psizes, unsigned long& indcol, long& indpermut, Boolean_Trait<false>::BooleanType ) {
                // Do not try first in the same column
            return false;
        }

        template<class BB, class Mmap>
        bool SameColumnPivotingTrait(unsigned long& p, const BB& LigneA, const Mmap& psizes, unsigned long& indcol, long& c, Boolean_Trait<true>::BooleanType truetrait) {
            c=-2;
            for( typename Mmap::const_iterator iter = psizes.begin(); iter != psizes.end(); ++iter) {
                p = (unsigned long) (*iter).second;
                SameColumnPivoting(LigneA[(size_t)p], indcol, c, truetrait ) ;
                if (c > -2 ) break;
            }
            if (c > -2)
                return true;
            else
                return false;

        }

        template<class Vecteur>
        void CherchePivot( Vecteur& lignepivot, unsigned long& indcol , long& indpermut ) {
            unsigned long nj = (unsigned long) lignepivot.size() ;
            if (nj) {
                indpermut=(long) lignepivot[0].first;
                if (indpermut != (long)indcol)
                    lignepivot[0].first = (indcol);
                ++indcol;
            }
            else
                indpermut = -1;
        }



        template<class Vecteur, class D>
        void CherchePivot(Vecteur& lignepivot, unsigned long& indcol , long& indpermut, D& columns ) {
            typedef typename Vecteur::value_type E;
            long nj =  (long) lignepivot.size() ;
            if (nj) {
                indpermut = (long)lignepivot[0].first;
                long pp=0;
                for(;pp<nj;++pp)
                    if (this->isOdd((UInt_t)lignepivot[(size_t)pp].second) ) break;

                if (pp < nj) {
                    long ds = (long)columns[ lignepivot[(size_t)pp].first ],p=pp,j=pp;
                    for(++j;j<nj;++j){
			    long dl;
                        if ( ( (dl=(long)columns[(size_t)lignepivot[(size_t)j].first] ) < ds ) && (this->isOdd((UInt_t)lignepivot[(size_t)j].second) ) ) {
                            ds = dl;
                            p = j;
                        }
		    }
                    if (p != 0) {
                        if (indpermut == (long)indcol) {
                            UInt_t ttm = (UInt_t)lignepivot[(size_t)p].second;
                            indpermut = (long)lignepivot[(size_t)p].first;
                            lignepivot[(size_t)p].second = (lignepivot[0].second);
                            lignepivot[0].second = (UInt_t)(ttm);
                        }
                        else {
                            E ttm = (E) lignepivot[(size_t)p];
                            indpermut = (long)ttm.first;
                            for(long m=p;m;--m)
                                lignepivot[(size_t)m] = lignepivot[(size_t)m-1];
                            lignepivot[0] = ttm;
                        }
                    }
                    if (indpermut != (long)indcol)
                        lignepivot[0].first = (indcol);
                    indcol++ ;
                    for(j=nj;j--;)
                        --columns[ lignepivot[(size_t)j].first ];
                }
                else
                    indpermut = -2;
            }
            else
                indpermut = -1;
        }

        template<class Vecteur>
        void PreserveUpperMatrixRow(Vecteur& ligne, Boolean_Trait<true>::BooleanType ) {}

        template<class Vecteur>
        void PreserveUpperMatrixRow(Vecteur& ligne, Boolean_Trait<false>::BooleanType ) {
            ligne = Vecteur(0);
        }


        template<class Vecteur, class De>
        void FaireElimination( const size_t EXPONENT, const UInt_t& TWOK, const UInt_t& TWOKMONE,
                               Vecteur& lignecourante,
                               const Vecteur& lignepivot,
                               const long& indcol,
                               const long& indpermut,
                               De& columns) {

                //     typedef typename Vecteur::coefficientSpace F;
                //     typedef typename Vecteur::value_types E;
            typedef typename Vecteur::value_type E;

            unsigned long k  = (unsigned long) indcol - 1;
            unsigned long nj = (unsigned long) lignecourante.size() ;
            if (nj) {
                unsigned long j_head(0);
                for(; j_head<nj; ++j_head)
                    if (long(lignecourante[(size_t)j_head].first) >= indpermut) break;
                unsigned long bjh(j_head-1);
                if ((j_head<nj) && (long(lignecourante[(size_t)j_head].first) == indpermut)) {
                        // -------------------------------------------
                        // Permutation
                    if (indpermut != (long)k) {
                        if (lignecourante[0].first == k) {
                                // non zero  <--> non zero
                            UInt_t tmp = (UInt_t) lignecourante[0].second ;
                            lignecourante[0].second = lignecourante[(size_t)j_head].second;
                            lignecourante[(size_t)j_head].second = tmp;
                        }
                        else {
                                // zero <--> non zero
                            E tmp = (E) lignecourante[(size_t)j_head];
                            --columns[ tmp.first ];
                            ++columns[k];
                            tmp.first = (k);
                            for(long l=(long)j_head; l>0; l--)
                                lignecourante[(size_t)l] = lignecourante[(size_t)l-1];
                            lignecourante[0] = tmp;
                        }
                        j_head = 0;
                    }
                        // -------------------------------------------
                        // Elimination
                    unsigned long npiv = (unsigned long) lignepivot.size();
                    Vecteur construit(nj + npiv);
                        // construit : <-- ci
                        // courante  : <-- m
                        // pivot     : <-- l
                    typedef typename Vecteur::iterator Viter;
                    Viter ci = construit.begin();
                    unsigned long m=1;
                    unsigned long l(0);
                        // A[(size_t)i,k] <-- A[(size_t)i,k] / A[(size_t)k,k]

                    UInt_t headcoeff = TWOK-(UInt_t)(lignecourante[0].second);

                    
//                     UInt_t invpiv; MY_Zpz_inv(invpiv, lignepivot[0].second, TWOK);
                    UInt_t invpiv;
                    MY_Zpz_inv(invpiv, (UInt_t) (lignepivot[0].second), EXPONENT, TWOKMONE);
                    headcoeff *= invpiv;
                    headcoeff &= TWOKMONE ;


//                     lignecourante[0].second = (  ((UModulo)( ( MOD-(lignecourante[0].second) ) * ( MY_Zpz_inv( lignepivot[0].second, MOD) ) ) ) % (UModulo)MOD ) ;
//                     UInt_t headcoeff = lignecourante[0].second ;
                    --columns[ lignecourante[0].first ];

                    for(;l<npiv;++l)
                        if (lignepivot[(size_t)l].first > k) break;
                        // for all j such that (j>k) and A[(size_t)k,j]!=0
                    for(;l<npiv;++l) {
                    unsigned long j_piv;
                        j_piv = (unsigned long) lignepivot[(size_t)l].first;
                            // if A[(size_t)k,j]=0, then A[(size_t)i,j] <-- A[(size_t)i,j]
                        for (;(m<nj) && (lignecourante[(size_t)m].first < j_piv);)
                            *ci++ = lignecourante[(size_t)m++];
                            // if A[(size_t)i,j]!=0, then A[(size_t)i,j] <-- A[(size_t)i,j] - A[(size_t)i,k]*A[(size_t)k,j]
                        if ((m<nj) && (lignecourante[(size_t)m].first == j_piv)) {
//                             lignecourante[(size_t)m].second = ( ((UModulo)( headcoeff  *  lignepivot[(size_t)l].second  + lignecourante[(size_t)m].second ) ) % (UModulo)MOD );
                            STATE( UInt_t lcs = lignecourante[(size_t)m].second);
                            

                            lignecourante[(size_t)m].second += ( headcoeff  *  (UInt_t)lignepivot[(size_t)l].second );
                            lignecourante[(size_t)m].second &= TWOKMONE;

                            if (isNZero((UInt_t)(lignecourante[(size_t)m].second)))
                                *ci++ = lignecourante[(size_t)m++];
                            else
                                --columns[ lignecourante[(size_t)m++].first ];
                                //                         m++;
                        }
                        else {
                            UInt_t tmp(headcoeff);
                            tmp *= (UInt_t)lignepivot[(size_t)l].second;
                            tmp &= TWOKMONE;
                            if (isNZero(tmp)) {
                                ++columns[(size_t)j_piv];
                                *ci++ =  E(j_piv, (UInt_t)tmp );
                            }
                        }
                    }
                        // if A[(size_t)k,j]=0, then A[(size_t)i,j] <-- A[(size_t)i,j]
                    for (;m<nj;)
                        *ci++ = lignecourante[(size_t)m++];

                    construit.erase(ci,construit.end());
                    lignecourante = construit;
                }
                else
                        // -------------------------------------------
                        // Permutation
                    if (indpermut != (long)k) {
                        unsigned long l(0);
                        for(; l<nj; ++l)
                            if (lignecourante[(size_t)l].first >= k) break;
                        if ((l<nj) && (lignecourante[(size_t)l].first == k))  {
                                // non zero <--> zero
                            E tmp = (E) lignecourante[(size_t)l];
                            --columns[tmp.first ];
                            ++columns[(size_t)indpermut];
                            tmp.first = (unsigned long)(indpermut);
                            for(;l<bjh;l++)
                                lignecourante[(size_t)l] = lignecourante[(size_t)l+1];
                            lignecourante[bjh] = tmp;
                        } // else
                            // zero <--> zero
                    }
            }
        }

            // ------------------------------------------------------
            // Rank calculators, defining row strategy
            // ------------------------------------------------------

        template<class BB, class D, class Container, bool PrivilegiateNoColumnPivoting, bool PreserveUpperMatrix>
        void gauss_rankin(size_t EXPONENTMAX, Container& ranks, BB& LigneA, const size_t Ni, const size_t Nj, const D& density_trait)
            {
                commentator().start ("Gaussian elimination with reordering modulo a prime power of 2",
                                     "PRGEPo2", Ni);

                ranks.resize(0);

                typedef typename BB::Row Vecteur;
                size_t EXPONENT = EXPONENTMAX;
                UInt_t TWOK(1U); TWOK <<= EXPONENT;
                UInt_t TWOKMONE(TWOK); --TWOKMONE;
ENSURE( TWOK == (UInt_t(1U) << EXPONENT) );
ENSURE( TWOKMONE == (TWOK - 1U) );



#ifdef LINBOX_PRANK_OUT
                std::cerr << "Elimination mod " << TWOK << std::endl;
#endif

                D col_density(Nj);

                    // assignment of LigneA with the domain object
                size_t jj;
                for(jj=0; jj<Ni; ++jj) {
                    Vecteur tmp = LigneA[(size_t)jj];
                    Vecteur toto(tmp.size());
                    unsigned long k=0,rs=0;
                    for(; k<tmp.size(); ++k) {
                        UInt_t r = (UInt_t)tmp[k].second;
//                         if (r <0) r %= TWOK ;
//                         if (r <0) r += TWOK ;
//                         if (r >= TWOK) r &= TWOKMONE;
                        r &= TWOKMONE;
                        if (isNZero(r)) {
                            ++col_density[ tmp[k].first ];
                            toto[rs] =tmp[k];
                            toto[rs].second = ( r );
                            ++rs;
                        }
                    }
                    toto.resize(rs);
                    LigneA[(size_t)jj] = toto;
                        //                 LigneA[(size_t)jj].reactualsize(Nj);

                }

                unsigned long last = Ni-1;
                long c(0);
                unsigned long indcol(0);
                unsigned long ind_pow = 1;
                unsigned long maxout = Ni/100; maxout = (maxout<10 ? 10 : (maxout>1000 ? 1000 : maxout) );
                unsigned long thres = Ni/maxout; thres = (thres >0 ? thres : 1);


                for (unsigned long k=0; k<last;++k) {
                    if ( ! (k % maxout) ) commentator().progress ((long)k);


                    unsigned long p=k;
                    for(;;) {


                        std::multimap< long, long > psizes;
                        for(p=k; p<Ni; ++p)
                            psizes.insert( psizes.end(), std::pair<long,long>( (long)LigneA[(size_t)p].size(), (long)p) );

#ifdef  LINBOX_pp_gauss_steps_OUT
                        std::cerr << "------------ ordered rows " << k << " -----------" << std::endl;
                        for( std::multimap< long, long >::const_iterator iter = psizes.begin(); iter != psizes.end(); ++iter)
                        {
                            std::cerr << (*iter).second << " : #" << (*iter).first << std::endl;
                        }
                        std::cerr << "---------------------------------------" << std::endl;
#endif



                        if ( SameColumnPivotingTrait(p, LigneA, psizes, indcol, c, typename Boolean_Trait<PrivilegiateNoColumnPivoting>::BooleanType() ) )
                            break;

                        for( typename std::multimap< long, long >::const_iterator iter = psizes.begin(); iter != psizes.end(); ++iter) {
                            p = (unsigned long) (*iter).second;

                            CherchePivot( LigneA[(size_t)p], indcol, c , col_density) ;
                            if (c > -2 ) break;
                        }

                        if (c > -2) break;
                        for(unsigned long ii=k;ii<Ni;++ii)
                            for(unsigned long jjj=LigneA[(size_t)ii].size();jjj--;)
                                LigneA[(size_t)ii][(size_t)jjj].second >>= 1;
                        --EXPONENT;
                        TWOK >>= 1;
                        TWOKMONE >>=1;

ENSURE( TWOK == (UInt_t(1U) << EXPONENT) );
ENSURE( TWOKMONE == (TWOK - 1U) );

                        ranks.push_back( indcol );
                        ++ind_pow;
#ifdef LINBOX_PRANK_OUT
                        std::cerr << "Rank mod 2^" << ind_pow << " : " << indcol << std::endl;
                        if (TWOK == 1) std::cerr << "wattadayada inhere ?" << std::endl;
#endif

                    }
                    if (p != k) {
#ifdef  LINBOX_pp_gauss_steps_OUT
                        std::cerr << "------------ permuting rows " << p << " and " << k << " ---" << std::endl;
#endif
                        Vecteur vtm = LigneA[(size_t)k];
                        LigneA[(size_t)k] = LigneA[(size_t)p];
                        LigneA[(size_t)p] = vtm;
                    }
#ifdef  LINBOX_pp_gauss_steps_OUT
                    if (c != (long(indcol)-1L))
                        std::cerr << "------------ permuting cols " << (indcol-1) << " and " << c << " ---" << std::endl;
#endif
                    if (c != -1)
                        for(unsigned long l=k + 1; l < Ni; ++l)
                            FaireElimination(EXPONENT, TWOK, TWOKMONE, LigneA[(size_t)l], LigneA[(size_t)k], (long)indcol, c, col_density);

#ifdef  LINBOX_pp_gauss_steps_OUT
                    std::cerr << "step[" << k << "], pivot: " << c << std::endl;
#endif

#ifdef  LINBOX_pp_gauss_intermediate_OUT
                    LigneA.write(std::cerr) << std::endl;
#endif

                    PreserveUpperMatrixRow(LigneA[(size_t)k], typename Boolean_Trait<PreserveUpperMatrix>::BooleanType());
                }

                c = -2;
                SameColumnPivoting(LigneA[(size_t)last], indcol, c, typename Boolean_Trait<PrivilegiateNoColumnPivoting>::BooleanType() );
                if (c == -2) CherchePivot( LigneA[(size_t)last], indcol, c, col_density );
                while( c == -2) {
                    ranks.push_back( indcol );
                    for(long jjj=(long)LigneA[(size_t)last].size();jjj--;)
                        LigneA[(size_t)last][(size_t)jjj].second >>= 1;
                    TWOK >>= 1;
                    CherchePivot( LigneA[(size_t)last], indcol, c, col_density );
                }
                while( TWOK > 1) {
                    TWOK >>= 1;
                    ranks.push_back( indcol );
                }

                    //             ranks.push_back(indcol);
#ifdef LINBOX_pp_gauss_steps_OUT
                std::cerr << "step[" << Ni-1 << "], pivot: " << c << std::endl;
#endif
#ifdef LINBOX_pp_gauss_intermediate_OUT
                LigneA.write(std::cerr) << std::endl;
#endif
#ifdef LINBOX_PRANK_OUT
                std::cerr << "Rank mod 2^" << EXPONENTMAX << " : " << indcol << std::endl;
#endif
                commentator().stop ("done", 0, "PRGEPo2");

            }

        template<class BB, class D, class Container>
        void prime_power_rankin (size_t EXPONENT, Container& ranks, BB& SLA, const size_t Ni, const size_t Nj, const D& density_trait, int StaticParameters=0) {
            if (PRIVILEGIATE_NO_COLUMN_PIVOTING & StaticParameters) {
                if (PRESERVE_UPPER_MATRIX & StaticParameters) {
                    gauss_rankin<BB,D,Container,true,true>(EXPONENT,ranks, SLA, Ni, Nj, density_trait);
                } else {
                    gauss_rankin<BB,D,Container,true,false>(EXPONENT,ranks, SLA, Ni, Nj, density_trait);
                }
            } else {
                if (PRESERVE_UPPER_MATRIX & StaticParameters) {
                    gauss_rankin<BB,D,Container,false,true>(EXPONENT,ranks, SLA, Ni, Nj, density_trait);
                } else {
                    gauss_rankin<BB,D,Container,false,false>(EXPONENT,ranks, SLA, Ni, Nj, density_trait);
                }
            }
        }


        template<class Matrix, template<class, class> class Container, template<class> class Alloc>
        Container<std::pair<size_t,UInt_t>, Alloc<std::pair<size_t,UInt_t> > >& operator()(Container<std::pair<size_t,UInt_t>, Alloc<std::pair<size_t,UInt_t> > >& L, Matrix& A, size_t EXPONENT, int StaticParameters=0) {
            Container<size_t, Alloc<size_t> > ranks;
            prime_power_rankin( EXPONENT, ranks, A, A.rowdim(), A.coldim(), std::vector<size_t>(),StaticParameters);
            L.resize( 0 ) ;
            UInt_t MOD(1);
            size_t num = 0;
            for( typename Container<size_t, Alloc<size_t> >::const_iterator it = ranks.begin(); it != ranks.end(); ++it) {
                size_t diff = *it-num;
                if (diff > 0)
                    L.push_back( std::pair<size_t,UInt_t>(*it-num,MOD) );
                MOD <<= 1;
                num = *it;
            }
            return L;
        }

    };



} // end of LinBox namespace

#endif  //__LINBOX_pp_gauss_poweroftwo_H

// vim:sts=8:sw=8:ts=8:noet:sr:cino=>s,f0,{0,g0,(0,:0,t0,+0,=s
// Local Variables:
// mode: C++
// tab-width: 8
// indent-tabs-mode: nil
// c-basic-offset: 8
// End:

