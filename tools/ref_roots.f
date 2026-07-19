C     ref_roots.f -- reference output for roots.f (polynomial root modulus/freq).
C       gfortran -O2 -ffp-contract=off -Ioracle/fortran tools/ref_roots.f \
C         oracle/fortran/{roots,revrse,rpoly,fxshfr,quadit,realit,calcsc,nextk,\
C         newest,quadsd,quad,dpeq}.f -o ref_roots && ./ref_roots
C     thetab is theta(B) in INCREASING powers. All three cases keep rpoly in its
C     success regime (max|coeff| < 10), so the fail-path WRITE is never reached.
      PROGRAM ref_roots
      IMPLICIT NONE
      INCLUDE 'srslen.prm'
      INCLUDE 'model.prm'
      DOUBLE PRECISION thetab(PORDER+1),zr(PORDER),zi(PORDER)
      DOUBLE PRECISION zm(PORDER),zf(PORDER)
      INTEGER deg,i,iainv
      LOGICAL allinv

C     ===== Case 1: MA(2) (1-0.2B)(1-0.3B)=1-0.5B+0.06B^2, invertible =====
      deg=2
      thetab(1)=1D0
      thetab(2)=-0.5D0
      thetab(3)=0.06D0
      allinv=.false.
      CALL roots(thetab,deg,allinv,zr,zi,zm,zf)
      iainv=0
      IF(allinv)iainv=1
      WRITE(*,'(A,I6)') 'ro1_deg ',deg
      WRITE(*,'(A,I6)') 'ro1_ainv ',iainv
      DO i=1,deg
       WRITE(*,'(A,I1,A,ES24.16)') 'ro1_zr',i,' ',zr(i)
       WRITE(*,'(A,I1,A,ES24.16)') 'ro1_zi',i,' ',zi(i)
       WRITE(*,'(A,I1,A,ES24.16)') 'ro1_zm',i,' ',zm(i)
       WRITE(*,'(A,I1,A,ES24.16)') 'ro1_zf',i,' ',zf(i)
      END DO

C     ===== Case 2: MA(1) 1-2B, root 0.5 -> non-invertible =====
      deg=1
      thetab(1)=1D0
      thetab(2)=-2D0
      allinv=.false.
      CALL roots(thetab,deg,allinv,zr,zi,zm,zf)
      iainv=0
      IF(allinv)iainv=1
      WRITE(*,'(A,I6)') 'ro2_deg ',deg
      WRITE(*,'(A,I6)') 'ro2_ainv ',iainv
      DO i=1,deg
       WRITE(*,'(A,I1,A,ES24.16)') 'ro2_zr',i,' ',zr(i)
       WRITE(*,'(A,I1,A,ES24.16)') 'ro2_zi',i,' ',zi(i)
       WRITE(*,'(A,I1,A,ES24.16)') 'ro2_zm',i,' ',zm(i)
       WRITE(*,'(A,I1,A,ES24.16)') 'ro2_zf',i,' ',zf(i)
      END DO

C     ===== Case 3: MA(2) 1+0.25B^2, roots +-2i, invertible (complex) =====
      deg=2
      thetab(1)=1D0
      thetab(2)=0D0
      thetab(3)=0.25D0
      allinv=.false.
      CALL roots(thetab,deg,allinv,zr,zi,zm,zf)
      iainv=0
      IF(allinv)iainv=1
      WRITE(*,'(A,I6)') 'ro3_deg ',deg
      WRITE(*,'(A,I6)') 'ro3_ainv ',iainv
      DO i=1,deg
       WRITE(*,'(A,I1,A,ES24.16)') 'ro3_zr',i,' ',zr(i)
       WRITE(*,'(A,I1,A,ES24.16)') 'ro3_zi',i,' ',zi(i)
       WRITE(*,'(A,I1,A,ES24.16)') 'ro3_zm',i,' ',zm(i)
       WRITE(*,'(A,I1,A,ES24.16)') 'ro3_zf',i,' ',zf(i)
      END DO
      END
