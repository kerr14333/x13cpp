C     ref_fdjac2.f -- reference output for fdjac2 (forward-difference Jacobian),
C     driven by a simple analytic test fcn (N=2 params, M=3 residuals):
C       w1 = x1 + x2 ;  w2 = x1*x2 ;  w3 = x1^2 + x2 .
C       gfortran -O2 -ffp-contract=off -Ioracle/fortran tools/ref_fdjac2.f \
C         oracle/fortran/{fdjac2,dpmpar,dpeq}.f -o ref_fdjac2 && ./ref_fdjac2
      PROGRAM ref_fdjac2
      IMPLICIT NONE
      EXTERNAL tstfcn
      DOUBLE PRECISION x(2),fvec(3),fjac(3,2),wa(3)
      INTEGER iflag,i,j
      LOGICAL F
      PARAMETER(F=.false.)

      x(1)=1.5D0
      x(2)=-0.5D0
      iflag=1
      CALL tstfcn(3,2,x,fvec,F,F,iflag,F)
      CALL fdjac2(tstfcn,3,2,x,fvec,F,F,fjac,3,iflag,0D0,wa,F)
      DO j=1,2
       DO i=1,3
        WRITE(*,'(A,I1,I1,A,ES24.16)') 'fj',i,j,' ',fjac(i,j)
       END DO
      END DO
      END

C     ---- test fcn matching fcn(M,N,X,Wa,Lauto,Gudrun,Iflag,Lckinv) ----
      SUBROUTINE tstfcn(M,N,X,Wa,Lauto,Gudrun,Iflag,Lckinv)
      IMPLICIT NONE
      INTEGER M,N,Iflag
      DOUBLE PRECISION X(N),Wa(M)
      LOGICAL Lauto,Gudrun,Lckinv
      Wa(1)=X(1)+X(2)
      Wa(2)=X(1)*X(2)
      Wa(3)=X(1)*X(1)+X(2)
      RETURN
      END
