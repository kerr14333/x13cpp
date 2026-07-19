C     ref_estimate.f -- reference output for olsreg + resid (regression solve /
C     residuals). Small OLS: 4 obs, intercept + one regressor, y in col 3.
C     errhdr/abend are stubbed (their guard branches are never taken here).
C       gfortran -O2 -ffp-contract=off -Ioracle/fortran tools/ref_estimate.f \
C         oracle/fortran/{olsreg,resid,xprmx,dppfa,copy,dcopy,daxpy,ddot,dpmpar}.f \
C         -o ref_estimate && ./ref_estimate
      PROGRAM ref_estimate
      IMPLICIT NONE
      DOUBLE PRECISION xy(12),b(2),chlxpx(10),rsd(4)
      INTEGER info,i

C     [X:y], pcxy=3, nr=4; per obs: [1, x2, y]
      xy(1)=1D0
      xy(2)=1D0
      xy(3)=2.1D0
      xy(4)=1D0
      xy(5)=2D0
      xy(6)=3.9D0
      xy(7)=1D0
      xy(8)=3D0
      xy(9)=6.2D0
      xy(10)=1D0
      xy(11)=4D0
      xy(12)=7.8D0

      info=-99
      CALL olsreg(xy,4,3,3,b,chlxpx,10,info)
      WRITE(*,'(A,I4)') 'ols_info ',info
      WRITE(*,'(A,ES24.16)') 'ols_b1 ',b(1)
      WRITE(*,'(A,ES24.16)') 'ols_b2 ',b(2)

C     residuals y - X*b over cols 1..2 (fac<0 -> subtract)
      CALL resid(xy,4,3,3,1,2,-1D0,b,rsd)
      DO i=1,4
       WRITE(*,'(A,I1,A,ES24.16)') 'rsd',i,' ',rsd(i)
      END DO
      END

C     ---- stubs (guard branches are unreachable in this driver) ----
      SUBROUTINE errhdr
      RETURN
      END
      SUBROUTINE abend
      RETURN
      END
