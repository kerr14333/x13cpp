C     ref_chisq.f -- golden values for chisq (chi-squared upper-tail prob,
C     Statistics Canada) and its helper gauss (central normal prob). Used by
C     chitst for the automdl regressor-group chi-square test p-value. Prints
C     chisq(x,n) across even/odd df and both series regimes (x<90), plus a few
C     gauss(x) points spanning its three |x|/2 regimes.
C       gfortran -O2 -ffp-contract=off tools/ref_chisq.f \
C         oracle/fortran/{chisq,gauss,dpeq}.f -o ref_chisq && ./ref_chisq
      PROGRAM ref_chisq
      IMPLICIT NONE
      DOUBLE PRECISION chisq,gauss,xlist(6),glist(5)
      INTEGER nlist(4),i,j
      EXTERNAL chisq,gauss
      DATA xlist/0.5D0,2.0D0,3.841459D0,10.0D0,25.0D0,88.0D0/
      DATA nlist/1,2,6,7/
      DATA glist/0.5D0,1.0D0,1.959964D0,3.5D0,6.5D0/
      DO i=1,6
       DO j=1,4
        WRITE(*,'(A,F9.5,A,I2,A,1PE24.16)')
     &       'chisq x=',xlist(i),' n=',nlist(j),' = ',chisq(xlist(i),
     &       nlist(j))
       END DO
      END DO
      DO i=1,5
       WRITE(*,'(A,F9.5,A,1PE24.16)')
     &      'gauss x=',glist(i),' = ',gauss(glist(i))
      END DO
      END
