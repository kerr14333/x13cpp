C     ref_tier0.f -- reference-output driver for the M3 Tier-0 numeric leaves.
C
C     Calls the vendored oracle routines (dpmpar/dpeq/scrmlt/maxvec/dcopy/
C     daxpy/ddot/revrse/enorm) with the exact inputs used by
C     tests/unit/test_numeric.cpp and prints the expected values. Compile
C     against the oracle sources to regenerate the goldens:
C
C       gfortran -O2 tools/ref_tier0.f \
C         oracle/fortran/dpmpar.f oracle/fortran/dpeq.f \
C         oracle/fortran/scrmlt.f oracle/fortran/maxvec.f \
C         oracle/fortran/dcopy.f  oracle/fortran/daxpy.f \
C         oracle/fortran/ddot.f   oracle/fortran/revrse.f \
C         oracle/fortran/enorm.f  -o ref_tier0 && ./ref_tier0
C
      PROGRAM ref_tier0
      IMPLICIT NONE
      DOUBLE PRECISION dpmpar,ddot,enorm
      LOGICAL dpeq
      EXTERNAL dpmpar,dpeq,ddot,enorm,scrmlt,maxvec,dcopy,daxpy,revrse
      DOUBLE PRECISION c(4),a4(4),mx,b8(8),a3(3),bcp(8)
      DOUBLE PRECISION dxa(5),dyb(5),s,p6(6),q6(6),ap3(3),bp3(3)
      DOUBLE PRECISION rin(8),rout(8),m2(2),sm3(3),lg2(2),mix3(3)
      INTEGER i

C     dpmpar
      WRITE(*,'(A,ES24.16)') 'dpmpar1 ', dpmpar(1)
      WRITE(*,'(A,ES24.16)') 'dpmpar2 ', dpmpar(2)
      WRITE(*,'(A,ES24.16)') 'dpmpar3 ', dpmpar(3)

C     dpeq
      WRITE(*,'(A,L2)') 'dpeq_3e20  ', dpeq(3.0D-20,0.0D0)
      WRITE(*,'(A,L2)') 'dpeq_4e20  ', dpeq(4.0D-20,0.0D0)
      WRITE(*,'(A,L2)') 'dpeq_delta ', dpeq(3.834D-20,0.0D0)
      WRITE(*,'(A,L2)') 'dpeq_11    ', dpeq(1.0D0,1.0D0)

C     scrmlt(2.5, 4, [1,2,3,4])
      c(1)=1.0D0
      c(2)=2.0D0
      c(3)=3.0D0
      c(4)=4.0D0
      CALL scrmlt(2.5D0,4,c)
      WRITE(*,'(A,ES24.16)') 'scrmlt_c1 ', c(1)
      WRITE(*,'(A,ES24.16)') 'scrmlt_c3 ', c(3)

C     maxvec([-7,3,-9.5,2])
      a4(1)=-7.0D0
      a4(2)=3.0D0
      a4(3)=-9.5D0
      a4(4)=2.0D0
      CALL maxvec(a4,4,mx)
      WRITE(*,'(A,ES24.16)') 'maxvec    ', mx

C     dcopy(3,[11,22,33],1,b,2)
      a3(1)=11.0D0
      a3(2)=22.0D0
      a3(3)=33.0D0
      DO i=1,8
       b8(i)=-1.0D0
      END DO
      CALL dcopy(3,a3,1,b8,2)
      WRITE(*,'(A,ES24.16)') 'dcopy_b1 ', b8(1)
      WRITE(*,'(A,ES24.16)') 'dcopy_b3 ', b8(3)
      WRITE(*,'(A,ES24.16)') 'dcopy_b5 ', b8(5)
      WRITE(*,'(A,ES24.16)') 'dcopy_b2 ', b8(2)

C     daxpy(5,2,[1..5],1,[10,20,30,40,50],1)
      DO i=1,5
       dxa(i)=DBLE(i)
       dyb(i)=10.0D0*DBLE(i)
      END DO
      CALL daxpy(5,2.0D0,dxa,1,dyb,1)
      s=0.0D0
      DO i=1,5
       s=s+dyb(i)
      END DO
      WRITE(*,'(A,ES24.16)') 'daxpy_sum ', s
      WRITE(*,'(A,ES24.16)') 'daxpy_b1 ', dyb(1)

C     ddot(3,[1e-160,2,3],1,[1e-160,5,7],1) underflow-skip -> 31
      ap3(1)=1.0D-160
      ap3(2)=2.0D0
      ap3(3)=3.0D0
      bp3(1)=1.0D-160
      bp3(2)=5.0D0
      bp3(3)=7.0D0
      WRITE(*,'(A,ES24.16)') 'ddot1 ', ddot(3,ap3,1,bp3,1)
C     ddot(6,[1..6],1,[6..1],1) -> 56
      DO i=1,6
       p6(i)=DBLE(i)
       q6(i)=6.0D0-DBLE(i-1)
      END DO
      WRITE(*,'(A,ES24.16)') 'ddot2 ', ddot(6,p6,1,q6,1)

C     revrse 4x2 : rows [41,42],[51,52],[61,62],[71,72]
      DO i=1,4
       rin(2*(i-1)+1)=10.0D0*DBLE(i)+1.0D0
       rin(2*(i-1)+2)=10.0D0*DBLE(i)+2.0D0
      END DO
      CALL revrse(rin,4,2,rout)
      WRITE(*,'(A,ES24.16)') 'revrse_b1 ', rout(1)
      WRITE(*,'(A,ES24.16)') 'revrse_b2 ', rout(2)
      WRITE(*,'(A,ES24.16)') 'revrse_b7 ', rout(7)
      WRITE(*,'(A,ES24.16)') 'revrse_b8 ', rout(8)

C     enorm cases
      m2(1)=3.0D0
      m2(2)=4.0D0
      WRITE(*,'(A,ES24.16)') 'enorm1 ', enorm(2,m2)
      sm3(1)=1.0D-20
      sm3(2)=2.0D-20
      sm3(3)=3.0D-20
      WRITE(*,'(A,ES24.16)') 'enorm2 ', enorm(3,sm3)
      lg2(1)=1.0D19
      lg2(2)=2.0D19
      WRITE(*,'(A,ES24.16)') 'enorm3 ', enorm(2,lg2)
      mix3(1)=1.0D-20
      mix3(2)=3.0D0
      mix3(3)=1.0D19
      WRITE(*,'(A,ES24.16)') 'enorm4 ', enorm(3,mix3)

C     silence unused-warning for bcp
      bcp(1)=0.0D0
      END
