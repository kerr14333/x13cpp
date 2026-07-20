C     ref_chsppf.f -- golden values for chsppf (chi-squared percent-point
C     function, DATAPAC/Filliben). Used by the automdl AIC-test family
C     (tdaic/lomaic/easaic/usraic) to turn a user pvaictest probability +
C     regressor-count df into a chi-square critical value. Prints chsppf(P,NU)
C     for the probabilities the AIC tests pass (0.90/0.95/0.99) across the df
C     they use (1..7), plus a few well-known chi-square quantiles for a sanity
C     cross-check against standard tables.
C       gfortran -O2 -ffp-contract=off tools/ref_chsppf.f \
C         oracle/fortran/chsppf.f -o ref_chsppf && ./ref_chsppf
      PROGRAM ref_chsppf
      IMPLICIT NONE
      DOUBLE PRECISION ppf,plist(3)
      INTEGER nulist(4),i,j
      DATA plist/0.90D0,0.95D0,0.99D0/
      DATA nulist/1,2,6,7/
      DO i=1,3
       DO j=1,4
        CALL chsppf(plist(i),nulist(j),ppf,6)
        WRITE(*,'(A,F5.2,A,I2,A,1PE24.16)')
     &       'chsppf P=',plist(i),' NU=',nulist(j),' = ',ppf
       END DO
      END DO
      END
