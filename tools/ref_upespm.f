C     ref_upespm.f -- reference output for upespm (scatter estprm -> arimap).
C     AR(2) both free, MA(1) fixed: estprm=[0.4,-0.2] fills arimap(1..2); the
C     fixed MA lag arimap(3) keeps its preset value.
C       gfortran -O2 -ffp-contract=off -Ioracle/fortran tools/ref_upespm.f \
C         oracle/fortran/upespm.f -o ref_upespm && ./ref_upespm
      PROGRAM ref_upespm
      IMPLICIT NONE
      INCLUDE 'srslen.prm'
      INCLUDE 'model.prm'
      INCLUDE 'model.cmn'
      INCLUDE 'mdldat.cmn'
      DOUBLE PRECISION estprm(2)
      INTEGER i

      Nestpm=2
      Mdl(0)=1
      Mdl(1)=1
      Mdl(2)=2
      Mdl(3)=3
      Opr(0)=1
      Opr(1)=3
      Opr(2)=4
      Arimaf(1)=.false.
      Arimaf(2)=.false.
      Arimaf(3)=.true.
      Arimap(1)=-99D0
      Arimap(2)=-99D0
      Arimap(3)=0.99D0
      estprm(1)=0.4D0
      estprm(2)=-0.2D0
      CALL upespm(estprm)
      DO i=1,3
       WRITE(*,'(A,I1,A,ES24.16)') 'up',i,' ',Arimap(i)
      END DO
      END
