C     ref_tier1.f -- reference-output driver for the M3 Tier-1 numeric/ARMA
C     leaves (yprmy, logdet, uconv, xpand, ratneg). Same purpose and inputs as
C     the C++ unit tests in tests/unit/test_numeric.cpp. ratneg pulls PARIMA/
C     POPR from oracle/fortran/{srslen,model}.prm, so compile with -Ioracle/fortran:
C
C       gfortran -O2 -Ioracle/fortran tools/ref_tier1.f \
C         oracle/fortran/yprmy.f oracle/fortran/logdet.f \
C         oracle/fortran/uconv.f oracle/fortran/xpand.f \
C         oracle/fortran/ratneg.f -o ref_tier1 && ./ref_tier1
C
      PROGRAM ref_tier1
      IMPLICIT NONE
      DOUBLE PRECISION y(4),ypy,ap(6),lgdt
      DOUBLE PRECISION fulma(0:2),cu(0:2)
      DOUBLE PRECISION bx(0:1),cx(0:3)
      DOUBLE PRECISION arimap(25),cn(4)
      INTEGER arimal(25),opr(0:9),i

C     yprmy: y=[1,2,3,4] -> 30
      y(1)=1.0D0
      y(2)=2.0D0
      y(3)=3.0D0
      y(4)=4.0D0
      CALL yprmy(y,4,ypy)
      WRITE(*,'(A,ES24.16)') 'yprmy ', ypy

C     logdet: packed diag at ap(1),ap(3),ap(6) = 2,3,4 -> 2*ln(24)
      DO i=1,6
       ap(i)=9.0D0
      END DO
      ap(1)=2.0D0
      ap(3)=3.0D0
      ap(6)=4.0D0
      CALL logdet(ap,3,lgdt)
      WRITE(*,'(A,ES24.16)') 'logdet ', lgdt

C     uconv: MA [1,0.5,0.25], mxmalg=2 -> autocov [1.3125,0.625,0.25]
      fulma(0)=1.0D0
      fulma(1)=0.5D0
      fulma(2)=0.25D0
      CALL uconv(fulma,2,cu)
      WRITE(*,'(A,ES24.16)') 'uconv0 ', cu(0)
      WRITE(*,'(A,ES24.16)') 'uconv1 ', cu(1)
      WRITE(*,'(A,ES24.16)') 'uconv2 ', cu(2)

C     xpand: A=[1] (na=0), B with b(1)=0.5, mxarlg=1, nc=3, pc=3
      bx(0)=0.0D0
      bx(1)=0.5D0
      cx(0)=1.0D0
      cx(1)=0.0D0
      cx(2)=0.0D0
      cx(3)=0.0D0
      CALL xpand(bx,1,0,3,cx,3)
      WRITE(*,'(A,ES24.16)') 'xpand0 ', cx(0)
      WRITE(*,'(A,ES24.16)') 'xpand1 ', cx(1)
      WRITE(*,'(A,ES24.16)') 'xpand2 ', cx(2)
      WRITE(*,'(A,ES24.16)') 'xpand3 ', cx(3)

C     ratneg: single AR operator, lag 1, phi=0.5, nelta=4, c=[1,2,3,4]
C     opr(0)=1 (beglag), opr(1)=2 (endlag=1); arimal(1)=1; arimap(1)=0.5
      opr(0)=1
      opr(1)=2
      arimal(1)=1
      arimap(1)=0.5D0
      cn(1)=1.0D0
      cn(2)=2.0D0
      cn(3)=3.0D0
      cn(4)=4.0D0
      CALL ratneg(4,arimap,arimal,opr,1,1,cn)
      WRITE(*,'(A,ES24.16)') 'ratneg1 ', cn(1)
      WRITE(*,'(A,ES24.16)') 'ratneg2 ', cn(2)
      WRITE(*,'(A,ES24.16)') 'ratneg3 ', cn(3)
      WRITE(*,'(A,ES24.16)') 'ratneg4 ', cn(4)
      END
