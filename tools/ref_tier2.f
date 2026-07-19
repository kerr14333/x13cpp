C     ref_tier2.f -- reference-output driver for the M3 packed-Cholesky trio
C     (xprmx, dppfa, dsolve). Same inputs as tests/unit/test_numeric.cpp.
C
C       gfortran -O2 tools/ref_tier2.f \
C         oracle/fortran/xprmx.f oracle/fortran/dppfa.f oracle/fortran/dsolve.f \
C         oracle/fortran/ddot.f oracle/fortran/daxpy.f \
C         oracle/fortran/dpmpar.f oracle/fortran/dpeq.f -o ref_tier2 && ./ref_tier2
C
      PROGRAM ref_tier2
      IMPLICIT NONE
      DOUBLE PRECISION xy(9),xpx(6),ap(3),bb(2)
      INTEGER info,i

C     xprmx: 3 obs, 2 X cols + y in col 3 (pcxy=3, ncxy=2).
C     row-major: [x1 x2 y] per row. col1=[1,1,1] col2=[1,2,3] y=[2,3,5]
      xy(1)=1.0D0
      xy(2)=1.0D0
      xy(3)=2.0D0
      xy(4)=1.0D0
      xy(5)=2.0D0
      xy(6)=3.0D0
      xy(7)=1.0D0
      xy(8)=3.0D0
      xy(9)=5.0D0
      CALL xprmx(xy,3,2,3,xpx)
      DO i=1,6
       WRITE(*,'(A,I1,A,ES24.16)') 'xprmx',i,' ',xpx(i)
      END DO

C     dppfa: packed [[4,2],[2,10]] = ap[4,2,10] -> R packed [2,1,3], info=0
      ap(1)=4.0D0
      ap(2)=2.0D0
      ap(3)=10.0D0
      CALL dppfa(ap,2,info)
      WRITE(*,'(A,I3)')      'dppfa_info ', info
      WRITE(*,'(A,ES24.16)') 'dppfa1 ', ap(1)
      WRITE(*,'(A,ES24.16)') 'dppfa2 ', ap(2)
      WRITE(*,'(A,ES24.16)') 'dppfa3 ', ap(3)

C     dsolve: A x = b with the factor above, b=[6,28], Nr=2 Nc=1, Lainvb=T
C     B(Nc,Nr): B(1,1)=b1, B(1,2)=b2 -> stored [6,28]
      bb(1)=6.0D0
      bb(2)=28.0D0
      CALL dsolve(ap,2,1,.TRUE.,bb)
      WRITE(*,'(A,ES24.16)') 'dsolve1 ', bb(1)
      WRITE(*,'(A,ES24.16)') 'dsolve2 ', bb(2)
      END
