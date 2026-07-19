C     ref_strtvl.f -- reference output for strtvl (ARMA starting values).
C     AR(2)+MA(1) shell. Lag 1: DNOTST & free  -> set to .1 . Lag 2: already
C     0.5 (not DNOTST) & free -> untouched. Lag 3 (MA): DNOTST but fixed ->
C     untouched. Exercises all three branches of the dpeq/not-fixed test.
C       gfortran -O2 -ffp-contract=off -Ioracle/fortran tools/ref_strtvl.f \
C         oracle/fortran/{strtvl,dpeq}.f -o ref_strtvl && ./ref_strtvl
      PROGRAM ref_strtvl
      IMPLICIT NONE
      INCLUDE 'notset.prm'
      INCLUDE 'srslen.prm'
      INCLUDE 'model.prm'
      INCLUDE 'model.cmn'
      INCLUDE 'mdldat.cmn'
      INTEGER i

C     ---- ARMA(2,1) model shell (no differencing) ----
C     Mdl: DIFF empty, AR=operator 1, MA=operator 2
      Mdl(0)=1
      Mdl(1)=1
      Mdl(2)=2
      Mdl(3)=3
C     Opr: operator 1 -> lags 1..2 (AR), operator 2 -> lag 3 (MA)
      Opr(0)=1
      Opr(1)=3
      Opr(2)=4
C     Lag state
      Arimap(1)=DNOTST
      Arimaf(1)=.false.
      Arimap(2)=0.5D0
      Arimaf(2)=.false.
      Arimap(3)=DNOTST
      Arimaf(3)=.true.

      CALL strtvl()
      DO i=1,3
       WRITE(*,'(A,I1,A,ES24.16)') 'sv',i,' ',Arimap(i)
      END DO
      END
