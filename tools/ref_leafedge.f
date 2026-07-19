C     ref_leafedge.f -- reference output for the leaf-level "don't clean up the
C     math" edge cases: ratneg exact-zero no-write (A9), ratpos coefficient
C     underflow-skip (A10), chkrts sparse/decreasing lags (A11).
C       gfortran -O2 -ffp-contract=off -Ioracle/fortran tools/ref_leafedge.f \
C         oracle/fortran/{ratneg,ratpos,chkrts,setdp}.f -o ref_leafedge && ./ref_leafedge
      PROGRAM ref_leafedge
      IMPLICIT NONE
      INCLUDE 'srslen.prm'
      INCLUDE 'model.prm'
      INCLUDE 'model.cmn'
      INCLUDE 'mdldat.cmn'
      LOGICAL chkrts,res
      EXTERNAL chkrts
      DOUBLE PRECISION ap(2),c(6)
      INTEGER al(2),op(0:2),i

C     ---- A9: ratneg exact-zero no-write. theta=0.5, c=[-1,2,0,0]; at i=1 the
C          sum -1+0.5*2 = 0 exactly -> c(1) keeps its OLD value -1 (not 0). ----
      ap(1)=0.5D0
      al(1)=1
      op(0)=1
      op(1)=2
      c(1)=-1D0
      c(2)=2D0
      c(3)=0D0
      c(4)=0D0
      CALL ratneg(4,ap,al,op,1,1,c)
      DO i=1,4
       WRITE(*,'(A,I1,A,ES24.16)') 'rn',i,' ',c(i)
      END DO

C     ---- A10: ratpos coefficient underflow-skip. arimap=1e-160 (< 1e-150
C          threshold) -> the whole term is skipped, so c(2) stays 0, NOT 1e-160.
      ap(1)=1D-160
      al(1)=1
      op(0)=1
      op(1)=2
      c(1)=1D0
      c(2)=0D0
      c(3)=0D0
      c(4)=0D0
      CALL ratpos(1,ap,al,op,1,1,4,c)
      DO i=1,4
       WRITE(*,'(A,I1,A,ES24.16)') 'rp',i,' ',c(i)
      END DO

C     ---- A11a: chkrts sparse MA lags [2 4], theta=(0.3,0.2) -> degree 4 with
C          coef(1)=coef(3)=0. ----
      op(0)=1
      op(1)=3
      Oprfac(1)=1
      Arimal(1)=2
      Arimal(2)=4
      Arimap(1)=0.3D0
      Arimap(2)=0.2D0
      Arimaf(1)=.false.
      Arimaf(2)=.false.
      Prbfac=-99
      res=chkrts(1,1)
      WRITE(*,'(A,L2,I4)') 'saa',res,Prbfac

C     ---- A11b: decreasing lags [4 2] -- exercises the "lag is not the highest
C          lag" degree path (degree fixed from first/highest lag). ----
      Arimal(1)=4
      Arimal(2)=2
      Arimap(1)=0.3D0
      Arimap(2)=0.2D0
      Prbfac=-99
      res=chkrts(1,1)
      WRITE(*,'(A,L2,I4)') 'sab',res,Prbfac
      END
