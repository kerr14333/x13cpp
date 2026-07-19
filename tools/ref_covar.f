C     ref_covar.f -- reference output for covar (Census MINPACK).
C       gfortran -O2 -ffp-contract=off -Ioracle/fortran tools/ref_covar.f \
C         oracle/fortran/{covar,dpmpar}.f -o ref_covar && ./ref_covar
C     PowerShell (no brace expansion), from the repo root:
C       gfortran -O2 -ffp-contract=off -Ioracle/fortran tools/ref_covar.f
C         oracle/fortran/covar.f oracle/fortran/dpmpar.f -o ref_covar
C       ./ref_covar
C     covar.f INCLUDEs notset.prm/srslen.prm/model.prm (the -Ioracle/fortran is
C     required); it only uses PARIMA from model.prm to size its wa scratch, which
C     the C++ port sidesteps with a caller-supplied wa(n).
      PROGRAM ref_covar
      IMPLICIT NONE
      DOUBLE PRECISION r(9),tol
      INTEGER ipvt(3),info,i,j

C     ---- Case 1: 3x3 nonsingular upper-tri R (col-major), NON-identity
C          ipvt=(2,3,1) to exercise the permutation, Tol=0 (auto path) ----
      r(1)=2D0
      r(2)=0D0
      r(3)=0D0
      r(4)=1D0
      r(5)=3D0
      r(6)=0D0
      r(7)=1D0
      r(8)=1D0
      r(9)=4D0
      ipvt(1)=2
      ipvt(2)=3
      ipvt(3)=1
      tol=0D0
      CALL covar(3,r,3,ipvt,tol,info)
      DO j=1,3
       DO i=1,3
        WRITE(*,'(A,I1,I1,A,ES24.16)') 'cv_r',i,j,' ',r((j-1)*3+i)
       END DO
      END DO
      WRITE(*,'(A,I8)') 'cv_info ',info

C     ---- Case 2: same R + ipvt, explicit Tol=1D-10 (the ELSE tolerance
C          path); still nonsingular, so Info=0 as well ----
      r(1)=2D0
      r(2)=0D0
      r(3)=0D0
      r(4)=1D0
      r(5)=3D0
      r(6)=0D0
      r(7)=1D0
      r(8)=1D0
      r(9)=4D0
      ipvt(1)=2
      ipvt(2)=3
      ipvt(3)=1
      tol=1D-10
      CALL covar(3,r,3,ipvt,tol,info)
      DO j=1,3
       DO i=1,3
        WRITE(*,'(A,I1,I1,A,ES24.16)') 'cw_r',i,j,' ',r((j-1)*3+i)
       END DO
      END DO
      WRITE(*,'(A,I8)') 'cw_info ',info
      END
