C     ref_chkrts.f -- reference output for chkrts (reads model operator commons).
C       gfortran -O2 -Ioracle/fortran tools/ref_chkrts.f \
C         oracle/fortran/chkrts.f oracle/fortran/setdp.f -o ref_chkrts && ./ref_chkrts
      PROGRAM ref_chkrts
      IMPLICIT NONE
      INCLUDE 'srslen.prm'
      INCLUDE 'model.prm'
      INCLUDE 'model.cmn'
      INCLUDE 'mdldat.cmn'
      LOGICAL chkrts,res
      EXTERNAL chkrts

C     Case A: single AR lag1 phi=0.5, factor 1 -> invertible (F)
      Opr(0)=1
      Opr(1)=2
      Oprfac(1)=1
      Arimal(1)=1
      Arimap(1)=0.5D0
      Arimaf(1)=.false.
      Prbfac=-99
      res=chkrts(1,1)
      WRITE(*,'(A,L2,I4)') 'chkA',res,Prbfac

C     Case B: single AR lag1 phi=1.5 -> non-invertible (T), Prbfac=1
      Arimap(1)=1.5D0
      Prbfac=-99
      res=chkrts(1,1)
      WRITE(*,'(A,L2,I4)') 'chkB',res,Prbfac

C     Case C: degree-2 op (lags 1,2), phi=(0.3,0.4) -> invertible, exercises
C             the reflection update path (coef(1) rewritten)
      Opr(0)=1
      Opr(1)=3
      Oprfac(1)=1
      Arimal(1)=1
      Arimal(2)=2
      Arimap(1)=0.3D0
      Arimap(2)=0.4D0
      Arimaf(1)=.false.
      Arimaf(2)=.false.
      Prbfac=-99
      res=chkrts(1,1)
      WRITE(*,'(A,L2,I4)') 'chkC',res,Prbfac

C     Case D: single lag, param FIXED (arimaf T) -> operator skipped -> F
      Opr(0)=1
      Opr(1)=2
      Oprfac(1)=1
      Arimal(1)=1
      Arimap(1)=1.5D0
      Arimaf(1)=.true.
      Prbfac=-99
      res=chkrts(1,1)
      WRITE(*,'(A,L2,I4)') 'chkD',res,Prbfac

C     Case E: theta=1.0 exactly -> cfncsq=1-1=0 <=0 -> non-invertible (FMA canary)
      Opr(0)=1
      Opr(1)=2
      Oprfac(1)=1
      Arimal(1)=1
      Arimap(1)=1.0D0
      Arimaf(1)=.false.
      Prbfac=-99
      res=chkrts(1,1)
      WRITE(*,'(A,L2,I4)') 'chkE',res,Prbfac

C     Case F: theta=1-1e-16 -> cfncsq ~2.2e-16 > 0 -> invertible (1-ulp of 1)
      Arimap(1)=0.9999999999999999D0
      Prbfac=-99
      res=chkrts(1,1)
      WRITE(*,'(A,L2,I4)') 'chkF',res,Prbfac

C     Case G: degree-2 (0.5,0.5) -> reflection drives coef(1) to exactly 1.0 ->
C             cfncsq=0 at ic=1 -> non-invertible
      Opr(0)=1
      Opr(1)=3
      Oprfac(1)=1
      Arimal(1)=1
      Arimal(2)=2
      Arimap(1)=0.5D0
      Arimap(2)=0.5D0
      Arimaf(1)=.false.
      Arimaf(2)=.false.
      Prbfac=-99
      res=chkrts(1,1)
      WRITE(*,'(A,L2,I4)') 'chkG',res,Prbfac

C     Case H: seasonal Theta=1.0 at lag12, factor 12 -> degree=12/12=1 -> cfncsq=0
      Opr(0)=1
      Opr(1)=2
      Oprfac(1)=12
      Arimal(1)=12
      Arimap(1)=1.0D0
      Arimaf(1)=.false.
      Prbfac=-99
      res=chkrts(1,1)
      WRITE(*,'(A,L2,I4)') 'chkH',res,Prbfac
      END
