C     ref_rpoly.f -- reference output for the Jenkins-Traub RPOLY suite.
C       gfortran -O2 -ffp-contract=off -Ioracle/fortran tools/ref_rpoly.f \
C         oracle/fortran/{rpoly,fxshfr,quadit,realit,calcsc,nextk,newest,\
C         quadsd,quad,dpeq}.f -o ref_rpoly && ./ref_rpoly
C     Self-contained: rpoly + its suite share global.cmn (private scratch); the
C     driver only supplies polynomial coefficients (decreasing powers).
      PROGRAM ref_rpoly
      IMPLICIT NONE
      INCLUDE 'srslen.prm'
      INCLUDE 'model.prm'
      DOUBLE PRECISION op(PORDER+1),zr(PORDER),zi(PORDER)
      INTEGER deg,i,ifail
      LOGICAL fail

C     ===== Case 1: (x-1)(x-2)(x-3) = x^3 -6x^2 +11x -6 =====
      deg=3
      op(1)=1D0
      op(2)=-6D0
      op(3)=11D0
      op(4)=-6D0
      CALL rpoly(op,deg,zr,zi,fail)
      ifail=0
      IF(fail)ifail=1
      WRITE(*,'(A,I6)') 'r1_deg ',deg
      WRITE(*,'(A,I6)') 'r1_fail ',ifail
      DO i=1,deg
       WRITE(*,'(A,I1,A,ES24.16)') 'r1_zr',i,' ',zr(i)
       WRITE(*,'(A,I1,A,ES24.16)') 'r1_zi',i,' ',zi(i)
      END DO

C     ===== Case 2: (x^2+1)(x-2) = x^3 -2x^2 +x -2 (complex pair) =====
      deg=3
      op(1)=1D0
      op(2)=-2D0
      op(3)=1D0
      op(4)=-2D0
      CALL rpoly(op,deg,zr,zi,fail)
      ifail=0
      IF(fail)ifail=1
      WRITE(*,'(A,I6)') 'r2_deg ',deg
      WRITE(*,'(A,I6)') 'r2_fail ',ifail
      DO i=1,deg
       WRITE(*,'(A,I1,A,ES24.16)') 'r2_zr',i,' ',zr(i)
       WRITE(*,'(A,I1,A,ES24.16)') 'r2_zi',i,' ',zi(i)
      END DO

C     ===== Case 3: (x-1)(x-2)(x-3)(x-4) = x^4 -10x^3 +35x^2 -50x +24 =====
      deg=4
      op(1)=1D0
      op(2)=-10D0
      op(3)=35D0
      op(4)=-50D0
      op(5)=24D0
      CALL rpoly(op,deg,zr,zi,fail)
      ifail=0
      IF(fail)ifail=1
      WRITE(*,'(A,I6)') 'r3_deg ',deg
      WRITE(*,'(A,I6)') 'r3_fail ',ifail
      DO i=1,deg
       WRITE(*,'(A,I1,A,ES24.16)') 'r3_zr',i,' ',zr(i)
       WRITE(*,'(A,I1,A,ES24.16)') 'r3_zi',i,' ',zi(i)
      END DO

C     ===== Case 4: (x-0.5)(x-0.3)(x+0.2), real roots inside unit circle =====
C     x^3 -0.6x^2 -0.01x +0.03 (the ARMA-reversed regime)
      deg=3
      op(1)=1D0
      op(2)=-0.6D0
      op(3)=-0.01D0
      op(4)=0.03D0
      CALL rpoly(op,deg,zr,zi,fail)
      ifail=0
      IF(fail)ifail=1
      WRITE(*,'(A,I6)') 'r4_deg ',deg
      WRITE(*,'(A,I6)') 'r4_fail ',ifail
      DO i=1,deg
       WRITE(*,'(A,I1,A,ES24.16)') 'r4_zr',i,' ',zr(i)
       WRITE(*,'(A,I1,A,ES24.16)') 'r4_zi',i,' ',zi(i)
      END DO

C     ===== Case 5: (x-0.4)(x^2+0.25), real + complex pair inside circle =====
C     x^3 -0.4x^2 +0.25x -0.1
      deg=3
      op(1)=1D0
      op(2)=-0.4D0
      op(3)=0.25D0
      op(4)=-0.1D0
      CALL rpoly(op,deg,zr,zi,fail)
      ifail=0
      IF(fail)ifail=1
      WRITE(*,'(A,I6)') 'r5_deg ',deg
      WRITE(*,'(A,I6)') 'r5_fail ',ifail
      DO i=1,deg
       WRITE(*,'(A,I1,A,ES24.16)') 'r5_zr',i,' ',zr(i)
       WRITE(*,'(A,I1,A,ES24.16)') 'r5_zi',i,' ',zi(i)
      END DO

C     ===== Case 6: coefficient-magnitude failure pin =====
C     20x^3 -12x^2 -0.2x +0.6 has roots 0.5,0.3,-0.2 (INSIDE the unit
C     circle) yet fails: max|coeff|=20>=10 trips the Census scaling bug.
C     Proves the trigger is coefficient magnitude, not root location.
      deg=3
      op(1)=20D0
      op(2)=-12D0
      op(3)=-0.2D0
      op(4)=0.6D0
      CALL rpoly(op,deg,zr,zi,fail)
      ifail=0
      IF(fail)ifail=1
      WRITE(*,'(A,I6)') 'r6_deg ',deg
      WRITE(*,'(A,I6)') 'r6_fail ',ifail
      END
