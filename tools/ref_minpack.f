C     ref_minpack.f -- reference output for qrfac + qrsolv (Census MINPACK).
C       gfortran -O2 -ffp-contract=off -Ioracle/fortran tools/ref_minpack.f \
C         oracle/fortran/{qrfac,qrsolv,enorm,dpmpar,dpeq}.f -o ref_minpack && ./ref_minpack
      PROGRAM ref_minpack
      IMPLICIT NONE
      DOUBLE PRECISION a(12),rdiag(3),acnorm(3),wa(3)
      DOUBLE PRECISION r(9),diag(3),qtb(3),x(3),sdiag(3),wa2(3)
      INTEGER ipvt(3),i

C     ---- qrfac: 4x3 matrix, column pivoting ----
      a(1)=1D0
      a(2)=2D0
      a(3)=3D0
      a(4)=4D0
      a(5)=1D0
      a(6)=0D0
      a(7)=1D0
      a(8)=0D0
      a(9)=2D0
      a(10)=1D0
      a(11)=0D0
      a(12)=1D0
      CALL qrfac(4,3,a,4,.true.,ipvt,3,rdiag,acnorm,wa)
      DO i=1,3
       WRITE(*,'(A,I1,A,I4)') 'qf_ipvt',i,' ',ipvt(i)
      END DO
      DO i=1,3
       WRITE(*,'(A,I1,A,ES24.16)') 'qf_rd',i,' ',rdiag(i)
      END DO
      DO i=1,3
       WRITE(*,'(A,I1,A,ES24.16)') 'qf_ac',i,' ',acnorm(i)
      END DO
      WRITE(*,'(A,ES24.16)') 'qf_a1 ',a(1)
      WRITE(*,'(A,ES24.16)') 'qf_a2 ',a(2)
      WRITE(*,'(A,ES24.16)') 'qf_a6 ',a(6)

C     ---- qrsolv: 3x3 upper-tri R (col-major), diag, qtb ----
      r(1)=2D0
      r(2)=0D0
      r(3)=0D0
      r(4)=1D0
      r(5)=3D0
      r(6)=0D0
      r(7)=1D0
      r(8)=1D0
      r(9)=4D0
      ipvt(1)=1
      ipvt(2)=2
      ipvt(3)=3
      diag(1)=0.5D0
      diag(2)=0.5D0
      diag(3)=0.5D0
      qtb(1)=1D0
      qtb(2)=2D0
      qtb(3)=3D0
      CALL qrsolv(3,r,3,ipvt,diag,qtb,x,sdiag,wa2)
      DO i=1,3
       WRITE(*,'(A,I1,A,ES24.16)') 'qs_x',i,' ',x(i)
      END DO
      DO i=1,3
       WRITE(*,'(A,I1,A,ES24.16)') 'qs_sd',i,' ',sdiag(i)
      END DO
      END
