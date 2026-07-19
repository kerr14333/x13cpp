C     ref_dinvnr.f -- golden values for dinvnr (inverse normal CDF) and its
C     helper chain cumnor/stvaln/devlpl/spmpar/ipmpar. Prints dinvnr(P,1-P) for
C     a spread of probabilities, including the two-tailed CI points used by
C     prtfct (pval=(Ciprob+1)/2 for Ciprob in {0.90,0.95,0.99}), plus a couple
C     of cumnor round-trip checks.
C       gfortran -O2 -ffp-contract=off tools/ref_dinvnr.f \
C         oracle/fortran/{dinvnr,cumnor,stvaln,devlpl,spmpar,ipmpar}.f \
C         -o ref_dinvnr && ./ref_dinvnr
      PROGRAM ref_dinvnr
      IMPLICIT NONE
      DOUBLE PRECISION dinvnr,p,q,cum,ccum
      INTEGER i
      DOUBLE PRECISION plist(9)
      EXTERNAL cumnor
      DATA plist/0.5D0,0.75D0,0.90D0,0.95D0,0.975D0,0.99D0,0.995D0,
     &           0.025D0,0.001D0/
      DO i=1,9
       p=plist(i)
       q=1.0D0-p
       WRITE(*,'(A,F6.3,A,1PE24.16)') 'dinvnr P=',p,' = ',dinvnr(p,q)
      END DO
C     cumnor round trips
      CALL cumnor(1.959963984540054D0,cum,ccum)
      WRITE(*,'(A,1PE24.16,1X,1PE24.16)') 'cumnor(1.95996)=',cum,ccum
      CALL cumnor(-0.5D0,cum,ccum)
      WRITE(*,'(A,1PE24.16,1X,1PE24.16)') 'cumnor(-0.5)  =',cum,ccum
      CALL cumnor(3.0D0,cum,ccum)
      WRITE(*,'(A,1PE24.16,1X,1PE24.16)') 'cumnor(3.0)   =',cum,ccum
      END
